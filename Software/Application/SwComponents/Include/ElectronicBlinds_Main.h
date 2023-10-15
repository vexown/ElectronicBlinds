#ifndef ELECTRONICBLINDS_MAIN_H
#define ELECTRONICBLINDS_MAIN_H

/*--------------- MACROS ---------------*/

/* Prints control */
#define PRINTS_ENABLED 0 //1 - prints enabled, 0 - prints disabled

#if(PRINTS_ENABLED == 1)
#define LOG(...) printf(__VA_ARGS__)
#else
#define LOG(...) 
#endif

/* Priorities for the tasks */
#define MOTOR_CONTROLLER_TASK_PRIORITY		(tskIDLE_PRIORITY + 1)
#define	BUTTON_TASK_PRIORITY				(tskIDLE_PRIORITY + 2)
#define AUTOMATIC_CONTROL_TASK_PRIORITY     (tskIDLE_PRIORITY + 3)

/* Task periods (ms) */
#define BUTTON_TASK_PERIOD					(100)
#define MOTOR_CONTROLLER_TASK_PERIOD		(100)
#define AUTOMATIC_CONTROL_TASK_PERIOD       (50000)

/* The number of items the queue can hold */
#define mainQUEUE_LENGTH					(1)

/* By default the MPU6050 devices are on bus address 0x68 */ 
#define MPU6050_I2C_ADDRESS   				 0x68

/* GPIO IDs to Names mapping: */
#define BUTTON_UP 14U
#define BUTTON_DOWN 13U
#define BUTTON_TOP_LIMIT 8U
#define BUTTON_BOTTOM_LIMIT 9U
#define SOURCE_3V3_1 10U
#define SOURCE_3V3_2 11U
#define SOURCE_3V3_3 12U
#define SOURCE_3V3_4 15U
#define MOTOR_CONTROL_1 16U
#define MOTOR_CONTROL_2 17U

/* Timing macros */
#define DEBOUNCING_DELAY_IN_US 100000U //100ms
#define DEBOUNCING_DELAY_IN_US_LIMITTER 10000U //10ms
#define LIMIT_SWITCH_REACTION_DURATION_IN_US 100000 //100ms

/*--------------- GLOBAL VARIABLES DECLARATION (extern) ---------------*/
extern bool buttonDown_InitState, buttonUp_InitState, buttonTopLimit_InitState, buttonBottomLimit_InitState;

#endif /* ELECTRONICBLINDS_MAIN_H */
