#ifndef AUTOMATICCONTROLTASK_H
#define AUTOMATICCONTROLTASK_H

/*--------------- MACROS ---------------*/
/* I2C Register Read is set up to read from the DS1307 RTC module. You provide the register you want to read as argument 
            0 - seconds (all values in BDC), 1 - mins, 2 - hours, 3 - unused, 4 - DD, 5 - MM, 6 - YY */

#define DS1307_REG_ADDR_SECONDS         ((uint8_t)0)
#define DS1307_REG_ADDR_MINUTES         ((uint8_t)1)
#define DS1307_REG_ADDR_HOURS           ((uint8_t)2)
#define DS1307_REG_ADDR_DAYS            ((uint8_t)4)
#define DS1307_REG_ADDR_MONTHS          ((uint8_t)5)
#define DS1307_REG_ADDR_YEARS           ((uint8_t)6)

#define DS1307_REG_ADDR_IS_CLOSED       ((uint8_t)8)

#define LATITUDE_SIEROSZEWICE_NOWA_10   (51.635799) //in degrees

#define BLINDS_CLOSED   (1)
#define BLINDS_OPEN     (0)

/*--------------- GLOBAL VARIABLES DECLARATION (extern) ---------------*/
void AutomaticControlTask( void *pvParameters );

#endif /* AUTOMATICCONTROLTASK_H */

