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
#include "active.h"

Window *active_window;
TextLayer *active_time, *active_duration_text, *active_duration_text_label,
        *active_distance_text, *active_distance_text_label, *active_speed_text, *active_speed_text_label;
ActionBarLayer *active_action_bar;
bool active_paused=false;
GBitmap *img_stop, *img_pause, *img_play;
uint start_time = 0;
uint duration = 0;
int gps_ble_track = false;
char dur_buf[6] = "00:00";

/**
 * Log session to phone
**/
static void send_session(Session session) {
  DataLoggingSessionRef sessionsLog = data_logging_create(
    2223,                            // tag
    DATA_LOGGING_BYTE_ARRAY,        // DataLoggingItemType
    sizeof(Session),                // length
    true                            // resume
  );
  Session manualSession[1] = {(session)};
  data_logging_log(sessionsLog, manualSession, 1);
  data_logging_finish(sessionsLog);
}

/**
 * Calculate and set the current duration
**/
void active_set_duration() {
  if(active_window != NULL && window_is_loaded(active_window)) {
    uint dur = duration/60;
    if(!active_paused) {
      dur += (time(NULL) - start_time)/60;
    }
    snprintf(dur_buf, 6, "%.2u:%.2u", dur/60, dur%60);
    text_layer_set_text(active_duration_text, dur_buf);
  }
}

/**
 * Set the speed
**/
void active_set_speed(char *value) {
  if(active_window != NULL && window_is_loaded(active_window)) {    
    text_layer_set_text(active_speed_text, value);
  }
}

/**
 * Set heart rate
**/
void active_set_heart(char *value) {
  if(active_window != NULL && window_is_loaded(active_window)) {
    text_layer_set_text(active_speed_text_label, "Heart rate");
    if(gps_ble_track == GPS_OFF_BLE_ON)
      text_layer_set_text(active_speed_text, value);
  }
}

/**
 * Set distance
**/
void active_set_distance(char *value) {  
  if(active_window != NULL && window_is_loaded(active_window)) {
    text_layer_set_text(active_distance_text, value);
  }
}

/**
 * Set top time display and reset duration
**/
void active_set_time(char *value) {
  if(active_window != NULL && window_is_loaded(active_window)) {
    text_layer_set_text(active_time, value);
    
    active_set_duration();
  }
}

/**
 * Pause and unpause session
**/
void active_set_paused(bool paused) {
  active_paused = paused;
  persist_write_bool(PERSIST_ACTIVE_TRACK_PAUSE, active_paused);
  uint typ = (uint)persist_read_int(PERSIST_ACTIVE_TRACK_TYPE);
  uint this_time = time(NULL);
  if(paused) {
    start_time = (uint)persist_read_int(PERSIST_ACTIVE_TRACK_START);  
    if(start_time > 0) {
      if(typ != TYPE_UNKNOWN) {
        Session session = {.startTime=start_time, .endTime=time(NULL), .steps=0, .type=typ};
        send_session(session);
      }      
      persist_write_int(PERSIST_ACTIVE_TRACK_START, 0);      
      duration += this_time - start_time;
      persist_write_int(PERSIST_ACTIVE_TRACK_DURATION, duration);
    }
  }
  else {     
    start_time = this_time;
    persist_write_int(PERSIST_ACTIVE_TRACK_START, start_time);
  }
  if(window_is_loaded(active_window)) {
    if(paused) {
      action_bar_layer_set_icon_animated(active_action_bar, BUTTON_ID_SELECT, img_play, true);
    }
    else {
      action_bar_layer_set_icon_animated(active_action_bar, BUTTON_ID_SELECT, img_pause, true);
    }
  }
}

/**
 * Stop session(or cancel)
**/
void active_set_stop(bool save) {
  start_time = (uint)persist_read_int(PERSIST_ACTIVE_TRACK_START);
  uint typ = (uint)persist_read_int(PERSIST_ACTIVE_TRACK_TYPE);
  if(save && start_time > 0 && typ != TYPE_UNKNOWN) {
    Session session = {.startTime=start_time, .endTime=time(NULL), .steps=0, .type=typ};
    send_session(session);
  }
  
  persist_write_bool(PERSIST_ACTIVE_TRACK, false);
  persist_write_bool(PERSIST_ACTIVE_TRACK_PAUSE, false);
  persist_write_int(PERSIST_ACTIVE_TRACK_START, 0);
  persist_write_int(PERSIST_ACTIVE_TRACK_DURATION, 0);
  persist_write_int(PERSIST_ACTIVE_TRACK_TYPE, TYPE_UNKNOWN);
  persist_write_int(PERSIST_ACTIVE_TRACK_GPS_BLE, -1);
  window_stack_remove(active_window, true);
}

/**
 * On down click
**/
static void active_down_click_handler(ClickRecognizerRef recognizer, void *context) {
  active_set_stop(true);
  if(gps_ble_track > GPS_BLE_OFF) appmsg_send_val(APPMSG_STOP, 1);
}

/**
 * On select click
**/
static void active_select_click_handler(ClickRecognizerRef recognizer, void *context) {
  active_set_paused(!active_paused);
  int8_t pause = active_paused ? 1 : 0;
  if(gps_ble_track > GPS_BLE_OFF) appmsg_send_val(APPMSG_PAUSE, pause);
}

static void active_click_provider(void *context) {
  // Register the ClickHandlers
  window_single_click_subscribe(BUTTON_ID_DOWN, active_down_click_handler);
  window_single_click_subscribe(BUTTON_ID_SELECT, active_select_click_handler);
}

static void active_window_load(Window *window) {
  Layer* active_window_layer = window_get_root_layer(window);
  #ifdef PBL_COLOR
  window_set_background_color(window, GColorDarkGreen);
  #else
  window_set_background_color(window, COLOR_BACKGROUND);
  #endif
  
  #if defined(PBL_ROUND)
  active_time = macro_text_layer_create(GRect(10, 5, 160, 16), active_window_layer, COLOR_TEXT, COLOR_TEXT_BACKGROUND, fonts_get_system_font(FONT_KEY_GOTHIC_14), GTextAlignmentCenter);
  active_duration_text = macro_text_layer_create(GRect(35, 35, 100, 30), active_window_layer, COLOR_TEXT, COLOR_TEXT_BACKGROUND, fonts_get_system_font(FONT_KEY_BITHAM_30_BLACK), GTextAlignmentRight);
  active_duration_text_label = macro_text_layer_create(GRect(70, 22, 60, 20), active_window_layer, COLOR_TEXT, COLOR_TEXT_BACKGROUND, fonts_get_system_font(FONT_KEY_GOTHIC_14), GTextAlignmentRight);
  active_distance_text = macro_text_layer_create(GRect(35, 82, 100, 30), active_window_layer, COLOR_TEXT, COLOR_TEXT_BACKGROUND, fonts_get_system_font(FONT_KEY_BITHAM_30_BLACK), GTextAlignmentRight);
  active_distance_text_label = macro_text_layer_create(GRect(70, 69, 60, 20), active_window_layer, COLOR_TEXT, COLOR_TEXT_BACKGROUND, fonts_get_system_font(FONT_KEY_GOTHIC_14), GTextAlignmentRight);
  active_speed_text = macro_text_layer_create(GRect(35, 129, 100, 30), active_window_layer, COLOR_TEXT, COLOR_TEXT_BACKGROUND, fonts_get_system_font(FONT_KEY_BITHAM_30_BLACK), GTextAlignmentRight);
  active_speed_text_label = macro_text_layer_create(GRect(70, 116, 60, 20), active_window_layer, COLOR_TEXT, COLOR_TEXT_BACKGROUND, fonts_get_system_font(FONT_KEY_GOTHIC_14), GTextAlignmentRight);
  #else
  active_time = macro_text_layer_create(GRect(10, 0, 124, 16), active_window_layer, COLOR_TEXT, COLOR_TEXT_BACKGROUND, fonts_get_system_font(FONT_KEY_GOTHIC_14), GTextAlignmentCenter);
  active_duration_text = macro_text_layer_create(GRect(10, 31, 100, 30), active_window_layer, COLOR_TEXT, COLOR_TEXT_BACKGROUND, fonts_get_system_font(FONT_KEY_BITHAM_30_BLACK), GTextAlignmentRight);
  active_duration_text_label = macro_text_layer_create(GRect(15, 16, 90, 20), active_window_layer, COLOR_TEXT, COLOR_TEXT_BACKGROUND, fonts_get_system_font(FONT_KEY_GOTHIC_14), GTextAlignmentRight);
  active_distance_text = macro_text_layer_create(GRect(10, 82, 100, 30), active_window_layer, COLOR_TEXT, COLOR_TEXT_BACKGROUND, fonts_get_system_font(FONT_KEY_BITHAM_30_BLACK), GTextAlignmentRight);
  active_distance_text_label = macro_text_layer_create(GRect(15, 67, 90, 20), active_window_layer, COLOR_TEXT, COLOR_TEXT_BACKGROUND, fonts_get_system_font(FONT_KEY_GOTHIC_14), GTextAlignmentRight);
  active_speed_text = macro_text_layer_create(GRect(10, 133, 100, 30), active_window_layer, COLOR_TEXT, COLOR_TEXT_BACKGROUND, fonts_get_system_font(FONT_KEY_BITHAM_30_BLACK), GTextAlignmentRight);
  active_speed_text_label = macro_text_layer_create(GRect(15, 118, 90, 20), active_window_layer, COLOR_TEXT, COLOR_TEXT_BACKGROUND, fonts_get_system_font(FONT_KEY_GOTHIC_14), GTextAlignmentRight);
  #endif
  
  gps_ble_track = persist_read_int(PERSIST_ACTIVE_TRACK_GPS_BLE);
  
  text_layer_set_text(active_duration_text_label, "Duration");
  if(gps_ble_track == GPS_ON_BLE_OFF || gps_ble_track == GPS_BLE_ON) {
    text_layer_set_text(active_distance_text_label, "Distance");
    text_layer_set_text(active_speed_text_label, "Speed");
  }
  start_time = (uint)persist_read_int(PERSIST_ACTIVE_TRACK_START);
  duration = (uint)persist_read_int(PERSIST_ACTIVE_TRACK_DURATION);

  active_action_bar = action_bar_layer_create();
  action_bar_layer_add_to_window(active_action_bar, window);
  action_bar_layer_set_click_config_provider(active_action_bar, active_click_provider);
  img_stop = gbitmap_create_with_resource(RESOURCE_ID_IMG_STOP);
  img_pause = gbitmap_create_with_resource(RESOURCE_ID_IMG_PAUSE);
  img_play = gbitmap_create_with_resource(RESOURCE_ID_IMG_PLAY);
  active_paused = persist_read_bool(PERSIST_ACTIVE_TRACK_PAUSE);
  
  #ifndef PBL_COLOR
  action_bar_layer_set_background_color(active_action_bar, GColorClear);
  #endif
  if(active_paused) {
    action_bar_layer_set_icon_animated(active_action_bar, BUTTON_ID_SELECT, img_play, true);
  }
  else {
    action_bar_layer_set_icon_animated(active_action_bar, BUTTON_ID_SELECT, img_pause, true);
  }
  action_bar_layer_set_icon_animated(active_action_bar, BUTTON_ID_DOWN, img_stop, true);
}

static void active_window_unload(Window *window) {
  text_layer_destroy(active_time);
  text_layer_destroy(active_duration_text);
  text_layer_destroy(active_duration_text_label);
  text_layer_destroy(active_distance_text);
  text_layer_destroy(active_distance_text_label);
  text_layer_destroy(active_speed_text);
  text_layer_destroy(active_speed_text_label);
  gbitmap_destroy(img_stop);
  gbitmap_destroy(img_pause);
  gbitmap_destroy(img_play);
  action_bar_layer_destroy(active_action_bar);
  window_destroy(active_window);
  active_window = NULL;
}

void active_init() {
  window_stack_pop_all(true);
  if(active_window == NULL) {
    active_window = window_create();
    window_set_window_handlers(active_window, (WindowHandlers) {
      .load = active_window_load,
      .unload = active_window_unload
    });
  }
  if(!window_is_loaded(active_window)) {
    window_stack_push(active_window, true);
  }
  active_set_time(main_time_text);
}
