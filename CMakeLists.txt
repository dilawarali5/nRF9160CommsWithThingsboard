#
# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(nRF9160CommsWithThingsboard)

target_sources(app PRIVATE src/main.c)
add_subdirectory(src/Mqtt_Comm)
add_subdirectory(src/Network_Manager)
add_subdirectory(src/LedHandler)
add_subdirectory(src/storage)

zephyr_include_directories(src)

