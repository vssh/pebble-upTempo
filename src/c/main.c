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
#include "main.h"

uint idle_time = 120;
char main_time_text[6] = "";

int worker_start_msg = -1;

const int inbound_size = 96;
const int outbound_size = 16;

int* rec_page;

/**
 * Received messages from phone
**/
static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
  Tuple *t = dict_read_first(iterator);

  bool comm_activation;
  // Process all pairs present
  while(t != NULL) {
    // Process this pair's key    
    switch (t->key) {
      case APPMSG_RECEIVE_COMM_ACTIVATE_STATE:
        //sed comm acivation state
        comm_activation = persist_read_bool(PERSIST_COMM_ACTIVATION);
        appmsg_send_val(APPMSG_SEND_COMM_ACTIVATE_STATE, comm_activation);
        break;
      case APPMSG_CHANGE_COMM_ACTIVATE:
        //chage comm activation state
        comm_activation = t->value->uint8 != 0;
        persist_write_bool(PERSIST_COMM_ACTIVATION, comm_activation);
        persist_write_bool(PERSIST_DATALOGGING_ACTIVATION, comm_activation);
        persist_write_bool(PERSIST_DISPLAY_DATA_PHONE, comm_activation);
        persist_write_int(PERSIST_WORKER_WAKEUP_MESSAGE, WORKER_MSG_READ_SETTINGS);
        break;
      case APPMSG_DISTANCE:
        if(persist_read_bool(PERSIST_ACTIVE_TRACK))
          active_set_distance(t->value->cstring);
        else
          appmsg_send_val(APPMSG_STOP, 1);
        break;
      case APPMSG_SPEED:
        if(persist_read_bool(PERSIST_ACTIVE_TRACK))
          active_set_speed(t->value->cstring);
        break;
      case APPMSG_HEART:
        if(persist_read_bool(PERSIST_ACTIVE_TRACK))
          active_set_heart(t->value->cstring);
        break;
      case APPMSG_DAY_REC_VAL:
        day_val = t->value->uint32;
        break;
      case APPMSG_DAY_REC_AVG:
        day_avg_val = t->value->uint32;
        break;
      case APPMSG_DAY_REC_PAGE:
        *rec_page = t->value->int32;
        app_timer_register(200, (AppTimerCallback)set_phone_ui_vals, rec_page);
        break;
      case APPMSG_HISTORY_REC_VALS_0:
        history_vals[0] = t->value->uint32;
        break;
      case APPMSG_HISTORY_REC_VALS_1:
        history_vals[1] = t->value->uint32;
        break;
      case APPMSG_HISTORY_REC_VALS_2:
        history_vals[2] = t->value->uint32;
        break;
      case APPMSG_HISTORY_REC_VALS_3:
        history_vals[3] = t->value->uint32;
        break;
      case APPMSG_HISTORY_REC_VALS_4:
        history_vals[4] = t->value->uint32;
        break;
      case APPMSG_HISTORY_REC_VALS_5:
        history_vals[5] = t->value->uint32;
        break;
      case APPMSG_HISTORY_REC_VALS_6:
        history_vals[6] = t->value->uint32;
        break;
      case APPMSG_HISTORY_REC_PAGE:
        *rec_page = t->value->int32;
        app_timer_register(200, (AppTimerCallback)set_phone_history_vals, rec_page);
        break;
      case APPMSG_CANCEL:
        if(persist_read_bool(PERSIST_ACTIVE_TRACK))
          active_set_stop(false);
        break;
    }
    // Get next pair, if any
    t = dict_read_next(iterator);
  }
}

static void inbox_dropped_callback(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped! Reason: %d", reason);
}

static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed! Reason: %d", reason);
}

uint32_t get_avg(int key0) {
  uint32_t val = 0;
  uint8_t count = 0;
  for(int i=0; i<7; i++) {
        uint32_t temp = (uint32_t) persist_read_int(key0+i);
        if(temp > 0) {
          val += temp;
          count++;
        }
      }
  return (val/count);
}

/**
 * Reset values and calculate history in storage
**/
void reset_sync() {
  time_t now = time(NULL);
  time_t reset_time = persist_read_int(PERSIST_RESET_TIMESTAMP);
  //if last reset not too recent
  if(now-reset_time > 4*60*60) {
    struct tm time_now = *localtime(&now);
    uint8_t day = (time_now.tm_wday+6)%7;
    //reset walk, run, steps
    if(time_now.tm_hour < 3){
      persist_write_int(PERSIST_STEPS_DAY0+day, (uint32_t) persist_read_int(PERSIST_STEPS));
      persist_write_int(PERSIST_WALK_DAY0+day, (uint32_t) persist_read_int(PERSIST_WALK));
      persist_write_int(PERSIST_RUN_DAY0+day, (uint32_t) persist_read_int(PERSIST_JOG));
      
      persist_write_int(PERSIST_AVG_STEPS, get_avg(PERSIST_STEPS_DAY0));
      persist_write_int(PERSIST_AVG_WALK, get_avg(PERSIST_WALK_DAY0));
      persist_write_int(PERSIST_AVG_JOG, get_avg(PERSIST_RUN_DAY0));
      
      persist_write_int(PERSIST_WALK, 0);
      persist_write_int(PERSIST_STEPS, 0);
      persist_write_int(PERSIST_JOG, 0);
      
      persist_write_int(PERSIST_RESET_TIMESTAMP, now);
    }
    //reset sleep
    else if(time_now.tm_hour > 11 && time_now.tm_hour < 15) {
      persist_write_int(PERSIST_SLEEP_DAY0+day, (uint32_t) persist_read_int(PERSIST_SLEEP));
      
      persist_write_int(PERSIST_AVG_SLEEP, get_avg(PERSIST_SLEEP_DAY0));

      persist_write_int(PERSIST_SLEEP, 0);
      
      persist_write_int(PERSIST_RESET_TIMESTAMP, now);
    }
  }
  if(!persist_read_bool(PERSIST_DISPLAY_DATA_PHONE)) {
    set_ui_vals(-1, false);
  }
  persist_write_int(PERSIST_WORKER_WAKEUP_MESSAGE, WORKER_MSG_READ_SETTINGS);
}

/**
 * On message received from worker
**/
static void worker_message_handler(uint16_t type, AppWorkerMessage *data) {
  
  switch(type) {
    case WORKER_MSG_TYPE:
      //send type for smart alarm
      monitor_message_handler(type, data);
      break;
    case WORKER_MSG_UPDATE:
      //update UI values if displayed values not from phone
      if(!persist_read_bool(PERSIST_DISPLAY_DATA_PHONE)) {
        set_ui_vals(-1, false);
      }
      break;
    case WORKER_MSG_IDLE_ALERT:
      //show idle alert
      if(alarm_window == NULL || !window_is_loaded(alarm_window)) {
        if(day_window == NULL || !window_is_loaded(day_window)) {
          window_stack_pop_all(true);
          day_init(false);
        }
        set_ui_vals(UI_IDLE, true);
        persist_write_int(PERSIST_WORKER_WAKEUP_MESSAGE, -1);
      }
      break;
    case WORKER_MSG_RESET:
      //reset values
      reset_sync();
      if(launch_reason() == APP_LAUNCH_WORKER) {
        window_stack_pop_all(true);
      }
    break;
  }
}

/**
 * On time changed
**/
static void main_tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  char *time_format;
  if (clock_is_24h_style()) {
    time_format = "%R";
  }
  else {
    time_format = "%I:%M";
  }
  strftime(main_time_text, sizeof(main_time_text), time_format, tick_time);
  
  //send time update to various windows
  alarm_time_handler(main_time_text, tick_time->tm_hour, tick_time->tm_min);
  day_time_handler(main_time_text);
  active_set_time(main_time_text);
}

void handle_init(void) {
  app_worker_message_subscribe(worker_message_handler);
  
  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
  // Setup messaging
  app_message_open(inbound_size, outbound_size);
  
  tick_timer_service_subscribe(MINUTE_UNIT, main_tick_handler);  
  wakeup_service_subscribe(alarm_init);

  rec_page = malloc(sizeof(int));

  manage_persist();
  switch(launch_reason()) {
    case APP_LAUNCH_WORKER:
      worker_start_msg = persist_read_int(PERSIST_WORKER_WAKEUP_MESSAGE);
      if(worker_start_msg == WORKER_MSG_RESET) {
        //start reset
        day_init(true);
        reset_sync();
        worker_start_msg = -1;
        if(launch_reason() == APP_LAUNCH_WORKER) {
          window_stack_pop_all(true);
        }
      }
      else if(worker_start_msg == WORKER_MSG_IDLE_ALERT) {
        //show idle alert
        day_init(false);
        persist_write_int(PERSIST_WORKER_WAKEUP_MESSAGE, -1);
      }
      break;
    
    case APP_LAUNCH_WAKEUP:     
      // Get details and handle the wakeup
      alarm_init();
      break;
    default:
      //directly show active window
      if(persist_read_bool(PERSIST_ACTIVE_TRACK)) {
        active_init();
      }
      else {
        day_init(true);
      }
      break;
  }
  
  if(!app_worker_is_running()) {
    app_worker_launch();
  }
}

void handle_deinit(void) {
  free(rec_page);
  manage_alarms();
  tick_timer_service_unsubscribe();
  app_worker_message_unsubscribe();
  app_message_deregister_callbacks();
}

int main(void) {
  handle_init();
  app_event_loop();
  handle_deinit();
}
