#pragma once
#include "utils.h"
#include "main.h"

#define GPS_BLE_OFF 0
#define GPS_ON_BLE_OFF 1
#define GPS_OFF_BLE_ON 2
#define GPS_BLE_ON 3

void active_init();

void active_set_distance(char *value);
void active_set_speed(char *value);
void active_set_time(char *value);
void active_set_heart(char *value);
void active_set_stop(bool save);