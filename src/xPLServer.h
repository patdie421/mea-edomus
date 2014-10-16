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


struct xplServerData_s
{
   char **params_list;
   sqlite3 *sqlite3_param_db;
};


typedef int16_t (*xpl_f)(xPL_ServicePtr, xPL_MessagePtr theMessage, xPL_ObjectPtr userValue);


pthread_t *xPLServer(queue_t *interfaces);


xPL_ServicePtr mea_getXPLServicePtr();

char          *mea_setXPLVendorID(char *value);
char          *mea_setXPLDeviceID(char *value);
char          *mea_setXPLInstanceID(char *value);

char          *mea_getXPLInstanceID();
char          *mea_getXPLDeviceID();
char          *mea_getXPLVendorID();

uint16_t       mea_sendXPLMessage(xPL_MessagePtr xPLMsg);
xPL_MessagePtr mea_readXPLResponse(int id);
uint32_t       mea_getXplRequestId();

xPL_MessagePtr mea_createReceivedMessage(xPL_MessageType messageType);


//pthread_t    *start_xPLServer(int my_id, char **params_list, queue_t *interfaces, sqlite3 *sqlite3_param_db);
int           start_xPLServer(int my_id, void *data);
int           stop_xPLServer(int my_id, void *data);

#endif
