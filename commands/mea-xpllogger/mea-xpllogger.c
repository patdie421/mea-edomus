//
//  mea-xpllogger.c
//  mea-edomus
//
//  Created by Patrice Dietsch on 13/10/2014.
//
//

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <inttypes.h>

#include "xPL.h"

#define XPL_VERSION "0.1a2"

           
int16_t displayXPLMsg(xPL_MessagePtr theMessage)
{
   char xpl_source[48];
   char xpl_schema[48];
   char xpl_destination[48];

   snprintf(xpl_source,sizeof(xpl_source),"%s-%s.%s", xPL_getSourceVendor(theMessage), xPL_getSourceDeviceID(theMessage), xPL_getSourceInstanceID(theMessage));
   if(xPL_isBroadcastMessage(theMessage))
   {
      strcpy(xpl_destination, "*");
   }
   else
   {
      snprintf(xpl_destination,sizeof(xpl_destination),"%s-%s.%s", xPL_getTargetVendor(theMessage), xPL_getTargetDeviceID(theMessage), xPL_getTargetInstanceID(theMessage));
   }

   snprintf(xpl_schema,sizeof(xpl_schema),"%s.%s", xPL_getSchemaClass(theMessage), xPL_getSchemaType(theMessage));

   fprintf(stdout, "source = %s, destination = %s, schema = %s, body = [",xpl_source, xpl_destination
   , xpl_schema);

   xPL_NameValueListPtr xpl_body = xPL_getMessageBody(theMessage);
   int n = xPL_getNamedValueCount(xpl_body);
   for (int16_t i = 0; i < n; i++)
   {
      xPL_NameValuePairPtr keyValuePtr = xPL_getNamedValuePairAt(xpl_body, i);

      if(i)
         fprintf(stdout,", ");
      fprintf(stdout,"%s = %s",keyValuePtr->itemName, keyValuePtr->itemValue);
   }
   fprintf(stdout,"]\n");
   return 0;
}


void xplmsglogger(xPL_MessagePtr theMessage, xPL_ObjectPtr userValue)
{
   displayXPLMsg(theMessage);
}


static void _logger_stop(int signal_number)
{
    exit(0);
}


void logger()
{
   signal(SIGINT,  _logger_stop);
   signal(SIGQUIT, _logger_stop);
   signal(SIGTERM, _logger_stop);

#ifdef __APPLE__
   xPL_setBroadcastInterface("en0"); // comprendre pourquoi ca ne marche plus sans sur mac os x ...
#endif

   if ( !xPL_initialize(xPL_getParsedConnectionType()) )
      exit(1);
   
   xPL_ServicePtr xPLService = NULL;
   
   xPLService = xPL_createService("mea", "edomus", "debuglogger");
   xPL_setServiceVersion(xPLService, XPL_VERSION);
   xPL_setRespondingToBroadcasts(xPLService, TRUE);
   xPL_addMessageListener(xplmsglogger, NULL);

   xPL_setServiceEnabled(xPLService, TRUE);

   do
   {
      xPL_processMessages(500);
   }
   while (1);
}


int main(int argc, char *argv[])
{
   logger();
}
