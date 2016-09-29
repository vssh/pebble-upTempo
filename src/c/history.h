#pragma once

#include "utils.h"
#include "menu.h"
#include "day.h"

extern Window *history_window;
void history_init();
extern uint32_t history_vals[7];
void set_phone_history_vals(int *page);