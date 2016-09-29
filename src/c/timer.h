#pragma once
  #include "alarm.h"
  #include "main.h"

  #define TIMER_TYPE_PAUSE        91
  #define TIMER_TYPE_SINGLE       92
  #define TIMER_TYPE_WEEKDAY      93
  #define TIMER_TYPE_WEEKEND      94
  #define TIMER_TYPE_IDLE         95
  
  #define TIMER_MAX_MIN           59
  #define TIMER_MAX_HR_PAUSE      12
  #define TIMER_MAX_HR_SINGLE     23
  #define TIMER_MAX_HR_WEEKDAY    23
  #define TIMER_MAX_HR_WEEKEND    23
  #define TIMER_MAX_HR_IDLE       3
  
  #define TIMER_MIN_HR_PAUSE      0
  #define TIMER_MIN_HR_SINGLE     0
  #define TIMER_MIN_HR_WEEKDAY    0
  #define TIMER_MIN_HR_WEEKEND    0
  #define TIMER_MIN_HR_IDLE       1
  
  #define TIMER_CURRENT_SELECT_ENABLE    10
  #define TIMER_CURRENT_SELECT_HR        11
  #define TIMER_CURRENT_SELECT_MIN       12
  
void timer_init(uint8_t type);