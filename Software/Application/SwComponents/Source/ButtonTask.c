/* ButtonTask.c - source file for the OS Task which handles button presses */

/*---------------- INCLUDES ----------------------*/

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
#include "hardware/timer.h"

/* Include files from other tasks */
#include "ButtonTask.h"
#include "ElectronicBlinds_Main.h"
#include "MotorControllerTask.h"

/* Includes from the DS1307 library */
#include "DS1307.h"
#include "I2C_Driver.h"

/*---------------- LOCAL DATA TYPES ----------------------*/

typedef enum gpio_irq_level gpioLevelType;
typedef struct
{
	uint32_t gpio;
	uint32_t edge;
	bool pending;
}ButtonInfo_t;

typedef struct
{
	gpioLevelType ButtonDown;
	gpioLevelType ButtonUp;
	gpioLevelType TopLimitSwitch;
	gpioLevelType BottomLimitSwitch;
}ExpectedEdge_t;

typedef enum
{
	TIMER_UPDOWNBUTTONS,
	TIMER_LIMITSWITCHES
}TimerNum_t;

/*---------------- FILE-SCOPE, STATIC STORAGE DURATION VARIABLES (DECL & DEF) ----------------------*/

bool TopLimitReached, BottomLimitReached;
MotorState_t MotorState_Requested = STATE_OFF;
SemaphoreHandle_t ButtonSemaphore;
ButtonInfo_t UpDown_ButtonInfo, Limitter_ButtonInfo;
ExpectedEdge_t ExpctdEdges;

/*---------------- LOCAL FUNCTION DECLARATIONS ----------------------*/

void ButtonsInterruptCallback(uint gpio, uint32_t events);
void TimerInit(uint32_t delay_us, TimerNum_t timerNum);
void TimerHandler_UpDownButtons(void);
void TimerHandler_LimitSwitches(void);
void RecoveryMode(uint32_t button);
void DisableAllInterrupts(void);
void EnableAllInterrupts(void);

/*---------------- LOCAL FUNCTION DEFINITIONS ----------------------*/

void DisableAllInterrupts(void)
{
	/* In Raspberry Pi Pico, only one callback function can be used for GPIO interrupts, even if multiple pins are used. 
	This is because the interrupts are handled at the hardware level and there is only one interrupt handler for all the GPIO pins.*/
	gpio_set_irq_enabled_with_callback(BUTTON_TOP_LIMIT, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, false, &ButtonsInterruptCallback);
	gpio_set_irq_enabled(BUTTON_BOTTOM_LIMIT, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, false);
	gpio_set_irq_enabled(BUTTON_DOWN, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, false);
	gpio_set_irq_enabled(BUTTON_UP, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, false);	
}

void EnableAllInterrupts(void)
{
	/* In Raspberry Pi Pico, only one callback function can be used for GPIO interrupts, even if multiple pins are used. 
	This is because the interrupts are handled at the hardware level and there is only one interrupt handler for all the GPIO pins.*/
	gpio_set_irq_enabled_with_callback(BUTTON_TOP_LIMIT, ExpctdEdges.TopLimitSwitch, true, &ButtonsInterruptCallback);
	gpio_set_irq_enabled(BUTTON_BOTTOM_LIMIT, ExpctdEdges.BottomLimitSwitch, true);
	gpio_set_irq_enabled(BUTTON_DOWN, ExpctdEdges.ButtonDown, true);
	gpio_set_irq_enabled(BUTTON_UP, ExpctdEdges.ButtonUp, true);
}

void RecoveryMode(uint32_t button)
{
	/* Disable all interrupts until system is recovered (limit switches not pressed) */ 
	DisableAllInterrupts();

	/* Limit Switch has been cleared - back to normal operation */
	if(button == BUTTON_TOP_LIMIT)
	{
		while(gpio_get(BUTTON_TOP_LIMIT)) // as long as limit is hit - back up
		{
			stateClockwise();
		}
		ExpctdEdges.TopLimitSwitch = GPIO_IRQ_EDGE_RISE;
	}
	else
	{
		while(gpio_get(BUTTON_BOTTOM_LIMIT)) // as long as limit is hit - back up
		{
			stateAnticlockwise();
		}
		ExpctdEdges.BottomLimitSwitch = GPIO_IRQ_EDGE_RISE;
	}
	/* Limit Switches should be clear now, stop the motor */
    stateOFF();
}

void TimerInit(uint32_t delay_us, TimerNum_t timerNum)
{
	/* Enabling interrupts in the timer hardware ensures that the timer will 
		generate an interrupt signal when it reaches a certain condition (e.g., when the specified delay is reached) */
    hw_set_bits(&timer_hw->inte, 1u << timerNum);

	if(timerNum == TIMER_UPDOWNBUTTONS)
	{
		/* Set up interrupt handler for the selected timer */
    	irq_set_exclusive_handler(TIMER_IRQ_0, TimerHandler_UpDownButtons);
		/* Enabling interrupt handling by the Interrupt Controller allows the software to catch and process those interrupts 
		when they occur on the specified IRQ lines. This enables the software to respond to timer events or any other 
		hardware events that trigger interrupts */
		irq_set_enabled(TIMER_IRQ_0, true);
	}
	else if(timerNum == TIMER_LIMITSWITCHES)
	{
    	irq_set_exclusive_handler(TIMER_IRQ_1, TimerHandler_LimitSwitches);
		irq_set_enabled(TIMER_IRQ_1, true);
	}
	else
	{
		/* Unhandled timer, do nothing */
		LOG("Wrong timerNum - no handler function defined! \n");
	}

	/* Calculate the alarm time by adding the provided delay to the current time
		THIS ASSUMES THE TIMER IS INCREMENTING BY 1 EACH MICROSECOND - TO BE VERIFIED WITH DOCUMENTATION */
    uint64_t alarmTargetTime = timer_hw->timerawl + delay_us;
	/* Everything is configured - now just set the alarm to the calculated target 
		time and an interrupt will happen when it is reached */
    timer_hw->alarm[timerNum] = (uint32_t)alarmTargetTime;
}

void TimerHandler_UpDownButtons(void) 
{
	/* Clear interrupt in the timer hardware */
    hw_clear_bits(&timer_hw->intr, 1u << TIMER_UPDOWNBUTTONS);

	/* If the button is still high/low after debouncing delay, count it, otherwise it's treated as noise and ignored */
	bool GPIO_State = gpio_get(UpDown_ButtonInfo.gpio);
	if(UpDown_ButtonInfo.edge == GPIO_IRQ_EDGE_RISE && GPIO_State)
	{ /* Stable button press */
		LOG("button stable \n");
		/* Set button state as pending to be handled */
		UpDown_ButtonInfo.pending = true;
		/* Set a timer for another cycle to see if the button is still pressed (this is repeated until it's released) */
		TimerInit(DEBOUNCING_DELAY_IN_US, TIMER_UPDOWNBUTTONS);
	}
	else if(!GPIO_State)
	{ /* Button has been released (or it was noise) */
		LOG("button released or noise \n");
		/* Set button state as pending to be handled as button release */
		UpDown_ButtonInfo.edge = GPIO_IRQ_EDGE_FALL;
		UpDown_ButtonInfo.pending = true;

		/* Re-enable the interrupts - button press concluded */
		gpio_set_irq_enabled_with_callback(BUTTON_DOWN, GPIO_IRQ_EDGE_RISE, true, &ButtonsInterruptCallback);
		gpio_set_irq_enabled(BUTTON_UP, GPIO_IRQ_EDGE_RISE, true);
	}
}

void TimerHandler_LimitSwitches(void) 
{
    /* Clear interrupt in the timer hardware */
    hw_clear_bits(&timer_hw->intr, 1u << TIMER_LIMITSWITCHES);
	
	/* If the button is still high/low after debouncing delay, count it, otherwise it's treated as noise and ignored */
	bool GPIO_State = gpio_get(Limitter_ButtonInfo.gpio);
	if(Limitter_ButtonInfo.edge == GPIO_IRQ_EDGE_RISE && GPIO_State)
	{ /* Stable button press */
		LOG("button stable \n");
		Limitter_ButtonInfo.pending = true;
		/* Set a timer for another cycle to see if the button is still pressed (this is repeated until it's released) */
		if(Limitter_ButtonInfo.gpio == BUTTON_TOP_LIMIT)
		{
			TimerInit(LIMIT_SWITCH_REACTION_DURATION_IN_US * 10U, TIMER_LIMITSWITCHES);
		}
		else
		{
			TimerInit(LIMIT_SWITCH_REACTION_DURATION_IN_US, TIMER_LIMITSWITCHES);
		}	
	}
	else if(!GPIO_State)
	{ /* Button has been released (or it was noise)*/
		LOG("button released \n");
		Limitter_ButtonInfo.pending = true;
		Limitter_ButtonInfo.edge = GPIO_IRQ_EDGE_FALL;

		/* Re-enable the interrupts - assume all buttons/switches are released*/
		ExpctdEdges.ButtonDown = GPIO_IRQ_EDGE_RISE;
		ExpctdEdges.ButtonUp = GPIO_IRQ_EDGE_RISE;
		ExpctdEdges.TopLimitSwitch = GPIO_IRQ_EDGE_RISE;
		ExpctdEdges.BottomLimitSwitch = GPIO_IRQ_EDGE_RISE;
		EnableAllInterrupts();
	}

}

void ButtonsInterruptCallback(uint gpio, uint32_t events)
{
	LOG("GPIO: %d, EVENT: %d \n", gpio, events);
	if((gpio == BUTTON_DOWN) || (gpio == BUTTON_UP)) /* Check if the Up/Down buttons are the cause of this interrupt */
	{
		/* Disable the interrupts */ 
		gpio_set_irq_enabled_with_callback(BUTTON_DOWN, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, false, &ButtonsInterruptCallback);
		gpio_set_irq_enabled(BUTTON_UP, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, false);

		UpDown_ButtonInfo.pending = false;
		UpDown_ButtonInfo.gpio = gpio;
		UpDown_ButtonInfo.edge = events;

		/* Set a timer for debouncing delay - during that time interrupts are disabled - no button presses detected */
		TimerInit(DEBOUNCING_DELAY_IN_US, TIMER_UPDOWNBUTTONS);
	}
	else if((gpio == BUTTON_BOTTOM_LIMIT) || (gpio == BUTTON_TOP_LIMIT)) /* Check if the Limit Switches are the cause of this interrupt */
	{
		if(events == GPIO_IRQ_EDGE_RISE) /* system design to only work which limit switch presses (release is never detected by interrupt) */
		{
			/* Disable all interrupts - when limit switch is hit the system takes exclusive control, no user input counts */ 
			DisableAllInterrupts();

			Limitter_ButtonInfo.pending = false;
			Limitter_ButtonInfo.gpio = gpio;
			Limitter_ButtonInfo.edge = events;

			TimerInit(DEBOUNCING_DELAY_IN_US_LIMITTER, TIMER_LIMITSWITCHES);
		}
		else
		{
			LOG("INCORRECT LIMIT SWITCH EVENT!");
		}
	}
	else
	{
		LOG("Button unknown - interrupts not disabled! \n");
	}
}

/*---------------- GLOBAL FUNCTION DEFINITIONS ----------------------*/

/* TASK MAIN FUNCTION */
void ButtonTask( void *pvParameters )
{
	/* Create a binary semaphore */
	/* Once created, a semaphore can be used with the xSemaphoreTake and xSemaphoreGive functions to control access to the shared resource */
	/* Only one entity at the time can request to change the state of the motor */
 	ButtonSemaphore = xSemaphoreCreateBinary();

    /* Decide the expected edges of all switches/buttons based on their initial state (for example if init state is 0, we expect next edge to be 1) */
	buttonDown_InitState ? (ExpctdEdges.ButtonDown = GPIO_IRQ_EDGE_FALL) : (ExpctdEdges.ButtonDown = GPIO_IRQ_EDGE_RISE);
	buttonUp_InitState ? (ExpctdEdges.ButtonUp = GPIO_IRQ_EDGE_FALL) : (ExpctdEdges.ButtonUp = GPIO_IRQ_EDGE_RISE);
	/* If the Limit Switches are detected to be pressed at the start of the system - immedietaly react and roll the blinds to the working range */
	buttonTopLimit_InitState ? (RecoveryMode(BUTTON_TOP_LIMIT)) : (ExpctdEdges.TopLimitSwitch = GPIO_IRQ_EDGE_RISE);
	buttonBottomLimit_InitState ? (RecoveryMode(BUTTON_BOTTOM_LIMIT)) : (ExpctdEdges.BottomLimitSwitch = GPIO_IRQ_EDGE_RISE);
	
	EnableAllInterrupts();

	/* Set up task schedule */
	TickType_t xTaskStartTime;
	const TickType_t xTaskPeriod = pdMS_TO_TICKS(BUTTON_TASK_PERIOD);
	xTaskStartTime = xTaskGetTickCount();

	/* Infinite task loop */
	for( ;; )
	{
		/* This if statement for the limit switches has to be executed first in this task, since there is only 1 semaphore to take for the state change,
		   and limit switches shall have the priority to set the OFF State when limit is reached  */
		/* Limit Switch was activated - needs to be handled */
		if(Limitter_ButtonInfo.pending)
		{
			/* If Limit Switch was released */
			if((Limitter_ButtonInfo.edge & GPIO_IRQ_EDGE_FALL) == GPIO_IRQ_EDGE_FALL)
			{
				switch (Limitter_ButtonInfo.gpio)
				{
					case BUTTON_TOP_LIMIT:
						LOG("CLEAR OF THE TOP LIMIT! \n");
						TopLimitReached = false;
						if(xSemaphoreGive(ButtonSemaphore) == pdTRUE)
						{
							MotorState_Requested = STATE_OFF;
						}
						break;

					case BUTTON_BOTTOM_LIMIT:
						LOG("CLEAR OF THE BOTTOM LIMIT! \n");
						BottomLimitReached = false;
						if(xSemaphoreGive(ButtonSemaphore) == pdTRUE)
						{
							MotorState_Requested = STATE_OFF;
						}
						break;
					default: break;
				}
			}
			/* If Limit Switch was pressed (debounced, stable) */
			else if((Limitter_ButtonInfo.edge & GPIO_IRQ_EDGE_RISE) == GPIO_IRQ_EDGE_RISE)
			{
				switch (Limitter_ButtonInfo.gpio)
				{
					case BUTTON_TOP_LIMIT:
						LOG("TOP LIMIT REACHED! \n");
						TopLimitReached = true;
						if(xSemaphoreGive(ButtonSemaphore) == pdTRUE)
						{
							MotorState_Requested = STATE_CLOCKWISE;
						}
						break;

					case BUTTON_BOTTOM_LIMIT:
						LOG("BOTTOM LIMIT REACHED! \n");
						BottomLimitReached = true;
						if(xSemaphoreGive(ButtonSemaphore) == pdTRUE)
						{
							MotorState_Requested = STATE_ANTICLOCKWISE;
						}
						break;
					default: break;
				}
			}
			/* Limit Switch state change was handled, no longer pending */
			Limitter_ButtonInfo.pending = false;
		}

		/* Up/Down Button was activated - needs to be handled */
		if(UpDown_ButtonInfo.pending)
		{
			LOG("TL%d \n", TopLimitReached);
			LOG("BL%d \n", BottomLimitReached);
			/* If Up/Down Button was released */
			if((UpDown_ButtonInfo.edge & GPIO_IRQ_EDGE_FALL) == GPIO_IRQ_EDGE_FALL)
			{
				switch (UpDown_ButtonInfo.gpio)
				{
					case BUTTON_UP:
					case BUTTON_DOWN:
						if(xSemaphoreGive(ButtonSemaphore) == pdTRUE)
						{
							MotorState_Requested = STATE_OFF;
						}
						break;
					default: break;
				}
			}
			/* If Up/Down Button was pressed (debounced, stable) */
			else if((UpDown_ButtonInfo.edge & GPIO_IRQ_EDGE_RISE) == GPIO_IRQ_EDGE_RISE)
			{
				switch (UpDown_ButtonInfo.gpio)
				{
					case BUTTON_DOWN:
						if((xSemaphoreGive(ButtonSemaphore) == pdTRUE) && (!BottomLimitReached))
						{
							MotorState_Requested = STATE_CLOCKWISE;
						}
						break;
					case BUTTON_UP:
						if((xSemaphoreGive(ButtonSemaphore) == pdTRUE) && (!TopLimitReached))
						{
							MotorState_Requested = STATE_ANTICLOCKWISE;
						}
						break;
					default: break;
				}
			}
			/* Up/Down Button state change was handled, no longer pending */
			UpDown_ButtonInfo.pending = false;
		}

		/* Delay until next cycle of the task */
		vTaskDelayUntil(&xTaskStartTime, xTaskPeriod);
	}
}

/** 
 * 	TODO:
 * 
*/
