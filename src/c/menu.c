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
#include "menu.h"
  
Window *menu_window;

typedef struct{
  SimpleMenuLayer *menu_layer;
  SimpleMenuSection menu_sections[2];
  SimpleMenuItem other_menu_items[11];
  SimpleMenuItem phone_menu_items[2];
  bool logging;
  int session_type;
  #ifndef PBL_PLATFORM_APLITE
  char idle_str[6];
  char wkd_str[6];
  char wke_str[6];
  #endif
  char yes[4];
  char no[3];
}MenuVals;
MenuVals *menu_vals;

/*SimpleMenuLayer *menu_layer;
SimpleMenuSection menu_sections[2];
SimpleMenuItem other_menu_items[11];
SimpleMenuItem phone_menu_items[3];
bool logging;
bool data_source_phone;
int session_type = TYPE_UNKNOWN;
#ifndef PBL_PLATFORM_APLITE
char idle_str[6] = "--";
char wkd_str[6] = "--";
char wke_str[6] = "--";
#endif

char yes[4] = "Yes";
char no[3] = "No";*/

// We need to save a reference to the ClickConfigProvider originally set by the menu layer
ClickConfigProvider previous_ccp;

// Define what you want to do when the back button is pressed
void back_button_handler(ClickRecognizerRef recognizer, void *context) {
  window_stack_pop(true);
  day_init(true);
}
 
// This is the new ClickConfigProvider we will set, it just calls the old one and then subscribe
// for back button events.
void new_ccp(void *context) {
  previous_ccp(context);
  window_single_click_subscribe(BUTTON_ID_BACK, back_button_handler);
}
 
// Call this from your init function to do the hack
void force_back_button(Window *window, MenuLayer *menu_layer) {
  previous_ccp = window_get_click_config_provider(window);
  window_set_click_config_provider_with_context(window, new_ccp, menu_layer);
}

/**
 * Set idle alert time
**/
void show_idle_menu() {
  window_stack_pop(true);
  timer_init(TIMER_TYPE_IDLE);
}

/**
 * Set alarms
**/
void send_alarm(int index, void* ctx) {  
  window_stack_pop(true);
  switch(index) {
    case 0:
      timer_init(TIMER_TYPE_WEEKDAY);
      break;
    case 1:
      timer_init(TIMER_TYPE_WEEKEND);
      break;
  }  
}

/**
 * Datalogging enable or disable
**/
static void toggle_datalogging() {
  menu_vals->logging= !menu_vals->logging;
  if(menu_vals->logging) {
    menu_vals->phone_menu_items[0].subtitle = menu_vals->yes;
  }
  else {
    menu_vals->phone_menu_items[0].subtitle = menu_vals->no;
  }
  phoneDataSharing = menu_vals->logging;
  persist_write_bool(PERSIST_PHONE_DATA_SHARING, menu_vals->logging);
  persist_write_int(PERSIST_WORKER_WAKEUP_MESSAGE, WORKER_MSG_READ_SETTINGS);
  layer_mark_dirty(simple_menu_layer_get_layer(menu_vals->menu_layer));
}

/**
 * Enable or disable display of data from phone
**/
/*static void toggle_data_source() {
  menu_vals->data_source_phone = !menu_vals->data_source_phone;
  persist_write_bool(PERSIST_DISPLAY_DATA_PHONE, menu_vals->data_source_phone);
  if(menu_vals->data_source_phone) {
    menu_vals->phone_menu_items[1].subtitle = menu_vals->yes;
  }
  else {
    menu_vals->phone_menu_items[1].subtitle = menu_vals->no;
  }
  layer_mark_dirty(simple_menu_layer_get_layer(menu_vals->menu_layer));
}*/

/**
 * Show activity track window and set initial values
**/
static void start_session(int type) {
  persist_write_int(PERSIST_ACTIVE_TRACK_START, time(NULL));
  persist_write_int(PERSIST_ACTIVE_TRACK_DURATION, 0);
  persist_write_int(PERSIST_ACTIVE_TRACK_TYPE, (uint8_t)type);
  persist_write_bool(PERSIST_ACTIVE_TRACK, true);
  active_init();
}

/**
 * Start activity tracking
**/
void start_activity_callback(int index, void* ctx) {
  persist_write_int(PERSIST_ACTIVE_TRACK_GPS_BLE, index);
  if(index > 0) {
    appmsg_send_val(APPMSG_START, index);
  }
  start_session(menu_vals->session_type);
}

/**
 * Show GPS/BLE menu
**/
void record_activity_callback(int index, void* ctx) {
  Layer* menu_window_layer = window_get_root_layer(menu_window);
  GRect bounds = layer_get_frame(menu_window_layer);
  
  switch(index) {
    case 0:
      menu_vals->session_type = TYPE_UNKNOWN;
      break;
    case 1:
      menu_vals->session_type = TYPE_CYCLE;
      break;
    case 2:
      menu_vals->session_type = TYPE_JOG;
      break;
    case 3:
      menu_vals->session_type = TYPE_WALK;
      break;
    case 4:
      menu_vals->session_type = TYPE_SLEEP;
      break;
    case 5:
      menu_vals->session_type = TYPE_TRAIN;
      break;
    case 6:
      menu_vals->session_type = TYPE_SPORT;
      break;
    case 7:
      menu_vals->session_type = TYPE_MARTIAL;
      break;
    case 8:
      menu_vals->session_type = TYPE_WATER;
      break;
    case 9:
      menu_vals->session_type = TYPE_WINTER;
      break;
    case 10:
      menu_vals->session_type = TYPE_OTHER;
      break;
  }
  
  menu_vals->other_menu_items[0] = (SimpleMenuItem) {
    .title = "None",
    .callback = start_activity_callback,
  };
  menu_vals->other_menu_items[1] = (SimpleMenuItem) {
    .title = "GPS",
    .callback = start_activity_callback,
  };
  menu_vals->other_menu_items[2] = (SimpleMenuItem) {
    .title = "BLE",
    .callback = start_activity_callback,
  };
  menu_vals->other_menu_items[3] = (SimpleMenuItem) {
    .title = "Both",
    .callback = start_activity_callback,
  };
  
  menu_vals->menu_sections[0] = (SimpleMenuSection) {
    .num_items = 4,
    .items = menu_vals->other_menu_items,
  };
  
  if(bluetooth_connection_service_peek()) {
    layer_remove_child_layers(menu_window_layer);
    simple_menu_layer_destroy(menu_vals->menu_layer);
    menu_vals->menu_layer = simple_menu_layer_create(bounds, menu_window, menu_vals->menu_sections, 1, NULL);
    layer_add_child(menu_window_layer, simple_menu_layer_get_layer(menu_vals->menu_layer));
    force_back_button(menu_window, simple_menu_layer_get_menu_layer(menu_vals->menu_layer));
  }
  else {
    start_activity_callback(0, NULL);
  }
}

/**
 * Show activities menu
**/
static void record_activity() {
  Layer* menu_window_layer = window_get_root_layer(menu_window);
  GRect bounds = layer_get_frame(menu_window_layer);
  
  menu_vals->other_menu_items[8] = (SimpleMenuItem) {
    .title = "Water sp.",
    .callback = record_activity_callback,
  };
  menu_vals->other_menu_items[7] = (SimpleMenuItem) {
    .title = "Martial art",
    .callback = record_activity_callback,
  };
  menu_vals->other_menu_items[6] = (SimpleMenuItem) {
    .title = "Sport",
    .callback = record_activity_callback,
  };
  menu_vals->other_menu_items[5] = (SimpleMenuItem) {
    .title = "Train",
    .callback = record_activity_callback,
  };
  menu_vals->other_menu_items[4] = (SimpleMenuItem) {
    .title = "Sleep",
    .callback = record_activity_callback,
  };
  menu_vals->other_menu_items[0] = (SimpleMenuItem) {
    .title = "Auto",
    .callback = record_activity_callback,
  };
  menu_vals->other_menu_items[1] = (SimpleMenuItem) {
    .title = "Cycle",
    .callback = record_activity_callback,
  };
  menu_vals->other_menu_items[2] = (SimpleMenuItem) {
    .title = "Run",
    .callback = record_activity_callback,
  };
  menu_vals->other_menu_items[3] = (SimpleMenuItem) {
    .title = "Walk",
    .callback = record_activity_callback,
  };
  menu_vals->other_menu_items[9] = (SimpleMenuItem) {
    .title = "Winter sp.",
    .callback = record_activity_callback,
  };
  menu_vals->other_menu_items[10] = (SimpleMenuItem) {
    .title = "Other",
    .callback = record_activity_callback,
  };
  
  menu_vals->menu_sections[0] = (SimpleMenuSection) {
    .num_items = 11,
    .items = menu_vals->other_menu_items,
  };
  
  layer_remove_child_layers(menu_window_layer);
  simple_menu_layer_destroy(menu_vals->menu_layer);
  menu_vals->menu_layer = simple_menu_layer_create(bounds, menu_window, menu_vals->menu_sections, 1, NULL);
  
  layer_add_child(menu_window_layer, simple_menu_layer_get_layer(menu_vals->menu_layer));
  force_back_button(menu_window, simple_menu_layer_get_menu_layer(menu_vals->menu_layer));
}
 
/**
 * Show alarm menu
**/
static void show_alarm_menu() {
  Layer* menu_window_layer = window_get_root_layer(menu_window);
  GRect bounds = layer_get_frame(menu_window_layer);
  
  #ifndef PBL_PLATFORM_APLITE
  int wk_t = persist_read_int(PERSIST_WKD);
  if(wk_t > -1)
    snprintf(menu_vals->wkd_str, 6, "%.2u:%.2u", wk_t/60, wk_t%60);
  else {
    strcpy(menu_vals->wkd_str, "--");
  }
  wk_t = persist_read_int(PERSIST_WKE);
  if(wk_t > -1)
    snprintf(menu_vals->wke_str, 6, "%.2u:%.2u", wk_t/60, wk_t%60);
  else {
    strcpy(menu_vals->wke_str, "--");
  }
  #endif
  
  menu_vals->other_menu_items[0] = (SimpleMenuItem) {
    .title = "Weekdays",
    .callback = send_alarm,
    #ifndef PBL_PLATFORM_APLITE
    .subtitle = menu_vals->wkd_str,
    #endif
  };
  menu_vals->other_menu_items[1] = (SimpleMenuItem) {
    .title = "Weekends",
    .callback = send_alarm,
    #ifndef PBL_PLATFORM_APLITE
    .subtitle = menu_vals->wke_str,
    #endif
  };
  
  menu_vals->menu_sections[0] = (SimpleMenuSection) {
    .num_items = 2,
    .items = menu_vals->other_menu_items,
  };
  
  layer_remove_child_layers(menu_window_layer);
  simple_menu_layer_destroy(menu_vals->menu_layer);
  menu_vals->menu_layer = simple_menu_layer_create(bounds, menu_window, menu_vals->menu_sections, 1, NULL);
  
  layer_add_child(menu_window_layer, simple_menu_layer_get_layer(menu_vals->menu_layer));
  force_back_button(menu_window, simple_menu_layer_get_menu_layer(menu_vals->menu_layer));
}

static void menu_window_load(Window *window) {
  Layer* menu_window_layer = window_get_root_layer(menu_window);
  GRect bounds = layer_get_frame(menu_window_layer);
  menu_vals->logging = persist_read_bool(PERSIST_PHONE_DATA_SHARING);
  //menu_vals->data_source_phone = persist_read_bool(PERSIST_DISPLAY_DATA_PHONE);
  
  menu_vals->other_menu_items[0] = (SimpleMenuItem) {
    .title = "Alarms",
    .callback= show_alarm_menu,
  };
  
  #ifndef PBL_PLATFORM_APLITE
  uint idle_t = persist_read_int(PERSIST_IDLE_TIME);
  if(idle_t > 0)
    snprintf(menu_vals->idle_str, 6, "%.2u:%.2u", idle_t/60, idle_t%60);
  else {
    strcpy(menu_vals->idle_str, "--");
  }
  #endif
  menu_vals->other_menu_items[1] = (SimpleMenuItem) {
    .title = "Idle alert",
    .callback = show_idle_menu,
    #ifndef PBL_PLATFORM_APLITE
    .subtitle = menu_vals->idle_str,
    #endif
  };
  
  menu_vals->phone_menu_items[0] = (SimpleMenuItem) {
    .title = "Data sharing",
    .callback= toggle_datalogging,
  };
  if(menu_vals->logging) {
    menu_vals->phone_menu_items[0].subtitle = menu_vals->yes;
  }
  else {
    menu_vals->phone_menu_items[0].subtitle = menu_vals->no;
  }
  
  menu_vals->phone_menu_items[1] = (SimpleMenuItem) {
    .title = "Start activity",
    .callback = record_activity,
  };
  
  /*menu_vals->phone_menu_items[1] = (SimpleMenuItem) {
    .title = "Data from phone",
    .callback = toggle_data_source,
  };
  if(menu_vals->data_source_phone) {
    menu_vals->phone_menu_items[1].subtitle = menu_vals->yes;
  }
  else {
    menu_vals->phone_menu_items[1].subtitle = menu_vals->no;
  }*/
  
  menu_vals->menu_sections[0] = (SimpleMenuSection) {
    .title = APP_VERSION,
    .num_items = 2,
    .items = menu_vals->other_menu_items,
  };
  
  menu_vals->menu_sections[1] = (SimpleMenuSection) {
    .title = "Phone integration",
    .num_items = 2,
    .items = menu_vals->phone_menu_items,
  };
  
  int num_sections = 1;
  if(persist_read_bool(PERSIST_COMM_ACTIVATION)) num_sections = 2;
  menu_vals->menu_layer = simple_menu_layer_create(bounds, menu_window, menu_vals->menu_sections, num_sections, NULL);
  layer_add_child(menu_window_layer, simple_menu_layer_get_layer(menu_vals->menu_layer));
  force_back_button(window, simple_menu_layer_get_menu_layer(menu_vals->menu_layer));
}

static void menu_window_unload(Window *window) {
  simple_menu_layer_destroy(menu_vals->menu_layer);
  menu_vals->menu_layer = NULL;

  window_destroy(menu_window);
  menu_window = NULL;
  free(menu_vals);
}

void menu_init() {
  menu_vals = malloc(sizeof(MenuVals));
  menu_vals->session_type = TYPE_UNKNOWN;
  #ifndef PBL_PLATFORM_APLITE
  snprintf(menu_vals->idle_str, 6, "--");
  snprintf(menu_vals->wkd_str, 6, "--");
  snprintf(menu_vals->wke_str, 6, "--");
  #endif
  snprintf(menu_vals->yes, 4, "Yes");
  snprintf(menu_vals->no, 3, "No");
  
  window_destroy(menu_window);
  menu_window = window_create();
  window_set_window_handlers(menu_window, (WindowHandlers) {
    .load = menu_window_load,
    .unload = menu_window_unload
  });
  window_stack_push(menu_window, true);
}