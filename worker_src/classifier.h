#pragma once
#include <pebble_worker.h>
  
#ifndef CLASSIFIER_H
#define CLASSIFIER_H
  
  #define TYPE_SLEEP     74
  #define TYPE_IDLE      73
  #define TYPE_WALK      76
  #define TYPE_JOG       77
  
typedef struct {
  double meanH;
  double meanV;
  double deviationH;
  double deviationV;
} Feature;

uint8_t classify(Feature feature);

#endif