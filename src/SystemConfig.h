
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __SYSTEMCONFIG_H
#define __SYSTEMCONFIG_H

#ifdef __cplusplus
extern "C"
{
#endif

/* Includes ------------------------------------------------------------------*/

/* Exported macro ------------------------------------------------------------*/
#define PROVISION_FILE_NAME "provision"
#define MAX_USERNAME_LENGTH 32
#define MAX_SUBSCRIBE_TOPIC_COUNT 5

/* Exported types ------------------------------------------------------------*/
typedef struct
{
    int InternalTemp;
    uint8_t isNetworkConnected;
    uint8_t isBrokerConnected;
    uint8_t isProvisioned;
    uint8_t ledState;
    uint8_t DeviceIMEI[20];
    uint8_t deviceUsername[MAX_USERNAME_LENGTH];
    uint8_t SubscribedTopicsCount;
}system_config_struct;

/* Exported constants --------------------------------------------------------*/


/*
******************************************************************************
* GLOBAL VARIABLES
******************************************************************************
*/
extern struct k_work_delayable device_provisioning_work;
extern system_config_struct systemConfig;

/*
******************************************************************************
* GLOBAL Functions
******************************************************************************
*/

#ifdef __cplusplus
}
#endif

#endif /* __SYSTEMCONFIG_H */




