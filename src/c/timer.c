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

typedef struct{
  TextLayer *header_text, *footer_text, *hr_text, *min_text, *colon_text;
  BitmapLayer *up_bmp_layer, *down_bmp_layer, *enable_bmp_layer;
  GBitmap *img_up, *img_down, *img_enable, *img_disable;
  
  int16_t hr_val, min_val, hr_max_val, hr_min_val;
  uint8_t timer_type, current_select;
  bool enable_val;
  char hr_buf[3];
  char min_buf[3];
}TimerVals;
TimerVals *timer_vals;

/*TextLayer *header_text, *footer_text, *hr_text, *min_text, *colon_text;
BitmapLayer *up_bmp_layer, *down_bmp_layer, *enable_bmp_layer;
GBitmap *img_up, *img_down, *img_enable, *img_disable;

int16_t hr_val, min_val, hr_max_val, hr_min_val;
uint8_t timer_type, current_select;
bool enable_val;
char hr_buf[3] = "00";
char min_buf[3] = "00";*/

/**
 * Update the UI in accordance with current values
**/
static void update_ui() {
  layer_set_hidden(bitmap_layer_get_layer(timer_vals->enable_bmp_layer), false);
  if(timer_vals->enable_val) {
    bitmap_layer_set_bitmap(timer_vals->enable_bmp_layer, timer_vals->img_enable);
  }
  else {
    bitmap_layer_set_bitmap(timer_vals->enable_bmp_layer, timer_vals->img_disable);
  }
  
  snprintf(timer_vals->hr_buf, 3, "%.2u", timer_vals->hr_val);
  text_layer_set_text(timer_vals->hr_text, timer_vals->hr_buf);

  snprintf(timer_vals->min_buf, 3, "%.2u", timer_vals->min_val);
  text_layer_set_text(timer_vals->min_text, timer_vals->min_buf);
  
  switch(timer_vals->current_select) {
    case TIMER_CURRENT_SELECT_ENABLE:
      bitmap_layer_set_alignment(timer_vals->up_bmp_layer, GAlignLeft);
      bitmap_layer_set_alignment(timer_vals->down_bmp_layer, GAlignLeft);
      break;
    case TIMER_CURRENT_SELECT_HR:
      bitmap_layer_set_alignment(timer_vals->up_bmp_layer, GAlignCenter);
      bitmap_layer_set_alignment(timer_vals->down_bmp_layer, GAlignCenter);
      break;
    case TIMER_CURRENT_SELECT_MIN:
      bitmap_layer_set_alignment(timer_vals->up_bmp_layer, GAlignRight);
      bitmap_layer_set_alignment(timer_vals->down_bmp_layer, GAlignRight);
      break;
  }
}

/**
 * On up click
**/
static void timer_up_click_handler(ClickRecognizerRef recognizer, void *context) {
  switch(timer_vals->current_select) {
    case TIMER_CURRENT_SELECT_ENABLE:
      timer_vals->enable_val = !timer_vals->enable_val;
      break;
    case TIMER_CURRENT_SELECT_HR:
      if(timer_vals->hr_val >= timer_vals->hr_max_val) {
        timer_vals->hr_val = timer_vals->hr_min_val;
      }
      else {
        timer_vals->hr_val++;
      }
      break;
    case TIMER_CURRENT_SELECT_MIN:
      if(timer_vals->min_val >= TIMER_MAX_MIN-4) {
        timer_vals->min_val = 0;
      }
      else {
        timer_vals->min_val += 5;
      }
      break;
  }
  update_ui();
}

/**
 * On select click
**/
static void timer_select_click_handler(ClickRecognizerRef recognizer, void *context) {
  timer_vals->current_select++;
  if(timer_vals->current_select>TIMER_CURRENT_SELECT_MIN || (!timer_vals->enable_val &&
                  timer_vals->current_select>TIMER_CURRENT_SELECT_ENABLE)) {
    int timespan = timer_vals->hr_val*60 + timer_vals->min_val;
    uint idle_time = 120;
    switch(timer_vals->timer_type) {
      case TIMER_TYPE_IDLE:
        if(!timer_vals->enable_val) idle_time = 0;
        else idle_time = timespan;
        persist_write_int(PERSIST_IDLE_TIME, idle_time);
        persist_write_int(PERSIST_WORKER_WAKEUP_MESSAGE, WORKER_MSG_READ_SETTINGS);
        break;
      case TIMER_TYPE_WEEKDAY:
        if(timer_vals->enable_val) {
          wkday = timespan;  
        }
        else {
          wkday = -1;
        }
        persist_write_int(PERSIST_WKD, wkday);
        reset_all_alarms();
        break;
      case TIMER_TYPE_WEEKEND:
      if(timer_vals->enable_val) {
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
  switch(timer_vals->current_select) {
    case TIMER_CURRENT_SELECT_ENABLE:
      timer_vals->enable_val = !timer_vals->enable_val;
      break;
    case TIMER_CURRENT_SELECT_HR:
      if(timer_vals->hr_val <= timer_vals->hr_min_val) {
        timer_vals->hr_val = timer_vals->hr_max_val;
      }
      else {
        timer_vals->hr_val--;
      }
      break;
    case TIMER_CURRENT_SELECT_MIN:
      if(timer_vals->min_val <= 0) {
        timer_vals->min_val = TIMER_MAX_MIN;
      }
      else {
        timer_vals->min_val--;
      }
      break;
  }
  update_ui();
}

/**
 * On back click
**/
static void timer_back_click_handler(ClickRecognizerRef recognizer, void *context) {
  timer_vals->current_select--;
  if(timer_vals->current_select < TIMER_CURRENT_SELECT_ENABLE) {
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
  
  timer_vals->img_up = gbitmap_create_with_resource(RESOURCE_ID_IMG_UP);
  timer_vals->img_down = gbitmap_create_with_resource(RESOURCE_ID_IMG_DOWN);
  timer_vals->img_enable = gbitmap_create_with_resource(RESOURCE_ID_IMG_TICK);
  timer_vals->img_disable = gbitmap_create_with_resource(RESOURCE_ID_IMG_CROSS);
  
  #if defined(PBL_ROUND)
  timer_vals->up_bmp_layer = macro_bitmap_layer_create(timer_vals->img_up, GRect(28, 52, 115, 16), timer_window_layer, true);
  timer_vals->down_bmp_layer = macro_bitmap_layer_create(timer_vals->img_down, GRect(28, 120, 115, 16), timer_window_layer, true);
  timer_vals->enable_bmp_layer = macro_bitmap_layer_create(timer_vals->img_disable, GRect(28, 86, 16, 16), timer_window_layer, false);
  
  timer_vals->header_text = macro_text_layer_create(GRect(28, 17, 124, 30), timer_window_layer, COLOR_TEXT, COLOR_TEXT_BACKGROUND, fonts_get_system_font(FONT_KEY_GOTHIC_24), GTextAlignmentCenter);
  timer_vals->footer_text = macro_text_layer_create(GRect(28, 132, 124, 30), timer_window_layer, COLOR_TEXT, COLOR_TEXT_BACKGROUND, fonts_get_system_font(FONT_KEY_GOTHIC_24), GTextAlignmentCenter);
  timer_vals->hr_text = macro_text_layer_create(GRect(58, 68, 45, 40), timer_window_layer, COLOR_TEXT, COLOR_TEXT_BACKGROUND, forcedSquare, GTextAlignmentCenter);
  timer_vals->min_text = macro_text_layer_create(GRect(118, 68, 45, 40), timer_window_layer, COLOR_TEXT, COLOR_TEXT_BACKGROUND, forcedSquare, GTextAlignmentCenter);
  timer_vals->colon_text = macro_text_layer_create(GRect(103, 68, 15, 40), timer_window_layer, COLOR_TEXT, COLOR_TEXT_BACKGROUND, forcedSquare, GTextAlignmentCenter);
  #else
  GSize size = layer_get_bounds(timer_window_layer).size;
  int mainWidth = 130;
  int leftShift = (size.w-mainWidth)/2;
  int mainHeight = 76;
  int topShift = (size.h-mainHeight)/2;
  
  timer_vals->up_bmp_layer = macro_bitmap_layer_create(timer_vals->img_up, GRect(leftShift, topShift, 115, 16), timer_window_layer, true);
  timer_vals->down_bmp_layer = macro_bitmap_layer_create(timer_vals->img_down, GRect(leftShift, 60+topShift, 115, 16), timer_window_layer, true);
  timer_vals->enable_bmp_layer = macro_bitmap_layer_create(timer_vals->img_disable, GRect(leftShift, 30+topShift, 16, 16), timer_window_layer, false);
  
  timer_vals->header_text = macro_text_layer_create(GRect(10, (topShift-30)/2, size.w-20, 30), timer_window_layer, COLOR_TEXT, COLOR_TEXT_BACKGROUND, fonts_get_system_font(FONT_KEY_GOTHIC_24), GTextAlignmentCenter);
  timer_vals->footer_text = macro_text_layer_create(GRect(10, size.h-((topShift-30)/2)-30, size.w-20, 30), timer_window_layer, COLOR_TEXT, COLOR_TEXT_BACKGROUND, fonts_get_system_font(FONT_KEY_GOTHIC_24), GTextAlignmentCenter);
  timer_vals->hr_text = macro_text_layer_create(GRect(40+leftShift, 12+topShift, 40, 40), timer_window_layer, COLOR_TEXT, COLOR_TEXT_BACKGROUND, forcedSquare, GTextAlignmentCenter);
  timer_vals->min_text = macro_text_layer_create(GRect(90+leftShift, 12+topShift, 40, 40), timer_window_layer, COLOR_TEXT, COLOR_TEXT_BACKGROUND, forcedSquare, GTextAlignmentCenter);
  timer_vals->colon_text = macro_text_layer_create(GRect(80+leftShift, 12+topShift, 10, 40), timer_window_layer, COLOR_TEXT, COLOR_TEXT_BACKGROUND, forcedSquare, GTextAlignmentCenter);
  #endif
  
  //set initial values for each timer type
  uint idle_time = persist_exists(PERSIST_IDLE_TIME) ? persist_read_int(PERSIST_IDLE_TIME) : 120;
  timer_vals->current_select = TIMER_CURRENT_SELECT_ENABLE;
  switch(timer_vals->timer_type) {
    case TIMER_TYPE_WEEKEND:
      wkend = (uint16_t)persist_read_int(PERSIST_WKE);
      if(wkend >= 0) {
        timer_vals->enable_val = true;
        timer_vals->hr_val = wkend/60;
        timer_vals->min_val = wkend%60;
      }
      else {
        timer_vals->enable_val = false;
        timer_vals->hr_val = TIMER_MIN_HR_WEEKEND;
        timer_vals->min_val = 0; 
      }
      timer_vals->hr_max_val = TIMER_MAX_HR_WEEKEND;
      timer_vals->hr_min_val = TIMER_MIN_HR_WEEKDAY;
      text_layer_set_text(timer_vals->header_text, "Set alarm");
      text_layer_set_text(timer_vals->footer_text, "weekends");
      break;
    case TIMER_TYPE_WEEKDAY:
      wkday = (uint16_t)persist_read_int(PERSIST_WKD);
      if(wkday >= 0) {
        timer_vals->enable_val = true;
        timer_vals->hr_val = wkday/60;
        timer_vals->min_val = wkday%60;
      }
      else {
        timer_vals->enable_val = false;
        timer_vals->hr_val = TIMER_MIN_HR_WEEKDAY;
        timer_vals->min_val = 0; 
      }
      timer_vals->hr_max_val = TIMER_MAX_HR_WEEKDAY;
      timer_vals->hr_min_val = TIMER_MIN_HR_WEEKDAY;
      text_layer_set_text(timer_vals->header_text, "Set alarm");
      text_layer_set_text(timer_vals->footer_text, "weekdays");
      break;
    case TIMER_TYPE_IDLE:
      if(idle_time > 0) {
        timer_vals->enable_val = true;
        timer_vals->hr_val = idle_time/60;
        timer_vals->min_val = idle_time%60;
      }
      else {
        timer_vals->enable_val = false;
        timer_vals->hr_val = TIMER_MIN_HR_IDLE;
        timer_vals->min_val = 0;
      }
      timer_vals->hr_max_val = TIMER_MAX_HR_IDLE;
      timer_vals->hr_min_val = TIMER_MIN_HR_IDLE;
      text_layer_set_text(timer_vals->header_text, "When idle for");
      text_layer_set_text(timer_vals->footer_text, "show alert");
      break;
  }
  text_layer_set_text(timer_vals->colon_text, ":");
  update_ui();
}


static void timer_window_unload(Window *window) {
  text_layer_destroy(timer_vals->header_text);
  text_layer_destroy(timer_vals->footer_text);
  text_layer_destroy(timer_vals->hr_text);
  text_layer_destroy(timer_vals->min_text);
  text_layer_destroy(timer_vals->colon_text);
  bitmap_layer_destroy(timer_vals->up_bmp_layer);
  bitmap_layer_destroy(timer_vals->down_bmp_layer);
  bitmap_layer_destroy(timer_vals->enable_bmp_layer);
  gbitmap_destroy(timer_vals->img_up);
  gbitmap_destroy(timer_vals->img_down);
  gbitmap_destroy(timer_vals->img_enable);
  gbitmap_destroy(timer_vals->img_disable);
  window_destroy(timer_window);
  timer_window = NULL;
  free(timer_vals);
}
  
void timer_init(uint8_t type) {
  timer_vals = malloc(sizeof(TimerVals));
  snprintf(timer_vals->hr_buf, 3, "00");
  snprintf(timer_vals->min_buf, 3, "00");
  
  timer_vals->timer_type = type;
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