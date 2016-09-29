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
TextLayer *day_avg_text, *day_avg_val_text, *day_val_text;
BitmapLayer *day_bmp_layer;
GBitmap *img_day = NULL;
int8_t curr_page;
char buf_val[10] = "";
char buf_avg[10] = "";
uint day_val, day_avg_val;
AppTimer* startTimer;
int8_t timerCount = 0;

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
  gbitmap_destroy(img_day);
  
  if(page == UI_IDLE) {
    text_layer_set_text(day_avg_text, "You're idle too long");
    text_layer_set_text(day_val_text, main_time_text);
    text_layer_set_text(day_avg_val_text, "");
  }
  else {
    text_layer_set_text(day_avg_text, "Average");
    
    if(page == UI_STEPS) {
      if(day_val > 9999) {
          snprintf(buf_val, 10, "%u %.3u", day_val/1000, day_val%1000);
        }
        else {
          snprintf(buf_val, 10, "%u", day_val);
        }
      snprintf(buf_avg, 10, "%u", day_avg_val);
    }
    else if(page == UI_FLOORS) {
      snprintf(buf_val, 10, "%u", day_val);
      snprintf(buf_avg, 10, "%u", day_avg_val);
    }
    else if(page == UI_ENERGY) {
      snprintf(buf_val, 10, "%u kCal", day_val);
      snprintf(buf_avg, 10, "%u kCal", day_avg_val);
    }
    else {
      set_time_val(day_val, buf_val);
      set_time_val(day_avg_val, buf_avg);
    }
    
    text_layer_set_text(day_val_text, buf_val);        
    text_layer_set_text(day_avg_val_text, buf_avg);
  }
  
  switch(page) {
    case UI_STEPS:
        img_day = gbitmap_create_with_resource(RESOURCE_ID_IMG_FOOT);
        #ifdef PBL_COLOR
        color = GColorCobaltBlue;
        #endif
        break;
    case UI_WALK:
        img_day = gbitmap_create_with_resource(RESOURCE_ID_IMG_WALK);
        #ifdef PBL_COLOR
        color = GColorDarkCandyAppleRed;
        #endif
        break;
    case UI_RUN:
        img_day = gbitmap_create_with_resource(RESOURCE_ID_IMG_RUN);
        #ifdef PBL_COLOR
        color = GColorDarkGreen;
        #endif
        break;
    case UI_SLEEP:
        img_day = gbitmap_create_with_resource(RESOURCE_ID_IMG_SLEEP);
        #ifdef PBL_COLOR
        color = GColorArmyGreen;
        #endif
        break;
    case UI_ENERGY:
        img_day = gbitmap_create_with_resource(RESOURCE_ID_IMG_ENERGY);
        #ifdef PBL_COLOR
        color = GColorWindsorTan;
        #endif
        break;
    case UI_FLOORS:
        img_day = gbitmap_create_with_resource(RESOURCE_ID_IMG_FLOORS);
        #ifdef PBL_COLOR
        color = GColorIndigo;
        #endif
        break;
    case UI_CYCLE:
        img_day = gbitmap_create_with_resource(RESOURCE_ID_IMG_BIKE);
        #ifdef PBL_COLOR
        color = GColorBlue;
        #endif
        break;
    case UI_SPORT:
        img_day = gbitmap_create_with_resource(RESOURCE_ID_IMG_SPORT);
        #ifdef PBL_COLOR
        color = GColorMidnightGreen;
        #endif
        break;
    case UI_TRAIN:
        img_day = gbitmap_create_with_resource(RESOURCE_ID_IMG_TRAIN);
        #ifdef PBL_COLOR
        color = GColorBulgarianRose;
        #endif
        break;
    case UI_WATER:
        img_day = gbitmap_create_with_resource(RESOURCE_ID_IMG_WATER);
        #ifdef PBL_COLOR
        color = GColorBlueMoon;
        #endif
        break;
    case UI_WINTER:
        img_day = gbitmap_create_with_resource(RESOURCE_ID_IMG_WINTER);
        #ifdef PBL_COLOR
        color = GColorIndigo;
        #endif
        break;
    case UI_MARTIAL:
        img_day = gbitmap_create_with_resource(RESOURCE_ID_IMG_MARTIAL);
        #ifdef PBL_COLOR
        color = GColorOrange;
        #endif
        break;
    case UI_OTHER:
        img_day = gbitmap_create_with_resource(RESOURCE_ID_IMG_OTHER);
        #ifdef PBL_COLOR
        color = GColorImperialPurple;
        #endif
        break;
    case UI_IDLE:
        img_day = gbitmap_create_with_resource(RESOURCE_ID_IMG_TREK);
        #ifdef PBL_COLOR
        color = GColorOxfordBlue;
        #endif
        vibes_double_pulse();
        break;
  }
  
  bitmap_layer_set_bitmap(day_bmp_layer, img_day);
  #ifdef PBL_COLOR
  window_set_background_color(day_window, color);
  #endif
}

/**
 * Use values from phone
**/
void set_phone_ui_vals(int *page) {
  if(startTimer != NULL) {
    app_timer_cancel(startTimer);
    startTimer = NULL;
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
    if(persist_read_bool(PERSIST_DISPLAY_DATA_PHONE)) {
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
    if(persist_read_bool(PERSIST_DISPLAY_DATA_PHONE)) {
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
    text_layer_set_text(day_val_text, time_string);
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
  day_bmp_layer = macro_bitmap_layer_create(NULL, GRect(15, 40, 50, 50), day_window_layer, true);
  day_val_text = macro_text_layer_create(GRect(65, 30, 95, 70), day_window_layer, COLOR_TEXT, COLOR_TEXT_BACKGROUND, fonts_get_system_font(FONT_KEY_BITHAM_30_BLACK), GTextAlignmentCenter);
  day_avg_text = macro_text_layer_create(GRect(20, 110, 140, 25), day_window_layer, COLOR_TEXT, COLOR_TEXT_BACKGROUND, fonts_get_system_font(FONT_KEY_GOTHIC_18), GTextAlignmentCenter);
  day_avg_val_text = macro_text_layer_create(GRect(0, 130, 180, 30), day_window_layer, COLOR_TEXT, COLOR_TEXT_BACKGROUND, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD), GTextAlignmentCenter);
  
  text_layer_enable_screen_text_flow_and_paging(day_val_text, 2);
  #else
  day_bmp_layer = macro_bitmap_layer_create(NULL, GRect(0, 35, 50, 50), day_window_layer, true);
  day_val_text = macro_text_layer_create(GRect(50, 20, 95, 70), day_window_layer, COLOR_TEXT, COLOR_TEXT_BACKGROUND, fonts_get_system_font(FONT_KEY_BITHAM_30_BLACK), GTextAlignmentRight);
  day_avg_text = macro_text_layer_create(GRect(10, 100, 124, 25), day_window_layer, COLOR_TEXT, COLOR_TEXT_BACKGROUND, fonts_get_system_font(FONT_KEY_GOTHIC_18), GTextAlignmentLeft);
  day_avg_val_text = macro_text_layer_create(GRect(15, 125, 124, 30), day_window_layer, COLOR_TEXT, COLOR_TEXT_BACKGROUND, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD), GTextAlignmentRight);
  #endif
  text_layer_set_overflow_mode(day_val_text, GTextOverflowModeWordWrap);
}

static void day_window_unload(Window *window) {
  text_layer_destroy(day_val_text);
  text_layer_destroy(day_avg_text);
  text_layer_destroy(day_avg_val_text);
  bitmap_layer_destroy(day_bmp_layer);
  if(img_day != NULL) { gbitmap_destroy(img_day); img_day = NULL; }
  window_destroy(day_window);
  day_window = NULL;
}

/**
 * Repeatedly try to request values from phone
**/
static void request_phone_vals(void* data) {
  if(connection_service_peek_pebble_app_connection()) {
    appmsg_send_val(APPMSG_DAY_VAL, PHONE_SEND_NO_BUTTON);
    if(timerCount < 5) {
      startTimer = app_timer_register(300, request_phone_vals, NULL);
      timerCount++;
    }
    else {
      gbitmap_destroy(img_day);
      img_day = gbitmap_create_with_resource(RESOURCE_ID_IMG_ERROR);
      bitmap_layer_set_bitmap(day_bmp_layer, img_day);
      text_layer_set_text(day_avg_text, "Phone unreachable");
    }
  }
  else {
    gbitmap_destroy(img_day);
    img_day = gbitmap_create_with_resource(RESOURCE_ID_IMG_ERROR);
    bitmap_layer_set_bitmap(day_bmp_layer, img_day);
    text_layer_set_text(day_avg_text, "Phone disconnected");
  }
}

void day_init(bool standard) {
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
    if(persist_read_bool(PERSIST_DISPLAY_DATA_PHONE)) {
      text_layer_set_text(day_avg_text, "Access phone data...");
      #ifdef PBL_COLOR
      window_set_background_color(day_window, GColorBlueMoon);
      #endif
      
      appmsg_send_val(APPMSG_DAY_VAL, PHONE_SEND_NO_BUTTON);
      startTimer = app_timer_register(300, request_phone_vals, NULL);
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
