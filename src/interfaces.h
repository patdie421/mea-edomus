//
//  interfaces.h
//
//  Created by Patrice DIETSCH on 13/09/12.
//
//

#ifndef __interfaces_h
#define __interfaces_h

#define INTERFACE_TYPE_001 100
#define INTERFACE_TYPE_002 200

typedef struct interfaces_queue_elem_s
{
   int type;
   void *context;
} interfaces_queue_elem_t;

#endif
