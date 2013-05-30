/*
 *  debug.c
 *
 *  Created by Patrice Dietsch on 14/08/12.
 *  Copyright 2012 -. All rights reserved.
 *
 */

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


int debug_status()
{
   return debug_msg;
}