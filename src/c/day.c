/** upTempo for Pebble

    Copyright (C) 2015  Varun Shrivastav

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program. If not, see <http://www.gnu.org/licenses/>.
**/

#include <pebble.h>
#include "day.h"
 
Window *day_window;
int8_t curr_page;
uint day_val, day_avg_val;

typedef struct {
  TextLayer *day_avg_text, *day_avg_val_text, *day_val_text;
  BitmapLayer *day_bmp_layer;
  GBitmap *img_day;
  char buf_val[10];
  char buf_avg[10];
  AppTimer* startTimer;
  int8_t timerCount;
}DayVars;

DayVars *day_vars;

/*TextLayer *day_avg_text, *day_avg_val_text, *day_val_text;
BitmapLayer *day_bmp_layer;
GBitmap *img_day = NULL;
char buf_val[10] = "";
char buf_avg[10] = "";
AppTimer* startTimer;
int8_t timerCount = 0;*/

/**
 * Make time string
**/
static void set_time_val(uint value, char* buf_value) {
  value = value/60;
  if(value > 59) {
    snprintf(buf_value, 10, "%uh %um", value/60, value%60);
  }
  else {
    snprintf(buf_value, 10, "%um", value);
  }
}

/**
 * Update UI
**/
void make_day_ui(int8_t page) {
  #ifdef PBL_COLOR
  GColor color;
  #endif
  gbitmap_destroy(day_vars->img_day);
  
  if(page == UI_IDLE) {
    text_layer_set_text(day_vars->day_avg_text, "You idle too long");
    text_layer_set_text(day_vars->day_val_text, main_time_text);
    text_layer_set_text(day_vars->day_avg_val_text, "");
  }
  else {
    if(page == UI_HR) {
      text_layer_set_text(day_vars->day_avg_text, "Resting HR");
    }
    else if(page == UI_SLEEP) {
      text_layer_set_text(day_vars->day_avg_text, "Deep sleep");
    }
    else {
      text_layer_set_text(day_vars->day_avg_text, "Average");
    }
    
    if(page == UI_STEPS) {
      if(day_val > 9999) {
          snprintf(day_vars->buf_val, 10, "%u %.3u", day_val/1000, day_val%1000);
        }
        else {
          snprintf(day_vars->buf_val, 10, "%u", day_val);
        }
      snprintf(day_vars->buf_avg, 10, "%u", day_avg_val);
    }
    else if(page == UI_FLOORS) {
      snprintf(day_vars->buf_val, 10, "%u", day_val);
      snprintf(day_vars->buf_avg, 10, "%u", day_avg_val);
    }
    else if(page == UI_HR) {
      #if PBL_API_EXISTS(health_service_peek_current_value)
      HealthServiceAccessibilityMask hr = health_service_metric_accessible(HealthMetricHeartRateBPM, time(NULL), time(NULL));
      if (hr & HealthServiceAccessibilityMaskAvailable) {
        uint16_t val = (int16_t) health_service_peek_current_value(HealthMetricHeartRateBPM);
        if(val > 0) {
          day_val = val;
        }
      }
      #endif
      snprintf(day_vars->buf_val, 10, "%u bpm", day_val);
      snprintf(day_vars->buf_avg, 10, "%u bpm", day_avg_val);
    }
    else if(page == UI_ENERGY) {
      snprintf(day_vars->buf_val, 10, "%u kcal", day_val);
      snprintf(day_vars->buf_avg, 10, "%u kcal", day_avg_val);
    }
    else {
      set_time_val(day_val, day_vars->buf_val);
      set_time_val(day_avg_val, day_vars->buf_avg);
    }
    
    text_layer_set_text(day_vars->day_val_text, day_vars->buf_val);        
    text_layer_set_text(day_vars->day_avg_val_text, day_vars->buf_avg);
  }
  
  switch(page) {
    case UI_STEPS:
        day_vars->img_day = gbitmap_create_with_resource(RESOURCE_ID_IMG_FOOT);
        #ifdef PBL_COLOR
        color = GColorCobaltBlue;
        #endif
        break;
    case UI_WALK:
        day_vars->img_day = gbitmap_create_with_resource(RESOURCE_ID_IMG_WALK);
        #ifdef PBL_COLOR
        color = GColorDarkCandyAppleRed;
        #endif
        break;
    case UI_RUN:
        day_vars->img_day = gbitmap_create_with_resource(RESOURCE_ID_IMG_RUN);
        #ifdef PBL_COLOR
        color = GColorDarkGreen;
        #endif
        break;
    case UI_SLEEP:
        day_vars->img_day = gbitmap_create_with_resource(RESOURCE_ID_IMG_SLEEP);
        #ifdef PBL_COLOR
        color = GColorArmyGreen;
        #endif
        break;
    case UI_ENERGY:
        day_vars->img_day = gbitmap_create_with_resource(RESOURCE_ID_IMG_ENERGY);
        #ifdef PBL_COLOR
        color = GColorWindsorTan;
        #endif
        break;
    case UI_FLOORS:
        day_vars->img_day = gbitmap_create_with_resource(RESOURCE_ID_IMG_FLOORS);
        #ifdef PBL_COLOR
        color = GColorIndigo;
        #endif
        break;
    case UI_HR:
        day_vars->img_day = gbitmap_create_with_resource(RESOURCE_ID_IMG_HEART_RATE);
        #ifdef PBL_COLOR
        color = GColorBulgarianRose;
        #endif
        break;
    case UI_CYCLE:
        day_vars->img_day = gbitmap_create_with_resource(RESOURCE_ID_IMG_BIKE);
        #ifdef PBL_COLOR
        color = GColorBlue;
        #endif
        break;
    case UI_SPORT:
        day_vars->img_day = gbitmap_create_with_resource(RESOURCE_ID_IMG_SPORT);
        #ifdef PBL_COLOR
        color = GColorMidnightGreen;
        #endif
        break;
    case UI_TRAIN:
        day_vars->img_day = gbitmap_create_with_resource(RESOURCE_ID_IMG_TRAIN);
        #ifdef PBL_COLOR
        color = GColorBulgarianRose;
        #endif
        break;
    case UI_WATER:
        day_vars->img_day = gbitmap_create_with_resource(RESOURCE_ID_IMG_WATER);
        #ifdef PBL_COLOR
        color = GColorBlueMoon;
        #endif
        break;
    case UI_WINTER:
        day_vars->img_day = gbitmap_create_with_resource(RESOURCE_ID_IMG_WINTER);
        #ifdef PBL_COLOR
        color = GColorIndigo;
        #endif
        break;
    case UI_MARTIAL:
        day_vars->img_day = gbitmap_create_with_resource(RESOURCE_ID_IMG_MARTIAL);
        #ifdef PBL_COLOR
        color = GColorOrange;
        #endif
        break;
    case UI_OTHER:
        day_vars->img_day = gbitmap_create_with_resource(RESOURCE_ID_IMG_OTHER);
        #ifdef PBL_COLOR
        color = GColorImperialPurple;
        #endif
        break;
    case UI_IDLE:
        day_vars->img_day = gbitmap_create_with_resource(RESOURCE_ID_IMG_IDLE);
        #ifdef PBL_COLOR
        color = GColorOxfordBlue;
        #endif
        vibes_double_pulse();
        break;
  }
  
  bitmap_layer_set_bitmap(day_vars->day_bmp_layer, day_vars->img_day);
  #ifdef PBL_COLOR
  window_set_background_color(day_window, color);
  #endif
}

/**
 * Use values from phone
**/
void set_phone_ui_vals(int *page) {
  if(day_vars->startTimer != NULL) {
    app_timer_cancel(day_vars->startTimer);
    day_vars->startTimer = NULL;
  }
  if(*page >= 0) {
    curr_page = *page;
  }
  //if page is already special
  else if(curr_page > NUM_UI_MAX) {
    return;
  }
  
  if(day_window != NULL && window_is_loaded(day_window)) {
    make_day_ui(curr_page);
  }
}

/**
 * Read values from storage
**/
void set_ui_vals(int8_t page, bool force) {
  if(day_window != NULL && window_is_loaded(day_window)) {
    if(page >= 0) {
      curr_page = page;
    }
    //if page is already special
    else if(curr_page > NUM_UI_MAX) {
      return;
    }
    
    switch(curr_page) {
      case UI_STEPS:
        day_val = (uint32_t) persist_read_int(PERSIST_STEPS);
        day_avg_val = (uint32_t) persist_read_int(PERSIST_AVG_STEPS);
        //day_val = 3082;
        //day_avg_val = 8532;
        break;
      case UI_WALK:
        day_val = (uint32_t) persist_read_int(PERSIST_WALK);
        day_avg_val = (uint32_t) persist_read_int(PERSIST_AVG_WALK);
        //val = 1378;
        //avg_val = 1378;
        break;
      case UI_RUN:
        day_val = (uint32_t) persist_read_int(PERSIST_JOG);
        day_avg_val = (uint32_t) persist_read_int(PERSIST_AVG_JOG);
        break;
      case UI_SLEEP:
        day_val = (uint32_t) persist_read_int(PERSIST_SLEEP);
        day_avg_val = (uint32_t) persist_read_int(PERSIST_AVG_SLEEP);
        break;
    }
    
    make_day_ui(curr_page);
  }
}

/**
 * On up click
**/
static void day_up_click_handler(ClickRecognizerRef recognizer, void *context) {
  if(curr_page < NUM_UI_MAX){
    if(phoneDataSharing) {
      appmsg_send_val(APPMSG_DAY_VAL, PHONE_SEND_BUTTON_UP);
    }
    else {
      int8_t page = (curr_page-1)%NUM_UI_PAGES;
      if(page < 0) {
        page = NUM_UI_PAGES-1;
      }
      set_ui_vals(page, false);
    }
  }
}

/**
 * On select click
**/
static void day_select_click_handler(ClickRecognizerRef recognizer, void *context) {
  if(curr_page < NUM_UI_MAX) {
    window_stack_pop(true);
    history_init();
  }
}

/**
 * On select long click
**/
static void day_select_long_click_handler(ClickRecognizerRef recognizer, void *context) {
  if(curr_page < NUM_UI_MAX) {
    window_stack_pop(true);
    menu_init();
  }
}

/**
 * On down click
**/
static void day_down_click_handler(ClickRecognizerRef recognizer, void *context) {
  if(curr_page < NUM_UI_MAX){
    if(phoneDataSharing) {
      appmsg_send_val(APPMSG_DAY_VAL, PHONE_SEND_BUTTON_DOWN);
    }
    else {
      int8_t page = (curr_page+1)%NUM_UI_PAGES;
      set_ui_vals(page, false);
    }
  }
}

/**
 * On back click
**/
static void day_back_click_handler(ClickRecognizerRef recognizer, void *context) {
  window_stack_pop(true);
}

/**
 * Show time on special pages
**/
void day_time_handler(char *time_string) {
  if(day_window!=NULL && window_is_loaded(day_window) && curr_page > NUM_UI_MAX) {
    text_layer_set_text(day_vars->day_val_text, time_string);
  }
}

static void day_click_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_UP, day_up_click_handler);
  window_single_click_subscribe(BUTTON_ID_SELECT, day_select_click_handler);
  window_long_click_subscribe(BUTTON_ID_SELECT, 500, day_select_long_click_handler, NULL);
  window_single_click_subscribe(BUTTON_ID_DOWN, day_down_click_handler);
  window_single_click_subscribe(BUTTON_ID_BACK, day_back_click_handler);
}

static void day_window_load(Window *window) {
  Layer* day_window_layer = window_get_root_layer(window);
  window_set_background_color(window, COLOR_BACKGROUND);
  
  #if defined(PBL_ROUND)  
  day_vars->day_bmp_layer = macro_bitmap_layer_create(NULL, GRect(15, 40, 50, 50), day_window_layer, true);
  day_vars->day_val_text = macro_text_layer_create(GRect(65, 25, 95, 80), day_window_layer, COLOR_TEXT, COLOR_TEXT_BACKGROUND, forcedSquare, GTextAlignmentCenter);
  day_vars->day_avg_text = macro_text_layer_create(GRect(20, 110, 140, 40), day_window_layer, COLOR_TEXT, COLOR_TEXT_BACKGROUND, fonts_get_system_font(FONT_KEY_GOTHIC_18), GTextAlignmentCenter);
  day_vars->day_avg_val_text = macro_text_layer_create(GRect(0, 130, 180, 30), day_window_layer, COLOR_TEXT, COLOR_TEXT_BACKGROUND, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD), GTextAlignmentCenter);
  
  text_layer_enable_screen_text_flow_and_paging(day_vars->day_val_text, 2);
  text_layer_enable_screen_text_flow_and_paging(day_vars->day_avg_text, 2);
  
  /*#elif defined(PBL_PLATFORM_APLITE)
  day_bmp_layer = macro_bitmap_layer_create(NULL, GRect(0, 30, 50, 50), day_window_layer, true);
  day_val_text = macro_text_layer_create(GRect(50, 20, 94, 70), day_window_layer, COLOR_TEXT, COLOR_TEXT_BACKGROUND, fonts_get_system_font(FONT_KEY_BITHAM_30_BLACK), GTextAlignmentRight);
  day_avg_text = macro_text_layer_create(GRect(10, 100, 124, 25), day_window_layer, COLOR_TEXT, COLOR_TEXT_BACKGROUND, fonts_get_system_font(FONT_KEY_GOTHIC_18), GTextAlignmentLeft);
  day_avg_val_text = macro_text_layer_create(GRect(10, 125, 124, 30), day_window_layer, COLOR_TEXT, COLOR_TEXT_BACKGROUND, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD), GTextAlignmentRight);
  */
  #else
  GSize size = layer_get_bounds(day_window_layer).size;
  int mainWidth = 144;
  int margin = (size.w-mainWidth)/3;
  int height3rd = size.h/3;
  
  day_vars->day_bmp_layer = macro_bitmap_layer_create(NULL, GRect(margin, (2*height3rd-50)/2, 50, 50), day_window_layer, true);
  day_vars->day_val_text = macro_text_layer_create(GRect(50+2*margin, (2*height3rd-75)/2, 94, 80), day_window_layer, COLOR_TEXT, COLOR_TEXT_BACKGROUND, forcedSquare, GTextAlignmentRight);
  day_vars->day_avg_text = macro_text_layer_create(GRect(10, 2*height3rd, size.w-20, 40), day_window_layer, COLOR_TEXT, COLOR_TEXT_BACKGROUND, fonts_get_system_font(FONT_KEY_GOTHIC_18), GTextAlignmentLeft);
  day_vars->day_avg_val_text = macro_text_layer_create(GRect(10, 9*size.h/12, size.w-20, 30), day_window_layer, COLOR_TEXT, COLOR_TEXT_BACKGROUND, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD), GTextAlignmentRight);
  #endif
  
  text_layer_set_overflow_mode(day_vars->day_val_text, GTextOverflowModeWordWrap);
  text_layer_set_overflow_mode(day_vars->day_avg_text, GTextOverflowModeWordWrap);
}

static void day_window_unload(Window *window) {
  //text_layer_destroy(day_title_text);
  text_layer_destroy(day_vars->day_val_text);
  text_layer_destroy(day_vars->day_avg_text);
  text_layer_destroy(day_vars->day_avg_val_text);
  bitmap_layer_destroy(day_vars->day_bmp_layer);
  if(day_vars->img_day != NULL) { gbitmap_destroy(day_vars->img_day); day_vars->img_day = NULL; }
  window_destroy(day_window);
  day_window = NULL;
  free(day_vars);
}

/**
 * Repeatedly try to request values from phone
**/
static void request_phone_vals(void* data) {
  if(connection_service_peek_pebble_app_connection()) {
    appmsg_send_val(APPMSG_DAY_VAL, PHONE_SEND_NO_BUTTON);
    if(day_vars->timerCount < 5) {
      day_vars->startTimer = app_timer_register(300, request_phone_vals, NULL);
      day_vars->timerCount++;
    }
    else {
      gbitmap_destroy(day_vars->img_day);
      day_vars->img_day = gbitmap_create_with_resource(RESOURCE_ID_IMG_ERROR);
      bitmap_layer_set_bitmap(day_vars->day_bmp_layer, day_vars->img_day);
      text_layer_set_text(day_vars->day_avg_text, "Phone unreachable. Show local data...");
      phoneDataSharing = false;
    }
  }
  else {
    gbitmap_destroy(day_vars->img_day);
    day_vars->img_day = gbitmap_create_with_resource(RESOURCE_ID_IMG_ERROR);
    bitmap_layer_set_bitmap(day_vars->day_bmp_layer, day_vars->img_day);
    text_layer_set_text(day_vars->day_avg_text, "Phone disconnected. Show local data..");
    phoneDataSharing = false;
  }
}

void day_init(bool standard) {
  day_vars = malloc(sizeof(DayVars));
  day_vars->img_day = NULL;
  snprintf(day_vars->buf_val, 10, " ");
  snprintf(day_vars->buf_avg, 10, " ");
  
  if(day_window == NULL) {
    day_window = window_create();
    window_set_window_handlers(day_window, (WindowHandlers) {
      .load = day_window_load,
      .unload = day_window_unload
    });
    window_set_click_config_provider(day_window, day_click_provider);
  }
  if(!window_is_loaded(day_window)) {
    window_stack_push(day_window, true);
  }
  if(standard) {
    if(phoneDataSharing) {
      text_layer_set_text(day_vars->day_avg_text, "Access phone...");
      #ifdef PBL_COLOR
      window_set_background_color(day_window, GColorBlueMoon);
      #endif
      
      appmsg_send_val(APPMSG_DAY_VAL, PHONE_SEND_NO_BUTTON);
      day_vars->timerCount = 0;
      day_vars->startTimer = app_timer_register(300, request_phone_vals, NULL);
    }
    else {
      if(curr_page == -1) {
        time_t time_now = time(NULL);
        struct tm now = *localtime(&time_now);
        if(now.tm_hour < 9 || now.tm_hour > 21) {
          curr_page = UI_SLEEP;
        }
        else {
          curr_page = UI_STEPS;
        }
      }
      set_ui_vals(curr_page, true);
    }
  }
  else if(worker_start_msg == WORKER_MSG_IDLE_ALERT) {
    set_ui_vals(UI_IDLE, true);
    worker_start_msg = -1;
  }
}