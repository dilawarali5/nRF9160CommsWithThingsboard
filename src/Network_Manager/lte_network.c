/* Includes ----------------------------------------------------------- */
#include "lte_network.h"

#include <zephyr/kernel.h>
#include "SystemConfig.h"
#include <zephyr/sys/printk.h>
#include <modem/lte_lc.h>
#include <modem/modem_info.h>
#include <modem/modem_key_mgmt.h>
#include <modem/nrf_modem_lib.h>
#include <nrf_modem_at.h>
#include <date_time.h>
#include <zephyr/posix/time.h>
#include <zephyr/posix/sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include "mqtt_comm.h"


/* Private defines ---------------------------------------------------- */

/* Private enumerate/structure ---------------------------------------- */
/* Private macros ----------------------------------------------------- */
/* Public variables --------------------------------------------------- */
/* Private variables -------------------------------------------------- */
static const char cert[] = {
	#include "ca-root.pem"
};

/* Private function prototypes ---------------------------------------- */
static void lte_handler(const struct lte_lc_evt *const evt);
static void print_modem_info(enum modem_info info);
static void reset_network(struct k_work *work);


K_WORK_DELAYABLE_DEFINE(network_reset, reset_network);
/* Private function definitions ---------------------------------------- */
/**@brief 				Handler for LTE events.
 *
 * @details 			This function is called when an LTE event is received.
 *
 * @param[in]	 		evt		Pointer to the event data.
 * @return 				None.
 */
static void lte_handler(const struct lte_lc_evt *const evt)
{
	printk("LTE event: %d\n", evt->type);
	switch (evt->type) 
	{
		case LTE_LC_EVT_NW_REG_STATUS:
				printk("Network registration status: %d: %s\n", evt->nw_reg_status,
					evt->nw_reg_status == LTE_LC_NW_REG_REGISTERED_HOME ? "Connected - home network" :
					evt->nw_reg_status == LTE_LC_NW_REG_REGISTERED_ROAMING ? "Connected - roaming" :
					evt->nw_reg_status == LTE_LC_NW_REG_SEARCHING ? "Searching" :
					evt->nw_reg_status == LTE_LC_NW_REG_UNKNOWN ? "Unknown" :
					"Invalid");
				if (evt->nw_reg_status != LTE_LC_NW_REG_REGISTERED_HOME &&
				evt->nw_reg_status != LTE_LC_NW_REG_REGISTERED_ROAMING) 
                {
					systemConfig.isNetworkConnected = 0;
					// Schedule the network reset work after 5 minutes.
					// Need to do this when there is no events from modem and network is not connected.
					(void)k_work_reschedule(&network_reset, K_SECONDS(300));
					break;
				}

				printk("\nConnected to: %s network\n", evt->nw_reg_status == LTE_LC_NW_REG_REGISTERED_HOME ? "home" : "roaming");
				print_modem_info(MODEM_INFO_APN);
				print_modem_info(MODEM_INFO_IP_ADDRESS);
				print_modem_info(MODEM_INFO_RSRP);
				systemConfig.isNetworkConnected = 1;
				break;

		case LTE_LC_EVT_LTE_MODE_UPDATE:
				printk("LTE mode update: %d: %s\n", evt->lte_mode,
					evt->lte_mode == LTE_LC_LTE_MODE_NONE ? "None" :
					evt->lte_mode == LTE_LC_LTE_MODE_LTEM ? "LTE-M" :
					evt->lte_mode == LTE_LC_LTE_MODE_NBIOT ? "NB-IoT" :
					"Unknown");
				break;
		default:
				break;
	}
}


/**@brief 				print modem information.
 *
 * @details 			This function is called to require modem information.
 *
 * @param[in]	 		info		Modem information.
 * @return 				None.
 */
static void print_modem_info(enum modem_info info)
{
	int len;
	char buf[80];

	switch (info) 
	{
		case MODEM_INFO_RSRP:
			printk("Signal Strength: ");
			break;
		case MODEM_INFO_IP_ADDRESS:
			printk("IP Addr: ");
			break;
		case MODEM_INFO_FW_VERSION:
			printk("Modem FW Ver: ");
			break;
		case MODEM_INFO_ICCID:
			printk("SIM ICCID: ");
			break;
		case MODEM_INFO_IMSI:
			printk("IMSI: ");
			break;
		case MODEM_INFO_IMEI:
			printk("IMEI: ");
			break;
		case MODEM_INFO_DATE_TIME:
			printk("Network Date/Time: ");
			break;
		case MODEM_INFO_APN:
			printk("APN: ");
			break;
		default:
			printk("Unsupported: ");
			break;
	}

	len = modem_info_string_get(info, buf, 80);
	if (len > 0) {
		printk("%s\n",buf);
	} else {
		printk("Error\n");
	}
}


/**@brief 				Provision certificate.
 *
 * @details 			This function is called to provision the certificate.
 *
 * @param[in]	 		None.
 * @return 				0 if successful, otherwise error code.
 */
static int cert_provision(void)
{
	bool exists;

	/* It may be sufficient for you application to check whether the correct
	 * certificate is provisioned with a given tag directly using modem_key_mgmt_cmp().
	 * Here, for the sake of the completeness, we check that a certificate exists
	 * before comparing it with what we expect it to be.
	 */
	int err = modem_key_mgmt_exists(CONFIG_MQTT_HELPER_SEC_TAG, MODEM_KEY_MGMT_CRED_TYPE_CA_CHAIN, &exists);
	if (err) {
		printk("Failed to check for certificates err %d, try updating your 9160 modem firmware.\n", err);
		return err;
	}

	if (exists) {
		int mismatch = modem_key_mgmt_cmp(CONFIG_MQTT_HELPER_SEC_TAG,
					      MODEM_KEY_MGMT_CRED_TYPE_CA_CHAIN,
					      cert, strlen(cert));
		if (!mismatch) {
			printk("Certificate match\n");
			return 0;
		}

		printk("Certificate mismatch\n");
		err = modem_key_mgmt_delete(CONFIG_MQTT_HELPER_SEC_TAG, MODEM_KEY_MGMT_CRED_TYPE_CA_CHAIN);
		if (err) {
			printk("Failed to delete existing certificate, err %d\n", err);
		}
	}

	printk("Provisioning certificate\n");

	/*  Provision certificate to the modem */
	err = modem_key_mgmt_write(CONFIG_MQTT_HELPER_SEC_TAG,
				   MODEM_KEY_MGMT_CRED_TYPE_CA_CHAIN,
				   cert, sizeof(cert) - 1);
	if (err) {
		printk("Failed to provision certificate, err %d\n", err);
		return err;
	}

	return 0;
}

/**@brief 				Reset network.
 *
 * @details 			This function is called to reset the network.
 *
 * @param[in]	 		None.
 * @return 				None.
 */
static void reset_network(struct k_work *work)
{
	int32_t err = 0;
	uint8_t atbuf[64] = {0};

	if (!systemConfig.isNetworkConnected)
	{
		err = lte_lc_offline();
		printk("Power off modem: %d\n", err);

		err = nrf_modem_at_cmd(atbuf, sizeof(atbuf), "AT%%XFACTORYRESET=0");
		printk("MODEM: Factory reset: %s\n", atbuf);

		err = lte_lc_connect_async(lte_handler);
		printk("Waiting for network... \n");
	}

}

/* Global Function definitions ----------------------------------------------- */
/**@brief 				Initialize network.
 *
 * @details 			This function is called to initialize the network.
 *
 * @param[in]	 		None.
 * @return 				0 if successful, otherwise error code.
 */
int lte_network_init(void)
{
	int err;

	printk("Starting Network on board: (%s)\n", CONFIG_BOARD);

	err = nrf_modem_lib_init();
	if (err) {
		printk("Modem initialization failed, err %d\n", err);
		return -1;
	}

	err = lte_lc_func_mode_set(LTE_LC_FUNC_MODE_ACTIVATE_UICC);
	if (err) printk("MODEM: Failed enabling UICC power, error: %d\n", err);
	k_msleep(100);
	
	err = modem_info_init();
	if (err) printk("MODEM: Failed initializing modem info module, error: %d\n", err);

	print_modem_info(MODEM_INFO_FW_VERSION);
	print_modem_info(MODEM_INFO_IMEI);
	print_modem_info(MODEM_INFO_ICCID);

	// Get device IMEI
	err = modem_info_string_get(MODEM_INFO_IMEI, systemConfig.DeviceIMEI, sizeof(systemConfig.DeviceIMEI));
	if (err < 0) printk("MODEM: Failed to get IMEI, error: %d\n", err);

    err = cert_provision();
    if (err) {
        printk("Failed to provision certificate, err %d\n", err);
        return -1;
    }

	err = lte_lc_psm_req(true);
	if (err) printk("MODEM: Failed to enable PSM, error: %d\n", err);

	err = lte_lc_connect_async(lte_handler);
	if (err) {
		printk("Failed to connect to the LTE network, err %d\n", err);
		return -1;
	}

	printk("Waiting for network... \n");

    return err;
}


/* End of file -------------------------------------------------------- */
