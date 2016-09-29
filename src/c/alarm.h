#pragma once
#include <pebble.h>
#include "utils.h"  

#define REASON_ALARM_MONITOR 24
#define REASON_ALARM_RING    25

#define MONITOR_PERIOD        30

#define WKDAY        171
#define WKEND        172

extern Window *alarm_window;
extern bool use_alarm;
extern int16_t wkday;
extern int16_t wkend;
void alarm_time_handler(char *time_string, int time_hr, int time_min);
  
void alarm_init();
void manage_alarms();
void reset_all_alarms();
void monitor_message_handler(uint16_t type, AppWorkerMessage *data);