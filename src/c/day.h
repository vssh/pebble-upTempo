#pragma once
#include "utils.h"
#include "menu.h"
#include "history.h"
#include "main.h"

#define NUM_UI_PAGES      4
#define NUM_UI_MAX        13

#define UI_OTHER          10
#define UI_WINTER         9
#define UI_WATER          8
#define UI_MARTIAL        7
#define UI_TRAIN          6
#define UI_SPORT          5
#define UI_CYCLE          4
#define UI_ENERGY         11
#define UI_FLOORS         12
#define UI_STEPS          0
#define UI_WALK           2
#define UI_RUN            3
#define UI_SLEEP          1
#define UI_IDLE           25
#define UI_SYNC           26

extern Window *day_window;
extern int8_t curr_page;
extern uint day_val;
extern uint day_avg_val;
void day_init(bool standard);
void set_phone_ui_vals(int *page);
void set_ui_vals(int8_t page, bool force);
void day_time_handler(char *time_string);

#ifdef PBL_COLOR
  #define COLOR_BACKGROUND0 GColorArmyGreen
  #define COLOR_BACKGROUND1 GColorDarkGreen
  #define COLOR_BACKGROUND2 GColorBulgarianRose
  #define COLOR_BACKGROUND3 GColorCobaltBlue
#else
  #define COLOR_BACKGROUND0 GColorBlack
  #define COLOR_BACKGROUND1 GColorBlack
  #define COLOR_BACKGROUND2 GColorBlack
  #define COLOR_BACKGROUND3 GColorBlack
#endif