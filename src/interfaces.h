//
//  interfaces.h
//
//  Created by Patrice DIETSCH on 13/09/12.
//
//

#ifndef __interfaces_h
#define __interfaces_h

#include <termios.h>
#include <inttypes.h>

#include "types.h"

typedef struct interfaces_queue_elem_s
{
   int type;
   void *context;
} interfaces_queue_elem_t;

int16_t get_dev_and_speed(char *device, char *dev, int16_t dev_l, speed_t *speed);

#endif
