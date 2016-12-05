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

#include <pebble_worker.h>
#include "recognizer.h"

static LowPassFilter mFilter;
static Counter mCounter = {.movement=0,
                           .sleepTime=0,
                           .walkTime=0,
                           .jogTime=0,
                           .steps=0,
                           .dataSize = 0,
                           .state=STATE_IDLE,
                           .movementCount=0,
                           .idleTime=90,
                           .phoneIntegration=false,
                           .lastHr=0};

static DataLoggingSessionRef sessionsDataLog, mSensorDataLog;

static AccelData mAcceleration[SAMPLE_SIZE];
static Session mCurrentSession = {.type=TYPE_IDLE, .startTime=0, .endTime=0, .steps=0};
static Session mPreviousSession = {.type=TYPE_IDLE, .startTime=0, .endTime=0, .steps=0};
static Session mNewSession = {.type=TYPE_IDLE, .startTime=0, .endTime=0, .steps=0};

/**
 * Send appMessage
 **/
static void sendMsgToApp(uint16_t value0, uint16_t id) { 
  AppWorkerMessage send_msg = {
    .data0 = value0
  };
  app_worker_send_message(id, &send_msg);
}

/**
 * Send dataLog
 **/
static void sendLog(Session session) {
  //if datalogging is on
  if(mCounter.phoneIntegration) {
    Session finishedSession[1] = {(session)};
    data_logging_log(sessionsDataLog, finishedSession, 1);
  }
}

/**
 * Read settings and values from storage
 **/
static void readSettings() {  
  mCounter.sleepTime = persist_read_int(PERSIST_SLEEP);
  mCounter.walkTime = persist_read_int(PERSIST_WALK);
  mCounter.jogTime = persist_read_int(PERSIST_JOG);
  mCounter.steps = persist_read_int(PERSIST_STEPS);
  
  mCounter.idleTime = persist_read_int(PERSIST_IDLE_TIME);
  mCounter.phoneIntegration = persist_read_bool(PERSIST_PHONE_INTEGRATION);
}

/**
 * Integrate new session
**/
void resetSession() {
  
  //read settings if indicated by watchapp
  if(persist_read_int(PERSIST_WORKER_WAKEUP_MESSAGE) == WORKER_MSG_READ_SETTINGS) {
    readSettings();
    persist_write_int(PERSIST_WORKER_WAKEUP_MESSAGE, -1);
  }
  
  if(mNewSession.endTime >= mNewSession.startTime) {
    
    //if the session continues
    if(mNewSession.type == mCurrentSession.type && mNewSession.startTime>=mCurrentSession.endTime) {
      
      //if sesion is walking or running
      if(mNewSession.type >= TYPE_WALK) {
        //add steps if state is active
        if(mCounter.state == STATE_ACTIVE || mCurrentSession.steps > SESSION_WALK_MIN_STEPS) {
          if(mCounter.state == STATE_ACTIVE) {
            mCounter.steps += mNewSession.steps;
          }
          else {
            mCounter.steps += mNewSession.steps+mCurrentSession.steps;
            mCounter.state = STATE_ACTIVE;
          }
          persist_write_int(PERSIST_STEPS, mCounter.steps);
          sendMsgToApp(0, WORKER_MSG_UPDATE);
        }
      }
      else if(mNewSession.type == TYPE_IDLE) {
        if(mCounter.state == STATE_ACTIVE) {
          mCounter.state = STATE_IDLE;
        }
        
        if(mCounter.idleTime > 0 && time(NULL) - mPreviousSession.endTime > mCounter.idleTime*60) {
          mPreviousSession.endTime = time(NULL);
          mPreviousSession.type = TYPE_IDLE;
          
          //show idle alert
          persist_write_int(PERSIST_WORKER_WAKEUP_MESSAGE, WORKER_MSG_IDLE_ALERT);
          worker_launch_app();
          sendMsgToApp(0, WORKER_MSG_IDLE_ALERT);
        }
      }
      
      mCurrentSession.endTime = mNewSession.endTime;
      mCurrentSession.steps += mNewSession.steps;
    }
    //session changes
    else {
      uint32_t currentDuration = mCurrentSession.endTime - mCurrentSession.startTime;
      if(mCurrentSession.type == TYPE_SLEEP) {
        //if sleep session is not too short
        if(currentDuration > SESSION_SLEEP_MIN_DURATION_SHORT) {
          uint32_t timeGap = mCurrentSession.startTime - mPreviousSession.endTime;
          uint32_t previousDuration;
          //if sleep session should be merged with last sleep session
          if(mPreviousSession.type == mCurrentSession.type && timeGap < SESSION_TIMEGAP_LONG) {
            mCurrentSession.startTime = mPreviousSession.startTime;
            currentDuration = mCurrentSession.endTime - mCurrentSession.startTime;
            //use steps to track breaks in sleep(reducing memory usage)
            mCurrentSession.steps = mPreviousSession.steps + 1;
            previousDuration = mPreviousSession.endTime-mPreviousSession.startTime;
          }
          else {
            mCounter.state = STATE_IDLE;
            previousDuration = 0;
          }
          
          uint16_t allowed_breaks = (currentDuration/(60*30))+1;        //2 breaks per hour
          //should sleep session be saved?
          if(currentDuration > SESSION_SLEEP_MIN_DURATION_LONG &&
             (mCurrentSession.steps > allowed_breaks)) {
            if(previousDuration > SESSION_SLEEP_MIN_DURATION_LONG && mCounter.state == STATE_SLEEP) {
              mCounter.sleepTime += currentDuration-previousDuration;
            }
            mCounter.state = STATE_SLEEP;
            persist_write_int(PERSIST_SLEEP, mCounter.sleepTime);
            sendLog(mCurrentSession);          
            
            //limit the amount of breaks(to disallow long sessions of sleep when not worn)
            if(mCurrentSession.steps > (uint32_t)(allowed_breaks+4)) {
              mCurrentSession.steps = (uint32_t)(allowed_breaks+4);
            }
          }
          mPreviousSession = mCurrentSession;
        }
      }
      //if state is active
      else if(mCounter.state == STATE_ACTIVE) {
        if(mCurrentSession.type == TYPE_WALK) {
          mCounter.walkTime += currentDuration;
          persist_write_int(PERSIST_WALK, mCounter.walkTime);
        }
        else if(mCurrentSession.type == TYPE_JOG) {
          mCounter.jogTime += currentDuration;
          persist_write_int(PERSIST_JOG, mCounter.jogTime);
        }
        mPreviousSession = mCurrentSession;
        sendLog(mCurrentSession);
      }
      
      //update current session
      mCurrentSession = mNewSession;
      //update UI on app
      sendMsgToApp(0, WORKER_MSG_UPDATE);
    }
    #if PBL_API_EXISTS(health_service_peek_current_value)
    if(mCounter.phoneIntegration) {
      HealthServiceAccessibilityMask hr = health_service_metric_accessible(HealthMetricHeartRateBPM, time(NULL), time(NULL));
      if (hr & HealthServiceAccessibilityMaskAvailable) {
        uint16_t val = (int16_t) health_service_peek_current_value(HealthMetricHeartRateBPM);
        if(val > 0 && mCounter.lastHr != val) {
          uint32_t now = time(NULL);
          DataTime dataTime[] = {(DataTime){.data = val, .timestamp = now, .sensor = SENSOR_TYPE_HR}};
          if(mCurrentSession.type == TYPE_IDLE && mPreviousSession.endTime > 0 &&
             (now-mPreviousSession.endTime > 10*60 || mPreviousSession.type == TYPE_IDLE)) {
            dataTime[0].sensor = SENSOR_TYPE_HR_RESTING;
          }
          data_logging_log(mSensorDataLog, &dataTime, 1);
          mCounter.lastHr = val;
        }
      }
    }
    #endif
  }
  
  //send app activity type(for use in smart alarm)
  sendMsgToApp(mCurrentSession.type, WORKER_MSG_TYPE);
}

//every hour to reset values on day change(replace with alarms when possible)
static void hourTickHandler(struct tm *tick_time, TimeUnits units_changed) {
  if(tick_time->tm_hour < 3 || (tick_time->tm_hour > 11 && tick_time->tm_hour < 15)) {    
    persist_write_int(PERSIST_WORKER_WAKEUP_MESSAGE, WORKER_MSG_RESET);    
    worker_launch_app();
    sendMsgToApp(0, WORKER_MSG_RESET);
  }
}

// Handle accleration data
void processAccelerometerData(AccelData* acceleration, uint32_t size) {
    // Add samples
    uint16_t i = 0, j = 0;
    //reject all the rest in case of vibration
    for (; i < size && mCounter.dataSize + j < SAMPLE_SIZE && !acceleration[i].did_vibrate; i++) {
      mAcceleration[mCounter.dataSize + j] = acceleration[i];
      j++;
    }
    mCounter.dataSize += j;
    // Not enough, so add data to collection first
    if (mCounter.dataSize >= SAMPLE_SIZE) { // Enough for classification
      Feature feature;
      feature.meanV = 0.0, feature.meanH = 0.0, feature.deviationV = 0.0, feature.deviationH = 0.0;
      
      for (uint16_t i = 0; i < SAMPLE_SIZE; i++) {
        // Filter out the gravity vector
        goThroughFilter(&mFilter, mAcceleration[i].x, mAcceleration[i].y, mAcceleration[i].z);
        
        // Convert to linear acceleration
        mAcceleration[i].x = mAcceleration[i].x - mFilter.x;
        mAcceleration[i].y = mAcceleration[i].y - mFilter.y;
        mAcceleration[i].z = mAcceleration[i].z - mFilter.z;
        
        // Project 3D acceleration vector to gravity direction
        double v = ((double) mAcceleration[i].x * (double) mFilter.x
          + (double) mAcceleration[i].y * (double) mFilter.y
          + (double) mAcceleration[i].z * (double) mFilter.z) / (double) norm((int16_t) mFilter.x, (int16_t) mFilter.y, (int16_t) mFilter.z);
        double h = (double) wdSqrt((uint32_t)((double) mAcceleration[i].x * (double) mAcceleration[i].x
          + (double) mAcceleration[i].y * (double) mAcceleration[i].y
          + (double) mAcceleration[i].z * (double) mAcceleration[i].z - v * v));
        feature.meanV += v;
        feature.meanH += h;
        feature.deviationV += v * v;
        feature.deviationH += h * h;
        
        // Reuse it for later step counter
        //mAcceleration[i].x = (uint16_t)v;
        mAcceleration[i].y = (uint16_t)h;        
      }
      feature.meanV /= SAMPLE_SIZE;
      feature.meanH /= SAMPLE_SIZE;
      
      feature.deviationV = feature.deviationV / SAMPLE_SIZE - feature.meanV * feature.meanV;
      feature.deviationH = feature.deviationH / SAMPLE_SIZE - feature.meanH * feature.meanH;
      
      // Classification
      mNewSession.type = classify(feature);
      mNewSession.startTime = (uint32_t) (mAcceleration[0].timestamp/1000);
      mNewSession.endTime = (uint32_t) (mAcceleration[mCounter.dataSize-1].timestamp/1000);
      mNewSession.steps = 0;
      
      if (mNewSession.type >= TYPE_WALK) { // Walking or Jogging
        bool direction = false;
                
        uint16_t thisPeak = 0;
        uint16_t avgPeak = 0;
        uint8_t count = 1;
        uint16_t thisPeakTime = 0;
        
        //figure out the peaks in dataset to count steps
        for (uint8_t i = 0; i < SAMPLE_SIZE; i++) {
          if (mAcceleration[i].y > (uint16_t)feature.meanH + 10) {
            if(mAcceleration[i].y > thisPeak) {
              thisPeak = mAcceleration[i].y;
              thisPeakTime = (uint16_t)(mAcceleration[i].timestamp - mAcceleration[0].timestamp);
            }
            direction = true;
          } 
          else if (mAcceleration[i].y < (uint16_t)feature.meanH - 10 && direction == true) {
            direction = false;
            avgPeak += thisPeak;
            //reuse for peak values and times
            mAcceleration[count].x = thisPeak;
            mAcceleration[count].timestamp = thisPeakTime;
            count++;
            thisPeak = 0;
          }
        }
        avgPeak /= count-1;
        
        //set buffer sizes around the average accel values(empirical values)
        uint8_t ratio_high = 3;
        uint8_t ratio_low = 4;
        if (mNewSession.type == TYPE_JOG) { // Only filter steps for walking, but NOT for jogging!
          ratio_high = 9;
          ratio_low = 12;
        }
        
        thisPeakTime = 0;          //reuse as last step time
        thisPeak = 0;              //reuse as first peak time
        uint16_t bufferLow = (uint16_t)feature.meanH + (avgPeak-(uint16_t)feature.meanH)/ratio_low;
        uint16_t bufferHigh = (uint16_t)feature.meanH + (avgPeak-(uint16_t)feature.meanH)*ratio_high;
        
        //count steps if peaks fall in buffer zones
        for(uint8_t i=1; i<count; i++) {
          if((uint16_t)mAcceleration[i].timestamp-thisPeakTime < MAX_STEP_PERIOD &&
             /*(uint16_t)mAcceleration[i].timestamp-thisPeakTime > MIN_STEP_PERIOD &&*/
              mAcceleration[i].x>bufferLow && mAcceleration[i].x<bufferHigh) {
            mNewSession.steps++;
            thisPeakTime = (uint16_t)mAcceleration[i].timestamp;
            if(thisPeak == 0) thisPeak = (uint16_t)mAcceleration[i].timestamp;
          }          
        }
        
        //reject session if not enough steps
        if((mNewSession.steps < MIN_STEP_NEW_SESSION && (mCounter.state != STATE_ACTIVE || mNewSession.steps < MIN_STEP_ACTIVE_SESSION)) ||
           (thisPeakTime-thisPeak)/(mNewSession.steps-1) < MIN_STEP_PERIOD) {
          mNewSession.type = TYPE_IDLE;
          //mNewSession.steps = 0;
        }
      }
      mCounter.dataSize = 0;
      
      if(mCounter.phoneIntegration) {
        mCounter.movement += (uint16_t)(wdSqrt((uint32_t) (feature.meanV*feature.meanV + feature.meanH*feature.meanH)));
        mCounter.movementCount++;
        //send acceleration values to app
        if(mCounter.movementCount > 7) {
          DataTime dataTime[] = {(DataTime){.data = mCounter.movement/8, .timestamp = mNewSession.endTime, .sensor = SENSOR_TYPE_MOVEMENT}};
          data_logging_log(mSensorDataLog, &dataTime, 1);
          mCounter.movement = 0;
          mCounter.movementCount = 0;
        }
        
        //send steps to app
        if(mNewSession.steps > 0) {
          DataTime dataTime[] = {(DataTime){.data = mNewSession.steps, .timestamp = mNewSession.endTime, .sensor = SENSOR_TYPE_STEPS}};
          data_logging_log(mSensorDataLog, &dataTime, 1);
        }
      }
      
      resetSession();
    }
}

/*#if PBL_API_EXISTS(health_service_peek_current_value)
static void health_handler(HealthEventType event, void *context) {
  if(event == HealthEventHeartRateUpdate) {
    uint16_t val = (uint16_t) health_service_peek_current_value(HealthMetricHeartRateBPM);
    //data_logging_log(mSensorDataLog, &dataTime, 1);
    if(val > 0 && mCounter.lastHr != val) {
      uint32_t now = time(NULL);
      DataTime dataTime[] = {(DataTime){.data = val, .timestamp = now, .sensor = SENSOR_TYPE_HR}};
      if(mCurrentSession.type == TYPE_IDLE && (mPreviousSession.endTime-now > 10*60)) {
        dataTime[0].sensor = SENSOR_TYPE_HR_RESTING;
      }
      data_logging_log(mSensorDataLog, &dataTime, 1);
      mCounter.lastHr = val;
    }
  }
}
#endif*/


static void init(void) {
  mPreviousSession.endTime = time(NULL);
  
  readSettings();
  
  // Initiate low pass filter for later use
  initLowPassFilter(&mFilter);
  // Setup accelerometer API
  accel_data_service_subscribe(BATCH_SIZE, &processAccelerometerData);
  accel_service_set_sampling_rate(ACCEL_SAMPLING_10HZ);
  //get hourly updates
  tick_timer_service_subscribe(HOUR_UNIT, hourTickHandler);
  
  //Initialize data logging
  sessionsDataLog = data_logging_create(
    2213,                            // tag
    DATA_LOGGING_BYTE_ARRAY,        // DataLoggingItemType
    sizeof(Session),                // length
    true                            // resume
  );
  
  mSensorDataLog = data_logging_create(
    1112,                            // tag
    DATA_LOGGING_BYTE_ARRAY,        // DataLoggingItemType
    sizeof(DataTime),                // length
    true                            // resume
  );

  /*mMovementDataLog = data_logging_create(
    1112,                            // tag
    DATA_LOGGING_BYTE_ARRAY,        // DataLoggingItemType
    sizeof(DataTime),                // length
    true                            // resume
  );
  
  mStepsDataLog = data_logging_create(
    1737,                            // tag
    DATA_LOGGING_BYTE_ARRAY,        // DataLoggingItemType
    sizeof(DataTime),                // length
    true                            // resume
  );*/
   
  /*#if PBL_API_EXISTS(health_service_peek_current_value)
  health_service_events_subscribe(health_handler, NULL);
  #endif*/
}

static void deinit(void) {
  /*accel_data_service_unsubscribe();
  tick_timer_service_unsubscribe();
  #if PBL_API_EXISTS(health_service_peek_current_value)
  health_service_events_unsubscribe();
  #endif
  data_logging_finish(sessionsDataLog);
  data_logging_finish(mSensorDataLog);*/
  //data_logging_finish(mStepsDataLog);
}

int main(void) {
  init();
  worker_event_loop();
  deinit();
}