
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __SYSTEMCONFIG_H
#define __SYSTEMCONFIG_H

#ifdef __cplusplus
extern "C"
{
#endif

/* Includes ------------------------------------------------------------------*/

/* Exported types ------------------------------------------------------------*/
typedef struct
{
    int InternalTemp;
    uint8_t isBrokerConnected;
    uint8_t ledState;
}system_config_struct;

/* Exported constants --------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/

/*
******************************************************************************
* GLOBAL VARIABLES
******************************************************************************
*/
extern struct k_work_delayable mqtt_work;
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




