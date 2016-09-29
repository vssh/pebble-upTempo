#pragma once

#include "timer.h"
#include "utils.h"
#include "active.h"
#include "day.h"
  
  void menu_init();

  #define TIMESPAN_INDEFINITE   0
  #define TIMESPAN_30MIN        1
  #define TIMESPAN_1HR          2
  #define TIMESPAN_1_5HR        3
  #define TIMESPAN_2HR          4
  #define TIMESPAN_3HR          5
  #define TIMESPAN_5HR          6
  #define TIMESPAN_8HR          7
    
  #define MIN_INDEFINITE   0*30
  #define MIN_30MIN        1*30
  #define MIN_1HR          2*30
  #define MIN_1_5HR        3*30
  #define MIN_2HR          4*30
  #define MIN_3HR          6*30
  #define MIN_5HR          10*30
  #define MIN_8HR          16*30