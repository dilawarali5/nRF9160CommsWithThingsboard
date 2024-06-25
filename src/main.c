
/* Includes ----------------------------------------------------------- */
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <nrf_modem_at.h>
#include "lte_network.h"
#include "mqtt_comm.h"
#include "LedHandler.h"
#include "SystemConfig.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Private defines ---------------------------------------------------- */

/* Private enumerate/structure ---------------------------------------- */
/* Private macros ----------------------------------------------------- */
/* Public variables --------------------------------------------------- */
system_config_struct systemConfig;

/* Private variables -------------------------------------------------- */
/* Private function prototypes ---------------------------------------- */



/* Global Function definitions ----------------------------------------------- */
int main(void)
{
	int32_t ret = 0;
	uint8_t ATcommandResponseBuffer[128] = {0};

	printk("Starting application\n");

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

	while (1)
	{
		ret = nrf_modem_at_cmd(ATcommandResponseBuffer, sizeof(ATcommandResponseBuffer), "AT%%XTEMP?");

		if (ret == 0)
		{
			sscanf(ATcommandResponseBuffer, "%%XTEMP: %d", &systemConfig.InternalTemp);
		}

		k_sleep(K_SECONDS(5));

	}
	

	return 0;

}
/* End of file -------------------------------------------------------- */