# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(ztest_module)

# Le code testé est le fichier de l'exemple shell_module lui-même (pas de copie) :
# les tests Ztest et les tests Robot portent donc sur exactement la même source.
target_sources(app PRIVATE
  src/test_dynamic_cmd.c
  ../shell_module/src/dynamic_cmd.c
)
