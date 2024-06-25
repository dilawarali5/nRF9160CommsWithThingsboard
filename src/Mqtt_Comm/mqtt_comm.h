
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MQTT_COMM_H
#define __MQTT_COMM_H

#ifdef __cplusplus
extern "C"
{
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
/* Exported types ------------------------------------------------------------*/

/* Exported constants --------------------------------------------------------*/
#define MQTT_PUB_BUFF_SIZE 1024

/* Exported macro ------------------------------------------------------------*/

/*
******************************************************************************
* GLOBAL VARIABLES
******************************************************************************
*/

/*
******************************************************************************
* GLOBAL Functions
******************************************************************************
*/
int32_t mqtt_comm_init(void);
void mqtt_comm_start();

#ifdef __cplusplus
}
#endif

#endif /* __MQTT_COMM_H */
