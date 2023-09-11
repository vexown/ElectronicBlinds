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
/* Includes from the DS1307 library */
#include "DS1307.h"
#include "I2C_Driver.h"

typedef struct
{
	uint32_t gpio;
	uint32_t edge;
	bool pending;
}ButtonInfoType;

MotorState_t MotorState_Requested = STATE_OFF;
static bool TopLimitReached, BottomLimitReached;
SemaphoreHandle_t buttonSemaphore;
static ButtonInfoType UpDown_ButtonInfo, Limitter_ButtonInfo;
static uint32_t ExpctdEdges[4]; /* from 0 to 3: BUTTON_DOWN, BUTTON_UP, BUTTON_TOP_LIMIT, BUTTON_BOTTOM_LIMIT - TODO -this is temporary, change to smth more readable later */

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
	else if(timerNum == 2)
	{
		irq_set_exclusive_handler(TIMER_IRQ_2, alarm2_InterruptHandler);
		irq_set_enabled(TIMER_IRQ_2, true);
	}
	else
	{
		LOG("Wrong timerNum - no handler function defined! \n");
	}

	/* Set the alarm time */
    uint64_t target = timer_hw->timerawl + delay_us;
    timer_hw->alarm[timerNum] = (uint32_t) target;
}

static void alarm0_InterruptHandler(void) 
{
    /* Clear the alarm irq */
    hw_clear_bits(&timer_hw->intr, 1u << 0);

	/* If the button is still high/low after debouncing delay, count it, otherwise it's treated as noise and ignored */
	bool GPIO_State = gpio_get(UpDown_ButtonInfo.gpio);
	if(UpDown_ButtonInfo.edge == GPIO_IRQ_EDGE_RISE && GPIO_State)
	{ /* Stable button press */
		LOG("stable \n");
		UpDown_ButtonInfo.pending = true;
		/* set a timer for another cycle to see if the button is still pressed (this is repeated until it's released) */
		timerInit(DEBOUNCING_DELAY_IN_US, 0);
	}
	else if(!GPIO_State)
	{ /* Button has been released (or it was noise)*/
		LOG("released \n");
		UpDown_ButtonInfo.pending = true;
		UpDown_ButtonInfo.edge = GPIO_IRQ_EDGE_FALL;

		/* Re-enable the interrupts*/
		LOG("EI \n");
		gpio_set_irq_enabled_with_callback(BUTTON_DOWN, GPIO_IRQ_EDGE_RISE, true, &buttons_callback);
		gpio_set_irq_enabled(BUTTON_UP, GPIO_IRQ_EDGE_RISE, true);
	}

}

static void alarm1_InterruptHandler(void) 
{
    /* Clear the alarm irq */
    hw_clear_bits(&timer_hw->intr, 1u << 1);
	
	/* It is assumed here, that when this timer goes off, the blinds have cleared the limit switch and are in the normal range of motion */
	Limitter_ButtonInfo.pending = true;
	if(gpio_get(Limitter_ButtonInfo.gpio))
	{
		Limitter_ButtonInfo.edge = GPIO_IRQ_EDGE_RISE;
		if(Limitter_ButtonInfo.gpio == BUTTON_TOP_LIMIT)
		{
			timerInit(TOP_LIMIT_SWITCH_REACTION_DURATION_IN_US, 1);
		}
		else
		{
			timerInit(BOTTOM_LIMIT_SWITCH_REACTION_DURATION_IN_US, 1);
		}
		
	}
	else
	{
		Limitter_ButtonInfo.edge = GPIO_IRQ_EDGE_FALL;
	}
	
	/* Re-enable the interrupts*/
	ExpctdEdges[0] = GPIO_IRQ_EDGE_RISE;
	ExpctdEdges[1] = GPIO_IRQ_EDGE_RISE;
	ExpctdEdges[2] = GPIO_IRQ_EDGE_RISE;
	ExpctdEdges[3] = GPIO_IRQ_EDGE_RISE;
	LOG("EI1 \n");
	gpio_set_irq_enabled_with_callback(BUTTON_TOP_LIMIT, ExpctdEdges[2], true, &buttons_callback);
	gpio_set_irq_enabled(BUTTON_BOTTOM_LIMIT, ExpctdEdges[3], true);
	gpio_set_irq_enabled(BUTTON_DOWN, ExpctdEdges[0], true);
	gpio_set_irq_enabled(BUTTON_UP, ExpctdEdges[1], true);

}

static void alarm2_InterruptHandler(void) 
{
    /* Clear the alarm irq */
    hw_clear_bits(&timer_hw->intr, 1u << 2);

	/* If the button is still high/low after debouncing delay, count it, otherwise it's treated as noise and ignored */
	if(Limitter_ButtonInfo.edge == GPIO_IRQ_EDGE_RISE && gpio_get(Limitter_ButtonInfo.gpio))
	{
		LOG("DEBOUNCED LIMIT SWITCH PRESS \n");
		Limitter_ButtonInfo.pending = true;

		/* Set a timer for reaction to hitting the limit switch - which is turn off all button interrupts, reverse the motor
			and after the below delay, reenable interrupts and stop the motor (switch release is never detected, it is implied is was released)  */
		if(Limitter_ButtonInfo.gpio == BUTTON_TOP_LIMIT)
		{
			timerInit(TOP_LIMIT_SWITCH_REACTION_DURATION_IN_US, 1);
		}
		else
		{
			timerInit(BOTTOM_LIMIT_SWITCH_REACTION_DURATION_IN_US, 1);
		}
		
	}
	else
	{
		ExpctdEdges[0] = GPIO_IRQ_EDGE_RISE;
		ExpctdEdges[1] = GPIO_IRQ_EDGE_RISE;
		ExpctdEdges[2] = GPIO_IRQ_EDGE_RISE;
		ExpctdEdges[3] = GPIO_IRQ_EDGE_RISE;
		LOG("EI1 \n");
		gpio_set_irq_enabled_with_callback(BUTTON_TOP_LIMIT, ExpctdEdges[2], true, &buttons_callback);
		gpio_set_irq_enabled(BUTTON_BOTTOM_LIMIT, ExpctdEdges[3], true);
		gpio_set_irq_enabled(BUTTON_DOWN, ExpctdEdges[0], true);
		gpio_set_irq_enabled(BUTTON_UP, ExpctdEdges[1], true);
	}
	
}

void buttons_callback(uint gpio, uint32_t events)
{
	LOG("GPIO: %d, EVENT: %d \n", gpio, events);
	/* Disable the interrupts */ 
	LOG("DI \n");
	if((gpio == BUTTON_DOWN) || (gpio == BUTTON_UP))
	{
		gpio_set_irq_enabled_with_callback(BUTTON_DOWN, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, false, &buttons_callback);
		gpio_set_irq_enabled(BUTTON_UP, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, false);

		UpDown_ButtonInfo.pending = false;
		UpDown_ButtonInfo.gpio = gpio;
		UpDown_ButtonInfo.edge = events;

		/* Set a timer for debouncing delay - during that time interrupts are disabled - no button presses detected */
		timerInit(DEBOUNCING_DELAY_IN_US, 0);
	}
	else if((gpio == BUTTON_BOTTOM_LIMIT) || (gpio == BUTTON_TOP_LIMIT))
	{
		if(events == GPIO_IRQ_EDGE_RISE)
		{
			gpio_set_irq_enabled_with_callback(BUTTON_TOP_LIMIT, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, false, &buttons_callback);
			gpio_set_irq_enabled(BUTTON_BOTTOM_LIMIT, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, false);
			gpio_set_irq_enabled(BUTTON_DOWN, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, false);
			gpio_set_irq_enabled(BUTTON_UP, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, false);

			Limitter_ButtonInfo.pending = false;
			Limitter_ButtonInfo.gpio = gpio;
			Limitter_ButtonInfo.edge = events;

			timerInit(DEBOUNCING_DELAY_IN_US, 2);
		}
		else
		{
			LOG("INCORRECT LIMIT SWITCH EVENT!"); /* This should never happen */
		}
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

	buttonDown_InitState ? (ExpctdEdges[0] = GPIO_IRQ_EDGE_FALL) : (ExpctdEdges[0] = GPIO_IRQ_EDGE_RISE);
	buttonUp_InitState ? (ExpctdEdges[1] = GPIO_IRQ_EDGE_FALL) : (ExpctdEdges[1] = GPIO_IRQ_EDGE_RISE);
	buttonTopLimit_InitState ? (ExpctdEdges[2] = GPIO_IRQ_EDGE_FALL, TopLimitReached = true) : (ExpctdEdges[2] = GPIO_IRQ_EDGE_RISE);
	buttonBottomLimit_InitState ? (ExpctdEdges[3] = GPIO_IRQ_EDGE_FALL, BottomLimitReached = true) : (ExpctdEdges[3] = GPIO_IRQ_EDGE_RISE);

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
		/* This if statement for the limit switches has to be executed first in this task, since there is only 1 semaphore to take for the state change,
		   and limit switches shall have the priority to set the OFF State when limit is reached  */
		if(Limitter_ButtonInfo.pending)
		{
			/* If button released: (falling edge)*/
			if((Limitter_ButtonInfo.edge & GPIO_IRQ_EDGE_FALL) == GPIO_IRQ_EDGE_FALL)
			{
				switch (Limitter_ButtonInfo.gpio)
				{
					case BUTTON_TOP_LIMIT:
						LOG("CLEAR OF THE TOP LIMIT! \n");
						TopLimitReached = false;
						if(xSemaphoreGive(buttonSemaphore) == pdTRUE)
						{
							MotorState_Requested = STATE_OFF;
						}
						break;

					case BUTTON_BOTTOM_LIMIT:
						LOG("CLEAR OF THE BOTTOM LIMIT! \n");
						BottomLimitReached = false;
						if(xSemaphoreGive(buttonSemaphore) == pdTRUE)
						{
							MotorState_Requested = STATE_OFF;
						}
						break;
					default: break;
				}
			}
			/* If button pressed: (rising edge)*/
			else if((Limitter_ButtonInfo.edge & GPIO_IRQ_EDGE_RISE) == GPIO_IRQ_EDGE_RISE)
			{
				switch (Limitter_ButtonInfo.gpio)
				{
					case BUTTON_TOP_LIMIT:
						LOG("TOP LIMIT REACHED! \n");
						TopLimitReached = true;
						if(xSemaphoreGive(buttonSemaphore) == pdTRUE)
						{
							MotorState_Requested = STATE_CLOCKWISE;
						}
						break;

					case BUTTON_BOTTOM_LIMIT:
						LOG("BOTTOM LIMIT REACHED! \n");
						BottomLimitReached = true;
						
						if(xSemaphoreGive(buttonSemaphore) == pdTRUE)
						{
							MotorState_Requested = STATE_ANTICLOCKWISE;
						}
						break;
					default: break;
				}
			}
			Limitter_ButtonInfo.pending = false;
		}

		if(UpDown_ButtonInfo.pending)
		{
			LOG("TL%d \n", TopLimitReached);
			LOG("BL%d \n", BottomLimitReached);
			/* If button released: (falling edge)*/
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
			/* If button pressed: (rising edge)*/
			else if((UpDown_ButtonInfo.edge & GPIO_IRQ_EDGE_RISE) == GPIO_IRQ_EDGE_RISE)
			{
				switch (UpDown_ButtonInfo.gpio)
				{
					case BUTTON_DOWN:
						if((xSemaphoreGive(buttonSemaphore) == pdTRUE) && (!BottomLimitReached))
						{
							MotorState_Requested = STATE_CLOCKWISE;
						}
						break;
					case BUTTON_UP:
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

		vTaskDelayUntil(&xTaskStartTime, xTaskPeriod);
	}
}

/** 
 * 	TODO:
 * 		- Bugfix - when hits the bottom limitter, and then u press UP button, it gets stuck in UP state (possibly fixed when prints disabled but keep an eye)
 * 		- Bugfix - VERY IMPORTANT - ADD MORE IMMEDIATE REACTION FOR LIMITTERS COZ RIGHT NOW IM OVERSHOOTING BOTTOM AND RAMMING INTO TOP!
 * 				 - POSSIBLE MECHANICAL SOLUTION - ADD SPRING AT THE TOP LIMITTER!
 * 
*/
