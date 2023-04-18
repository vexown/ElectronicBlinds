#ifndef ELECTRONICBLINDS_MAIN_H
#define ELECTRONICBLINDS_MAIN_H

/* Priorities for the tasks */
#define MOTOR_CONTROLLER_TASK_PRIORITY		( tskIDLE_PRIORITY + 1 )
#define	BUTTON_TASK_PRIORITY				( tskIDLE_PRIORITY + 2 )
#define INTERRUPT_CONTROL_TASK_PRIORITY     ( tskIDLE_PRIORITY + 3 )

/* Task periods (ms) */
#define BUTTON_TASK_PERIOD						( 100 )
#define MOTOR_CONTROLLER_TASK_PERIOD			( 100 )
#define INTERRUPT_CONTROL_TASK_PERIOD			( 50 )

/* The number of items the queue can hold */
#define mainQUEUE_LENGTH					( 1 )

/* By default the MPU6050 devices are on bus address 0x68 */ 
#define MPU6050_I2C_ADDRESS   				 0x68

/* Motor direction/state */
#define CLOCKWISE 	  (1)
#define ANTICLOCKWISE (-1)
#define MOTOR_OFF (0)

/* GPIO numbers */
#define BUTTON_UP 14U
#define BUTTON_DOWN 15U
#define SOURCE_3V3_1 12U
#define SOURCE_3V3_2 13U
#define MOTOR_CONTROL_1 16U
#define MOTOR_CONTROL_2 17U

#define GPIO_INTERRUPTS_ENABLED 1
#define GPIO_INTERRUPTS_DISABLED 0

#endif /* ELECTRONICBLINDS_MAIN_H */
