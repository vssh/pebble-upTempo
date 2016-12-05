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

typedef struct{
  TextLayer *active_time, *active_duration_text, *active_duration_text_label,
          *active_distance_text, *active_distance_text_label, *active_speed_text, *active_speed_text_label;
  ActionBarLayer *active_action_bar;
  bool active_paused;
  bool showing_pace ;
  GBitmap *img_stop, *img_pause, *img_play;
  uint start_time;
  uint duration;
  int gps_ble_track;
  char dur_buf[6];
}ActiveVals;

/*Window *active_window;
TextLayer *active_time, *active_duration_text, *active_duration_text_label,
        *active_distance_text, *active_distance_text_label, *active_speed_text, *active_speed_text_label;
ActionBarLayer *active_action_bar;
bool active_paused = false;
bool showing_pace = false;
GBitmap *img_stop, *img_pause, *img_play;
uint start_time = 0;
uint duration = 0;
int gps_ble_track = false;
char dur_buf[6] = "00:00";*/

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

ActiveVals *active_vals;

/**
 * Calculate and set the current duration
**/
void active_set_duration() {
  if(active_window != NULL && window_is_loaded(active_window)) {
    uint dur = active_vals->duration/60;
    //APP_LOG(APP_LOG_LEVEL_DEBUG, "duration: %u", duration);
    if(!active_vals->active_paused) {
      dur += (time(NULL) - active_vals->start_time)/60;
      //APP_LOG(APP_LOG_LEVEL_DEBUG, "curr_time: %lu", (time(NULL) - start_time));
    }
    snprintf(active_vals->dur_buf, 6, "%.2u:%.2u", dur/60, dur%60);
    text_layer_set_text(active_vals->active_duration_text, active_vals->dur_buf);
  }
}

/**
 * Set the speed
**/
void active_set_speed(char *value) {
  if(active_window != NULL && window_is_loaded(active_window)) {    
    text_layer_set_text(active_vals->active_speed_text, value);
    if(active_vals->showing_pace) {
      text_layer_set_text(active_vals->active_speed_text_label, "Speed");
      active_vals->showing_pace = false;
    }
  }
}

/**
 * Set the pace
**/
void active_set_pace(char *value) {
  if(active_window != NULL && window_is_loaded(active_window)) {    
    text_layer_set_text(active_vals->active_speed_text, value);
    if(!active_vals->showing_pace) {
      text_layer_set_text(active_vals->active_speed_text_label, "Pace");
      active_vals->showing_pace = true;
    }
  }
}

/**
 * Set heart rate
**/
void active_set_heart(char *value) {
  if(active_window != NULL && window_is_loaded(active_window)) {
    text_layer_set_text(active_vals->active_speed_text_label, "Heart rate");
    if(active_vals->gps_ble_track == GPS_OFF_BLE_ON)
      text_layer_set_text(active_vals->active_speed_text, value);
  }
}

/**
 * Set distance
**/
void active_set_distance(char *value) {  
  if(active_window != NULL && window_is_loaded(active_window)) {
    text_layer_set_text(active_vals->active_distance_text, value);
  }
}

/**
 * Set top time display and reset duration
**/
void active_set_time(char *value) {
  if(active_window != NULL && window_is_loaded(active_window)) {
    text_layer_set_text(active_vals->active_time, value);
    
    active_set_duration();
  }
}

/**
 * Show units(not used)
**/
/*void active_set_unit(bool metric) {
  if(active_window != NULL && window_is_loaded(active_window)) {
    if(metric) {
      text_layer_set_text(active_distance_text_label, "Distance(km)");
      text_layer_set_text(active_speed_text_label, "Speed(kmph)");
    }
    else {
      text_layer_set_text(active_distance_text_label, "Distance(mi)");
      text_layer_set_text(active_speed_text_label, "Speed(mph)");
    }
  }
}*/

/**
 * Pause and unpause session
**/
void active_set_paused(bool paused) {
  active_vals->active_paused = paused;
  persist_write_bool(PERSIST_ACTIVE_TRACK_PAUSE, active_vals->active_paused);
  uint typ = (uint)persist_read_int(PERSIST_ACTIVE_TRACK_TYPE);
  uint this_time = time(NULL);
  if(paused) {
    active_vals->start_time = (uint)persist_read_int(PERSIST_ACTIVE_TRACK_START);  
    if(active_vals->start_time > 0) {
      if(typ != TYPE_UNKNOWN) {
        Session session = {.startTime=active_vals->start_time, .endTime=time(NULL), .steps=0, .type=typ};
        send_session(session);
      }      
      persist_write_int(PERSIST_ACTIVE_TRACK_START, 0);      
      active_vals->duration += this_time - active_vals->start_time;
      persist_write_int(PERSIST_ACTIVE_TRACK_DURATION, active_vals->duration);
    }
  }
  else {     
    active_vals->start_time = this_time;
    persist_write_int(PERSIST_ACTIVE_TRACK_START, active_vals->start_time);
  }
  if(window_is_loaded(active_window)) {
    if(paused) {
      action_bar_layer_set_icon_animated(active_vals->active_action_bar, BUTTON_ID_SELECT, active_vals->img_play, true);
    }
    else {
      action_bar_layer_set_icon_animated(active_vals->active_action_bar, BUTTON_ID_SELECT, active_vals->img_pause, true);
    }
  }
}

/**
 * Stop session(or cancel)
**/
void active_set_stop(bool save) {
  active_vals->start_time = (uint)persist_read_int(PERSIST_ACTIVE_TRACK_START);
  uint typ = (uint)persist_read_int(PERSIST_ACTIVE_TRACK_TYPE);
  if(save && active_vals->start_time > 0 && typ != TYPE_UNKNOWN) {
    Session session = {.startTime=active_vals->start_time, .endTime=time(NULL), .steps=0, .type=typ};
    send_session(session);
  }
  //APP_LOG(APP_LOG_LEVEL_DEBUG, "type: %d", typ);
  
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
  if(active_vals->gps_ble_track > GPS_BLE_OFF) appmsg_send_val(APPMSG_STOP, 1);
}

/**
 * On select click
**/
static void active_select_click_handler(ClickRecognizerRef recognizer, void *context) {
  active_set_paused(!active_vals->active_paused);
  int8_t pause = active_vals->active_paused ? 1 : 0;
  if(active_vals->gps_ble_track > GPS_BLE_OFF) appmsg_send_val(APPMSG_PAUSE, pause);
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
  
  active_vals->active_action_bar = action_bar_layer_create();
  action_bar_layer_add_to_window(active_vals->active_action_bar, window);
  action_bar_layer_set_click_config_provider(active_vals->active_action_bar, active_click_provider);
  
  #if defined(PBL_ROUND)
  active_vals->active_time = macro_text_layer_create(GRect(10, 5, 160, 16), active_window_layer, COLOR_TEXT, COLOR_TEXT_BACKGROUND, fonts_get_system_font(FONT_KEY_GOTHIC_14), GTextAlignmentCenter);
  active_vals->active_duration_text = macro_text_layer_create(GRect(35, 30, 100, 40), active_window_layer, COLOR_TEXT, COLOR_TEXT_BACKGROUND, forcedSquare, GTextAlignmentRight);
  active_vals->active_duration_text_label = macro_text_layer_create(GRect(70, 22, 60, 20), active_window_layer, COLOR_TEXT, COLOR_TEXT_BACKGROUND, fonts_get_system_font(FONT_KEY_GOTHIC_14), GTextAlignmentRight);
  active_vals->active_distance_text = macro_text_layer_create(GRect(35, 77, 100, 40), active_window_layer, COLOR_TEXT, COLOR_TEXT_BACKGROUND, forcedSquare, GTextAlignmentRight);
  active_vals->active_distance_text_label = macro_text_layer_create(GRect(70, 69, 60, 20), active_window_layer, COLOR_TEXT, COLOR_TEXT_BACKGROUND, fonts_get_system_font(FONT_KEY_GOTHIC_14), GTextAlignmentRight);
  active_vals->active_speed_text = macro_text_layer_create(GRect(35, 124, 100, 40), active_window_layer, COLOR_TEXT, COLOR_TEXT_BACKGROUND, forcedSquare, GTextAlignmentRight);
  active_vals->active_speed_text_label = macro_text_layer_create(GRect(70, 116, 60, 20), active_window_layer, COLOR_TEXT, COLOR_TEXT_BACKGROUND, fonts_get_system_font(FONT_KEY_GOTHIC_14), GTextAlignmentRight);
  #else
  GSize size = layer_get_bounds(active_window_layer).size;
  GSize sizeAct = layer_get_bounds((Layer*)active_vals->active_action_bar).size;
  int top = 16;
  int height6th = (size.h-top)/6;
  
  active_vals->active_time = macro_text_layer_create(GRect(10, 0, size.w-sizeAct.w-20, 16), active_window_layer, COLOR_TEXT, COLOR_TEXT_BACKGROUND, fonts_get_system_font(FONT_KEY_GOTHIC_14), GTextAlignmentCenter);
  active_vals->active_duration_text = macro_text_layer_create(GRect(10, top+height6th-10, size.w-sizeAct.w-20, 40), active_window_layer, COLOR_TEXT, COLOR_TEXT_BACKGROUND, forcedSquare, GTextAlignmentRight);
  active_vals->active_duration_text_label = macro_text_layer_create(GRect(10, top+height6th-20, size.w-sizeAct.w-20, 20), active_window_layer, COLOR_TEXT, COLOR_TEXT_BACKGROUND, fonts_get_system_font(FONT_KEY_GOTHIC_14), GTextAlignmentRight);
  active_vals->active_distance_text = macro_text_layer_create(GRect(10, top+3*height6th-10, size.w-sizeAct.w-20, 40), active_window_layer, COLOR_TEXT, COLOR_TEXT_BACKGROUND, forcedSquare, GTextAlignmentRight);
  active_vals->active_distance_text_label = macro_text_layer_create(GRect(10, top+3*height6th-20, size.w-sizeAct.w-20, 20), active_window_layer, COLOR_TEXT, COLOR_TEXT_BACKGROUND, fonts_get_system_font(FONT_KEY_GOTHIC_14), GTextAlignmentRight);
  active_vals->active_speed_text = macro_text_layer_create(GRect(10, top+5*height6th-10, size.w-sizeAct.w-20, 40), active_window_layer, COLOR_TEXT, COLOR_TEXT_BACKGROUND, forcedSquare, GTextAlignmentRight);
  active_vals->active_speed_text_label = macro_text_layer_create(GRect(10, top+5*height6th-20, size.w-sizeAct.w-20, 20), active_window_layer, COLOR_TEXT, COLOR_TEXT_BACKGROUND, fonts_get_system_font(FONT_KEY_GOTHIC_14), GTextAlignmentRight);
  #endif
  
  active_vals->gps_ble_track = persist_read_int(PERSIST_ACTIVE_TRACK_GPS_BLE);
  
  text_layer_set_text(active_vals->active_duration_text_label, "Duration");
  if(active_vals->gps_ble_track == GPS_ON_BLE_OFF || active_vals->gps_ble_track == GPS_BLE_ON) {
    text_layer_set_text(active_vals->active_distance_text_label, "Distance");
    text_layer_set_text(active_vals->active_speed_text_label, "Speed");
  }
  active_vals->start_time = (uint)persist_read_int(PERSIST_ACTIVE_TRACK_START);
  active_vals->duration = (uint)persist_read_int(PERSIST_ACTIVE_TRACK_DURATION);
    
  /*text_layer_set_text(active_duration_text, "00:25");
  text_layer_set_text(active_distance_text, "2.1");
  text_layer_set_text(active_speed_text, "4.6");*/

  active_vals->img_stop = gbitmap_create_with_resource(RESOURCE_ID_IMG_STOP);
  active_vals->img_pause = gbitmap_create_with_resource(RESOURCE_ID_IMG_PAUSE);
  active_vals->img_play = gbitmap_create_with_resource(RESOURCE_ID_IMG_PLAY);
  active_vals->active_paused = persist_read_bool(PERSIST_ACTIVE_TRACK_PAUSE);
  
  #ifndef PBL_COLOR
  action_bar_layer_set_background_color(active_vals->active_action_bar, GColorClear);
  #endif
  if(active_vals->active_paused) {
    action_bar_layer_set_icon_animated(active_vals->active_action_bar, BUTTON_ID_SELECT, active_vals->img_play, true);
  }
  else {
    action_bar_layer_set_icon_animated(active_vals->active_action_bar, BUTTON_ID_SELECT, active_vals->img_pause, true);
  }
  action_bar_layer_set_icon_animated(active_vals->active_action_bar, BUTTON_ID_DOWN, active_vals->img_stop, true);
  
  //appmsg_send_int(APPMSG_STATUS, 1);
}

static void active_window_unload(Window *window) {
  text_layer_destroy(active_vals->active_time);
  text_layer_destroy(active_vals->active_duration_text);
  text_layer_destroy(active_vals->active_duration_text_label);
  text_layer_destroy(active_vals->active_distance_text);
  text_layer_destroy(active_vals->active_distance_text_label);
  text_layer_destroy(active_vals->active_speed_text);
  text_layer_destroy(active_vals->active_speed_text_label);
  gbitmap_destroy(active_vals->img_stop);
  gbitmap_destroy(active_vals->img_pause);
  gbitmap_destroy(active_vals->img_play);
  action_bar_layer_destroy(active_vals->active_action_bar);
  window_destroy(active_window);
  active_window = NULL;
  free(active_vals);
}

/*void active_request_status() {
  if(gps_ble_track > GPS_BLE_OFF) appmsg_send_val(APPMSG_STATUS, 1);
  active_set_time(main_time_text);
}*/

void active_init() {
  active_vals = malloc(sizeof(ActiveVals));
  active_vals->active_paused = false;
  active_vals->showing_pace = false;
  active_vals->start_time = 0;
  active_vals->duration = 0;
  active_vals->gps_ble_track = false;
  snprintf(active_vals->dur_buf, 6, "00:00");
  
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
  //active_set_duration();
  //appmsg_send_val(APPMSG_STATUS, 1);
  //app_timer_register(1000, active_request_status, NULL);
}