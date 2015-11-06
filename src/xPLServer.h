//
//  xPLServer.h
//
//  Created by Patrice DIETSCH on 17/10/12.
//
//

#ifndef __xPLServer_h
#define __xPLServer_h

#include <inttypes.h>
#include <sqlite3.h>

#include "xPL.h"

extern char *xpl_vendorID;
extern char *xpl_deviceID;
extern char *xpl_instanceID;

extern char *xpl_server_name_str;
extern char *xpl_server_xplin_str;
extern char *xpl_server_xplout_str;

   
typedef struct xplRespQueue_elem_s
{
   int id;
   xPL_MessagePtr msg;
   uint32_t tsp;
} xplRespQueue_elem_t;


struct xplServer_start_stop_params_s
{
   char **params_list;
   sqlite3 *sqlite3_param_db;
};


typedef int16_t (*xpl_f)(xPL_ServicePtr, xPL_MessagePtr theMessage, xPL_ObjectPtr userValue);


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

extern xPL_MessagePtr xPL_AllocMessage();
extern xPL_NameValueListPtr xPL_AllocNVList();

xPL_MessagePtr mea_createReceivedMessage(xPL_MessageType messageType);
xPL_MessagePtr mea_createSendableMessage(xPL_MessageType messageType, char *vendorID, char *deviceID, char *instanceID);

int           start_xPLServer(int my_id, void *data, char *errmsg, int l_errmsg);
int           stop_xPLServer(int my_id, void *data, char *errmsg, int l_errmsg);
int           restart_xPLServer(int my_id, void *data, char *errmsg, int l_errmsg);

#endif
