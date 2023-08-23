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
	bool pending;
}ButtonInfoType;

MotorState_t MotorState_Requested = STATE_OFF;
static bool TopLimitReached = false;
static bool BottomLimitReached = false;
SemaphoreHandle_t buttonSemaphore;
static ButtonInfoType UpDown_ButtonInfo, Limitter_ButtonInfo;
static uint32_t ExpctdEdges[4]; /* from 0 to 3: BUTTON_DOWN, BUTTON_UP, BUTTON_TOP_LIMIT, BUTTON_BOTTOM_LIMIT - TODO -this is temporary, change to smth more readable later */

static void alarm0_InterruptHandler(void) 
{
    /* Clear the alarm irq */
    hw_clear_bits(&timer_hw->intr, 1u << 0);

	/* Debouncing delay gives the button time to stabilize its' state. The below code handles a situation when the button is released 
	   within the debouncing delay (150ms currently) - so basically in case of very very fast button press. Without this code, in case
	   of such press, the falling edge (release of the button) would not be detected (since interrupts are disabled during debouncing 
	   delay). This would result in the state getting stuck and not returning to STATE_OFF after button release as it should */
    bool is_high = gpio_get(UpDown_ButtonInfo.gpio);
	if(!is_high && !(UpDown_ButtonInfo.edge & GPIO_IRQ_EDGE_FALL))
	{
		LOG("BUTTON CHANGED TO LOW DURING DEBOUNCING DELAY - SETTING DETECTED EDGE TO FALL \n");
		UpDown_ButtonInfo.pending = true;
		UpDown_ButtonInfo.edge = GPIO_IRQ_EDGE_FALL;

		(UpDown_ButtonInfo.gpio == BUTTON_DOWN) ? (ExpctdEdges[0] = GPIO_IRQ_EDGE_RISE) : (ExpctdEdges[1] = GPIO_IRQ_EDGE_RISE);
	}
	
	/* Re-enable the interrupts*/
	LOG("RE-ENABLE INTERRUPTS \n");
	gpio_set_irq_enabled_with_callback(BUTTON_DOWN, ExpctdEdges[0], true, &buttons_callback);
	gpio_set_irq_enabled(BUTTON_UP, ExpctdEdges[1], true);
}

static void alarm1_InterruptHandler(void) 
{
    /* Clear the alarm irq */
    hw_clear_bits(&timer_hw->intr, 1u << 1);

	/* Debouncing delay gives the button time to stabilize its' state. The below code handles a situation when the button is released 
	   within the debouncing delay (150ms currently) - so basically in case of very very fast button press. Without this code, in case
	   of such press, the falling edge (release of the button) would not be detected (since interrupts are disabled during debouncing 
	   delay). This would result in the state getting stuck and not returning to STATE_OFF after button release as it should */
    bool is_high = gpio_get(Limitter_ButtonInfo.gpio);
	if(!is_high && !(Limitter_ButtonInfo.edge & GPIO_IRQ_EDGE_FALL))
	{
		LOG("BUTTON CHANGED TO LOW DURING DEBOUNCING DELAY - SETTING DETECTED EDGE TO FALL \n");
		Limitter_ButtonInfo.pending = true;
		Limitter_ButtonInfo.edge = GPIO_IRQ_EDGE_FALL;

		(Limitter_ButtonInfo.gpio == BUTTON_TOP_LIMIT) ? (ExpctdEdges[2] = GPIO_IRQ_EDGE_RISE) : (ExpctdEdges[3] = GPIO_IRQ_EDGE_RISE);
	}
	
	/* Re-enable the interrupts*/
	LOG("RE-ENABLE INTERRUPTS \n");
	gpio_set_irq_enabled_with_callback(BUTTON_TOP_LIMIT, ExpctdEdges[2], true, &buttons_callback);
	gpio_set_irq_enabled(BUTTON_BOTTOM_LIMIT, ExpctdEdges[3], true);
}

static void timerInit(uint32_t delay_us, uint8_t timerNum)
{
    hw_set_bits(&timer_hw->inte, 1u << timerNum);

	if(timerNum == 0)
	{
    	irq_set_exclusive_handler(TIMER_IRQ_0, alarm0_InterruptHandler);
		irq_set_enabled(TIMER_IRQ_0, true);
	}
	else if(timerNum == 1)
	{
    	irq_set_exclusive_handler(TIMER_IRQ_1, alarm1_InterruptHandler);
		irq_set_enabled(TIMER_IRQ_1, true);
	}
	else
	{
		LOG("Wrong timerNum - no handler function defined! \n");
	}

	/* Set the alarm time */
    uint64_t target = timer_hw->timerawl + delay_us;
    timer_hw->alarm[timerNum] = (uint32_t) target;
}

void buttons_callback(uint gpio, uint32_t events)
{
	LOG("GPIO: %d, EVENT: %d \n", gpio, events);
	/* Disable the interrupts */ 
	LOG("DISABLE INTERRUPTS \n");
	if((gpio == BUTTON_DOWN) || (gpio == BUTTON_UP))
	{
		gpio_set_irq_enabled_with_callback(BUTTON_DOWN, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, false, &buttons_callback);
		gpio_set_irq_enabled(BUTTON_UP, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, false);

		UpDown_ButtonInfo.pending = true;
		UpDown_ButtonInfo.gpio = gpio;
		UpDown_ButtonInfo.edge = events;

		(gpio == BUTTON_DOWN && events == GPIO_IRQ_EDGE_RISE) ? (ExpctdEdges[0] = GPIO_IRQ_EDGE_FALL) : (ExpctdEdges[0] = GPIO_IRQ_EDGE_RISE);
		(gpio == BUTTON_UP && events == GPIO_IRQ_EDGE_RISE) ? (ExpctdEdges[1] = GPIO_IRQ_EDGE_FALL) : (ExpctdEdges[1] = GPIO_IRQ_EDGE_RISE);

		/* Set a timer for debouncing delay - during that time interrupts are disabled - no button presses detected */
		timerInit(DEBOUNCING_DELAY_IN_US, 0);
	}
	else if((gpio == BUTTON_BOTTOM_LIMIT) || (gpio == BUTTON_TOP_LIMIT))
	{
		gpio_set_irq_enabled_with_callback(BUTTON_TOP_LIMIT, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, false, &buttons_callback);
		gpio_set_irq_enabled(BUTTON_BOTTOM_LIMIT, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, false);

		Limitter_ButtonInfo.pending = true;
		Limitter_ButtonInfo.gpio = gpio;
		Limitter_ButtonInfo.edge = events;

		(gpio == BUTTON_TOP_LIMIT && events == GPIO_IRQ_EDGE_RISE) ? (ExpctdEdges[2] = GPIO_IRQ_EDGE_FALL) : (ExpctdEdges[2] = GPIO_IRQ_EDGE_RISE);
		(gpio == BUTTON_BOTTOM_LIMIT && events == GPIO_IRQ_EDGE_RISE) ? (ExpctdEdges[3] = GPIO_IRQ_EDGE_FALL) : (ExpctdEdges[3] = GPIO_IRQ_EDGE_RISE);

		/* Set a timer for debouncing delay - during that time interrupts are disabled - no button presses detected */
		timerInit(DEBOUNCING_DELAY_IN_US, 1);
	}
	else
	{
		LOG("Button unknown - interrupts not disabled! \n");
	}
}

void ButtonTask( void *pvParameters )
{
    /* Enabled the IRQs for the button pins 
	In Raspberry Pi Pico, only one callback function can be used for GPIO interrupts, even if multiple pins are used. 
	This is because the interrupts are handled at the hardware level and there is only one interrupt handler for all the GPIO pins.*/
	gpio_get(BUTTON_DOWN) ? (ExpctdEdges[0] = GPIO_IRQ_EDGE_FALL) : (ExpctdEdges[0] = GPIO_IRQ_EDGE_RISE);
	gpio_get(BUTTON_UP) ? (ExpctdEdges[1] = GPIO_IRQ_EDGE_FALL) : (ExpctdEdges[1] = GPIO_IRQ_EDGE_RISE);
	gpio_get(BUTTON_TOP_LIMIT) ? (ExpctdEdges[2] = GPIO_IRQ_EDGE_FALL) : (ExpctdEdges[2] = GPIO_IRQ_EDGE_RISE);
	gpio_get(BUTTON_BOTTOM_LIMIT) ? (ExpctdEdges[3] = GPIO_IRQ_EDGE_FALL) : (ExpctdEdges[3] = GPIO_IRQ_EDGE_RISE);

	gpio_set_irq_enabled_with_callback(BUTTON_DOWN, ExpctdEdges[0], true, &buttons_callback);
	/* For the second and the concurrent GPIOs we dont have to specify the callback - the first GPIO already set the generic callback used for 
	GPIO IRQ events for the current core (see inside the gpio_set_irq... function. There is a function gpio_set_irq_callback that doesnt care about the pin number*/
	gpio_set_irq_enabled(BUTTON_UP, ExpctdEdges[1], true);
	gpio_set_irq_enabled(BUTTON_TOP_LIMIT, ExpctdEdges[2], true);
	gpio_set_irq_enabled(BUTTON_BOTTOM_LIMIT, ExpctdEdges[3], true);

    /* Create a binary semaphore */
	/* Once created, a semaphore can be used with the xSemaphoreTake and xSemaphoreGive functions to control access to the shared resource */
 	buttonSemaphore = xSemaphoreCreateBinary();

	TickType_t xTaskStartTime;
	const TickType_t xTaskPeriod = pdMS_TO_TICKS(BUTTON_TASK_PERIOD);

	xTaskStartTime = xTaskGetTickCount();

	for( ;; )
	{
		if(UpDown_ButtonInfo.pending)
		{
			/* If button pressed: (falling edge)*/
			if((UpDown_ButtonInfo.edge & GPIO_IRQ_EDGE_FALL) == GPIO_IRQ_EDGE_FALL)
			{
				switch (UpDown_ButtonInfo.gpio)
				{
					case BUTTON_UP:
					case BUTTON_DOWN:
						if(xSemaphoreGive(buttonSemaphore) == pdTRUE)
						{
							MotorState_Requested = STATE_OFF;
						}
						break;
					default: break;
				}
			}
			/* If button released: (rising edge)*/
			else if((UpDown_ButtonInfo.edge & GPIO_IRQ_EDGE_RISE) == GPIO_IRQ_EDGE_RISE)
			{
				switch (UpDown_ButtonInfo.gpio)
				{
					case BUTTON_DOWN:
						LOG("DOWN BUTTON PRESSED! \n");
						if((xSemaphoreGive(buttonSemaphore) == pdTRUE) && (!BottomLimitReached))
						{
							MotorState_Requested = STATE_CLOCKWISE;
						}
						break;
					case BUTTON_UP:
						LOG("UP BUTTON PRESSED! \n");
						if((xSemaphoreGive(buttonSemaphore) == pdTRUE) && (!TopLimitReached))
						{
							MotorState_Requested = STATE_ANTICLOCKWISE;
						}
						break;
					default: break;
				}
			}
			UpDown_ButtonInfo.pending = false;
		}
		if(Limitter_ButtonInfo.pending)
		{
			/* If button pressed: (falling edge)*/
			if((Limitter_ButtonInfo.edge & GPIO_IRQ_EDGE_FALL) == GPIO_IRQ_EDGE_FALL)
			{
				switch (Limitter_ButtonInfo.gpio)
				{
					case BUTTON_TOP_LIMIT:
						LOG("CLEAR OF THE TOP LIMIT! \n");
						TopLimitReached = false;
						break;

					case BUTTON_BOTTOM_LIMIT:
						LOG("CLEAR OF THE BOTTOM LIMIT! \n");
						BottomLimitReached = false;
						break;
					default: break;
				}
			}
			/* If button released: (rising edge)*/
			else if((Limitter_ButtonInfo.edge & GPIO_IRQ_EDGE_RISE) == GPIO_IRQ_EDGE_RISE)
			{
				switch (Limitter_ButtonInfo.gpio)
				{
					case BUTTON_TOP_LIMIT:
						LOG("TOP LIMIT REACHED! \n");
						TopLimitReached = true;
						if(xSemaphoreGive(buttonSemaphore) == pdTRUE)
						{
							MotorState_Requested = STATE_OFF;
						}
						break;

					case BUTTON_BOTTOM_LIMIT:
						LOG("BOTTOM LIMIT REACHED! \n");
						BottomLimitReached = true;
						
						if(xSemaphoreGive(buttonSemaphore) == pdTRUE)
						{
							MotorState_Requested = STATE_OFF;
						}
						break;
					default: break;
				}
			}
			Limitter_ButtonInfo.pending = false;
		}

		vTaskDelayUntil(&xTaskStartTime, xTaskPeriod);
	}
}

/** 
 * 	TODO:
 * 		- Bugfix - when hits the bottom limitter, and then u press UP button, it gets stuck in UP state
 * 		- Bugfix - bottom limitter works 99% of the time, but sometimes it still allows to go down - IMPORTANT FIX
 * 				   I've notice it happens when you: go down, hit the limitter, and then very quicky press down again
 * 		- Bugfix - if you hit a limitter and cycle the power, then the system doesnt know the limit switch is pressed since it works on interrupts
 * 		- Bugfix - sometimes the button press is detected as event 12 (both rising and falling edge), find a way to separate that, it messes up
 * 			       the logic in some places coz for example events = 12 so which one should I detect it as, rising or falling? Figure that out
 * 
*/
