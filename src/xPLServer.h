//
//  xPLServer.h
//
//  Created by Patrice DIETSCH on 17/10/12.
//
//

#ifndef __xPLServer_h
#define __xPLServer_h

#include <inttypes.h>

#include "queue.h"
#include "xPL.h"

extern char *xpl_vendorID;
extern char *xpl_deviceID;
extern char *xpl_instanceID;


typedef struct xplRespQueue_elem_s
{
   int id;
   xPL_MessagePtr msg;
   uint32_t tsp;
} xplRespQueue_elem_t;

typedef int16_t (*xpl_f)(xPL_ServicePtr,xPL_MessagePtr theMessage, xPL_ObjectPtr userValue);

pthread_t *xPLServer(queue_t *interfaces);
xPL_ServicePtr get_xPL_ServicePtr();

char *set_xPL_vendorID(char *value);
char *set_xPL_deviceID(char *value);
char *set_xPL_instanceID(char *value);

char *get_xPL_instanceID();
char *get_xPL_deviceID();
char *get_xPL_vendorID();

uint16_t sendXplMessage(xPL_MessagePtr xPLMsg);
xPL_MessagePtr readResponseFromQueue(int id);

int is_started();

#endif
