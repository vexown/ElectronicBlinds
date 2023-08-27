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

/* Includes from the DS1307 library */
#include "DS1307.h"
#include "I2C_Driver.h"

void AutomaticControlTask( void *pvParameters )
{
	TickType_t xTaskStartTime;
	const TickType_t xTaskPeriod = pdMS_TO_TICKS(AUTOMATIC_CONTROL_TASK_PERIOD);

	xTaskStartTime = xTaskGetTickCount();

	for( ;; )
	{
        /* I2C Register Read is set up to read from the DS1307 RTC module. You provide the register you want to read as argument 
            0 - seconds (all values in BDC), 1 - mins, 2 - hours, 3 - unused, 4 - DD, 5 - MM, 6 - YY */
        uint8_t hour = I2C_Register_Read(2);
        uint8_t minute = I2C_Register_Read(1);
        uint8_t isClosed = I2C_Register_Read(8);
        printf("hour:%x minute:%x isClosed:%d \n", hour, minute, isClosed);
        if(((hour >= 21 && hour <= 23) || (hour >= 0 && hour < 9)) && (isClosed == 1)) /* Close the blinds after 9PM */
        {
            gpio_put(MOTOR_CONTROL_1, 0);
            gpio_put(MOTOR_CONTROL_2, 1);
            I2C_Register_Write(8,1);
        }
        else if((hour >= 0x09 && hour < 0x21) && (isClosed == 1)) /* Open the blinds after 9AM */
        {
            gpio_put(MOTOR_CONTROL_1, 1);
            gpio_put(MOTOR_CONTROL_2, 0);
            I2C_Register_Write(8,0);
        }

		vTaskDelayUntil(&xTaskStartTime, xTaskPeriod);
	}
}

