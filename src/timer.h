//
//  timer.h
//  mea-edomus
//
//  Created by Patrice Dietsch on 08/07/13.
//
//

#ifndef __timer_h
#define __timer_h

#include <time.h>
#include <inttypes.h>

typedef struct mea_timer_s
{
   uint16_t stat; // (0 = inactive, 1=active)
   time_t start_time;
   uint32_t delay; // timer delay
   uint16_t autorestart; // (0 = no, 1=yes)
} mea_timer_t;


uint16_t init_timer(mea_timer_t *aTimer, uint32_t aDelay, uint16_t restartStatus);
void start_timer(mea_timer_t *aTimer);
void stop_timer(mea_timer_t *aTimer);
uint16_t test_timer(mea_timer_t *aTimer);

#endif
