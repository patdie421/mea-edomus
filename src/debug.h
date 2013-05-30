/*
 *  debug.h
 *
 *  Created by Patrice Dietsch on 14/08/12.
 *  Copyright 2012 -. All rights reserved.
 *
 */

#ifndef __debug_h
#define __debug_h

#define DEBUG_SECTION if(debug_msg)
#define VERBOSE(v) if(v <= verbose_level)

extern int debug_msg;
extern int verbose_level;

void debug_on();
void debug_off();
int debug_status();
void set_verbose_level(int level);

#endif