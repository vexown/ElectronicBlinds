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
#include "hardware/timer.h"

/* Task includes */
#include "ButtonTask.h"
#include "MotorControllerTask.h"
#include "AutomaticControlTask.h"

/* Includes from the DS1307 library */
#include "DS1307.h"
#include "I2C_Driver.h"

/* Hardware setup function */
static void prvSetupHardware( void );

/* Prototypes for the standard FreeRTOS callback/hook functions implemented within this file. */
void vApplicationMallocFailedHook( void );
void vApplicationIdleHook( void );
void vApplicationStackOverflowHook( TaskHandle_t pxTask, char *pcTaskName );
void vApplicationTickHook( void );

/*----------------FUNCTION DECLARATION----------------------*/


/*---------------GLOBAL VARIABLES DECLARATION---------------*/
bool buttonDown_InitState, buttonUp_InitState, buttonTopLimit_InitState, buttonBottomLimit_InitState;

void main( void )
{
    /* Startup delay - wait until the whole system is stabilized before starting */
    sleep_ms(4000);

    /* Configure the Raspberry Pico hardware for blinky */
    prvSetupHardware();

    /* Get initial pin states: */
	uint8_t consistentReads;
	/* Read each pin multiple times to get a stable result */
	for(uint8_t i = 0; i < 100; i++ ){ 
		gpio_get(BUTTON_DOWN) ? consistentReads++ : 0;
	}
	buttonDown_InitState = (consistentReads >= 70) ? 1 : 0;
    LOG("buttonDown_InitState = %d \n", buttonDown_InitState);
	consistentReads = 0;
	for(uint8_t i = 0; i < 100; i++ ){
		gpio_get(BUTTON_UP) ? consistentReads++ : 0;
	}
	buttonUp_InitState = (consistentReads >= 70) ? 1 : 0;
    LOG("buttonUp_InitState = %d \n", buttonUp_InitState);
	consistentReads = 0;
	for(uint8_t i = 0; i < 100; i++ ){
		gpio_get(BUTTON_TOP_LIMIT) ? consistentReads++ : 0;
	}
	buttonTopLimit_InitState = (consistentReads >= 70) ? 1 : 0;
    LOG("buttonTopLimit_InitState = %d \n", buttonTopLimit_InitState);
	consistentReads = 0;
	for(uint8_t i = 0; i < 100; i++ ){
		gpio_get(BUTTON_BOTTOM_LIMIT) ? consistentReads++ : 0;
	}
	buttonBottomLimit_InitState = (consistentReads >= 70) ? 1 : 0;
    LOG("buttonBottomLimit_InitState = %d \n", buttonBottomLimit_InitState);

	/* Reset the I2C0 controller to get a fresh clear state */
	Reset_I2C0();
    /* Initial Configuration of the I2C0 */
    I2C_Initialize(I2C_FAST_MODE);

	(void)setupPinsI2C0();
	(void)Disable_DS1307_SquareWaveOutput();
	(void)Enable_DS1307_Oscillator();
#if (SPECIAL_BUILD_FOR_SETTING_DATE == 1)
	(void)SetCurrentDate((const char*)__DATE__, (const char*)__TIME__ );
#endif

/*
    while (1) 
    {
		printf("Going into I2C reg read... \n");
		for(int i = 0; i<=7; i++)
		{
			printf("Read reg %x = 0x%x \n", i, I2C_Register_Read(i));
		}
		sleep_ms(1000);
        printf("/n");
    }
*/

	/* Create the tasks */
	xTaskCreate( MotorControllerTask,"MotorControllerTask",configMINIMAL_STACK_SIZE,NULL,MOTOR_CONTROLLER_TASK_PRIORITY, NULL );								
	xTaskCreate( ButtonTask, "ButtonTask", configMINIMAL_STACK_SIZE, NULL, BUTTON_TASK_PRIORITY, NULL );
    xTaskCreate( AutomaticControlTask, "AutomaticControlTask", configMINIMAL_STACK_SIZE, NULL, AUTOMATIC_CONTROL_TASK_PRIORITY, NULL );

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

/*-----------------------------------------------------------*/

static void prvSetupHardware( void )
{
    stdio_init_all();

    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
    gpio_put(PICO_DEFAULT_LED_PIN, 0);

    gpio_init(SOURCE_3V3_1);
    gpio_set_dir(SOURCE_3V3_1, GPIO_OUT);
    gpio_put(SOURCE_3V3_1, 0);

    gpio_init(SOURCE_3V3_2);
    gpio_set_dir(SOURCE_3V3_2, GPIO_OUT);
    gpio_put(SOURCE_3V3_2, 0);

	gpio_init(SOURCE_3V3_3);
    gpio_set_dir(SOURCE_3V3_3, GPIO_OUT);
    gpio_put(SOURCE_3V3_3, 0);

    gpio_init(SOURCE_3V3_4);
    gpio_set_dir(SOURCE_3V3_4, GPIO_OUT);
    gpio_put(SOURCE_3V3_4, 0);

    gpio_init(BUTTON_UP);
    gpio_set_dir(BUTTON_UP, GPIO_IN);
    gpio_set_pulls(BUTTON_UP, true, false);

    gpio_init(BUTTON_DOWN);
    gpio_set_dir(BUTTON_DOWN, GPIO_IN);
    gpio_set_pulls(BUTTON_DOWN, true, false);

	gpio_init(BUTTON_TOP_LIMIT);
    gpio_set_dir(BUTTON_TOP_LIMIT, GPIO_IN);
    gpio_set_pulls(BUTTON_TOP_LIMIT, true, false);

    gpio_init(BUTTON_BOTTOM_LIMIT);
    gpio_set_dir(BUTTON_BOTTOM_LIMIT, GPIO_IN);
    gpio_set_pulls(BUTTON_BOTTOM_LIMIT, true, false);
    
    gpio_init(MOTOR_CONTROL_1);
    gpio_set_dir(MOTOR_CONTROL_1, GPIO_OUT);
    gpio_put(MOTOR_CONTROL_1, 0);

    gpio_init(MOTOR_CONTROL_2);
    gpio_set_dir(MOTOR_CONTROL_2, GPIO_OUT);
    gpio_put(MOTOR_CONTROL_2, 0);

}
/*-----------------------------------------------------------*/

void vApplicationMallocFailedHook( void )
{
    /* Called if a call to pvPortMalloc() fails because there is insufficient
    free memory available in the FreeRTOS heap.  pvPortMalloc() is called
    internally by FreeRTOS API functions that create tasks, queues, software
    timers, and semaphores.  The size of the FreeRTOS heap is set by the
    configTOTAL_HEAP_SIZE configuration constant in FreeRTOSConfig.h. */

    /* Force an assert. */
    configASSERT( ( volatile void * ) NULL );
}
/*-----------------------------------------------------------*/

void vApplicationStackOverflowHook( TaskHandle_t pxTask, char *pcTaskName )
{
    ( void ) pcTaskName;
    ( void ) pxTask;

    /* Run time stack overflow checking is performed if
    configCHECK_FOR_STACK_OVERFLOW is defined to 1 or 2.  This hook
    function is called if a stack overflow is detected. */

    /* Force an assert. */
    configASSERT( ( volatile void * ) NULL );
}
/*-----------------------------------------------------------*/

void vApplicationIdleHook( void )
{
    volatile size_t xFreeHeapSpace;

    /* This is just a trivial example of an idle hook.  It is called on each
    cycle of the idle task.  It must *NOT* attempt to block.  In this case the
    idle task just queries the amount of FreeRTOS heap that remains.  See the
    memory management section on the http://www.FreeRTOS.org web site for memory
    management options.  If there is a lot of heap memory free then the
    configTOTAL_HEAP_SIZE value in FreeRTOSConfig.h can be reduced to free up
    RAM. */
    xFreeHeapSpace = xPortGetFreeHeapSize();

    /* Remove compiler warning about xFreeHeapSpace being set but never used. */
    ( void ) xFreeHeapSpace;
}
/*-----------------------------------------------------------*/

void vApplicationTickHook( void )
{


}
/** 
 * 	TODO:
 * 		- Add if pressed for 2,3 sec goes all the way up/down on its own (add top/bottom edge detection first)
 * 		- Add OLED screen
 * 		- Add brightness level detection
 * 		- Add RTC and auto raise/lower at sunrise/sunset (maybe not necessary with brightness level detection?)
 * 		- Press both buttons to activate/deactivate auto mode
 * 		- Add bluetooth control? (maybe too much lol)
*/