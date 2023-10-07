/* AutomaticControlTask.c - source file for the OS Task which handles automatic control of the Window Blinds */

/*---------------- INCLUDES ----------------------*/

/* Standard includes. */
#include <stdio.h>
#include <math.h>

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

/*---------------- LOCAL FUNCTION DEFINITIONS ----------------------*/

uint8_t ZellersCongruence(int year, int month, int day) 
{
    if (month < 3) 
    {
        month += 12;
        year--;
    }

    int h = (day + ((13 * (month + 1)) / 5) + year + (year / 4) - (year / 100) + (year / 400)) % 7;
    
    /* Map the result to the corresponding day of the week (Monday = 0, ... , Sunday = 6) */
    return (h + 5) % 7;
}

bool isLeapYear(uint32_t year) 
{
    return ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0));
}

bool isDST(uint32_t yearBCD, uint32_t monthBCD, uint32_t dayBCD)
{
    bool isTodayDST = false;

    uint32_t dayDEC = ConvertBCD(dayBCD, BCD_TO_DEC);
    uint32_t monthDEC = ConvertBCD(monthBCD, BCD_TO_DEC);
    uint32_t yearDEC = ConvertBCD(yearBCD, BCD_TO_DEC) + 2000; /* This code only works for 2nd millennium, should be enough lol */

    /* Check if DST is in effect (last Sunday in March to last Sunday in October) */
    if (monthDEC >= 3 && monthDEC <= 10) 
    {
        int lastSundayInMonth = 31; // Default to last day of the month
        // Check if the current month is March or October
        if (monthDEC == 3 || monthDEC == 10) 
        {
            // Calculate the last Sunday of the month
            while (ZellersCongruence(yearDEC, monthDEC, lastSundayInMonth) != 6) 
            {
                lastSundayInMonth--;
            }
            if((dayDEC >= lastSundayInMonth) && (monthDEC == 3)) /* If first, or any later, day of DST in March */
            {
                isTodayDST = true;
            }
            else if((dayDEC <= lastSundayInMonth) && (monthDEC == 10)) /* If last, or any previous, day of DST in October */
            {
                isTodayDST = true;
            }
            else /* If in March before DST starts (last Sunday) or if in October after DST ends (last Sunday) */
            {
                isTodayDST = false; 
            }
        }
        else /* Months from 4 to 9 */
        {
            isTodayDST = true;
        }
    } 
    else 
    {
        isTodayDST = false; // DST is not in effect outside the specified months
    }

    return isTodayDST;
}

uint32_t CalculateDayOfYear(uint32_t yearBCD, uint32_t monthBCD, uint32_t dayBCD) 
{
    uint32_t dayDEC = ConvertBCD(dayBCD, BCD_TO_DEC);
    uint32_t monthDEC = ConvertBCD(monthBCD, BCD_TO_DEC);
    uint32_t yearDEC = ConvertBCD(yearBCD, BCD_TO_DEC) + 2000; /* This code only works for 2nd millennium, should be enough lol */

    uint32_t daysInMonth[] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    
    if (isLeapYear(yearDEC)) 
    {
        daysInMonth[2] = 29;
    }

    uint32_t currentDay = 0;
    for (uint8_t i = 1; i < monthDEC; i++) 
    {
        currentDay += daysInMonth[i];
    }

    /* Calculated currentDay: */
    currentDay += dayDEC; 

    return currentDay;
}

/* Function to calculate the end of Nautical Twilight during Dawn  */
double CalculateSunrise(double latitude, int dayOfYear) 
{
    double latitudeRad = (latitude * M_PI / 180.0);
    double solarDeclination = -23.44 * (M_PI / 180.0) * cos(2.0 * M_PI * (dayOfYear + 10) / 365.0);
    double hourAngle = acos(-tan(latitudeRad) * tan(solarDeclination));
    double sunriseTime = 12.0 - (hourAngle * 180.0 / M_PI) / 15.0;
    
    return sunriseTime;
}

/* Function to calculate the beginning of Nautical Twilight during Dusk  */
double CalculateSunset(double latitude, int dayOfYear) 
{
    double latitudeRad = (latitude * M_PI / 180.0);
    double solarDeclination = -23.44 * (M_PI / 180.0) * cos(2.0 * M_PI * (dayOfYear + 10) / 365.0);
    double hourAngle = acos(-tan(latitudeRad) * tan(solarDeclination));
    double sunsetTime = 12.0 + (hourAngle * 180.0 / M_PI) / 15.0;

    return sunsetTime;
}

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
        /* Read current hour and minute (warning - will be incorrect during DST since it's adjusted at sunrise/sunset time) */
        uint8_t hour = I2C_Register_Read(DS1307_REG_ADDR_HOURS);
        uint8_t minute = I2C_Register_Read(DS1307_REG_ADDR_MINUTES);
        /* Convert hour and minute to one double value - example: hour = 0x15, minute = 0x53 would be converted to time = 15.53 */
        double time = ConvertBCD(hour, BCD_TO_DEC) + (ConvertBCD(minute, BCD_TO_DEC)/100.0);
        /* Check if the blinds are currently closed (this is stored in RTC's RAM so it persists as long as RTC has power) */
        uint8_t isClosed = I2C_Register_Read(DS1307_REG_ADDR_IS_CLOSED);

        uint8_t day = I2C_Register_Read(DS1307_REG_ADDR_DAYS);
        uint8_t month = I2C_Register_Read(DS1307_REG_ADDR_MONTHS);
        uint8_t year = I2C_Register_Read(DS1307_REG_ADDR_YEARS);
        uint32_t dayOfYear = CalculateDayOfYear(year, month, day);

        double sunrise = CalculateSunrise(LATITUDE_SIEROSZEWICE_NOWA_10, dayOfYear);
        double sunset = CalculateSunset(LATITUDE_SIEROSZEWICE_NOWA_10, dayOfYear);

        /* Adjust hour for DST if needed */
        if(isDST(year, month, day))
        {
            sunrise += 1.0;
            sunset += 1.0;
        }
        
        LOG("sunrise = %lf \n", sunrise);
        LOG("sunset = %lf \n", sunset);
        LOG("time = %lf \n", time);
        LOG("hour:%x minute:%x isClosed:%d \n", hour, minute, isClosed);
        if(((time >= sunset) || (time < sunrise)) && (isClosed == 0)) /* Blinds closed */
        {
            stateClockwise(); /* Close the blinds, the motor will stop when it hits bottom limitter */           
            I2C_Register_Write(DS1307_REG_ADDR_IS_CLOSED, BLINDS_CLOSED); /* Change blinds current state to CLOSED */
        }
        else if((time >= sunrise && time < sunset) && (isClosed == 1)) /* Blinds open */
        {
            stateAnticlockwise(); /* Open the blinds, the motor will stop when it hits top limitter */ 
            I2C_Register_Write(DS1307_REG_ADDR_IS_CLOSED, BLINDS_OPEN); /* Change blinds current state to OPEN */
        }

        /* Delay until next cycle of the task */
		vTaskDelayUntil(&xTaskStartTime, xTaskPeriod);
	}
}

