#pragma once
#include <pebble_worker.h>
#ifndef RECOGNIZER_H
#define RECOGNIZER_H  
#include "lowpassfilter.h"
#include "classifier.h"
    
  #define SAMPLE_INTERVAL_S	8
  #define BATCH_SIZE	10
  #define SAMPLE_SIZE	(SAMPLE_INTERVAL_S * BATCH_SIZE)
    
  #define SESSION_SLEEP_MIN_DURATION_LONG   40*60
  #define SESSION_SLEEP_MIN_DURATION_SHORT  3*60
  #define SESSION_WALK_MIN_DURATION      90//2*60
  #define SESSION_IDLE_MIN_DURATION      60
  #define SESSION_WALK_MIN_STEPS         40
  #define SESSION_TIMEGAP_LONG           10*60
  #define SESSION_TIMEGAP_SHORT          5
    
  /*#define WORKER_MSG_STEPS    81
  #define WORKER_MSG_SLEEP    82
  #define WORKER_MSG_WALK     83
  #define WORKER_MSG_JOG      84*/

  #define SENSOR_TYPE_MOVEMENT     31
  #define SENSOR_TYPE_STEPS        32
  #define SENSOR_TYPE_HR           33
  #define SENSOR_TYPE_HR_RESTING   34
  

  #define WORKER_MSG_READ_SETTINGS 115
  #define WORKER_MSG_UPDATE    116
  #define WORKER_MSG_TYPE      117
  #define WORKER_MSG_PAUSE     118
  #define WORKER_MSG_STATUS    119
  //#define WORKER_MSG_BLUETOOTH 120
  #define WORKER_MSG_IDLE_ALERT 121
  #define WORKER_MSG_RESET      122
  #define WORKER_MSG_SLEEP_DETAIL 123
  //#define WORKER_MSG_IS_PAUSED  123
  //#define WORKER_MSG_UNPAUSE    124
  //#define WORKER_MSG_RESET_OTHERS 125
  //#define WORKER_MSG_RESET_SLEEP 126
  
  #define PERSIST_SLEEP        231
  #define PERSIST_IDLE         232
  #define PERSIST_WALK         233
  #define PERSIST_JOG          234
  //#define PERSIST_TIMESTAMP    235
  #define PERSIST_STEPS        236
  
  #define PERSIST_AVG_SLEEP        131
  #define PERSIST_AVG_WALK         133
  #define PERSIST_AVG_JOG          134
  #define PERSIST_AVG_STEPS        136
  
  //#define PERSIST_USE_BLUETOOTH_ALERT  141
  #define PERSIST_IDLE_TIME            142
  #define PERSIST_RESET_TIMESTAMP      143
  #define PERSIST_PHONE_INTEGRATION    144

  #define PERSIST_WORKER_WAKEUP_MESSAGE    160
  
  #define STATE_ACTIVE 2
  #define STATE_IDLE   1
  #define STATE_SLEEP  0

  //#define WALK_FILTER_RATIO    20  //17
  //#define RUN_FILTER_RATIO     50

  #define MIN_STEP_PERIOD    250
  #define MAX_STEP_PERIOD    2000
  #define MIN_STEP_NEW_SESSION  5
  #define MIN_STEP_ACTIVE_SESSION 3

typedef struct {
  uint32_t sleepTime;
  //uint32_t sitTime;
  uint32_t walkTime;
  uint32_t jogTime;
  //uint32_t timestamp;
  uint32_t steps;
  uint16_t dataSize;
  uint16_t movement;
  //uint16_t avgStepBuffer;
  uint16_t movementCount;
  uint8_t state;
  uint8_t idleTime;
  uint8_t lastHr;
  bool phoneIntegration;
} Counter;

typedef struct {
  uint32_t startTime;
  uint32_t endTime;
  uint32_t steps;
  uint32_t type;
} Session;

typedef struct {
  time_t timestamp;
  uint16_t data;
  uint8_t sensor;
} DataTime;

#endif