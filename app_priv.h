/*
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#pragma once
#include <stdint.h>
#include <stdbool.h>

#define DEFAULT_MOTION 0
#define REPORTING_PERIOD 3 /* Seconds */
#define DEFAULT_POWER true

extern esp_rmaker_device_t *motion_sensor_device;
extern esp_rmaker_device_t *switch_device;

void app_driver_init(void);
int app_driver_set_state(bool state);
bool app_driver_get_state(void);
int app_get_current_motion();
