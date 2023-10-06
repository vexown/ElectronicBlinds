/* AutomaticControlTask.c - source file for the OS Task which handles automatic control of the Window Blinds */

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
#include "AutomaticControlTask.h"
#include "ElectronicBlinds_Main.h"
#include "MotorControllerTask.h"
#include "ButtonTask.h"

/* Includes from the DS1307 library */
#include "DS1307.h"
#include "I2C_Driver.h"

/*---------------- GLOBAL FUNCTION DEFINITIONS ----------------------*/

/* TASK MAIN FUNCTION */
void AutomaticControlTask( void *pvParameters )
{
    /* Set up task schedule */
	TickType_t xTaskStartTime;
	const TickType_t xTaskPeriod = pdMS_TO_TICKS(AUTOMATIC_CONTROL_TASK_PERIOD);
	xTaskStartTime = xTaskGetTickCount();

    /* Infinite task loop */
	for( ;; )
	{
        /* Read current hour and minute */
        uint8_t hour = I2C_Register_Read(DS1307_REG_ADDR_HOURS);
        uint8_t minute = I2C_Register_Read(DS1307_REG_ADDR_MINUTES);
        /* Check if the blinds are currently closed (this is stored in RTC's RAM so it persists as long as RTC has power) */
        uint8_t isClosed = I2C_Register_Read(DS1307_REG_ADDR_IS_CLOSED);
        
        LOG("hour:%x minute:%x isClosed:%d \n", hour, minute, isClosed);
        if(((hour >= 0x19 && hour <= 0x23) || (hour >= 0x0 && hour < 0x7)) && (isClosed == 0)) /* Blinds closed */
        {
            stateClockwise(); /* Close the blinds, the motor will stop when it hits bottom limitter */           
            I2C_Register_Write(DS1307_REG_ADDR_IS_CLOSED, BLINDS_CLOSED);
        }
        else if((hour >= 0x07 && hour < 0x19) && (isClosed == 1)) /* Blinds open */
        {
            stateAnticlockwise(); /* Open the blinds, the motor will stop when it hits top limitter */ 
            I2C_Register_Write(DS1307_REG_ADDR_IS_CLOSED, BLINDS_OPEN);
        }

        /* Delay until next cycle of the task */
		vTaskDelayUntil(&xTaskStartTime, xTaskPeriod);
	}
}

