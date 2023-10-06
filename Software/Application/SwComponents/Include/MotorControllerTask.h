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
void stateOFF(void);
void stateAnticlockwise(void);
void stateClockwise(void);

#endif /* MOTORCONTROLLERTASK_H */

