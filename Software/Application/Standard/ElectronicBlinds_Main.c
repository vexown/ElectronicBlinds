/********************************* SYSTEM *************************************/
/* FreeRTOS V202107.00
 * Copyright (C) 2020 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * http://www.FreeRTOS.org
 * http://aws.amazon.com/freertos
 *
 * 1 tab == 4 spaces!
 *******************************************************************************/

/********************************* HARDWARE ************************************/
/* MPU6050 - world’s first integrated 6-axis MotionTracking device that combines
 * a 3-axis gyroscope, 3-axis accelerometer, and a Digital Motion Processor™ (DMP) 
 * Datasheet: https://invensense.tdk.com/wp-content/uploads/2015/02/MPU-6000-Datasheet1.pdf
 * 
 *******************************************************************************/

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

/*-----------------------------------------------------------*/

/* Main function called by main() from main.c (lol). This one does some setup and starts the scheduler */
void ElectronicBlinds_Main( void );

/*-----------------------------------------------------------*/

/* Tasks declarations */
static void MotorControllerTask( void *pvParameters );
static void ButtonTask( void *pvParameters );

/*-----------------------------------------------------------*/

static int MotorDirection_Requested;
static int MotorDirection_Current;
static uint32_t ExpectedEdgeDir;

/* Semaphore instance for signaling the button press */
SemaphoreHandle_t buttonSemaphore;

void buttons_callback(uint gpio, uint32_t events)
{
	/* Disable the interrupts */ 
	gpio_set_irq_enabled_with_callback(BUTTON_DOWN, ExpectedEdgeDir, false, &buttons_callback);
    gpio_set_irq_enabled(BUTTON_UP, ExpectedEdgeDir, false);

	if((events & GPIO_IRQ_EDGE_FALL) == GPIO_IRQ_EDGE_FALL)
	{
		BaseType_t xHigherPriorityTaskWoken = pdFALSE;

		if(xSemaphoreGiveFromISR(buttonSemaphore, &xHigherPriorityTaskWoken) == pdTRUE)
		{
			MotorDirection_Requested = MOTOR_OFF;
		}
		portYIELD_FROM_ISR(xHigherPriorityTaskWoken);	
	}
	else if((gpio == BUTTON_UP) && ((events & GPIO_IRQ_EDGE_RISE) == GPIO_IRQ_EDGE_RISE))
	{
		
		BaseType_t xHigherPriorityTaskWoken = pdFALSE;

		if(xSemaphoreGiveFromISR(buttonSemaphore, &xHigherPriorityTaskWoken) == pdTRUE)
		{
			MotorDirection_Requested = ANTICLOCKWISE;
		}
		portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
	}
	else if((gpio == BUTTON_DOWN) && ((events & GPIO_IRQ_EDGE_RISE) == GPIO_IRQ_EDGE_RISE))
	{
		
		BaseType_t xHigherPriorityTaskWoken = pdFALSE;

		if(xSemaphoreGiveFromISR(buttonSemaphore, &xHigherPriorityTaskWoken) == pdTRUE)
		{
			MotorDirection_Requested = CLOCKWISE;
		}
		portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
	}

	/* After RISE event a FALL event is expected, and after FALL the next event should be RISE (since you PRESS and RELEASE the button) */
	(ExpectedEdgeDir == GPIO_IRQ_EDGE_RISE) ? (ExpectedEdgeDir = GPIO_IRQ_EDGE_FALL) : (ExpectedEdgeDir = GPIO_IRQ_EDGE_RISE);
	
	/* Re-enable the interrupts*/
	gpio_set_irq_enabled_with_callback(BUTTON_DOWN, ExpectedEdgeDir, true, &buttons_callback);
	gpio_set_irq_enabled(BUTTON_UP, ExpectedEdgeDir, true);
}

void ElectronicBlinds_Main( void )
{
	/* Create a binary semaphore */
	/* Once created, a semaphore can be used with the xSemaphoreTake and xSemaphoreGive functions to control access to the shared resource */
 	buttonSemaphore = xSemaphoreCreateBinary();
	/* Start with semaphore count = 0 */
	xSemaphoreTake(buttonSemaphore, portMAX_DELAY);

	/* Enabled the IRQs for the button pins 
	In Raspberry Pi Pico, only one callback function can be used for GPIO interrupts, even if multiple pins are used. 
	This is because the interrupts are handled at the hardware level and there is only one interrupt handler for all the GPIO pins.*/
	gpio_set_irq_enabled_with_callback(BUTTON_DOWN, GPIO_IRQ_EDGE_RISE, true, &buttons_callback);
	/* For the second and the concurrent GPIOs we dont have to specify the callback - the first GPIO already set the generic callback used for 
	GPIO IRQ events for the current core (see inside the gpio_set_irq... function. There is a function gpio_set_irq_callback that doesnt care about the pin number*/
    gpio_set_irq_enabled(BUTTON_UP, GPIO_IRQ_EDGE_RISE, true);
	
	ExpectedEdgeDir = GPIO_IRQ_EDGE_RISE;

	/* Create the tasks */
	xTaskCreate( MotorControllerTask,"MotorControllerTask",configMINIMAL_STACK_SIZE,NULL,MOTOR_CONTROLLER_TASK_PRIORITY, NULL );								
	xTaskCreate( ButtonTask, "ButtonTask", configMINIMAL_STACK_SIZE, NULL, BUTTON_TASK_PRIORITY, NULL );

	/* Start the tasks and timer running. */
	vTaskStartScheduler();

	/* If all is well, the scheduler will now be running, and the following
	line will never be reached.  If the following line does execute, then
	there was insufficient FreeRTOS heap memory available for the Idle and/or
	timer tasks to be created.  See the memory management section on the
	FreeRTOS web site for more details on the FreeRTOS heap
	http://www.freertos.org/a00111.html. */
	for( ;; );
}

static void ButtonTask( void *pvParameters )
{
	TickType_t xTaskStartTime;
	const TickType_t xTaskPeriod = pdMS_TO_TICKS(BUTTON_TASK_PERIOD);

	xTaskStartTime = xTaskGetTickCount();

	for( ;; )
	{
		/* Attempt to obtain the semaphore - if not available task is blocked for xBlockTime (second arg) */
		
		BaseType_t SemaphoreObtained = xSemaphoreTake(buttonSemaphore, portMAX_DELAY);
		
		if ( SemaphoreObtained && (MotorDirection_Requested == ANTICLOCKWISE) && (MotorDirection_Current != MotorDirection_Requested)) 
		{
			MotorDirection_Current = MotorDirection_Requested;

			gpio_put(PICO_DEFAULT_LED_PIN, 1);
			gpio_put(MOTOR_CONTROL_1, 1);
			gpio_put(MOTOR_CONTROL_2, 0);
		}
		else if (SemaphoreObtained && (MotorDirection_Requested == CLOCKWISE) && (MotorDirection_Current != MotorDirection_Requested)) 
		{
			MotorDirection_Current = MotorDirection_Requested;
			gpio_put(PICO_DEFAULT_LED_PIN, 1);
			gpio_put(MOTOR_CONTROL_1, 0);
			gpio_put(MOTOR_CONTROL_2, 1);
		}
		else if (SemaphoreObtained && (MotorDirection_Requested == MOTOR_OFF) && (MotorDirection_Current != MotorDirection_Requested)) 
		{
			MotorDirection_Current = MotorDirection_Requested;
			gpio_put(PICO_DEFAULT_LED_PIN, 0);
			gpio_put(MOTOR_CONTROL_1, 0);
			gpio_put(MOTOR_CONTROL_2, 0);
		}

		vTaskDelayUntil(&xTaskStartTime, xTaskPeriod);
	}
}

static void MotorControllerTask( void *pvParameters )
{
	TickType_t xTaskStartTime;
	const TickType_t xTaskPeriod = pdMS_TO_TICKS(BUTTON_TASK_PERIOD);

	xTaskStartTime = xTaskGetTickCount();

	for( ;; )
	{
		vTaskDelayUntil(&xTaskStartTime, xTaskPeriod);
	}
}


/** I have connected the controller to the motor, flashed this code and these are my observations:
 *  - Burned the board (idk how, it was working at first but then the chip started getting hot and gg, maybe I accidentaly shorted smthing?)
 * 		(or is there something fundementally wrong with my circuit?)
 *  - I've noticed that turning the motor OFF after release was not really working if one of the buttons? Double check if this feature works correctly
 *    (but maybe it stopped working after the short, I think it was fine at first idk...)
 *  
 * 	TODO:
 * 		- Analyze the software to find why the buttons stick sometimes
 * 
 *  Updates - Printed a stronger shaft + used superglue so it has to stick now hehe.
 * 			- Issue found with button control - sometimes the button "sticks", it doesn't turn off the motor on button release.
*/


