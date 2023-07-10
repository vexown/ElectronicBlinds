#ifndef BUTTONTASK_H
#define BUTTONTASK_H

#include "MotorControllerTask.h"

/* Constants and Macros */

/* Data Types */

/* Global Variables */
extern MotorState_t MotorState_Requested;
extern bool MotorStateChangeSemaphoreObtained;

/* Function Declarations */
void ButtonTask( void *pvParameters );
static void buttons_callback(uint gpio, uint32_t events);
static void alarm_irq(void);
static void alarm_in_us(uint32_t delay_us);

#endif /* BUTTONTASK_H */