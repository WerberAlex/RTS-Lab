/* SPDX-License-Identifier: Apache-2.0 */

#ifndef BUTTON_LED_H_
#define BUTTON_LED_H_

#include <zephyr/drivers/gpio.h>

/* Vérifie les deux GPIO, configure la LED en sortie (éteinte) et le bouton en entrée.
 * Renvoie 0, -ENODEV si un port n'est pas prêt, ou l'erreur du driver. */
int button_led_init(const struct gpio_dt_spec *led, const struct gpio_dt_spec *btn);

/* Lit le bouton et recopie son état sur la LED.
 * Renvoie l'état lu (0 ou 1) ou une erreur négative ; en cas d'erreur de lecture,
 * la LED n'est pas modifiée. */
int button_led_update(const struct gpio_dt_spec *led, const struct gpio_dt_spec *btn);

#endif /* BUTTON_LED_H_ */
