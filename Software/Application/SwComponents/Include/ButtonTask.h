#ifndef BUTTONTASK_H
#define BUTTONTASK_H

/*---------------- INCLUDES ----------------------*/
#include "MotorControllerTask.h"

/*--------------- GLOBAL VARIABLE DECLARATIONS (extern) ---------------*/
extern MotorState_t MotorState_Requested;
extern SemaphoreHandle_t ButtonSemaphore;

/*--------------- GLOBAL FUNCTION DECLARTIONS ---------------*/
void ButtonTask( void *pvParameters );

#endif /* BUTTONTASK_H */