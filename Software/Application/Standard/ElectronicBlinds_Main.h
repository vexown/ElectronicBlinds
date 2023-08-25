#ifndef ELECTRONICBLINDS_MAIN_H
#define ELECTRONICBLINDS_MAIN_H

extern bool buttonDown_InitState, buttonUp_InitState, buttonTopLimit_InitState, buttonBottomLimit_InitState;

/* Priorities for the tasks */
#define MOTOR_CONTROLLER_TASK_PRIORITY		( tskIDLE_PRIORITY + 1 )
#define	BUTTON_TASK_PRIORITY				( tskIDLE_PRIORITY + 2 )

/* Task periods (ms) */
#define BUTTON_TASK_PERIOD						( 100 )
#define MOTOR_CONTROLLER_TASK_PERIOD			( 100 )

/* The number of items the queue can hold */
#define mainQUEUE_LENGTH					( 1 )

/* By default the MPU6050 devices are on bus address 0x68 */ 
#define MPU6050_I2C_ADDRESS   				 0x68

/* GPIO numbers */
#define BUTTON_UP 14U
#define BUTTON_DOWN 15U
#define BUTTON_TOP_LIMIT 8U
#define BUTTON_BOTTOM_LIMIT 9U
#define SOURCE_3V3_1 10U
#define SOURCE_3V3_2 11U
#define SOURCE_3V3_3 12U
#define SOURCE_3V3_4 13U
#define MOTOR_CONTROL_1 16U
#define MOTOR_CONTROL_2 17U

/* Misc */
#define GPIO_INTERRUPTS_ENABLED 1
#define GPIO_INTERRUPTS_DISABLED 0

#define DEBOUNCING_DELAY_IN_US 100000U //100ms
#define LIMIT_SWITCH_REACTION_DURATION_IN_US 500000 //500ms

#endif /* ELECTRONICBLINDS_MAIN_H */
