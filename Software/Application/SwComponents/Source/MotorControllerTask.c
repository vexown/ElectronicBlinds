/* MotorControllerTask.c - source file for the OS Task which handles the state of the Motor Controller */

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
#include "ElectronicBlinds_Main.h"
#include "MotorControllerTask.h"
#include "ButtonTask.h"

/*---------------- FILE-SCOPE, STATIC STORAGE DURATION VARIABLES (DECL & DEF) ----------------------*/

MotorState_t CurrentState = STATE_OFF;

/*---------------- LOCAL FUNCTION DEFINITIONS ----------------------*/

/* State machine function */
void stateMachine(MotorState_t state) 
{
    CurrentState = state;

    switch (state) 
    {
        case STATE_OFF:
            stateOFF();
            break;
        case STATE_ANTICLOCKWISE:
            stateAnticlockwise();
            break;
        case STATE_CLOCKWISE:
            stateClockwise();
            break;
        default:
            LOG("Invalid state!\n");
    }
}

/*---------------- GLOBAL FUNCTION DEFINITIONS ----------------------*/

/* State functions: */
void stateOFF(void) 
{
    LOG("OFF\n");
    gpio_put(PICO_DEFAULT_LED_PIN, 0);
    gpio_put(MOTOR_CONTROL_1, 0);
    gpio_put(MOTOR_CONTROL_2, 0);
}

void stateAnticlockwise(void) 
{
    LOG("anticlockwise.\n");
    gpio_put(PICO_DEFAULT_LED_PIN, 1);
    gpio_put(MOTOR_CONTROL_1, 0);
    gpio_put(MOTOR_CONTROL_2, 1);
}

void stateClockwise(void) 
{
    LOG("clockwise\n");
    gpio_put(PICO_DEFAULT_LED_PIN, 1);
    gpio_put(MOTOR_CONTROL_1, 1);
    gpio_put(MOTOR_CONTROL_2, 0);
}

/* TASK MAIN FUNCTION */
void MotorControllerTask( void *pvParameters )
{
    /* Set up task schedule */
	TickType_t xTaskStartTime;
	const TickType_t xTaskPeriod = pdMS_TO_TICKS(BUTTON_TASK_PERIOD);
	xTaskStartTime = xTaskGetTickCount();

	/* Infinite task loop */
	for( ;; )
	{
		/* Attempt to obtain the semaphore - if not available task is blocked for xBlockTime (second arg) */
		BaseType_t SemaphoreObtained = xSemaphoreTake(ButtonSemaphore, portMAX_DELAY);
        if((CurrentState != MotorState_Requested) && (SemaphoreObtained))
        {
            stateMachine(MotorState_Requested);
        }
        /* Delay until next cycle of the task */
		vTaskDelayUntil(&xTaskStartTime, xTaskPeriod);
	}
}
