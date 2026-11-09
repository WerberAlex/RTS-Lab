/* SPDX-License-Identifier: Apache-2.0 */

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>

#include "button_led.h"

static const int32_t sleep_time_ms = 100;
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(my_led), gpios);
static const struct gpio_dt_spec btn = GPIO_DT_SPEC_GET(DT_ALIAS(my_button), gpios);

int main(void)
{
	int ret = button_led_init(&led, &btn);

	if (ret < 0) {
		printk("ERROR %d: GPIO init failed\r\n", ret);
		return 0;
	}

	printk("LED spec flags: 0x%x\r\n", led.dt_flags);
	printk("Button spec flags: 0x%x\r\n", btn.dt_flags);

	while (1) {
		int state = button_led_update(&led, &btn);

		if (state < 0) {
			printk("Error %d: failed to read button or set LED\r\n", state);
		} else {
			printk("Button state: %d\r\n", state);
		}

		k_msleep(sleep_time_ms);
	}

	return 0;
}
