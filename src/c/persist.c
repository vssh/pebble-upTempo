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
#include "persist.h"

/**
 * Remove all stored values
**/
void clear_storage() {
  for(int i=100; i<350; i++) {
    persist_delete(i);
  }
}

/**
 * Initialize storage
**/
bool init_storage() {
  bool error = false;
  if(!persist_exists(PERSIST_STEPS)) error = error || persist_write_int(PERSIST_STEPS, 0) == S_SUCCESS;
  if(!persist_exists(PERSIST_WALK)) error = error || persist_write_int(PERSIST_WALK, 0) == S_SUCCESS;
  if(!persist_exists(PERSIST_JOG)) error = error || persist_write_int(PERSIST_JOG, 0) == S_SUCCESS;
  if(!persist_exists(PERSIST_SLEEP)) error = error || persist_write_int(PERSIST_SLEEP, 0) == S_SUCCESS;
  
  if(!persist_exists(PERSIST_AVG_STEPS)) error = error || persist_write_int(PERSIST_AVG_STEPS, 0) == S_SUCCESS;
  if(!persist_exists(PERSIST_AVG_WALK)) error = error || persist_write_int(PERSIST_AVG_WALK, 0) == S_SUCCESS;
  if(!persist_exists(PERSIST_AVG_JOG)) error = error || persist_write_int(PERSIST_AVG_JOG, 0) == S_SUCCESS;
  if(!persist_exists(PERSIST_AVG_SLEEP)) error = error || persist_write_int(PERSIST_AVG_SLEEP, 0) == S_SUCCESS;
  
  if(!persist_exists(PERSIST_WKD)) error = error || persist_write_int(PERSIST_WKD, -1) == S_SUCCESS;
  if(!persist_exists(PERSIST_WKE)) error = error || persist_write_int(PERSIST_WKE, -1) == S_SUCCESS;
  if(!persist_exists(PERSIST_SINGLE_ALARM_TIME)) error = error || persist_write_int(PERSIST_SINGLE_ALARM_TIME, 0) == S_SUCCESS;
  if(!persist_exists(PERSIST_SINGLE_ALARM)) error = error || persist_write_int(PERSIST_SINGLE_ALARM, -1) == S_SUCCESS;
  if(!persist_exists(PERSIST_AUTO_SNOOZE)) error = error || persist_write_int(PERSIST_AUTO_SNOOZE, 0) == S_SUCCESS;
  for(int i=0; i<7; i++) {if(!persist_exists(PERSIST_ALARM_TIME0+i)) error = error || persist_write_int(PERSIST_ALARM_TIME0+i, 0) == S_SUCCESS;}
  

  if(!persist_exists(PERSIST_IDLE_TIME)) error = error || persist_write_int(PERSIST_IDLE_TIME, 90) == S_SUCCESS;
  if(!persist_exists(PERSIST_RESET_TIMESTAMP)) error = error || persist_write_int(PERSIST_RESET_TIMESTAMP, time(NULL)) == S_SUCCESS;
  if(!persist_exists(PERSIST_PHONE_DATA_SHARING)) error = error || persist_write_bool(PERSIST_PHONE_DATA_SHARING, false) == S_SUCCESS;
  //if(!persist_exists(PERSIST_DISPLAY_DATA_PHONE)) error = error || persist_write_bool(PERSIST_DISPLAY_DATA_PHONE, false) == S_SUCCESS;
  if(!persist_exists(PERSIST_COMM_ACTIVATION)) error = error || persist_write_bool(PERSIST_COMM_ACTIVATION, false) == S_SUCCESS;
  if(!persist_exists(PERSIST_ACTIVE_TRACK)) error = error || persist_write_bool(PERSIST_ACTIVE_TRACK, false) == S_SUCCESS;
  if(!persist_exists(PERSIST_ACTIVE_TRACK_PAUSE)) error = error || persist_write_bool(PERSIST_ACTIVE_TRACK_PAUSE, false) == S_SUCCESS;
  if(!persist_exists(PERSIST_ACTIVE_TRACK_TYPE)) error = error || persist_write_int(PERSIST_ACTIVE_TRACK_TYPE, TYPE_UNKNOWN) == S_SUCCESS;
  if(!persist_exists(PERSIST_ACTIVE_TRACK_START)) error = error || persist_write_int(PERSIST_ACTIVE_TRACK_START, 0) == S_SUCCESS;
  if(!persist_exists(PERSIST_ACTIVE_TRACK_GPS_BLE)) error = error || persist_write_int(PERSIST_ACTIVE_TRACK_GPS_BLE, -1) == S_SUCCESS;
  if(!persist_exists(PERSIST_ACTIVE_TRACK_DURATION)) error = error || persist_write_int(PERSIST_ACTIVE_TRACK_DURATION, 0) == S_SUCCESS;
  
  if(!persist_exists(PERSIST_WORKER_WAKEUP_MESSAGE)) error = error || persist_write_int(PERSIST_WORKER_WAKEUP_MESSAGE, -1) == S_SUCCESS;
  
  for(int i=0; i<7; i++) {if(!persist_exists(PERSIST_STEPS_DAY0+i)) error = error || persist_write_int(PERSIST_STEPS_DAY0+i, 0) == S_SUCCESS;}
  for(int i=0; i<7; i++) {if(!persist_exists(PERSIST_WALK_DAY0+i)) error = error || persist_write_int(PERSIST_WALK_DAY0+i, 0) == S_SUCCESS;}
  for(int i=0; i<7; i++) {if(!persist_exists(PERSIST_RUN_DAY0+i)) error = error || persist_write_int(PERSIST_RUN_DAY0+i, 0) == S_SUCCESS;}
  for(int i=0; i<7; i++) {if(!persist_exists(PERSIST_SLEEP_DAY0+i)) error = error || persist_write_int(PERSIST_SLEEP_DAY0+i, 0) == S_SUCCESS;}
  
  return error;
}

/**
 * Handle migration between storage versions
**/
void handle_migration(int version) {
  switch(version) {
    case 0:
      clear_storage();
      break;
    case 1:
      persist_delete(PERSIST_PAUSE_TIME);
      persist_delete(PERSIST_YEST_STEPS);
      persist_delete(PERSIST_YEST_WALK);
      persist_delete(PERSIST_YEST_JOG);
      persist_delete(PERSIST_YEST_SLEEP);
      break;
    case 4:
      persist_delete(PERSIST_DISPLAY_DATA_PHONE);
      break;
    default:
      break;
  }
}

/**
 * Manage storage
**/
void manage_persist() {
  int version = persist_exists(PERSIST_STORAGE_VERSION) ? persist_read_int(PERSIST_STORAGE_VERSION) : 0;
  if(version != CURRENT_STORAGE_VERSION) {
    handle_migration(version);
    bool error = init_storage();
    if(!error) {
      persist_write_int(PERSIST_STORAGE_VERSION, CURRENT_STORAGE_VERSION);
    }
    else {
      APP_LOG(APP_LOG_LEVEL_ERROR, "Storage error");
    }
  }
}