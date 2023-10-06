#ifndef AUTOMATICCONTROLTASK_H
#define AUTOMATICCONTROLTASK_H

/*--------------- MACROS ---------------*/
/* I2C Register Read is set up to read from the DS1307 RTC module. You provide the register you want to read as argument 
            0 - seconds (all values in BDC), 1 - mins, 2 - hours, 3 - unused, 4 - DD, 5 - MM, 6 - YY */

#define DS1307_REG_ADDR_SECONDS     ((uint8)0)
#define DS1307_REG_ADDR_MINUTES     ((uint8)1)
#define DS1307_REG_ADDR_HOURS       ((uint8)2)
#define DS1307_REG_ADDR_DAYS        ((uint8)4)
#define DS1307_REG_ADDR_MONTHS      ((uint8)5)
#define DS1307_REG_ADDR_YEARS       ((uint8)6)

#define DS1307_REG_ADDR_IS_CLOSED   ((uint8)8)

#define BLINDS_CLOSED   (1)
#define BLINDS_OPEN     (0)

/*--------------- GLOBAL VARIABLES DECLARATION (extern) ---------------*/
void AutomaticControlTask( void *pvParameters );

#endif /* AUTOMATICCONTROLTASK_H */

