#ifndef MOTORCONTROLLERTASK_H
#define MOTORCONTROLLERTASK_H

/* Constants and Macros */

/* Data Types */

typedef enum 
{
    STATE_OFF,
    STATE_CLOCKWISE,
    STATE_ANTICLOCKWISE
} MotorState_t;

/* Function Declarations */
void MotorControllerTask( void *pvParameters );

#endif /* MOTORCONTROLLERTASK_H */

