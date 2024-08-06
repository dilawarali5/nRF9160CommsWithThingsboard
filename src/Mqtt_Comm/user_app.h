
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __USER_APP_H
#define __USER_APP_H

#ifdef __cplusplus
extern "C"
{
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include "mqtt_comm.h"
#include <net/mqtt_helper.h>

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/
#define MQTT_USERNAME_FILE_NAME "username"

#define MQTT_INTER_MESSAGE_DELAY 600
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
void StartDataCommunication(void *p1, void *p2, void *p3);
void parseAttributeRxMessage(struct mqtt_helper_buf *topic_buf, struct mqtt_helper_buf *payload_buf);
void parseJsonGetStringObject(uint8_t *jsonString, uint8_t *key, uint8_t *value);
#ifdef __cplusplus
}
#endif

#endif /* __USER_APP_H */

