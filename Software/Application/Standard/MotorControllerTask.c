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

#include "MotorControllerTask.h"
#include "ButtonTask.h"

MotorState_t CurrentState = STATE_OFF;

// State function implementations
void stateOFF(void) 
{
    printf("In OFF state.\n");

    gpio_put(PICO_DEFAULT_LED_PIN, 0);
    gpio_put(MOTOR_CONTROL_1, 0);
    gpio_put(MOTOR_CONTROL_2, 0);
}

void stateAnticlockwise(void) 
{
    printf("In state Anticlockwise.\n");

    gpio_put(PICO_DEFAULT_LED_PIN, 1);
    gpio_put(MOTOR_CONTROL_1, 1);
    gpio_put(MOTOR_CONTROL_2, 0);
}

void stateClockwise(void) 
{
    printf("In state Clockwise.\n");

    gpio_put(PICO_DEFAULT_LED_PIN, 1);
    gpio_put(MOTOR_CONTROL_1, 0);
    gpio_put(MOTOR_CONTROL_2, 1);
}

// State machine function
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
            printf("Invalid state!\n");
    }
}

void MotorControllerTask( void *pvParameters )
{
	TickType_t xTaskStartTime;
	const TickType_t xTaskPeriod = pdMS_TO_TICKS(BUTTON_TASK_PERIOD);

	xTaskStartTime = xTaskGetTickCount();

	for( ;; )
	{
        if((CurrentState != MotorState_Requested) && (MotorStateChangeSemaphoreObtained == true))
        {
            MotorStateChangeSemaphoreObtained = false;
            stateMachine(MotorState_Requested);
        }

		vTaskDelayUntil(&xTaskStartTime, xTaskPeriod);
	}
}
