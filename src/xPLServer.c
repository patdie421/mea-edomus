//
//  xPLServer.c
//
//  Created by Patrice DIETSCH on 17/10/12.
//
//
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>

#include "xPL.h"
#include "debug.h"
#include "queue.h"
#include "memory.h"

#include "interfaces.h"
#include "interface_type_001.h"
#include "interface_type_002.h"

#define XPL_VERSION "20121016"


xPL_ServicePtr xPLService = NULL;


char *xpl_vendorID=NULL;
char *xpl_deviceID=NULL;
char *xpl_instanceID=NULL;


char *set_xPL_vendorID(char *value)
{
   return string_free_malloc_and_copy(&xpl_vendorID, value, 1);
}


char *set_xPL_deviceID(char *value)
{
   return string_free_malloc_and_copy(&xpl_deviceID, value, 1);
}


char *set_xPL_instanceID(char *value)
{
   return string_free_malloc_and_copy(&xpl_instanceID, value, 1);
}


xPL_ServicePtr get_xPL_ServicePtr()
{
   return xPLService;
}


void cmndMsgHandler(xPL_ServicePtr theService, xPL_MessagePtr theMessage, xPL_ObjectPtr userValue)
{
   int ret;

   queue_t *interfaces;
   interfaces_queue_elem_t *iq;

   interfaces=(queue_t *)userValue;
   
   VERBOSE(9) fprintf(stderr,"INFO  (cmndMsgHandler) : Reception message xPL\n");
   
   if(first_queue(interfaces)==-1)
      return;
   while(1)
   {
      current_queue(interfaces, (void **)&iq);
      switch (iq->type)
      {
         case INTERFACE_TYPE_001:
         {
            interface_type_001_t *i001 = (interface_type_001_t *)(iq->context);
            i001->xPL_callback(theService, theMessage, (xPL_ObjectPtr)i001);
            break;
         }

         case INTERFACE_TYPE_002:
         {
            interface_type_002_t *i002 = (interface_type_002_t *)(iq->context);
            i002->xPL_callback(theService, theMessage, (xPL_ObjectPtr)i002);
            break;
         }
         default:
            break;
      }
      ret=next_queue(interfaces);
      if(ret<0)
         break;
   }
}


void *_xPL_server_thread(void *data)
{
/*
   char hostname[21];
   
   gethostname(hostname, 20);
   for(int i=0;hostname[i];i++)
      if(hostname[i]=='.')
      {
         hostname[i]=0;
         break;
      }
*/   
   if ( !xPL_initialize(xPL_getParsedConnectionType()) ) return 0 ;
   
   // myService = xPL_createService(myVendor, myDevice, myInstance);
   // xPLService = xPL_createService("mnntamoi", hostname, "default");
   // xPLService = xPL_createService("mea", "edomus", "cheznousdev")
   xPLService = xPL_createService(xpl_vendorID, xpl_deviceID, xpl_instanceID);
   xPL_setServiceVersion(xPLService, XPL_VERSION);
   
   xPL_setRespondingToBroadcasts(xPLService, TRUE);
   
   // xPL_setHeartbeatInterval(xPLService, 5000); // en milliseconde ?
   
   // xPL_MESSAGE_ANY, xPL_MESSAGE_COMMAND, xPL_MESSAGE_STATUS, xPL_MESSAGE_TRIGGER
   xPL_addServiceListener(xPLService, cmndMsgHandler, xPL_MESSAGE_COMMAND, "control", "basic", (xPL_ObjectPtr)data) ;
   xPL_addServiceListener(xPLService, cmndMsgHandler, xPL_MESSAGE_COMMAND, "sensor", "request", (xPL_ObjectPtr)data) ;
   
   xPL_setServiceEnabled(xPLService, TRUE);

   do
   {
      VERBOSE(9) {
         static char compteur=0;
         if(compteur>59)
         {
            compteur=0;
            fprintf(stderr,"INFO  (_xPL_server_thread) : xPLServer thread actif\n");
         }
         else
            compteur++;
      }
      
      xPL_processMessages(500);
      pthread_testcancel();
   }
   while (1);
}


// pthread_t *xPL_thread=NULL;

pthread_t *xPLServer(queue_t *interfaces)
{
   pthread_t *xPL_thread=NULL;
   
   if(!xpl_deviceID || !xpl_instanceID || !xpl_vendorID)
   {
      return NULL;
   }

   xPL_thread=(pthread_t *)malloc(sizeof(pthread_t));
   if(!xPL_thread)
   {
      VERBOSE(1) {
         fprintf (stderr, "ERROR (xPLServer) : malloc (%s/%d) - ",__FILE__,__LINE__);
         perror("");
      }
      return NULL;
   }

   if(pthread_create (xPL_thread, NULL, _xPL_server_thread, (void *)interfaces))
   {
      VERBOSE(1) fprintf(stderr, "ERROR (xPLServer) : pthread_create - can't start thread\n");
      return NULL;
   }
   
   return xPL_thread;
}


