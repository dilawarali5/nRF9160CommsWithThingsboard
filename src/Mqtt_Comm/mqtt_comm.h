
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
typedef struct mqtt_topic MQTT_TOPIC_STRUCT;

/* Exported constants --------------------------------------------------------*/
#define MQTT_PUB_BUFF_SIZE 1024
#define MQTT_PROVISION_BUFF_SIZE 512

#define MQTT_CONNECT_TIMEOUT 5000
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
int32_t MqttConnect(uint8_t *username);
int32_t MqttProvisionRequest(void);
int32_t MqttTopicsSubscribe(MQTT_TOPIC_STRUCT *topics, uint8_t topicCount);
int32_t MqttPublishMessage(uint8_t *topic, uint8_t *payload);
int32_t MqttDisconnect();

#ifdef __cplusplus
}
#endif

#endif /* __MQTT_COMM_H */

