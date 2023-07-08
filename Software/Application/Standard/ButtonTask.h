#ifndef BUTTONTASK_H
#define BUTTONTASK_H

/* Constants and Macros */

/* Data Types */

/* Function Declarations */
void ButtonTask( void *pvParameters );
static void buttons_callback(uint gpio, uint32_t events);
static void alarm_irq(void);
static void alarm_in_us(uint32_t delay_us);

#endif /* BUTTONTASK_H */