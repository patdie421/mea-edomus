/*
 *  debug.h
 *
 *  Created by Patrice Dietsch on 14/08/12.
 *  Copyright 2012 -. All rights reserved.
 *
 */

#ifndef __debug_h
#define __debug_h

#include <inttypes.h>

#include "pythonPluginServer.h"

#define DEBUG_SECTION if(debug_msg)
#define VERBOSE(v) if(v <= verbose_level)

#define DEBUG_PyEval_AcquireLock(id, last_time) { \
   printf("CHRONO : Demande Lock par %s a %u ms\n",(id),start_chrono((last_time))); \
   PyEval_AcquireLock(); \
   printf("CHRONO : Lock obtenu par %s apres %u ms\n",(id),take_chrono((last_time))); \
}

#define DEBUG_PyEval_ReleaseLock(id, last_time) { \
   PyEval_ReleaseLock(); \
   printf("CHRONO : Liberation Lock par %s apres %u\n",(id), take_chrono((last_time))); \
}

/*
#define DEBUG_PyEval_AcquireLock(id, last_time) { \
printf("CHRONO : Demande Lock par %s a %u ms\n",(id),start_chrono((last_time))); \
GIL_LOCK; \
printf("CHRONO : Lock obtenu par %s apres %u ms\n",(id),take_chrono((last_time))); \
}

#define DEBUG_PyEval_ReleaseLock(id, last_time) { \
GIL_UNLOCK; \
printf("CHRONO : Liberation Lock par %s apres %u\n",(id), take_chrono((last_time))); \
}
*/

extern int debug_msg;
extern int verbose_level;

void debug_on();
void debug_off();
int debug_status();
void set_verbose_level(int level);

uint32_t start_chrono(uint32_t *_last_time);
uint32_t take_chrono(uint32_t *_last_time);

#endif