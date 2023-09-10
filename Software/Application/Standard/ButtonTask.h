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
static void timerInit(uint32_t delay_us, uint8_t timerNum);
static void alarm0_InterruptHandler(void);
static void alarm1_InterruptHandler(void);
static void alarm2_InterruptHandler(void);


#endif /* BUTTONTASK_H */