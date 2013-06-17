/*
 *  debug.c
 *
 *  Created by Patrice Dietsch on 14/08/12.
 *  Copyright 2012 -. All rights reserved.
 *
 */
#include <time.h>
#include <sys/time.h>
#include <inttypes.h>

#include "debug.h"

int debug_msg=1;
int verbose_level=0;

void set_verbose_level(int level)
{
   verbose_level=level;
}

void debug_on()
{
   debug_msg=1;
}


void debug_off()
{
   debug_msg=0;
}


int16_t debug_status()
{
   return debug_msg;
}

uint32_t start_chrono(uint32_t *_last_time)
{
   struct timeval tv;
   
   gettimeofday(&tv, NULL);
   *_last_time=(uint32_t)tv.tv_sec * 1000 + tv.tv_usec / 1000;
   
   return *_last_time;
}


uint32_t take_chrono(uint32_t *_last_time)
{
   struct timeval tv;
   uint32_t ret;
   
   gettimeofday(&tv, NULL);
   
   uint32_t now=(uint32_t)tv.tv_sec * 1000 + tv.tv_usec / 1000;
   
   if(now>*_last_time)
	   ret=now - *_last_time;
   else
	   ret=0xFFFFFFFFF - *_last_time + now + 1;
   *_last_time = now;

   return ret;
}


