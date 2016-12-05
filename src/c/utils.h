#pragma once
#include <pebble.h>
#include "persist.h"
  
#define APP_VERSION     "upTempo v0.25"
  
#ifdef PBL_COLOR
  #define COLOR_BACKGROUND GColorBlack
  #define COLOR_TEXT GColorWhite
  #define COLOR_TEXT_BACKGROUND GColorClear
  #define BMP_COMPOSITING_MODE GCompOpSet
#else
  #define COLOR_BACKGROUND GColorBlack
  #define COLOR_TEXT GColorWhite
  #define COLOR_TEXT_BACKGROUND GColorClear
  #define BMP_COMPOSITING_MODE GCompOpAssign
#endif
  
  #define WORKER_MSG_READ_SETTINGS 115
  #define WORKER_MSG_UPDATE   116
  #define WORKER_MSG_TYPE     117
  #define WORKER_MSG_PAUSE     118
  #define WORKER_MSG_STATUS    119
  //#define WORKER_MSG_BLUETOOTH  120
  #define WORKER_MSG_IDLE_ALERT 121
  #define WORKER_MSG_RESET      122
  #define WORKER_MSG_SLEEP_DETAIL 123
  //#define WORKER_MSG_IS_PAUSED  123
  //#define WORKER_MSG_UNPAUSE    124
  //#define WORKER_MSG_RESET_OTHERS 125
  //#define WORKER_MSG_RESET_SLEEP 126

  #define TYPE_UNKNOWN 70

  #define TYPE_SLEEP   74
  #define TYPE_IDLE    73
  #define TYPE_WALK    76
  #define TYPE_JOG     77

  #define TYPE_CYCLE   78
  #define TYPE_DRIVE   79

  #define TYPE_OTHER  50
  #define TYPE_TRAIN   51
  #define TYPE_MARTIAL  52
  #define TYPE_SPORT     53
  #define TYPE_WATER      54
  #define TYPE_WINTER      55

  //#define SNOOZE       173
  //#define USE_BT     174
  //#define IDLE_TIME   175


  #define APPMSG_CHANGE_COMM_ACTIVATE 211
  #define APPMSG_DURATION      212
  #define APPMSG_DISTANCE      213
  #define APPMSG_SPEED         214
  #define APPMSG_PAUSE         215
  #define APPMSG_STOP          216
  #define APPMSG_START         217
  #define APPMSG_STATUS        218
  #define APPMSG_UNIT          219
  #define APPMSG_HEART         220
  #define APPMSG_SEND_COMM_ACTIVATE_STATE 221
  #define APPMSG_RECEIVE_COMM_ACTIVATE_STATE 222
  #define APPMSG_CANCEL        223
  #define APPMSG_PACE          224

  #define APPMSG_DAY_VAL       230
  #define APPMSG_HISTORY_VAL   231

  #define APPMSG_DAY_REC_VAL          240
  #define APPMSG_DAY_REC_AVG          241
  #define APPMSG_DAY_REC_PAGE         242
  #define APPMSG_HISTORY_REC_VALS_0   243
  #define APPMSG_HISTORY_REC_VALS_1   244
  #define APPMSG_HISTORY_REC_VALS_2   245
  #define APPMSG_HISTORY_REC_VALS_3   246
  #define APPMSG_HISTORY_REC_VALS_4   247
  #define APPMSG_HISTORY_REC_VALS_5   248
  #define APPMSG_HISTORY_REC_VALS_6   249
  #define APPMSG_HISTORY_REC_PAGE     250

  #define PHONE_SEND_BUTTON_UP    2
  #define PHONE_SEND_BUTTON_DOWN  1
  #define PHONE_SEND_NO_BUTTON    0

  #define APPMSG_NEXT_ALARM         341

extern GFont forcedSquare;
extern bool phoneDataSharing;

typedef struct {
  uint32_t startTime;
  uint32_t endTime;
  uint32_t steps;
  uint32_t type;
} Session;
  
TextLayer* macro_text_layer_create(GRect frame, Layer *parent, GColor tcolor, GColor bcolor, GFont font, GTextAlignment text_alignment);
BitmapLayer* macro_bitmap_layer_create(GBitmap *bitmap, GRect frame, Layer *parent, bool visible);
void appmsg_send_val(int key, int value);