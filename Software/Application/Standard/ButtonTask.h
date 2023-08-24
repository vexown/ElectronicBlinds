#ifndef BUTTONTASK_H
#define BUTTONTASK_H

#include "MotorControllerTask.h"

/* Constants and Macros */
#define PRINTS_ENABLED 0

#if(PRINTS_ENABLED == 1)
#define LOG(...) printf(__VA_ARGS__)
#else
#define LOG(...) 
#endif

/* Data Types */

/* Global Variables */
extern MotorState_t MotorState_Requested;
extern SemaphoreHandle_t buttonSemaphore;

/* Function Declarations */
void ButtonTask( void *pvParameters );
static void buttons_callback(uint gpio, uint32_t events);
static void alarm_irq(void);
static void alarm_in_us(uint32_t delay_us);

#endif /* BUTTONTASK_H */