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
#include "history.h"

Window *history_window;
uint32_t history_vals[7]/* = {28680, 18860, 23234, 18690, 27234, 21320, 24320}*/;

typedef struct{
  Layer *graph_layer;
  TextLayer *history_title_text, *history_empty_text, *history_max_text, *history_avg_text;
  uint32_t max_val;
  uint32_t avg_val/* = 23165*/;
  time_t now_time;
  struct tm time_now;
  char buf_m[15];
  char buf_a[15];
  GColor background;
}HistoryVals;
HistoryVals *hist_vals;

/*Layer *graph_layer;
TextLayer *history_title_text, *history_empty_text, *history_max_text, *history_avg_text;
uint32_t max_val = 0;
uint32_t avg_val;
time_t now_time;
struct tm time_now;
char buf_m[15] = "";
char buf_a[15] = "";
GColor background;*/

/**
 * Redraw graph layer according to values
**/
static void graph_update_proc(Layer *this_layer, GContext *ctx) {
  int16_t layer_height = layer_get_frame(this_layer).size.h;
  int16_t layer_width = layer_get_frame(this_layer).size.w;
  if(hist_vals->max_val > 0) {
    for(int i=0; i<7; i++) {
      double scale = (double)hist_vals->max_val/layer_height;
      //draw rectangle for each weekly value
      if(history_vals[i] > 0) {
        uint startx = i*layer_width/7+4;
        uint width = layer_width/7-8;
        uint height = (uint)(history_vals[i]/scale);
        uint starty = layer_height-height;
        graphics_context_set_fill_color(ctx, COLOR_TEXT);
        graphics_fill_rect(ctx, GRect(startx, starty, width, height), 0, GCornerNone);
      }
      //draw average line
      uint avg_height = (uint)(layer_height-hist_vals->avg_val/scale);
      if(avg_height > 0) {
        graphics_context_set_stroke_color(ctx, hist_vals->background);
        graphics_draw_line(ctx, GPoint(0, avg_height+1), GPoint(layer_width, avg_height+1));
        graphics_context_set_stroke_color(ctx, COLOR_TEXT);
        graphics_draw_line(ctx, GPoint(0, avg_height), GPoint(layer_width, avg_height));
        graphics_context_set_stroke_color(ctx, hist_vals->background);
        graphics_draw_line(ctx, GPoint(0, avg_height-1), GPoint(layer_width, avg_height-1));
      }      
    }
  }
}

/**
 * Redraw UI based on read values
**/
void make_history_ui(int8_t page) {  
  if(page == UI_STEPS || page == UI_FLOORS) {
    snprintf(hist_vals->buf_m, 15, "Max: %u", (uint)hist_vals->max_val);
    snprintf(hist_vals->buf_a, 15, "Avg: %u", (uint)hist_vals->avg_val);
  }
  else if(page == UI_HR) {
    snprintf(hist_vals->buf_m, 15, "Max: %u bpm", (uint)hist_vals->max_val);
    snprintf(hist_vals->buf_a, 15, "Avg: %u bpm", (uint)hist_vals->avg_val);
  }
  else if(page == UI_ENERGY) {
    snprintf(hist_vals->buf_m, 15, "Max:%u kCal", (uint)hist_vals->max_val);
    snprintf(hist_vals->buf_a, 15, "Avg:%u kCal", (uint)hist_vals->avg_val);
  }
  else {
    if(hist_vals->max_val >= 60*60) {
      snprintf(hist_vals->buf_m, 15, "Max:%uh %um", (uint)hist_vals->max_val/(60*60), (uint)(hist_vals->max_val/60)%60);
    }
    else {
      snprintf(hist_vals->buf_m, 15, "Max:%um", (uint)(hist_vals->max_val/60));
    }
    if(hist_vals->avg_val >= 60*60) {
      snprintf(hist_vals->buf_a, 15, "Avg:%uh %um", (uint)hist_vals->avg_val/(60*60), (uint)(hist_vals->avg_val/60)%60);
    }
    else {
      snprintf(hist_vals->buf_a, 15, "Avg:%um", (uint)(hist_vals->avg_val/60));
    }
  }
  text_layer_set_text(hist_vals->history_max_text, hist_vals->buf_m);
  text_layer_set_text(hist_vals->history_avg_text, hist_vals->buf_a);

  switch(page) {
    case UI_STEPS:
        text_layer_set_text(hist_vals->history_title_text, "Steps");
        #ifdef PBL_COLOR
        hist_vals->background = GColorCobaltBlue;
        #endif
        break;
    case UI_WALK:
        text_layer_set_text(hist_vals->history_title_text, "Walk");
        #ifdef PBL_COLOR
        hist_vals->background = GColorDarkCandyAppleRed;
        #endif
        break;
    case UI_RUN:
        text_layer_set_text(hist_vals->history_title_text, "Run");
        #ifdef PBL_COLOR
        hist_vals->background = GColorDarkGreen;
        #endif
        break;
    case UI_SLEEP:
        text_layer_set_text(hist_vals->history_title_text, "Sleep");
        #ifdef PBL_COLOR
        hist_vals->background = GColorArmyGreen;
        #endif
        break;
    case UI_ENERGY:
        text_layer_set_text(hist_vals->history_title_text, "Energy");
        #ifdef PBL_COLOR
        hist_vals->background = GColorWindsorTan;
        #endif
        break;
    case UI_FLOORS:
        text_layer_set_text(hist_vals->history_title_text, "Floors");
        #ifdef PBL_COLOR
        hist_vals->background = GColorIndigo;
        #endif
        break;
    case UI_HR:
        text_layer_set_text(hist_vals->history_title_text, "Resting HR");
        #ifdef PBL_COLOR
        hist_vals->background = GColorBulgarianRose;
        #endif
        break;
    case UI_CYCLE:
        text_layer_set_text(hist_vals->history_title_text, "Cycle");
        #ifdef PBL_COLOR
        hist_vals->background = GColorBlue;
        #endif
        break;
    case UI_SPORT:
        text_layer_set_text(hist_vals->history_title_text, "Sport");
        #ifdef PBL_COLOR
        hist_vals->background = GColorMidnightGreen;
        #endif
        break;
    case UI_TRAIN:
        text_layer_set_text(hist_vals->history_title_text, "Train");
        #ifdef PBL_COLOR
        hist_vals->background = GColorBulgarianRose;
        #endif
        break;
    case UI_WATER:
        text_layer_set_text(hist_vals->history_title_text, "Water sp.");
        #ifdef PBL_COLOR
        hist_vals->background = GColorBlueMoon;
        #endif
        break;
    case UI_WINTER:
        text_layer_set_text(hist_vals->history_title_text, "Winter sp.");
        #ifdef PBL_COLOR
        hist_vals->background = GColorIndigo;
        #endif
        break;
    case UI_MARTIAL:
        text_layer_set_text(hist_vals->history_title_text, "Martial art");
        #ifdef PBL_COLOR
        hist_vals->background = GColorOrange;
        #endif
        break;
    case UI_OTHER:
        text_layer_set_text(hist_vals->history_title_text, "Other");
        #ifdef PBL_COLOR
        hist_vals->background = GColorImperialPurple;
        #endif
        break;
  }
  #ifdef PBL_COLOR
  window_set_background_color(history_window, hist_vals->background);
  #endif
  
  layer_mark_dirty(hist_vals->graph_layer);
  curr_page = page;
  
  if(hist_vals->max_val > 0) {
    layer_set_hidden(text_layer_get_layer(hist_vals->history_empty_text), true);
  }
  else {
    text_layer_set_text(hist_vals->history_empty_text, "No data");
    layer_set_hidden(text_layer_get_layer(hist_vals->history_empty_text), false);
  }
}

/**
 * Use values received from phone
**/
void set_phone_history_vals(int *page) {
  //APP_LOG(APP_LOG_LEVEL_INFO, "page num: %d", *page);
  if(history_window != NULL && window_is_loaded(history_window)) {
    int8_t count = 0;
    hist_vals->avg_val = 0;
    hist_vals->max_val = 0;
    for(int8_t i=0; i<7; i++) {
      //history_vals[i] = p_vals[i];
      if(history_vals[i] > hist_vals->max_val) hist_vals->max_val = history_vals[i];
      if(history_vals[i] > 0) {
        hist_vals->avg_val += history_vals[i];
        count++;
      }
    }
    if(count > 0) hist_vals->avg_val /= count;
    make_history_ui(*page);
  }
}

/**
 * Read values from storage
**/
void set_history_vals(int8_t page) {
  uint8_t day;
  time_t reset_time = persist_read_int(PERSIST_RESET_TIMESTAMP);
  if(hist_vals->time_now.tm_hour > 2 || hist_vals->now_time - reset_time < 3*60*60) {
    day = hist_vals->time_now.tm_wday;
  }
  else {
    day = (hist_vals->time_now.tm_wday+6)%7;
  }
  int data_start = 0;
  switch(page) {
    case UI_STEPS:
      data_start = PERSIST_STEPS_DAY0;
      hist_vals->avg_val = persist_read_int(PERSIST_AVG_STEPS);
      break;
    case UI_WALK:
      data_start = PERSIST_WALK_DAY0;
      hist_vals->avg_val = persist_read_int(PERSIST_AVG_WALK);
      break;
    case UI_RUN:
      data_start = PERSIST_RUN_DAY0;
      hist_vals->avg_val = persist_read_int(PERSIST_AVG_JOG);
      break;
    case UI_SLEEP:
      data_start = PERSIST_SLEEP_DAY0;
      hist_vals->avg_val = persist_read_int(PERSIST_AVG_SLEEP);
      //avg_val = 23165;
      if(hist_vals->time_now.tm_hour > 14 || (hist_vals->time_now.tm_hour > 11 && hist_vals->now_time - reset_time < 3*60*60)) {
        day = hist_vals->time_now.tm_wday;
      }
      else {
        day = (hist_vals->time_now.tm_wday+6)%7;
      }
      break;
  }
  
  hist_vals->max_val = 0;
  
  //uint32_t dummy_vals[7] = {28680, 18860, 23234, 18690, 27234, 21320, 24320};
  for(int i=0; i<7; i++) {
    history_vals[i] = persist_read_int(data_start+(day+i)%7);
    //history_vals[i] = dummy_vals[i];
    if(history_vals[i] > hist_vals->max_val) hist_vals->max_val = history_vals[i];
  }
  make_history_ui(page);
}

/**
 * On up click
**/
static void history_up_click_handler(ClickRecognizerRef recognizer, void *context) {
  if(phoneDataSharing) {
    //ask phone for values
    appmsg_send_val(APPMSG_HISTORY_VAL, PHONE_SEND_BUTTON_UP);
  }
  else if(curr_page < NUM_UI_PAGES) {
    //use storage values
    int8_t page = (curr_page-1)%NUM_UI_PAGES;
    if(page < 0) {
      page = NUM_UI_PAGES-1;
    }
    set_history_vals(page);
  }
}

/**
 * On select click
**/
static void history_select_click_handler(ClickRecognizerRef recognizer, void *context) {
  window_stack_pop(true);
  menu_init();
}

/**
 * On back click
**/
static void history_back_click_handler(ClickRecognizerRef recognizer, void *context) {
  window_stack_pop(true);
  day_init(true);
}

/**
 * On down click
**/
static void history_down_click_handler(ClickRecognizerRef recognizer, void *context) {
  if(phoneDataSharing) {
    //ask phone for values
    appmsg_send_val(APPMSG_HISTORY_VAL, PHONE_SEND_BUTTON_DOWN);
  }
  else if(curr_page < NUM_UI_PAGES) {
    //use values from storage
    int8_t page = (curr_page+1)%NUM_UI_PAGES;
    set_history_vals(page);
  }
}

static void history_click_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_UP, history_up_click_handler);
  window_single_click_subscribe(BUTTON_ID_SELECT, history_select_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, history_down_click_handler);
  window_single_click_subscribe(BUTTON_ID_BACK, history_back_click_handler);
}

static void history_window_load(Window *window) {
  Layer* history_window_layer = window_get_root_layer(window);
  window_set_background_color(window, COLOR_BACKGROUND);
  hist_vals->background = COLOR_BACKGROUND;
  
  for(int i=0; i<7; i++) {
   history_vals[i] = 0;
  }
  hist_vals->avg_val = 0;
  hist_vals->max_val = 0;
  
  #if defined(PBL_ROUND)
  hist_vals->graph_layer = layer_create(GRect(20, 40, 140, 100));
  hist_vals->history_title_text = macro_text_layer_create(GRect(35, 15, 110, 25), history_window_layer, COLOR_TEXT, COLOR_TEXT_BACKGROUND, fonts_get_system_font(FONT_KEY_GOTHIC_18), GTextAlignmentCenter);
  hist_vals->history_empty_text = macro_text_layer_create(GRect(20, 80, 140, 20), history_window_layer, COLOR_TEXT, COLOR_TEXT_BACKGROUND, fonts_get_system_font(FONT_KEY_GOTHIC_14), GTextAlignmentCenter);
  hist_vals->history_max_text = macro_text_layer_create(GRect(20, 140, 140, 15), history_window_layer, COLOR_TEXT, COLOR_TEXT_BACKGROUND, fonts_get_system_font(FONT_KEY_GOTHIC_14), GTextAlignmentCenter);
  hist_vals->history_avg_text = macro_text_layer_create(GRect(20, 155, 140, 15), history_window_layer, COLOR_TEXT, COLOR_TEXT_BACKGROUND, fonts_get_system_font(FONT_KEY_GOTHIC_14), GTextAlignmentCenter);
  #else
  GSize size = layer_get_bounds(history_window_layer).size;
  
  //graph_layer = layer_create(GRect(2, 40, 140, 100));
  hist_vals->graph_layer = layer_create(GRect(2, 40, size.w-4, size.h-55));
  hist_vals->history_title_text = macro_text_layer_create(GRect(10, 5, 124, 25), history_window_layer, COLOR_TEXT, COLOR_TEXT_BACKGROUND, fonts_get_system_font(FONT_KEY_GOTHIC_24), GTextAlignmentLeft);
  hist_vals->history_empty_text = macro_text_layer_create(GRect(2, size.h/2, size.w-4, 20), history_window_layer, COLOR_TEXT, COLOR_TEXT_BACKGROUND, fonts_get_system_font(FONT_KEY_GOTHIC_14), GTextAlignmentCenter);
  hist_vals->history_max_text = macro_text_layer_create(GRect(2, size.h-17, size.w/2-4, 15), history_window_layer, COLOR_TEXT, COLOR_TEXT_BACKGROUND, fonts_get_system_font(FONT_KEY_GOTHIC_14), GTextAlignmentCenter);
  hist_vals->history_avg_text = macro_text_layer_create(GRect(74, size.h-17, size.w/2-4, 15), history_window_layer, COLOR_TEXT, COLOR_TEXT_BACKGROUND, fonts_get_system_font(FONT_KEY_GOTHIC_14), GTextAlignmentCenter);
  #endif
  
  layer_set_update_proc(hist_vals->graph_layer, graph_update_proc);
  layer_add_child(history_window_layer, hist_vals->graph_layer);
  layer_set_hidden(text_layer_get_layer(hist_vals->history_empty_text), true);
  
  hist_vals->now_time = time(NULL);
  hist_vals->time_now = *localtime(&hist_vals->now_time);
}

static void history_window_unload(Window *window) {
  text_layer_destroy(hist_vals->history_title_text);
  text_layer_destroy(hist_vals->history_empty_text);
  text_layer_destroy(hist_vals->history_max_text);
  text_layer_destroy(hist_vals->history_avg_text);
  layer_destroy(hist_vals->graph_layer);
  window_destroy(history_window);
  history_window = NULL;
  free(hist_vals);
}

void history_init() {
  hist_vals = malloc(sizeof(HistoryVals));
  hist_vals->max_val = 0;
  //snprintf(hist_vals->buf_m, 15, "");
  //snprintf(hist_vals->buf_a, 15, "");
  
  if(history_window == NULL) {
    history_window = window_create();
    window_set_window_handlers(history_window, (WindowHandlers) {
      .load = history_window_load,
      .unload = history_window_unload
    });
    window_set_click_config_provider(history_window, history_click_provider);
  }
  if(!window_is_loaded(history_window)) {
    window_stack_push(history_window, true);
  }
  if(phoneDataSharing) {
    if(connection_service_peek_pebble_app_connection()) {
      appmsg_send_val(APPMSG_HISTORY_VAL, PHONE_SEND_NO_BUTTON);
      text_layer_set_text(hist_vals->history_empty_text, "Access phone data...");
      layer_set_hidden(text_layer_get_layer(hist_vals->history_empty_text), false);
      #ifdef PBL_COLOR
      window_set_background_color(history_window, GColorBlueMoon);
      #endif
    }
    else {
      text_layer_set_text(hist_vals->history_empty_text, "Phone disconnected");
      layer_set_hidden(text_layer_get_layer(hist_vals->history_empty_text), false);
    }
  }
  else {
    set_history_vals(curr_page);
  }
}