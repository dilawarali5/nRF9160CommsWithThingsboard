
/* Includes ----------------------------------------------------------- */
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <nrf_modem_at.h>
#include "lte_network.h"
#include "mqtt_comm.h"
#include "LedHandler.h"
#include "SystemConfig.h"
#include "user_app.h"
#include "storage.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Private defines ---------------------------------------------------- */
#define BOOT_COUNTER_FILE_NAME "bc"
/* Private enumerate/structure ---------------------------------------- */
/* Private macros ----------------------------------------------------- */
/* Public variables --------------------------------------------------- */
system_config_struct systemConfig;

/* Private variables -------------------------------------------------- */
static K_THREAD_STACK_DEFINE(user_app_thread_stack_area, 10240);
static struct k_thread user_app_thread_data;
k_tid_t user_app_tid;


/* Private function prototypes ---------------------------------------- */
static int32_t flashSelfTest(void);

/* Private function definitions ---------------------------------------- */
/**@brief           Function to perform flash self test.
 * 
 * param[in]        None.
 * 
 * @return          0 if successful, negative otherwise.
 * 
 */
static int32_t flashSelfTest(void)
{
	int32_t ret = 0;
	uint32_t bootCounter = 0;
	
	for (uint32_t i = 0; i < 3; i++)
	{
		ret = read_file(BOOT_COUNTER_FILE_NAME, (uint8_t*)&bootCounter, sizeof(bootCounter), DIRECTORY);
		bootCounter++;
		ret = write_file(BOOT_COUNTER_FILE_NAME, (uint8_t*)&bootCounter, sizeof(bootCounter), DIRECTORY);
		if(ret >= 0) break;
	}
	
	return ret;
}


/* Global Function definitions ----------------------------------------------- */
int main(void)
{
	int32_t ret = 0;
	uint8_t ATcommandResponseBuffer[128] = {0};

	printk("Starting application_OTA_Version\n");
	// EraseExternalFlash();
	// return 0;

	ret = flashSelfTest();
	if (ret < 0)
	{
		printk("Failed to initialize flash storage\n");
		return ret;
	}

	ret = mqtt_comm_init();
	if (ret != 0)
	{
		printk("Failed to initialize MQTT communication module\n");
		return ret;
	}

	ret = lte_network_init();
	if (ret != 0)
	{
		printk("Failed to initialize network module\n");
		return ret;
	}

	ret = LedInit();
	if (ret != 0)
	{
		printk("Failed to initialize LED module\n");
		return ret;
	}

	// Start the cellular thread it will manage all of its activity its own
	user_app_tid = k_thread_create(&user_app_thread_data, user_app_thread_stack_area,
							K_THREAD_STACK_SIZEOF(user_app_thread_stack_area),
							StartDataCommunication,
							NULL, NULL, NULL,
							K_HIGHEST_APPLICATION_THREAD_PRIO, K_ESSENTIAL, K_NO_WAIT);


	// Read the nRF9160 internal temperature every 5 seconds
	while (1)
	{
		ret = nrf_modem_at_cmd(ATcommandResponseBuffer, sizeof(ATcommandResponseBuffer), "AT%%XTEMP?");

		if (ret == 0)
		{
			sscanf(ATcommandResponseBuffer, "%%XTEMP: %d", &systemConfig.InternalTemp);
		}

		ret = nrf_modem_at_cmd(ATcommandResponseBuffer, sizeof(ATcommandResponseBuffer), "AT%%XMONITOR");
		if (ret == 0) printk("AT Response: %s\n", ATcommandResponseBuffer);

		// printf("Internal temperature: %d\n", systemConfig.InternalTemp);
		k_sleep(K_SECONDS(60));

	}
	

	return 0;

}
/* End of file -------------------------------------------------------- */