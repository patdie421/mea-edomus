//
//  xPLServer.h
//
//  Created by Patrice DIETSCH on 17/10/12.
//
//

#ifndef __xPLServer_h
#define __xPLServer_h

#include "queue.h"
#include "xPL.h"

typedef int (*xpl_f)(xPL_ServicePtr,xPL_MessagePtr theMessage, xPL_ObjectPtr userValue);

int xPLServer(queue_t *interfaces);
xPL_ServicePtr get_xPL_ServicePtr();

#endif
