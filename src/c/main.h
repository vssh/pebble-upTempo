#pragma once
#include "alarm.h"
#include "menu.h"
#include "utils.h"
#include "timer.h"
#include "persist.h"
#include "day.h"
#include "history.h"
#include "active.h"
  
#define NUM_PAGES    3
  
extern uint idle_time;
extern char main_time_text[6];
extern int worker_start_msg;

void set_ui_values();