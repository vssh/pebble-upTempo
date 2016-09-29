#include <pebble_worker.h>
#include "classifier.h"
  
uint8_t classify(Feature feature) {
  uint8_t type = 0;
  double probability = 0.0;
  
  double class0 = 6.95 +
    feature.meanV * 0.87 +
    feature.meanH * -0.26 +
    feature.deviationV * -0.03 +
    feature.deviationH * -0.11;
  probability = class0;
  type = TYPE_SLEEP;
  
  double class1 = 2.7 +
    feature.meanV * 0.05 +
    feature.meanH * -0.05;
  
  if (class1 > probability) {
    probability = class1;
    type = TYPE_IDLE;    
  }
  
  double class2 = -4.15 +
    feature.meanV * -0.16 +
    feature.meanH * 0.1;
  
  if (class2 > probability) {
    probability = class2;
    type = TYPE_WALK;
  }
  double class3 = -65.76 +
    feature.meanH * 0.31;
  if (class3 > probability) {
    probability = class3;
    type = TYPE_JOG;
  }
  return type;
}
