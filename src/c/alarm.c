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
#include "alarm.h"
  
Window *alarm_window;

WakeupId alarm_ids[7];
time_t regular_alarm_time[7];
WakeupId snooze_id = 0;
WakeupId monitor_id = 0;
TextLayer *wake_text, *alarm_time_text;

WakeupId id = 0;
int32_t reason = 0;
bool monitoring = false;
uint8_t auto_snooze_counter = 0;

AppTimer *vibe_timer = NULL;
AppTimer *auto_snooze_timer = NULL;

ActionBarLayer *alarm_action_bar;
GBitmap *img_tick, *img_zz, *img_alarm = NULL;

BitmapLayer *alarm_bmp_layer;
int16_t wkday = -1;
int16_t wkend = -1;
uint8_t snooze_time = 10;
uint16_t this_alarm = 0;

int monitorState = 0;

struct tm now;

/**
 * Silence vibrations and exit
**/
void silence_alarm() {
  app_timer_cancel(vibe_timer);
  vibes_cancel();
  window_stack_pop(true);
  light_enable(false);
}

/**
 * Perform vibration continuously
**/
void do_vibrate() {  
  static const uint32_t segments[] = {250, 250, 250, 250, 250};
  VibePattern pat = {
    .durations = segments,
    .num_segments = ARRAY_LENGTH(segments),
  };
  vibes_enqueue_custom_pattern(pat);  
  vibe_timer = app_timer_register(2500, do_vibrate, NULL);
}

/**
 * Set snooze alarm
**/
void do_snooze() {
  app_timer_cancel(auto_snooze_timer);
  snooze_id = wakeup_schedule(time(NULL)+snooze_time*60, REASON_ALARM_RING, false);
  if(snooze_id < 0) {
    snooze_id = wakeup_schedule(time(NULL)+(snooze_time+5)*60, REASON_ALARM_RING, true);    
  }  
  persist_write_int(PERSIST_SNOOZE_ALARM, snooze_id);
  silence_alarm();
}

/**
 * Decide if monitor or ring
**/
static void set_monitoring_state(int32_t reason) {
  time_t time_now = time(NULL);
  now = *localtime(&time_now);
  int now_time = now.tm_hour*60 + now.tm_min;
  int day = now.tm_wday+1;
  monitoring = false;
  this_alarm = 25*60;
  
  time_t regular_alarm_time = persist_read_int(PERSIST_ALARM_TIME0+day-1);
  time_t this_alarm_timestamp = time_now + 25*60*60;
  
  if(regular_alarm_time > time_now-2) {
    this_alarm_timestamp = regular_alarm_time;
  }
  
  struct tm alarm = *localtime(&this_alarm_timestamp);
  this_alarm = alarm.tm_hour*60 + alarm.tm_min;
  
  if(reason == REASON_ALARM_MONITOR && launch_reason() == APP_LAUNCH_WAKEUP) {
    monitoring = true;
  }
  else if (reason == REASON_ALARM_RING && launch_reason() == APP_LAUNCH_WAKEUP) {
    monitoring = false;
  }
  else if(this_alarm-now_time>5 && this_alarm-now_time<MONITOR_PERIOD+5) {
    monitoring = true;
  }
}

/**
 * Switch state to alarm
**/
void switch_state_alarm() {
  time_t time_now = time(NULL);
  now = *localtime(&time_now);
  //monitoring = false;
  #ifdef PBL_COLOR
  window_set_background_color(alarm_window, GColorDarkCandyAppleRed);
  #endif
  
  text_layer_set_text(wake_text, "Wake Up!");
  gbitmap_destroy(img_alarm);
  img_alarm = gbitmap_create_with_resource(RESOURCE_ID_IMG_ALARM);
  bitmap_layer_set_bitmap(alarm_bmp_layer, img_alarm);
  do_vibrate();
  auto_snooze_counter = persist_read_int(PERSIST_AUTO_SNOOZE);
  if(auto_snooze_counter < 3) {
    auto_snooze_timer = app_timer_register(60000, do_snooze, NULL);
    auto_snooze_counter++;
    persist_write_int(PERSIST_AUTO_SNOOZE, auto_snooze_counter);
  }
  else {
    auto_snooze_timer = app_timer_register(60000, silence_alarm, NULL);
    auto_snooze_counter = 0;
    persist_write_int(PERSIST_AUTO_SNOOZE, auto_snooze_counter);
  }
  if(img_zz == NULL) img_zz = gbitmap_create_with_resource(RESOURCE_ID_IMG_ZZ);
  action_bar_layer_set_icon_animated(alarm_action_bar, BUTTON_ID_DOWN, img_zz, true);
  light_enable(true);
}

/**
 * Keep track of monitoring state
**/
void monitor_message_handler(uint16_t type, AppWorkerMessage *data) {
  if(alarm_window!=NULL && window_is_loaded(alarm_window) && monitoring) {
    uint32_t val = data->data0;
  
    if(type == WORKER_MSG_TYPE) {
      if((uint)val == TYPE_SLEEP) {
        monitorState = 0;
      }
      else if((uint)val == TYPE_IDLE) {
        monitorState += 1;
      }
      else if((uint)val == TYPE_WALK) {
        monitorState += 3;
      }
      else if((uint)val == TYPE_JOG) {
        monitorState += 5;
      }
  
      if(monitorState > 5) {
        monitoring = false;
        switch_state_alarm();
      }
    }
  }
}

/**
 * Switch state to monitoring
**/
static void switch_state_monitoring() {
  #ifdef PBL_COLOR
  window_set_background_color(alarm_window, GColorJaegerGreen);
  #endif
  text_layer_set_text(wake_text, "Monitoring");
  gbitmap_destroy(img_alarm);
  img_alarm = gbitmap_create_with_resource(RESOURCE_ID_IMG_MONITORING);
  bitmap_layer_set_bitmap(alarm_bmp_layer, img_alarm);
  action_bar_layer_clear_icon(alarm_action_bar, BUTTON_ID_DOWN);
  
  auto_snooze_counter = 0;
  persist_write_int(PERSIST_AUTO_SNOOZE, auto_snooze_counter);
}

/**
 * On up click
**/
static void alarm_up_click_handler(ClickRecognizerRef recognizer, void *context) {
  if(monitoring) {
    monitoring = false;
    window_stack_pop(true);
  }
  else {
    app_timer_cancel(auto_snooze_timer);
    silence_alarm();
  }
  auto_snooze_counter = 0;
  persist_write_int(PERSIST_AUTO_SNOOZE, auto_snooze_counter);
}

/**
 * On down click
**/
static void alarm_down_click_handler(ClickRecognizerRef recognizer, void *context) {
  if(!monitoring) {
    do_snooze();    
  }
  auto_snooze_counter = 0;
  persist_write_int(PERSIST_AUTO_SNOOZE, auto_snooze_counter);
}

/**
 * On back click
**/
static void alarm_back_click_handler(ClickRecognizerRef recognizer, void *context) {
  //do nothing
}

static void alarm_click_provider(void *context) {
  // Register the ClickHandlers
  window_long_click_subscribe(BUTTON_ID_UP, 1000, alarm_up_click_handler, NULL);
  window_single_click_subscribe(BUTTON_ID_DOWN, alarm_down_click_handler);
  window_single_click_subscribe(BUTTON_ID_BACK, alarm_back_click_handler);
}

/**
 * Update time and ring if alarm time
**/
void alarm_time_handler(char *time_string, int time_hr, int time_min) {
  if(alarm_window!=NULL && window_is_loaded(alarm_window)) {
    text_layer_set_text(alarm_time_text, time_string);
    
    if(time_hr*60+time_min >= this_alarm) {
        monitoring = false;
        switch_state_alarm();
      }
  }
}

static void alarm_window_load(Window *window) {
  
  Layer* alarm_window_layer = window_get_root_layer(window);
  window_set_background_color(window, COLOR_BACKGROUND);
  
  img_alarm = gbitmap_create_with_resource(RESOURCE_ID_IMG_ALARM);
  #if defined(PBL_ROUND)
  wake_text = macro_text_layer_create(GRect(20, 10, 120, 30), alarm_window_layer, COLOR_TEXT, COLOR_TEXT_BACKGROUND, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD), GTextAlignmentCenter);
  alarm_time_text = macro_text_layer_create(GRect(30, 115, 100, 40), alarm_window_layer, COLOR_TEXT, COLOR_TEXT_BACKGROUND, fonts_get_system_font(FONT_KEY_BITHAM_30_BLACK), GTextAlignmentCenter);
  
  alarm_bmp_layer = macro_bitmap_layer_create(img_alarm, GRect(55, 60, 50, 50), alarm_window_layer, true);
  #else
  wake_text = macro_text_layer_create(GRect(10, 10, 104, 30), alarm_window_layer, COLOR_TEXT, COLOR_TEXT_BACKGROUND, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD), GTextAlignmentCenter);
  alarm_time_text = macro_text_layer_create(GRect(10, 110, 104, 40), alarm_window_layer, COLOR_TEXT, COLOR_TEXT_BACKGROUND, fonts_get_system_font(FONT_KEY_BITHAM_30_BLACK), GTextAlignmentCenter);
  
  alarm_bmp_layer = macro_bitmap_layer_create(img_alarm, GRect(42, 50, 50, 50), alarm_window_layer, true);
  #endif
  
  // Initialize the action bar:
  alarm_action_bar = action_bar_layer_create();
  // Associate the action bar with the window:
  action_bar_layer_add_to_window(alarm_action_bar, window);
  // Set the click config provider:
  action_bar_layer_set_click_config_provider(alarm_action_bar, alarm_click_provider);
  
  img_tick = gbitmap_create_with_resource(RESOURCE_ID_IMG_TICK);
  
  action_bar_layer_set_icon_animated(alarm_action_bar, BUTTON_ID_UP, img_tick, true);
  
  #ifndef PBL_COLOR
  action_bar_layer_set_background_color(alarm_action_bar, GColorClear);
  #endif
}

static void alarm_window_unload(Window *window) {
  if(monitoring && !wakeup_query(monitor_id, NULL)){
    monitor_id = wakeup_schedule(clock_to_timestamp(TODAY, this_alarm/60, this_alarm%60), REASON_ALARM_RING, true);
    if(monitor_id < 0) {
      monitor_id = wakeup_schedule(clock_to_timestamp(TODAY, this_alarm/60, (this_alarm-5)%60), REASON_ALARM_RING, true);
    }
    if(monitor_id < 0) {
      monitor_id = wakeup_schedule(clock_to_timestamp(TODAY, this_alarm/60, (this_alarm-10)%60), REASON_ALARM_RING, true);
    }
    if(monitor_id < 0) {
      APP_LOG(APP_LOG_LEVEL_ERROR, "monitor alarm could not be set. returned: %d", (int)monitor_id);
    }
  }
  
  text_layer_destroy(wake_text);
  text_layer_destroy(alarm_time_text);
  gbitmap_destroy(img_tick);
  gbitmap_destroy(img_zz);
  if(img_alarm != NULL) { gbitmap_destroy(img_alarm); img_alarm = NULL; }
  action_bar_layer_destroy(alarm_action_bar);
  bitmap_layer_destroy(alarm_bmp_layer);
  window_destroy(alarm_window);
  alarm_window = NULL;
}

void alarm_init() {
  window_stack_pop_all(true);
  wkday = (uint16_t)persist_read_int(PERSIST_WKD);
  wkend = (uint16_t)persist_read_int(PERSIST_WKE);
  
  time_t timenow = time(NULL);
  now = *localtime(&timenow);
  
  int alarm_reason = -1;
  if(alarm_window == NULL) {
    alarm_window = window_create();
    window_set_window_handlers(alarm_window, (WindowHandlers) {
      .load = alarm_window_load,
      .unload = alarm_window_unload
    });
  }
  if(!window_is_loaded(alarm_window)) {
    wakeup_get_launch_event(&id, &reason);
    alarm_reason = reason;
    window_stack_push(alarm_window, true);
  }
  set_monitoring_state(alarm_reason);
  if(monitoring) {
    switch_state_monitoring();
  }
  else {
    switch_state_alarm();
  }
}

/**
 * Manage alarm times and set next alarm
**/
void manage_alarms() {
  wkday = (uint16_t)persist_read_int(PERSIST_WKD);
  wkend = (uint16_t)persist_read_int(PERSIST_WKE);
  
  if(wkday > -1 || wkend > -1) {
    time_t timenow = time(NULL);
    now = *localtime(&timenow);
    
    int alarm_time;
    //(1 added to account for different enum values)
    int day = now.tm_wday+1;
    
    if(day == SUNDAY || day == SATURDAY) {
      alarm_time = wkend;
    }
    else {
      alarm_time = wkday;
    }
    //alarm passed for today?
    if(alarm_time > -1 && alarm_time - (now.tm_hour*60 + now.tm_min) > MONITOR_PERIOD+5) {
      time_t alarm_timestamp = clock_to_timestamp(TODAY, (int)(alarm_time)/60, (int)(alarm_time)%60);
      alarm_ids[day-1] = persist_read_int(PERSIST_ALARM0+day-1);
      if(alarm_ids[day-1] < 0 || wakeup_query(alarm_ids[day-1], NULL) == false) {
        if(alarm_time < MONITOR_PERIOD) {
          alarm_ids[day-1] = -1;
        }
        else {
          alarm_ids[day-1] = wakeup_schedule(alarm_timestamp-MONITOR_PERIOD*60, REASON_ALARM_MONITOR, true);  
        }
        if(alarm_ids[day-1] < 0) {          
          alarm_ids[day-1] = wakeup_schedule(alarm_timestamp, REASON_ALARM_RING, true);
        }        
        if(alarm_ids[day-1] < 0) {
          APP_LOG(APP_LOG_LEVEL_ERROR, "alarm could not be set today. returned: %d", (int)alarm_ids[day-1]);
        }
        else {
          persist_write_int(PERSIST_ALARM0+day-1, alarm_ids[day-1]);
          persist_write_int(PERSIST_ALARM_TIME0+day-1, alarm_timestamp);
        }
      }
    }
        
    //for tomorrow
    for(uint8_t ind=1; ind<7; ind++) {
      day = (now.tm_wday + ind)%7 + 1;
      if(day == SUNDAY || day == SATURDAY) {
        alarm_time = wkend;
      }
      else {
        alarm_time = wkday;
      }
      if(alarm_time > -1) {
        break;
      }
    }
    
    alarm_ids[day-1] = persist_read_int(PERSIST_ALARM0+day-1);
    
    if(alarm_time > -1 && (alarm_ids[day-1] < 0 || wakeup_query(alarm_ids[day-1], NULL) == false)) {
      
      time_t alarm_timestamp = clock_to_timestamp((int)day, (int)(alarm_time/60), (int)(alarm_time%60));
      if(alarm_time < MONITOR_PERIOD) {
        alarm_ids[day-1] = -1;  
      }
      else {
        alarm_ids[day-1] = wakeup_schedule(alarm_timestamp-MONITOR_PERIOD*60, REASON_ALARM_MONITOR, true);
      }
      if(alarm_ids[day-1] < 0) {          
        alarm_ids[day-1] = wakeup_schedule(alarm_timestamp, REASON_ALARM_RING, true);
      }
      
      if(alarm_ids[day-1] < 0) {
        APP_LOG(APP_LOG_LEVEL_ERROR, "alarm could not be set for tomorrow. returned: %d", (int)alarm_ids[day-1]);
      }
      else {
        persist_write_int(PERSIST_ALARM0+day-1, alarm_ids[day-1]);
        persist_write_int(PERSIST_ALARM_TIME0+day-1, alarm_timestamp);
      }
    }
  }
}

/**
 * Reset all alarms
**/
void reset_all_alarms() {
  wakeup_cancel_all();
  manage_alarms();
}
