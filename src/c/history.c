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
Layer *graph_layer;
TextLayer *history_title_text, *history_empty_text, *history_max_text, *history_avg_text;
uint32_t history_vals[7];
uint32_t max_val = 0;
uint32_t avg_val;
time_t now_time;
struct tm time_now;
char buf_m[15] = "";
char buf_a[15] = "";
GColor background;

/**
 * Redraw graph layer according to values
**/
static void graph_update_proc(Layer *this_layer, GContext *ctx) {
  int16_t layer_height = layer_get_frame(this_layer).size.h;
  int16_t layer_width = layer_get_frame(this_layer).size.w;
  if(max_val > 0) {
    for(int i=0; i<7; i++) {
      double scale = (double)max_val/layer_height;
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
      uint avg_height = (uint)(layer_height-avg_val/scale);
      if(avg_height > 0) {
        graphics_context_set_stroke_color(ctx, background);
        graphics_draw_line(ctx, GPoint(0, avg_height+1), GPoint(layer_width, avg_height+1));
        graphics_context_set_stroke_color(ctx, COLOR_TEXT);
        graphics_draw_line(ctx, GPoint(0, avg_height), GPoint(layer_width, avg_height));
        graphics_context_set_stroke_color(ctx, background);
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
    snprintf(buf_m, 15, "Max: %u", (uint)max_val);
    snprintf(buf_a, 15, "Avg: %u", (uint)avg_val);
  }
  else if(page == UI_ENERGY) {
    snprintf(buf_m, 15, "Max:%u kCal", (uint)max_val);
    snprintf(buf_a, 15, "Avg:%u kCal", (uint)avg_val);
  }
  else {
    if(max_val >= 60*60) {
      snprintf(buf_m, 15, "Max:%uh %um", (uint)max_val/(60*60), (uint)(max_val/60)%60);
    }
    else {
      snprintf(buf_m, 15, "Max:%um", (uint)(max_val/60));
    }
    if(avg_val >= 60*60) {
      snprintf(buf_a, 15, "Avg:%uh %um", (uint)avg_val/(60*60), (uint)(avg_val/60)%60);
    }
    else {
      snprintf(buf_a, 15, "Avg:%um", (uint)(avg_val/60));
    }
  }
  text_layer_set_text(history_max_text, buf_m);
  text_layer_set_text(history_avg_text, buf_a);

  switch(page) {
    case UI_STEPS:
        text_layer_set_text(history_title_text, "Steps");
        #ifdef PBL_COLOR
        background = GColorCobaltBlue;
        #endif
        break;
    case UI_WALK:
        text_layer_set_text(history_title_text, "Walk");
        #ifdef PBL_COLOR
        background = GColorDarkCandyAppleRed;
        #endif
        break;
    case UI_RUN:
        text_layer_set_text(history_title_text, "Run");
        #ifdef PBL_COLOR
        background = GColorDarkGreen;
        #endif
        break;
    case UI_SLEEP:
        text_layer_set_text(history_title_text, "Sleep");
        #ifdef PBL_COLOR
        background = GColorArmyGreen;
        #endif
        break;
    case UI_ENERGY:
        text_layer_set_text(history_title_text, "Energy");
        #ifdef PBL_COLOR
        background = GColorWindsorTan;
        #endif
        break;
    case UI_FLOORS:
        text_layer_set_text(history_title_text, "Floors");
        #ifdef PBL_COLOR
        background = GColorIndigo;
        #endif
        break;
    case UI_CYCLE:
        text_layer_set_text(history_title_text, "Cycle");
        #ifdef PBL_COLOR
        background = GColorBlue;
        #endif
        break;
    case UI_SPORT:
        text_layer_set_text(history_title_text, "Sport");
        #ifdef PBL_COLOR
        background = GColorMidnightGreen;
        #endif
        break;
    case UI_TRAIN:
        text_layer_set_text(history_title_text, "Train");
        #ifdef PBL_COLOR
        background = GColorBulgarianRose;
        #endif
        break;
    case UI_WATER:
        text_layer_set_text(history_title_text, "Water sp.");
        #ifdef PBL_COLOR
        background = GColorBlueMoon;
        #endif
        break;
    case UI_WINTER:
        text_layer_set_text(history_title_text, "Winter sp.");
        #ifdef PBL_COLOR
        background = GColorIndigo;
        #endif
        break;
    case UI_MARTIAL:
        text_layer_set_text(history_title_text, "Martial art");
        #ifdef PBL_COLOR
        background = GColorOrange;
        #endif
        break;
    case UI_OTHER:
        text_layer_set_text(history_title_text, "Other");
        #ifdef PBL_COLOR
        background = GColorImperialPurple;
        #endif
        break;
  }
  #ifdef PBL_COLOR
  window_set_background_color(history_window, background);
  #endif
  
  layer_mark_dirty(graph_layer);
  curr_page = page;
  
  if(max_val > 0) {
    layer_set_hidden(text_layer_get_layer(history_empty_text), true);
  }
  else {
    text_layer_set_text(history_empty_text, "No data");
    layer_set_hidden(text_layer_get_layer(history_empty_text), false);
  }
}

/**
 * Use values received from phone
**/
void set_phone_history_vals(int *page) {
  if(history_window != NULL && window_is_loaded(history_window)) {
    int8_t count = 0;
    avg_val = 0;
    max_val = 0;
    for(int8_t i=0; i<7; i++) {
      if(history_vals[i] > max_val) max_val = history_vals[i];
      if(history_vals[i] > 0) {
        avg_val += history_vals[i];
        count++;
      }
    }
    if(count > 0) avg_val /= count;
    make_history_ui(*page);
  }
}

/**
 * Read values from storage
**/
void set_history_vals(int8_t page) {
  uint8_t day;
  time_t reset_time = persist_read_int(PERSIST_RESET_TIMESTAMP);
  if(time_now.tm_hour > 2 || now_time - reset_time < 3*60*60) {
    day = time_now.tm_wday;
  }
  else {
    day = (time_now.tm_wday+6)%7;
  }
  int data_start = 0;
  switch(page) {
    case UI_STEPS:
      data_start = PERSIST_STEPS_DAY0;
      avg_val = persist_read_int(PERSIST_AVG_STEPS);
      break;
    case UI_WALK:
      data_start = PERSIST_WALK_DAY0;
      avg_val = persist_read_int(PERSIST_AVG_WALK);
      break;
    case UI_RUN:
      data_start = PERSIST_RUN_DAY0;
      avg_val = persist_read_int(PERSIST_AVG_JOG);
      break;
    case UI_SLEEP:
      data_start = PERSIST_SLEEP_DAY0;
      avg_val = persist_read_int(PERSIST_AVG_SLEEP);
      if(time_now.tm_hour > 14 || (time_now.tm_hour > 11 && now_time - reset_time < 3*60*60)) {
        day = time_now.tm_wday;
      }
      else {
        day = (time_now.tm_wday+6)%7;
      }
      break;
  }
  
  max_val = 0;
  
  for(int i=0; i<7; i++) {
    history_vals[i] = persist_read_int(data_start+(day+i)%7);
    if(history_vals[i] > max_val) max_val = history_vals[i];
  }
  make_history_ui(page);
}

/**
 * On up click
**/
static void history_up_click_handler(ClickRecognizerRef recognizer, void *context) {
  if(persist_read_bool(PERSIST_DISPLAY_DATA_PHONE)) {
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
  if(persist_read_bool(PERSIST_DISPLAY_DATA_PHONE)) {
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
  background = COLOR_BACKGROUND;
  
  for(int i=0; i<7; i++) {
   history_vals[i] = 0;
  }
  avg_val = 0;
  max_val = 0;
  
  #if defined(PBL_ROUND)
  graph_layer = layer_create(GRect(20, 40, 140, 100));
  history_title_text = macro_text_layer_create(GRect(35, 15, 110, 25), history_window_layer, COLOR_TEXT, COLOR_TEXT_BACKGROUND, fonts_get_system_font(FONT_KEY_GOTHIC_18), GTextAlignmentCenter);
  history_empty_text = macro_text_layer_create(GRect(20, 80, 140, 20), history_window_layer, COLOR_TEXT, COLOR_TEXT_BACKGROUND, fonts_get_system_font(FONT_KEY_GOTHIC_14), GTextAlignmentCenter);
  history_max_text = macro_text_layer_create(GRect(20, 140, 140, 15), history_window_layer, COLOR_TEXT, COLOR_TEXT_BACKGROUND, fonts_get_system_font(FONT_KEY_GOTHIC_14), GTextAlignmentCenter);
  history_avg_text = macro_text_layer_create(GRect(20, 155, 140, 15), history_window_layer, COLOR_TEXT, COLOR_TEXT_BACKGROUND, fonts_get_system_font(FONT_KEY_GOTHIC_14), GTextAlignmentCenter);
  #else
  graph_layer = layer_create(GRect(2, 40, 140, 100));
  history_title_text = macro_text_layer_create(GRect(10, 5, 124, 25), history_window_layer, COLOR_TEXT, COLOR_TEXT_BACKGROUND, fonts_get_system_font(FONT_KEY_GOTHIC_24), GTextAlignmentLeft);
  history_empty_text = macro_text_layer_create(GRect(2, 100, 140, 20), history_window_layer, COLOR_TEXT, COLOR_TEXT_BACKGROUND, fonts_get_system_font(FONT_KEY_GOTHIC_14), GTextAlignmentCenter);
  history_max_text = macro_text_layer_create(GRect(2, 145, 68, 15), history_window_layer, COLOR_TEXT, COLOR_TEXT_BACKGROUND, fonts_get_system_font(FONT_KEY_GOTHIC_14), GTextAlignmentCenter);
  history_avg_text = macro_text_layer_create(GRect(74, 145, 68, 15), history_window_layer, COLOR_TEXT, COLOR_TEXT_BACKGROUND, fonts_get_system_font(FONT_KEY_GOTHIC_14), GTextAlignmentCenter);
  #endif
  
  layer_set_update_proc(graph_layer, graph_update_proc);
  layer_add_child(history_window_layer, graph_layer);
  layer_set_hidden(text_layer_get_layer(history_empty_text), true);
  
  now_time = time(NULL);
  time_now = *localtime(&now_time);
}

static void history_window_unload(Window *window) {
  text_layer_destroy(history_title_text);
  text_layer_destroy(history_empty_text);
  text_layer_destroy(history_max_text);
  text_layer_destroy(history_avg_text);
  layer_destroy(graph_layer);
  window_destroy(history_window);
  history_window = NULL;
}

void history_init() {
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
  if(persist_read_bool(PERSIST_DISPLAY_DATA_PHONE)) {
    if(connection_service_peek_pebble_app_connection()) {
      appmsg_send_val(APPMSG_HISTORY_VAL, PHONE_SEND_NO_BUTTON);
      text_layer_set_text(history_empty_text, "Access phone data...");
      layer_set_hidden(text_layer_get_layer(history_empty_text), false);
      #ifdef PBL_COLOR
      window_set_background_color(history_window, GColorBlueMoon);
      #endif
    }
    else {
      text_layer_set_text(history_empty_text, "Phone disconnected");
      layer_set_hidden(text_layer_get_layer(history_empty_text), false);
    }
  }
  else {
    set_history_vals(curr_page);
  }
}
