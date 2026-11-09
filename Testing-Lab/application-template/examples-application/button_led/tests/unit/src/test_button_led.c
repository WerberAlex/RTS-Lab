/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Tests unitaires de src/button_led.c, sur un GPIO émulé : le test impose l'état du
 * bouton avec gpio_emul_input_set() et lit la LED avec gpio_emul_output_get().
 */

#include <zephyr/drivers/gpio/gpio_emul.h>
#include <zephyr/ztest.h>

#include "button_led.h"

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(my_led), gpios);
static const struct gpio_dt_spec btn = GPIO_DT_SPEC_GET(DT_ALIAS(my_button), gpios);

static void press(int level)
{
	zassert_ok(gpio_emul_input_set(btn.port, btn.pin, level));
}

static int led_level(void)
{
	return gpio_emul_output_get(led.port, led.pin);
}

static void before(void *fixture)
{
	ARG_UNUSED(fixture);
	zassert_ok(button_led_init(&led, &btn));
}

ZTEST_SUITE(button_led, NULL, NULL, before, NULL, NULL);

ZTEST(button_led, test_init_configures_pins)
{
	gpio_flags_t flags;

	zassert_ok(gpio_emul_flags_get(led.port, led.pin, &flags));
	zassert_true(flags & GPIO_OUTPUT, "la LED doit etre une sortie");
	zassert_equal(led_level(), 0, "la LED doit etre eteinte apres init");

	zassert_ok(gpio_emul_flags_get(btn.port, btn.pin, &flags));
	zassert_true(flags & GPIO_INPUT, "le bouton doit etre une entree");
}

ZTEST(button_led, test_pressed_turns_led_on)
{
	press(1);
	zassert_equal(button_led_update(&led, &btn), 1);
	zassert_equal(led_level(), 1, "LED eteinte alors que le bouton est appuye");
}

ZTEST(button_led, test_released_turns_led_off)
{
	press(1);
	zassert_equal(button_led_update(&led, &btn), 1);

	press(0);
	zassert_equal(button_led_update(&led, &btn), 0);
	zassert_equal(led_level(), 0, "LED allumee alors que le bouton est relache");
}
