/* SPDX-License-Identifier: Apache-2.0 */

#include <errno.h>
#include "button_led.h"

int button_led_init(const struct gpio_dt_spec *led, const struct gpio_dt_spec *btn)
{
	int ret;

	if (!gpio_is_ready_dt(led) || !gpio_is_ready_dt(btn)) {
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(led, GPIO_OUTPUT_INACTIVE);
	if (ret < 0) {
		return ret;
	}

	return gpio_pin_configure_dt(btn, GPIO_INPUT);
}

int button_led_update(const struct gpio_dt_spec *led, const struct gpio_dt_spec *btn)
{
	int state = gpio_pin_get_dt(btn);
	int ret;

	if (state < 0) {
		return state;
	}

	ret = gpio_pin_set_dt(led, state);

	return (ret < 0) ? ret : state;
}
