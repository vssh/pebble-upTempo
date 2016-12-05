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
int16_t wkday = -1;
int16_t wkend = -1;

bool vars_allocated = false;
typedef struct{
  WakeupId alarm_ids[7];
  time_t regular_alarm_time[7];
  WakeupId snooze_id;
  WakeupId monitor_id;
  TextLayer *wake_text, *alarm_time_text;
  WakeupId id;
  int32_t reason;
  bool monitoring;
  uint8_t auto_snooze_counter;
  AppTimer *vibe_timer;
  AppTimer *auto_snooze_timer;
  ActionBarLayer *alarm_action_bar;
  GBitmap *img_tick, *img_zz, *img_alarm;
  BitmapLayer *alarm_bmp_layer;
  uint8_t snooze_time;
  uint16_t this_alarm;
  int monitorState;
  struct tm now;
}AlarmVals;
AlarmVals *alarm_vals;

/*WakeupId alarm_ids[7];
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
uint8_t snooze_time = 10;
uint16_t this_alarm = 0;

int monitorState = 0;

struct tm now;*/

/**
 * Silence vibrations and exit
**/
void silence_alarm() {
  app_timer_cancel(alarm_vals->vibe_timer);
  vibes_cancel();
  window_stack_pop(true);
  light_enable(false);
}

/**
 * Perform vibration continuously
**/
void do_vibrate() {  
  static const uint32_t segments[] = {500, 500, 500, 500, 500};
  VibePattern pat = {
    .durations = segments,
    .num_segments = ARRAY_LENGTH(segments),
  };
  vibes_enqueue_custom_pattern(pat);  
  alarm_vals->vibe_timer = app_timer_register(3500, do_vibrate, NULL);
}

/**
 * Set snooze alarm
**/
void do_snooze() {
  app_timer_cancel(alarm_vals->auto_snooze_timer);
  alarm_vals->snooze_id = wakeup_schedule(time(NULL)+alarm_vals->snooze_time*60, REASON_ALARM_RING, false);
  if(alarm_vals->snooze_id < 0) {
    alarm_vals->snooze_id = wakeup_schedule(time(NULL)+(alarm_vals->snooze_time+5)*60, REASON_ALARM_RING, true);    
  }  
  persist_write_int(PERSIST_SNOOZE_ALARM, alarm_vals->snooze_id);
  silence_alarm();
}

/**
 * Decide if monitor or ring
**/
static void set_monitoring_state(int32_t reason) {
  time_t time_now = time(NULL);
  alarm_vals->now = *localtime(&time_now);
  int now_time = alarm_vals->now.tm_hour*60 + alarm_vals->now.tm_min;
  int day = alarm_vals->now.tm_wday+1;
  alarm_vals->monitoring = false;
  alarm_vals->this_alarm = 25*60;
  
  time_t regular_alarm_time = persist_read_int(PERSIST_ALARM_TIME0+day-1);
  time_t this_alarm_timestamp = time_now + 25*60*60;
  
  if(regular_alarm_time > time_now-2) {
    this_alarm_timestamp = regular_alarm_time;
  }
  
  struct tm alarm = *localtime(&this_alarm_timestamp);
  alarm_vals->this_alarm = alarm.tm_hour*60 + alarm.tm_min;
  
  if(reason == REASON_ALARM_MONITOR && launch_reason() == APP_LAUNCH_WAKEUP) {
    alarm_vals->monitoring = true;
  }
  else if (reason == REASON_ALARM_RING && launch_reason() == APP_LAUNCH_WAKEUP) {
    alarm_vals->monitoring = false;
  }
  else if(alarm_vals->this_alarm-now_time>5 && alarm_vals->this_alarm-now_time<MONITOR_PERIOD+5) {
    alarm_vals->monitoring = true;
  }
}

/**
 * Switch state to alarm
**/
void switch_state_alarm() {
  time_t time_now = time(NULL);
  alarm_vals->now = *localtime(&time_now);
  //monitoring = false;
  #ifdef PBL_COLOR
  window_set_background_color(alarm_window, GColorDarkCandyAppleRed);
  #endif
  
  text_layer_set_text(alarm_vals->wake_text, "Wake Up!");
  gbitmap_destroy(alarm_vals->img_alarm);
  alarm_vals->img_alarm = gbitmap_create_with_resource(RESOURCE_ID_IMG_ALARM);
  bitmap_layer_set_bitmap(alarm_vals->alarm_bmp_layer, alarm_vals->img_alarm);
  do_vibrate();
  alarm_vals->auto_snooze_counter = persist_read_int(PERSIST_AUTO_SNOOZE);
  if(alarm_vals->auto_snooze_counter < 3) {
    alarm_vals->auto_snooze_timer = app_timer_register(60000, do_snooze, NULL);
    alarm_vals->auto_snooze_counter++;
    persist_write_int(PERSIST_AUTO_SNOOZE, alarm_vals->auto_snooze_counter);
  }
  else {
    alarm_vals->auto_snooze_timer = app_timer_register(60000, silence_alarm, NULL);
    alarm_vals->auto_snooze_counter = 0;
    persist_write_int(PERSIST_AUTO_SNOOZE, alarm_vals->auto_snooze_counter);
  }
  if(alarm_vals->img_zz == NULL) alarm_vals->img_zz = gbitmap_create_with_resource(RESOURCE_ID_IMG_ZZ);
  action_bar_layer_set_icon_animated(alarm_vals->alarm_action_bar, BUTTON_ID_DOWN, alarm_vals->img_zz, true);
  light_enable(true);
}

/**
 * Keep track of monitoring state
**/
void monitor_message_handler(uint16_t type, AppWorkerMessage *data) {
  if(alarm_window!=NULL && window_is_loaded(alarm_window) && alarm_vals->monitoring) {
    uint32_t val = data->data0;
  
    if(type == WORKER_MSG_TYPE) {
      if((uint)val == TYPE_SLEEP) {
        alarm_vals->monitorState = 0;
      }
      else if((uint)val == TYPE_IDLE) {
        alarm_vals->monitorState += 1;
      }
      else if((uint)val == TYPE_WALK) {
        alarm_vals->monitorState += 3;
      }
      else if((uint)val == TYPE_JOG) {
        alarm_vals->monitorState += 5;
      }
  
      if(alarm_vals->monitorState > 5) {
        alarm_vals->monitoring = false;
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
  text_layer_set_text(alarm_vals->wake_text, "Monitoring");
  gbitmap_destroy(alarm_vals->img_alarm);
  alarm_vals->img_alarm = gbitmap_create_with_resource(RESOURCE_ID_IMG_MONITORING);
  bitmap_layer_set_bitmap(alarm_vals->alarm_bmp_layer, alarm_vals->img_alarm);
  action_bar_layer_clear_icon(alarm_vals->alarm_action_bar, BUTTON_ID_DOWN);
  
  alarm_vals->auto_snooze_counter = 0;
  persist_write_int(PERSIST_AUTO_SNOOZE, alarm_vals->auto_snooze_counter);
}

/**
 * On up click
**/
static void alarm_up_click_handler(ClickRecognizerRef recognizer, void *context) {
  if(alarm_vals->monitoring) {
    alarm_vals->monitoring = false;
    window_stack_pop(true);
  }
  else {
    app_timer_cancel(alarm_vals->auto_snooze_timer);
    silence_alarm();
  }
  alarm_vals->auto_snooze_counter = 0;
  persist_write_int(PERSIST_AUTO_SNOOZE, alarm_vals->auto_snooze_counter);
}

/**
 * On down click
**/
static void alarm_down_click_handler(ClickRecognizerRef recognizer, void *context) {
  if(!alarm_vals->monitoring) {
    do_snooze();    
  }
  alarm_vals->auto_snooze_counter = 0;
  persist_write_int(PERSIST_AUTO_SNOOZE, alarm_vals->auto_snooze_counter);
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
    text_layer_set_text(alarm_vals->alarm_time_text, time_string);
    
    if(time_hr*60+time_min >= alarm_vals->this_alarm) {
        alarm_vals->monitoring = false;
        switch_state_alarm();
      }
  }
}

static void alarm_window_load(Window *window) {
  
  Layer* alarm_window_layer = window_get_root_layer(window);
  window_set_background_color(window, COLOR_BACKGROUND);
  
  alarm_vals->img_alarm = gbitmap_create_with_resource(RESOURCE_ID_IMG_ALARM);
  
  alarm_vals->alarm_action_bar = action_bar_layer_create();
  action_bar_layer_add_to_window(alarm_vals->alarm_action_bar, window);
  action_bar_layer_set_click_config_provider(alarm_vals->alarm_action_bar, alarm_click_provider);
  
  #if defined(PBL_ROUND)
  alarm_vals->wake_text = macro_text_layer_create(GRect(20, 10, 120, 30), alarm_window_layer, COLOR_TEXT, COLOR_TEXT_BACKGROUND, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD), GTextAlignmentCenter);
  alarm_vals->alarm_time_text = macro_text_layer_create(GRect(30, 115, 100, 40), alarm_window_layer, COLOR_TEXT, COLOR_TEXT_BACKGROUND, forcedSquare, GTextAlignmentCenter);
  
  alarm_vals->alarm_bmp_layer = macro_bitmap_layer_create(alarm_vals->img_alarm, GRect(55, 60, 50, 50), alarm_window_layer, true);
  #else
  GSize size = layer_get_bounds(alarm_window_layer).size;
  GSize sizeActionBar = layer_get_bounds((Layer*)alarm_vals->alarm_action_bar).size;
  int bmpMargin = (size.w-sizeActionBar.w-50)/2;
  int bmpMarginH = (size.h-50)/2;
  
  alarm_vals->wake_text = macro_text_layer_create(GRect(10, (bmpMarginH-30)/2, size.w-sizeActionBar.w-20, 30), alarm_window_layer, COLOR_TEXT, COLOR_TEXT_BACKGROUND, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD), GTextAlignmentCenter);
  alarm_vals->alarm_time_text = macro_text_layer_create(GRect(10, size.h-(bmpMarginH-40)/2-40, size.w-sizeActionBar.w-20, 40), alarm_window_layer, COLOR_TEXT, COLOR_TEXT_BACKGROUND, forcedSquare, GTextAlignmentCenter);
  
  alarm_vals->alarm_bmp_layer = macro_bitmap_layer_create(alarm_vals->img_alarm, GRect(bmpMargin, bmpMarginH, 50, 50), alarm_window_layer, true);
  #endif
  
  alarm_vals->img_tick = gbitmap_create_with_resource(RESOURCE_ID_IMG_TICK);
  
  action_bar_layer_set_icon_animated(alarm_vals->alarm_action_bar, BUTTON_ID_UP, alarm_vals->img_tick, true);
  
  #ifndef PBL_COLOR
  action_bar_layer_set_background_color(alarm_vals->alarm_action_bar, GColorClear);
  #endif
}

static void alarm_window_unload(Window *window) {
  if(alarm_vals->monitoring && !wakeup_query(alarm_vals->monitor_id, NULL)){
    alarm_vals->monitor_id = wakeup_schedule(clock_to_timestamp(TODAY, alarm_vals->this_alarm/60, alarm_vals->this_alarm%60), REASON_ALARM_RING, true);
    if(alarm_vals->monitor_id < 0) {
      alarm_vals->monitor_id = wakeup_schedule(clock_to_timestamp(TODAY, alarm_vals->this_alarm/60, (alarm_vals->this_alarm-5)%60), REASON_ALARM_RING, true);
    }
    if(alarm_vals->monitor_id < 0) {
      alarm_vals->monitor_id = wakeup_schedule(clock_to_timestamp(TODAY, alarm_vals->this_alarm/60, (alarm_vals->this_alarm-10)%60), REASON_ALARM_RING, true);
    }
    #ifndef PBL_PLATFORM_APLITE
    if(alarm_vals->monitor_id < 0) {
      APP_LOG(APP_LOG_LEVEL_ERROR, "monitor alarm not set: %d", (int)alarm_vals->monitor_id);
    }
    #endif
  }
  
  text_layer_destroy(alarm_vals->wake_text);
  text_layer_destroy(alarm_vals->alarm_time_text);
  gbitmap_destroy(alarm_vals->img_tick);
  gbitmap_destroy(alarm_vals->img_zz);
  if(alarm_vals->img_alarm != NULL) { gbitmap_destroy(alarm_vals->img_alarm); alarm_vals->img_alarm = NULL; }
  action_bar_layer_destroy(alarm_vals->alarm_action_bar);
  bitmap_layer_destroy(alarm_vals->alarm_bmp_layer);
  window_destroy(alarm_window);
  alarm_window = NULL;
  free(alarm_vals);
  vars_allocated = false;
}

void allocate_vars() {
  if(!vars_allocated) {
    alarm_vals = malloc(sizeof(AlarmVals));
    alarm_vals->snooze_id = 0;
    alarm_vals->monitor_id = 0;
    alarm_vals->id = 0;
    alarm_vals->reason = 0;
    alarm_vals->monitoring = false;
    alarm_vals->auto_snooze_counter = 0;
    //alarm_vals->*vibe_timer = NULL;
    //alarm_vals->*auto_snooze_timer = NULL;
    //alarm_vals->*img_alarm = NULL;
    alarm_vals->snooze_time = 10;
    alarm_vals->this_alarm = 0;
    alarm_vals->monitorState = 0;
    vars_allocated = true;
  }
}

void alarm_init() {
  allocate_vars();
  window_stack_pop_all(true);
  wkday = (uint16_t)persist_read_int(PERSIST_WKD);
  wkend = (uint16_t)persist_read_int(PERSIST_WKE);
  
  time_t timenow = time(NULL);
  alarm_vals->now = *localtime(&timenow);
  
  int alarm_reason = -1;
  if(alarm_window == NULL) {
    alarm_window = window_create();
    window_set_window_handlers(alarm_window, (WindowHandlers) {
      .load = alarm_window_load,
      .unload = alarm_window_unload
    });
  }
  if(!window_is_loaded(alarm_window)) {
    wakeup_get_launch_event(&alarm_vals->id, &alarm_vals->reason);
    alarm_reason = alarm_vals->reason;
    window_stack_push(alarm_window, true);
  }
  set_monitoring_state(alarm_reason);
  if(alarm_vals->monitoring) {
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
  allocate_vars();
  
  wkday = (uint16_t)persist_read_int(PERSIST_WKD);
  wkend = (uint16_t)persist_read_int(PERSIST_WKE);
  
  if(wkday > -1 || wkend > -1) {
    time_t timenow = time(NULL);
    alarm_vals->now = *localtime(&timenow);
    
    int alarm_time;
    //(1 added to account for different enum values)
    int day = alarm_vals->now.tm_wday+1;
    
    if(day == SUNDAY || day == SATURDAY) {
      alarm_time = wkend;
    }
    else {
      alarm_time = wkday;
    }
    //alarm passed for today?
    if(alarm_time > -1 && alarm_time - (alarm_vals->now.tm_hour*60 + alarm_vals->now.tm_min) > MONITOR_PERIOD+5) {
      time_t alarm_timestamp = clock_to_timestamp(TODAY, (int)(alarm_time)/60, (int)(alarm_time)%60);
      alarm_vals->alarm_ids[day-1] = persist_read_int(PERSIST_ALARM0+day-1);
      if(alarm_vals->alarm_ids[day-1] < 0 || wakeup_query(alarm_vals->alarm_ids[day-1], NULL) == false) {
        if(alarm_time < MONITOR_PERIOD) {
          alarm_vals->alarm_ids[day-1] = -1;
        }
        else {
          alarm_vals->alarm_ids[day-1] = wakeup_schedule(alarm_timestamp-MONITOR_PERIOD*60, REASON_ALARM_MONITOR, true);  
        }
        if(alarm_vals->alarm_ids[day-1] < 0) {          
          alarm_vals->alarm_ids[day-1] = wakeup_schedule(alarm_timestamp, REASON_ALARM_RING, true);
        }
        if(alarm_vals->alarm_ids[day-1] < 0) {
          #ifndef PBL_PLATFORM_APLITE
          APP_LOG(APP_LOG_LEVEL_ERROR, "alarm not set today: %d", (int)alarm_vals->alarm_ids[day-1]);
          #endif
        }
        else {
          persist_write_int(PERSIST_ALARM0+day-1, alarm_vals->alarm_ids[day-1]);
          persist_write_int(PERSIST_ALARM_TIME0+day-1, alarm_timestamp);
        }
      }
    }
        
    //for tomorrow
    for(uint8_t ind=1; ind<7; ind++) {
      day = (alarm_vals->now.tm_wday + ind)%7 + 1;
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
    
    alarm_vals->alarm_ids[day-1] = persist_read_int(PERSIST_ALARM0+day-1);
    
    if(alarm_time > -1 && (alarm_vals->alarm_ids[day-1] < 0 || wakeup_query(alarm_vals->alarm_ids[day-1], NULL) == false)) {
      
      time_t alarm_timestamp = clock_to_timestamp((int)day, (int)(alarm_time/60), (int)(alarm_time%60));
      if(alarm_time < MONITOR_PERIOD) {
        alarm_vals->alarm_ids[day-1] = -1;  
      }
      else {
        alarm_vals->alarm_ids[day-1] = wakeup_schedule(alarm_timestamp-MONITOR_PERIOD*60, REASON_ALARM_MONITOR, true);
      }
      if(alarm_vals->alarm_ids[day-1] < 0) {          
        alarm_vals->alarm_ids[day-1] = wakeup_schedule(alarm_timestamp, REASON_ALARM_RING, true);
      }
      
      if(alarm_vals->alarm_ids[day-1] < 0) {
        #ifndef PBL_PLATFORM_APLITE
        APP_LOG(APP_LOG_LEVEL_ERROR, "alarm not set tmrw: %d", (int)alarm_vals->alarm_ids[day-1]);
        #endif
      }
      else {
        persist_write_int(PERSIST_ALARM0+day-1, alarm_vals->alarm_ids[day-1]);
        persist_write_int(PERSIST_ALARM_TIME0+day-1, alarm_timestamp);
      }
    }
  }
  
  if(alarm_window==NULL || !window_is_loaded(alarm_window)) {
    free(alarm_vals);
    vars_allocated = false;
  }
}

/**
 * Reset all alarms
**/
void reset_all_alarms() {
  wakeup_cancel_all();
  manage_alarms();
}

/*time_t get_next_alarm() {
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
    
    time_t next_alarm;
    //alarm passed for today?
    if(alarm_time > -1 && alarm_time - (now.tm_hour*60 + now.tm_min) > MONITOR_PERIOD*4) {
      next_alarm = clock_to_timestamp(day, alarm_time/60, alarm_time%60);
    }
    else {
      day = (now.tm_wday)%7 + 1;
      if(day == SUNDAY || day == SATURDAY) {
        alarm_time = wkend;
      }
      else {
        alarm_time = wkday;
      }
      
      next_alarm = clock_to_timestamp(day, alarm_time/60, alarm_time%60);
    }
    return next_alarm;
  }
  return 0;
}*/