# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(button_led_unit)

# Le module testé est celui de l'application (pas de copie).
target_include_directories(app PRIVATE ../../src)
target_sources(app PRIVATE src/test_button_led.c ../../src/button_led.c)
