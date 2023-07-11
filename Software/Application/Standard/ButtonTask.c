/* Standard includes. */
#include <stdio.h>

/* Kernel includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

/* SDK includes */
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "hardware/gpio.h"
#include "ElectronicBlinds_Main.h"
#include "hardware/timer.h"

#include "ButtonTask.h"

typedef struct
{
	uint32_t gpio;
	uint32_t edge;
}ButtonInfoType;

MotorState_t MotorState_Requested = STATE_OFF;
static uint32_t ExpectedEdgeDir;
volatile static bool ButtonUsed = false;
static uint32_t InterruptGPIO_NumberGlobal;
static bool LimitReached = false;
bool MotorStateChangeSemaphoreObtained = false;
SemaphoreHandle_t buttonSemaphore;
static ButtonInfoType ButtonInfo;

static void alarm_irq(void) 
{
    /* Clear the alarm irq */
    hw_clear_bits(&timer_hw->intr, 1u << ALARM_NUM);

	/* After RISE event a FALL event is expected, and after FALL the next event should be RISE (since you PRESS and RELEASE the button) */
	(ExpectedEdgeDir == GPIO_IRQ_EDGE_RISE) ? (ExpectedEdgeDir = GPIO_IRQ_EDGE_FALL) : (ExpectedEdgeDir = GPIO_IRQ_EDGE_RISE);

	// Read the state of the LED pin
    bool is_high = gpio_get(InterruptGPIO_NumberGlobal);

	if(!is_high)
	{
		MotorState_Requested = STATE_OFF;
		ExpectedEdgeDir = GPIO_IRQ_EDGE_RISE;
	}
	
	/* Re-enable the interrupts*/
	gpio_set_irq_enabled_with_callback(BUTTON_DOWN, ExpectedEdgeDir, true, &buttons_callback);
	gpio_set_irq_enabled(BUTTON_UP, ExpectedEdgeDir, true);
}

static void alarm_in_us(uint32_t delay_us) 
{
    /* Enable the interrupt for the alarm */
    hw_set_bits(&timer_hw->inte, 1u << ALARM_NUM);
    /*  Set irq handler for alarm irq and enable it*/
    irq_set_exclusive_handler(ALARM_IRQ, alarm_irq);
    irq_set_enabled(ALARM_IRQ, true);

	/* Set the alarm time */
    uint64_t target = timer_hw->timerawl + delay_us;
    timer_hw->alarm[ALARM_NUM] = (uint32_t) target;
}

void buttons_callback(uint gpio, uint32_t events)
{
	/* Disable the interrupts */ 
	gpio_set_irq_enabled_with_callback(BUTTON_DOWN, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, false, &buttons_callback);
    gpio_set_irq_enabled(BUTTON_UP, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, false);
	gpio_set_irq_enabled(BUTTON_TOP_LIMIT, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, false);
	gpio_set_irq_enabled(BUTTON_BOTTOM_LIMIT, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, false);

	/* Set a timer for 50ms (for debouncing purposes) */
	InterruptGPIO_NumberGlobal = gpio;
	alarm_in_us(DEBOUNCING_DELAY_IN_US);

	ButtonUsed = true;
	ButtonInfo.gpio = gpio;
	ButtonInfo.edge = events;
}

void ButtonTask( void *pvParameters )
{
    /* Enabled the IRQs for the button pins 
	In Raspberry Pi Pico, only one callback function can be used for GPIO interrupts, even if multiple pins are used. 
	This is because the interrupts are handled at the hardware level and there is only one interrupt handler for all the GPIO pins.*/
	gpio_set_irq_enabled_with_callback(BUTTON_DOWN, GPIO_IRQ_EDGE_RISE, true, &buttons_callback);
	/* For the second and the concurrent GPIOs we dont have to specify the callback - the first GPIO already set the generic callback used for 
	GPIO IRQ events for the current core (see inside the gpio_set_irq... function. There is a function gpio_set_irq_callback that doesnt care about the pin number*/
    gpio_set_irq_enabled(BUTTON_UP, GPIO_IRQ_EDGE_RISE, true);
    
    /* Create a binary semaphore */
	/* Once created, a semaphore can be used with the xSemaphoreTake and xSemaphoreGive functions to control access to the shared resource */
 	buttonSemaphore = xSemaphoreCreateBinary();

    ExpectedEdgeDir = GPIO_IRQ_EDGE_RISE;

	TickType_t xTaskStartTime;
	const TickType_t xTaskPeriod = pdMS_TO_TICKS(BUTTON_TASK_PERIOD);

	xTaskStartTime = xTaskGetTickCount();

	for( ;; )
	{
		if(ButtonUsed)
		{
			/* If button pressed: (rising edge)*/
			if((ButtonInfo.edge & GPIO_IRQ_EDGE_FALL) == GPIO_IRQ_EDGE_FALL)
			{
				switch (ButtonInfo.gpio)
				{
				case BUTTON_UP:
				case BUTTON_DOWN:
					if(xSemaphoreGive(buttonSemaphore) == pdTRUE)
					{
						MotorState_Requested = STATE_OFF;
					}
					break;
				
				case BUTTON_TOP_LIMIT:
				case BUTTON_BOTTOM_LIMIT:
					LimitReached = false;
					break;
				
				default:
					/* Do nothing */
					break;
				}
			}
			/* If button released: (falling edge)*/
			else if((ButtonInfo.edge & GPIO_IRQ_EDGE_RISE) == GPIO_IRQ_EDGE_RISE)
			{
				switch (ButtonInfo.gpio)
				{
				case BUTTON_UP:
					if(xSemaphoreGive(buttonSemaphore) == pdTRUE)
					{
						MotorState_Requested = STATE_ANTICLOCKWISE;
					}
					break;
				case BUTTON_DOWN:
					if(xSemaphoreGive(buttonSemaphore) == pdTRUE)
					{
						MotorState_Requested = STATE_CLOCKWISE;
					}
					break;
				
				case BUTTON_TOP_LIMIT:
				case BUTTON_BOTTOM_LIMIT:
					LimitReached = true;
					if(xSemaphoreGive(buttonSemaphore) == pdTRUE)
					{
						MotorState_Requested = STATE_OFF;
					}
					break;
				
				default:
					/* Do nothing */
					break;
				}
			}
			ButtonUsed = false;
		}

		vTaskDelayUntil(&xTaskStartTime, xTaskPeriod);
	}
}