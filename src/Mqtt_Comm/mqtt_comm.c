/* Includes ----------------------------------------------------------- */
#include "mqtt_comm.h"
#include <zephyr/kernel.h>
#include "SystemConfig.h"
#include <zephyr/sys/printk.h>
#include <net/mqtt_helper.h>
#include <stdio.h>
#include <stdlib.h>
#include "user_app.h"
#include "storage.h"

/* Private defines ---------------------------------------------------- */
#define PUBLISH_TOPIC "v1/devices/me/telemetry"
#define ATTRIBUTE_TOPIC "v1/devices/me/attributes"

#define PROVISION_REQUEST_TOPIC  "/provision/request"
#define PROVISION_RESPONSE_TOPIC  "/provision/response"

/* ID for subscribe topic - Used to verify that a subscription succeeded in on_mqtt_suback(). */
#define SUBSCRIBE_TOPIC_ID 2469

/* Private enumerate/structure ---------------------------------------- */
/* Private macros ----------------------------------------------------- */
/* Public variables --------------------------------------------------- */
// K_WORK_DELAYABLE_DEFINE(mqtt_work, mqtt_comm_start);

/* Private variables -------------------------------------------------- */

/* Private function prototypes ---------------------------------------- */
static void MqttOnConnection(enum mqtt_conn_return_code return_code, bool session_present);
static void MqttOnDisconnection(int result);
static void MqttReceivedPublishedMessage(struct mqtt_helper_buf topic_buf, struct mqtt_helper_buf payload_buf);

/* Private function definitions ---------------------------------------- */
/**@brief           MQTT connection callback.
 * 
 * param[in]        return_code: Connection return code.
 * param[in]        session_present: Session present flag.
 * 
 * @return          None.
 * 
*/
static void MqttOnConnection(enum mqtt_conn_return_code return_code, bool session_present)
{
    if (return_code == MQTT_CONNECTION_ACCEPTED)
    {
        printk("MQTT connection accepted\n");
        systemConfig.isBrokerConnected = 1;
    }
    else if(return_code == MQTT_NOT_AUTHORIZED)
    {
        printk("MQTT connection not authorized\n");
        systemConfig.isBrokerConnected = 0;
        systemConfig.isProvisioned = 0;
        memset(systemConfig.deviceUsername, 0, sizeof(systemConfig.deviceUsername));
        eraseFile(MQTT_USERNAME_FILE_NAME, DIRECTORY);
    }
    else
    {
        printk("MQTT connection failed: %d\n", return_code);
    }
}

/**@brief           MQTT disconnection callback.
 * 
 * param[in]        result: Disconnection result.
 * 
 * @return          None.
 * 
*/
static void MqttOnDisconnection(int result)
{
    printk("MQTT disconnected: %d\n", result);
    systemConfig.isBrokerConnected = 0;
}


/**@brief           MQTT received published message callback.
 * 
 * param[in]        topic_buf: Topic buffer.
 * param[in]        payload_buf: Payload buffer.
 * 
 * @return          None.
 * 
 */
static void MqttReceivedPublishedMessage(struct mqtt_helper_buf topic_buf, struct mqtt_helper_buf payload_buf)
{
    printk("Received message on topic: %s\n", topic_buf.ptr);
    printk("Payload: %s\n", payload_buf.ptr);

    if (strncmp(topic_buf.ptr, ATTRIBUTE_TOPIC, topic_buf.size) == 0)
    {
        parseAttributeRxMessage(&topic_buf, &payload_buf);
    }
    else if (strncmp(topic_buf.ptr, PROVISION_RESPONSE_TOPIC, topic_buf.size) == 0)
    {
        if(strstr(payload_buf.ptr, "\"status\":\"SUCCESS\"") != NULL)
        {
            systemConfig.isProvisioned = 1;
            parseJsonGetStringObject(payload_buf.ptr, "credentialsValue", systemConfig.deviceUsername);
        }
    }
    else
    {
        printk("Unknown topic\n");
    }
}



/* Global Function definitions ----------------------------------------------- */
/**@brief           Initialize MQTT communication.
 * 
 * @return          0 if successful, otherwise a negative value.
 * 
 */
int32_t mqtt_comm_init(void)
{
    int32_t ret = 0;
    struct mqtt_helper_cfg cfg = {
        .cb.on_connack = MqttOnConnection,
        .cb.on_disconnect = MqttOnDisconnection,
        .cb.on_publish = MqttReceivedPublishedMessage,
    };

    ret = mqtt_helper_init(&cfg);
    if (ret != 0)
    {
        printk("Failed to initialize MQTT helper: %d\n", ret);
        return ret;
    }

    return 0;
}


/**@brief           Connect to the MQTT broker.
 * 
 * @param[in]       username: Username.
 * 
 * @return          0 if successful, otherwise a negative value.
 * 
 */
int32_t MqttConnect(uint8_t *username)
{
    int32_t ret = 0;
    struct mqtt_helper_conn_params conn_params = {
        .hostname = {
            .ptr = CONFIG_MQTT_BROKER_HOSTNAME,
            .size = strlen(CONFIG_MQTT_BROKER_HOSTNAME),
        },
        .device_id = {
            .ptr = CONFIG_MQTT_CLIENT_ID,
            .size = strlen(CONFIG_MQTT_CLIENT_ID),
        },
        .user_name = {
            .ptr = username,
            .size = strlen(username),
        },
        .password = {
            .ptr = NULL,
            .size = 0,
        },
    };

     // connect to the broker if not connected, and subscribe to attribute topic
    if(systemConfig.isBrokerConnected == 1) {
        ret = 0;
    }
    else {
        ret = mqtt_helper_connect(&conn_params);
        if (ret != 0)
        {
            printk("Failed to connect to MQTT broker: %d\n", ret);
        }
    }


    return ret;
}

/**@brief           Provision the device with the MQTT broker.
 * 
 * @param           None
 * 
 * @return          0 if successful, otherwise a negative value.
 * 
 */
int32_t MqttProvisionRequest(void )
{
    int32_t ret = 0;
    static uint8_t provisionRequestPayload[MQTT_PROVISION_BUFF_SIZE + 1] = {0};

    snprintf(provisionRequestPayload, MQTT_PROVISION_BUFF_SIZE, "{\"deviceName\": \"%s\", \"provisionDeviceKey\": \"%s\", \"provisionDeviceSecret\": \"%s\"}",
                                                                systemConfig.DeviceIMEI, CONFIG_MQTT_DEVICE_PROVISIONING_KEY, CONFIG_MQTT_DEVICE_PROVISIONING_SECRET);
    printk("Provisioning request payload: %s\n", provisionRequestPayload);

    struct mqtt_publish_param publish_param = {
        .message.topic.qos = MQTT_QOS_1_AT_LEAST_ONCE,
        .message.topic.topic = {
            .utf8 = PROVISION_REQUEST_TOPIC,
            .size = strlen(PROVISION_REQUEST_TOPIC),
        },
        .message.payload = {
            .data = provisionRequestPayload,
            .len = strlen(provisionRequestPayload),
        },
        .message_id = 85,
    };

    // Update the subscription topic list
    struct mqtt_topic mqtt_sub_topics[] = {
        {
            .topic = {
                .utf8 = PROVISION_RESPONSE_TOPIC,
                .size = strlen(PROVISION_RESPONSE_TOPIC),
            },
        },
    };

    struct mqtt_subscription_list sub_list = {
        .list = mqtt_sub_topics,
        .list_count = ARRAY_SIZE(mqtt_sub_topics),
        .message_id = SUBSCRIBE_TOPIC_ID,
    };

    ret = mqtt_helper_subscribe(&sub_list);
    if (ret != 0)
    {
        printk("Failed to subscribe to topic: %d\n", ret);
        return ret;
    }

    ret = mqtt_helper_publish(&publish_param);
    if (ret != 0)
    {
        printk("Failed to publish message: %d\n", ret);
    }
    else
    {
        printk("Published message\n");
        printk("Topic: %s\n", PROVISION_REQUEST_TOPIC);
        printk("Payload: %s\n", provisionRequestPayload);
    }

    uint32_t refTime = k_uptime_get_32();
    while (systemConfig.isProvisioned == 0)
    {
        k_sleep(K_SECONDS(1));

        if (k_uptime_get_32() - refTime > MQTT_CONNECT_TIMEOUT)
        {
            printk("Provisioning timeout\n");
            ret = -1;
            break;
        }
    }

    if (systemConfig.isProvisioned == 1)
    {
        printk("Provisioned username: %s\n", systemConfig.deviceUsername);
    }

    return ret;
}

/**@brief           Subscribe to MQTT topics.
 * 
 * @param[in]       topics: Array of topics to subscribe to.
 * @param[in]       topicCount: Number of topics in the array.
 * 
 * @return          0 if successful, otherwise a negative value.
 * 
 */
int32_t MqttTopicsSubscribe(MQTT_TOPIC_STRUCT *topics, uint8_t topicCount)
{
    int32_t ret = 0;
    struct mqtt_subscription_list sub_list = {
        .list = topics,
        .list_count = topicCount,
        .message_id = SUBSCRIBE_TOPIC_ID,
    };

    ret = mqtt_helper_subscribe(&sub_list);
    if (ret != 0)
    {
        printk("Failed to subscribe to topic: %d\n", ret);
        return ret;
    }

    return ret;
}

/**@brief           Publish a message to a topic.
 * 
 * @param[in]       topic: Topic to publish to.
 * @param[in]       payload: Payload to publish.
 * 
 * @return          0 if successful, otherwise a negative value.
 * 
 */
int32_t MqttPublishMessage(uint8_t *topic, uint8_t *payload)
{
    int32_t ret = 0;
    static uint16_t id = 0;
    struct mqtt_publish_param publish_param = {
        .message.topic.qos = MQTT_QOS_1_AT_LEAST_ONCE,
        .message.topic.topic = {
            .utf8 = topic,
            .size = strlen(topic),
        },
        .message.payload = {
            .data = payload,
            .len = strlen(payload),
        },
        .message_id = ++id,
    };

    ret = mqtt_helper_publish(&publish_param);
    if (ret != 0)
    {
        printk("Failed to publish message: %d\n", ret);
    }
    else
    {
        printk("Published message\n");
        printk("Topic: %s\n", topic);
        printk("Payload: %s\n", payload);
    }

    return ret;
}

/**@brief           Disconnect from the MQTT broker.
 * 
 * @return          0 if successful, otherwise a negative value.
 * 
 */
int32_t MqttDisconnect()
{
    int32_t ret = 0;
    ret = mqtt_helper_disconnect();
    if (ret != 0)
    {
        printk("Failed to disconnect from MQTT broker: %d\n", ret);
    }
    systemConfig.isBrokerConnected = 0;

    return ret;
}
/* End of file -------------------------------------------------------- */
