/* Includes ----------------------------------------------------------- */
#include "LedHandler.h"
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include "SystemConfig.h"
#include <zephyr/sys/printk.h>
#include <stdio.h>
#include <stdlib.h>

/* Private defines ---------------------------------------------------- */
/* The devicetree node identifier for the "led0" alias. */
#define LED1_NODE DT_ALIAS(led1)

/* Private enumerate/structure ---------------------------------------- */
/* Private macros ----------------------------------------------------- */
/* Public variables --------------------------------------------------- */
/* Private variables -------------------------------------------------- */
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED1_NODE, gpios);

/* Private function prototypes ---------------------------------------- */
/* Private function definitions ---------------------------------------- */


/* Global Function definitions ----------------------------------------------- */

/**
 * @brief       Function to initialize the LED Module
 * 
 * @return      0 if successful, -1 if failed
 *  
 */ 
int32_t LedInit(void)
{
    int32_t ret = -1;
    if (gpio_is_ready_dt(&led)) 
    {
        ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
        if (ret == 0)
        {
            gpio_pin_set_dt(&led, 0);
            printk("LED initialized\n");
        }
    }
	
    return ret;
}

/**
 * @brief       Function to set the state of the LED
 * 
 * @param[in]   state: 0 for OFF, 1 for ON
 * 
 * @return      0 if successful, -1 if failed
 *  
 */
int32_t SetLedState (uint8_t state)
{
    int32_t ret = -1;
    if (gpio_is_ready_dt(&led)) 
    {
        ret = gpio_pin_set_dt(&led, state);
        systemConfig.ledState = state;
        printk("LED state set to %d\n", state);
    }

    return ret;
}
/* End of file -------------------------------------------------------- */
