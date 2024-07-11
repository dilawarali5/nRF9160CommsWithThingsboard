/* Includes ----------------------------------------------------------- */
#include "user_app.h"
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <net/mqtt_helper.h>
#include <cJSON.h>
#include <stdio.h>
#include <stdlib.h>
#include "SystemConfig.h"
#include "LedHandler.h"
#include "storage.h"

/* Private defines ---------------------------------------------------- */
#define PUBLISH_TOPIC "v1/devices/me/telemetry"
#define ATTRIBUTE_TOPIC "v1/devices/me/attributes"

#define MQTT_PROVISION_USERNAME "provision"

/* ID for subscribe topic - Used to verify that a subscription succeeded in on_mqtt_suback(). */

/* Private enumerate/structure ---------------------------------------- */
/* Private macros ----------------------------------------------------- */
/* Public variables --------------------------------------------------- */

/* Private variables -------------------------------------------------- */
static uint8_t publishPayloadBuffer[MQTT_PUB_BUFF_SIZE + 1] = {0};
struct mqtt_topic mqtt_sub_topics[MAX_SUBSCRIBE_TOPIC_COUNT];


/* Private function prototypes ---------------------------------------- */
static void tunoff_led(struct k_work *work);

K_WORK_DELAYABLE_DEFINE(led_off_work, tunoff_led);
/* Private function definitions ---------------------------------------- */

static void tunoff_led(struct k_work *work)
{
    if (systemConfig.isBrokerConnected)
    {
        SetLedState(0);

        if (MqttPublishMessage(ATTRIBUTE_TOPIC," {\"LED\":false}") < 0)
        {
            printk("Failed to publish message\n");
        }
    }
}

/**@brief           Function to check if device is provisioned.
 * 
 * param[in]        None.
 * 
 * @return          1 if device is provisioned, 0 otherwise.
 * 
*/
static uint8_t isDeviceProvisioned(void)
{
    uint8_t data[MAX_USERNAME_LENGTH] = {0};
    int32_t ret = 0;

    if (!systemConfig.isProvisioned){
        printk("Read username from flash\n");
        ret = read_file(MQTT_USERNAME_FILE_NAME, data, MAX_USERNAME_LENGTH, DIRECTORY);
        if (ret > 0)
        {
            memcpy(systemConfig.deviceUsername, data, MAX_USERNAME_LENGTH);
            systemConfig.isProvisioned = 1;
        }
    }
    
    return systemConfig.isProvisioned;
}


/* Global Function definitions ----------------------------------------------- */

/**@brief           Function to start data communication.
 * 
 * param[in]        None.
 * 
 * @return          None.
 * 
*/
void StartDataCommunication(void *p1, void *p2, void *p3)
{
    int32_t ret = 0;
    int64_t refTime = 0;
    printk("Starting data communication Task\n");

    while(1)
    {
        // Wait for network to be connected
        if(systemConfig.isNetworkConnected == 0)
        {
            k_sleep(K_SECONDS(1));
            continue;
        }

        if (!isDeviceProvisioned())
        {
            ret = MqttConnect(MQTT_PROVISION_USERNAME);
            if( ret >= 0)
            {
                refTime = k_uptime_get();
                while (k_uptime_get() - refTime < MQTT_CONNECT_TIMEOUT)
                {
                    if (systemConfig.isBrokerConnected) break;
                    k_sleep(K_MSEC(100));
                }

                ret = MqttProvisionRequest();
                if (ret >= 0)
                {
                    systemConfig.isProvisioned = 1;
                    ret = write_file(MQTT_USERNAME_FILE_NAME, systemConfig.deviceUsername, MAX_USERNAME_LENGTH, DIRECTORY);
                    if (ret < 0)
                    {
                        printk("Failed to write username to file\n");
                    }
                }

                MqttDisconnect();
                systemConfig.isBrokerConnected = 0;
                k_sleep(K_SECONDS(1));
            }
        }

        if (systemConfig.isProvisioned)
        {
            if (!systemConfig.isBrokerConnected) {
                ret = MqttConnect(systemConfig.deviceUsername);

                if (ret >= 0)
                {
                    refTime = k_uptime_get();
                    while (k_uptime_get() - refTime < MQTT_CONNECT_TIMEOUT)
                    {
                        if (systemConfig.isBrokerConnected) break;
                        k_sleep(K_MSEC(100));
                    }

                    // Subscribe to the attribute topic
                    mqtt_sub_topics[0].topic.utf8 = ATTRIBUTE_TOPIC;
                    mqtt_sub_topics[0].topic.size = strlen(ATTRIBUTE_TOPIC);
                    
                    ret = MqttTopicsSubscribe(mqtt_sub_topics, 1);
                }
            }
            
            if (ret >= 0)
            {
                snprintf(publishPayloadBuffer, MQTT_PUB_BUFF_SIZE, "{\"temperature\":%d}", systemConfig.InternalTemp);
                ret = MqttPublishMessage(PUBLISH_TOPIC, publishPayloadBuffer);
                if (ret < 0)
                {
                    printk("Failed to publish message\n");
                }
            }
        }

        k_sleep(K_SECONDS(MQTT_INTER_MESSAGE_DELAY));
    }
}

/**@brief           Function to parse the provision response message.
 * 
 * param[in]        topic_buf: Pointer to the topic buffer.
 * param[in]        payload_buf: Pointer to the payload buffer.
 * 
 * @return          None.
 * 
*/
void parseAttributeRxMessage(struct mqtt_helper_buf *topic_buf, struct mqtt_helper_buf *payload_buf)
{
    if (strncmp(payload_buf->ptr, "{\"LED\":false}", payload_buf->size) == 0)
    {
        SetLedState(0);
    }
    else if (strncmp(payload_buf->ptr, "{\"LED\":true}", payload_buf->size) == 0)
    {
        SetLedState(1);
        (void)k_work_reschedule(&led_off_work, K_SECONDS(MQTT_INTER_MESSAGE_DELAY/2));
    }
}


/**@brief           parse the json string and get the value of the key.
 * 
 * param[in]        jsonString: Pointer to the json string.
 * param[in]        key: Pointer to the key.
 * param[in]        value: Pointer to the value.
 * 
 * @return          None.
 * 
*/
void parseJsonGetStringObject(uint8_t *jsonString, uint8_t *key, uint8_t *value)
{
    cJSON *root = cJSON_Parse(jsonString);
    cJSON *item = cJSON_GetObjectItem(root, key);
    if (item != NULL)
    {
        strcpy(value, item->valuestring);
    }
    cJSON_Delete(root);
}
/* End of file -------------------------------------------------------- */
