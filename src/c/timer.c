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
#include "timer.h"
#include "utils.h"

Window *timer_window;
TextLayer *header_text, *footer_text, *hr_text, *min_text, *colon_text;
BitmapLayer *up_bmp_layer, *down_bmp_layer, *enable_bmp_layer;
GBitmap *img_up, *img_down, *img_enable, *img_disable;

int16_t hr_val, min_val, hr_max_val, hr_min_val;
uint8_t timer_type, current_select;
bool enable_val;
char hr_buf[3] = "00";
char min_buf[3] = "00";

/**
 * Update the UI in accordance with current values
**/
static void update_ui() {
  layer_set_hidden(bitmap_layer_get_layer(enable_bmp_layer), false);
  if(enable_val) {
    bitmap_layer_set_bitmap(enable_bmp_layer, img_enable);
  }
  else {
    bitmap_layer_set_bitmap(enable_bmp_layer, img_disable);
  }
  
  snprintf(hr_buf, 3, "%.2u", hr_val);
  text_layer_set_text(hr_text, hr_buf);

  snprintf(min_buf, 3, "%.2u", min_val);
  text_layer_set_text(min_text, min_buf);
  
  switch(current_select) {
    case TIMER_CURRENT_SELECT_ENABLE:
      bitmap_layer_set_alignment(up_bmp_layer, GAlignLeft);
      bitmap_layer_set_alignment(down_bmp_layer, GAlignLeft);
      break;
    case TIMER_CURRENT_SELECT_HR:
      bitmap_layer_set_alignment(up_bmp_layer, GAlignCenter);
      bitmap_layer_set_alignment(down_bmp_layer, GAlignCenter);
      break;
    case TIMER_CURRENT_SELECT_MIN:
      bitmap_layer_set_alignment(up_bmp_layer, GAlignRight);
      bitmap_layer_set_alignment(down_bmp_layer, GAlignRight);
      break;
  }
}

/**
 * On up click
**/
static void timer_up_click_handler(ClickRecognizerRef recognizer, void *context) {
  switch(current_select) {
    case TIMER_CURRENT_SELECT_ENABLE:
      enable_val = !enable_val;
      break;
    case TIMER_CURRENT_SELECT_HR:
      if(hr_val >= hr_max_val) {
        hr_val = hr_min_val;
      }
      else {
        hr_val++;
      }
      break;
    case TIMER_CURRENT_SELECT_MIN:
      if(min_val >= TIMER_MAX_MIN-4) {
        min_val = 0;
      }
      else {
        min_val += 5;
      }
      break;
  }
  update_ui();
}

/**
 * On select click
**/
static void timer_select_click_handler(ClickRecognizerRef recognizer, void *context) {
  current_select++;
  if(current_select>TIMER_CURRENT_SELECT_MIN || (!enable_val &&
                  current_select>TIMER_CURRENT_SELECT_ENABLE)) {
    int timespan = hr_val*60 + min_val;
    switch(timer_type) {
      case TIMER_TYPE_IDLE:
        if(!enable_val) idle_time = 0;
        else idle_time = timespan;
        persist_write_int(PERSIST_IDLE_TIME, idle_time);
        persist_write_int(PERSIST_WORKER_WAKEUP_MESSAGE, WORKER_MSG_READ_SETTINGS);
        break;
      case TIMER_TYPE_WEEKDAY:
        if(enable_val) {
          wkday = timespan;  
        }
        else {
          wkday = -1;
        }
        persist_write_int(PERSIST_WKD, wkday);
        reset_all_alarms();
        break;
      case TIMER_TYPE_WEEKEND:
      if(enable_val) {
          wkend = timespan;
        }
        else {
          wkend = -1;
        }
        persist_write_int(PERSIST_WKE, wkend);
        reset_all_alarms();
        break;
    }
    window_stack_pop(true);
    day_init(true);
  }
  else {
   update_ui(); 
  }
}

/**
 * On down click
**/
static void timer_down_click_handler(ClickRecognizerRef recognizer, void *context) {
  switch(current_select) {
    case TIMER_CURRENT_SELECT_ENABLE:
      enable_val = !enable_val;
      break;
    case TIMER_CURRENT_SELECT_HR:
      if(hr_val <= hr_min_val) {
        hr_val = hr_max_val;
      }
      else {
        hr_val--;
      }
      break;
    case TIMER_CURRENT_SELECT_MIN:
      if(min_val <= 0) {
        min_val = TIMER_MAX_MIN;
      }
      else {
        min_val--;
      }
      break;
  }
  update_ui();
}

/**
 * On back click
**/
static void timer_back_click_handler(ClickRecognizerRef recognizer, void *context) {
  current_select--;
  if(current_select < TIMER_CURRENT_SELECT_ENABLE) {
    window_stack_pop(true);
    day_init(true);
  }
  else {
   update_ui(); 
  }
}

static void timer_click_provider(void *context) {
  // Register the ClickHandlers
  window_single_click_subscribe(BUTTON_ID_UP, timer_up_click_handler);
  window_single_click_subscribe(BUTTON_ID_SELECT, timer_select_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, timer_down_click_handler);
  window_single_click_subscribe(BUTTON_ID_BACK, timer_back_click_handler);
}

static void timer_window_load(Window *window) {
  Layer* timer_window_layer = window_get_root_layer(window);
  #ifdef PBL_COLOR
  window_set_background_color(window, GColorMidnightGreen);
  #else
  window_set_background_color(window, COLOR_BACKGROUND);
  #endif
  
  img_up = gbitmap_create_with_resource(RESOURCE_ID_IMG_UP);
  img_down = gbitmap_create_with_resource(RESOURCE_ID_IMG_DOWN);
  img_enable = gbitmap_create_with_resource(RESOURCE_ID_IMG_TICK);
  img_disable = gbitmap_create_with_resource(RESOURCE_ID_IMG_CROSS);
  
  #if defined(PBL_ROUND)
  up_bmp_layer = macro_bitmap_layer_create(img_up, GRect(28, 52, 115, 16), timer_window_layer, true);
  down_bmp_layer = macro_bitmap_layer_create(img_down, GRect(28, 120, 115, 16), timer_window_layer, true);
  enable_bmp_layer = macro_bitmap_layer_create(img_disable, GRect(28, 86, 16, 16), timer_window_layer, false);
  
  header_text = macro_text_layer_create(GRect(28, 17, 124, 30), timer_window_layer, COLOR_TEXT, COLOR_TEXT_BACKGROUND, fonts_get_system_font(FONT_KEY_GOTHIC_24), GTextAlignmentCenter);
  footer_text = macro_text_layer_create(GRect(28, 132, 124, 30), timer_window_layer, COLOR_TEXT, COLOR_TEXT_BACKGROUND, fonts_get_system_font(FONT_KEY_GOTHIC_24), GTextAlignmentCenter);
  hr_text = macro_text_layer_create(GRect(58, 76, 45, 30), timer_window_layer, COLOR_TEXT, COLOR_TEXT_BACKGROUND, fonts_get_system_font(FONT_KEY_BITHAM_30_BLACK), GTextAlignmentCenter);
  min_text = macro_text_layer_create(GRect(118, 76, 45, 30), timer_window_layer, COLOR_TEXT, COLOR_TEXT_BACKGROUND, fonts_get_system_font(FONT_KEY_BITHAM_30_BLACK), GTextAlignmentCenter);
  colon_text = macro_text_layer_create(GRect(103, 76, 15, 30), timer_window_layer, COLOR_TEXT, COLOR_TEXT_BACKGROUND, fonts_get_system_font(FONT_KEY_BITHAM_30_BLACK), GTextAlignmentCenter);
  #else
  up_bmp_layer = macro_bitmap_layer_create(img_up, GRect(10, 50, 115, 16), timer_window_layer, true);
  down_bmp_layer = macro_bitmap_layer_create(img_down, GRect(10, 110, 115, 16), timer_window_layer, true);
  enable_bmp_layer = macro_bitmap_layer_create(img_disable, GRect(10, 80, 16, 16), timer_window_layer, false);
  
  header_text = macro_text_layer_create(GRect(10, 5, 124, 30), timer_window_layer, COLOR_TEXT, COLOR_TEXT_BACKGROUND, fonts_get_system_font(FONT_KEY_GOTHIC_24), GTextAlignmentCenter);
  footer_text = macro_text_layer_create(GRect(10, 132, 124, 30), timer_window_layer, COLOR_TEXT, COLOR_TEXT_BACKGROUND, fonts_get_system_font(FONT_KEY_GOTHIC_24), GTextAlignmentCenter);
  hr_text = macro_text_layer_create(GRect(35, 70, 45, 30), timer_window_layer, COLOR_TEXT, COLOR_TEXT_BACKGROUND, fonts_get_system_font(FONT_KEY_BITHAM_30_BLACK), GTextAlignmentCenter);
  min_text = macro_text_layer_create(GRect(95, 70, 45, 30), timer_window_layer, COLOR_TEXT, COLOR_TEXT_BACKGROUND, fonts_get_system_font(FONT_KEY_BITHAM_30_BLACK), GTextAlignmentCenter);
  colon_text = macro_text_layer_create(GRect(80, 70, 15, 30), timer_window_layer, COLOR_TEXT, COLOR_TEXT_BACKGROUND, fonts_get_system_font(FONT_KEY_BITHAM_30_BLACK), GTextAlignmentCenter);
  #endif
  
  //set initial values for each timer type
  idle_time = persist_exists(PERSIST_IDLE_TIME) ? persist_read_int(PERSIST_IDLE_TIME) : 120;
  current_select = TIMER_CURRENT_SELECT_ENABLE;
  switch(timer_type) {
    case TIMER_TYPE_WEEKEND:
      wkend = (uint16_t)persist_read_int(PERSIST_WKE);
      if(wkend >= 0) {
        enable_val = true;
        hr_val = wkend/60;
        min_val = wkend%60;
      }
      else {
        enable_val = false;
        hr_val = TIMER_MIN_HR_WEEKEND;
        min_val = 0; 
      }
      hr_max_val = TIMER_MAX_HR_WEEKEND;
      hr_min_val = TIMER_MIN_HR_WEEKDAY;
      text_layer_set_text(header_text, "Set alarm");
      text_layer_set_text(footer_text, "weekends");
      break;
    case TIMER_TYPE_WEEKDAY:
      wkday = (uint16_t)persist_read_int(PERSIST_WKD);
      if(wkday >= 0) {
        enable_val = true;
        hr_val = wkday/60;
        min_val = wkday%60;
      }
      else {
        enable_val = false;
        hr_val = TIMER_MIN_HR_WEEKDAY;
        min_val = 0; 
      }
      hr_max_val = TIMER_MAX_HR_WEEKDAY;
      hr_min_val = TIMER_MIN_HR_WEEKDAY;
      text_layer_set_text(header_text, "Set alarm");
      text_layer_set_text(footer_text, "weekdays");
      break;
    case TIMER_TYPE_IDLE:
      if(idle_time > 0) {
        enable_val = true;
        hr_val = idle_time/60;
        min_val = idle_time%60;
      }
      else {
        enable_val = false;
        hr_val = TIMER_MIN_HR_IDLE;
        min_val = 0;
      }
      hr_max_val = TIMER_MAX_HR_IDLE;
      hr_min_val = TIMER_MIN_HR_IDLE;
      text_layer_set_text(header_text, "When idle for");
      text_layer_set_text(footer_text, "show alert");
      break;
  }
  text_layer_set_text(colon_text, ":");
  update_ui();
}


static void timer_window_unload(Window *window) {
  text_layer_destroy(header_text);
  text_layer_destroy(footer_text);
  text_layer_destroy(hr_text);
  text_layer_destroy(min_text);
  text_layer_destroy(colon_text);
  bitmap_layer_destroy(up_bmp_layer);
  bitmap_layer_destroy(down_bmp_layer);
  bitmap_layer_destroy(enable_bmp_layer);
  gbitmap_destroy(img_up);
  gbitmap_destroy(img_down);
  gbitmap_destroy(img_enable);
  gbitmap_destroy(img_disable);
  window_destroy(timer_window);
  timer_window = NULL;
}
  
void timer_init(uint8_t type) {
  timer_type = type;
  if(timer_window == NULL) {
    timer_window = window_create();
    window_set_window_handlers(timer_window, (WindowHandlers) {
      .load = timer_window_load,
      .unload = timer_window_unload
    });
    window_set_click_config_provider(timer_window, timer_click_provider);
  }
  if(!window_is_loaded(timer_window)) {
    window_stack_push(timer_window, true);
  }  
}