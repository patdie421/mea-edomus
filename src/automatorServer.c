//
//  automatorServer.c
//  mea-edomus
//
//  Created by Patrice Dietsch on 24/11/2013.
//
//

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>

#include "xPL.h"

#include "debug.h"
#include "error.h"
#include "macros.h"
#include "memory.h"
//#include "string_utils.h"

#define XPL_VERSION "0.1a2"

static void _automator_stop(int signal_number)
{
   exit(0);
}

//void printXPLMessage(xPL_ServicePtr theService, xPL_MessagePtr theMessage, xPL_ObjectPtr userValue)
void printXPLMessage(xPL_MessagePtr theMessage, xPL_ObjectPtr userValue)
{
//  printTimestamp();
  fprintf(stderr, "[xPL_MSG] TYPE=");
  switch(xPL_getMessageType(theMessage)) {
  case xPL_MESSAGE_COMMAND:
    fprintf(stderr, "xpl-cmnd");
    break;
  case xPL_MESSAGE_STATUS:
    fprintf(stderr, "xpl-stat");
    break;
  case xPL_MESSAGE_TRIGGER:
    fprintf(stderr, "xpl-trig");
    break;
  default:
    fprintf(stderr, "!UNKNOWN!");
    break;
  }

  /* Print hop count, if interesting */
  if (xPL_getHopCount(theMessage) != 1) fprintf(stderr, ", HOPS=%d", xPL_getHopCount(theMessage));
  
  /* Source Info */
  fprintf(stderr, ", SOURCE=%s-%s.%s, TARGET=",
	  xPL_getSourceVendor(theMessage),
	  xPL_getSourceDeviceID(theMessage),
	  xPL_getSourceInstanceID(theMessage));

  /* Handle various target types */
  if (xPL_isBroadcastMessage(theMessage)) {
    fprintf(stderr, "*");
  } else {
    if (xPL_isGroupMessage(theMessage)) {
      fprintf(stderr, "XPL-GROUP.%s", xPL_getTargetGroup(theMessage));
    } else {
         fprintf(stderr, "%s-%s.%s",
	      xPL_getTargetVendor(theMessage),
	      xPL_getTargetDeviceID(theMessage),
         xPL_getTargetInstanceID(theMessage));
      }
   }

   /* Echo Schema Info */
   fprintf(stderr, ", CLASS=%s, TYPE=%s", xPL_getSchemaClass(theMessage), xPL_getSchemaType(theMessage));
   fprintf(stderr,"\n          ");
   
   xPL_NameValueListPtr body = xPL_getMessageBody(theMessage);
   int n = xPL_getNamedValueCount(body);
   for (int16_t i = 0; i < n; i++)
   {
      xPL_NameValuePairPtr keyValuePtr = xPL_getNamedValuePairAt(body, i);
      fprintf(stderr,"%s=%s ", keyValuePtr->itemName, keyValuePtr->itemValue);
   }
   fprintf(stderr,"\n");
}


// 1 thread d'acquisition : trapper en continue les messages xpl
   // Les règles d'acquisition : quelles données utilisées ?
// 1 thread de mise à jour de la table
   // 1 tableau "mémoire de travail" mis à jour par le thread
// une file entre les deux thread

// 1 thread de gestion des timers

// 1 tableau des étapes


void automator()
{
   signal(SIGINT,  _automator_stop);
   signal(SIGQUIT, _automator_stop);
   signal(SIGTERM, _automator_stop);

/*
   while(1)
   {
      // attendre déclenchement : traitement uniquement si changement de données
      
      // acquisition des entrées
         // copie de données collectées dans la mémoire de traitement (les entrées sont figées)
      
      // traitements
      
      // affectation des sorties
      
      sleep(5);
   }
*/
   if ( !xPL_initialize(xPL_getParsedConnectionType()) )
   {
      exit(1);
   }
   
   xPL_ServicePtr xPLService = NULL;
   
   xPLService = xPL_createService("mea", "edomus", "cheznousdeva");
   xPL_setServiceVersion(xPLService, XPL_VERSION);
   xPL_setRespondingToBroadcasts(xPLService, TRUE);
   //xPL_addServiceListener(xPLService, printXPLMessage, xPL_MESSAGE_COMMAND, "control", "basic", NULL) ;
   xPL_addMessageListener(printXPLMessage, NULL);

   xPL_setServiceEnabled(xPLService, TRUE);

   do
   {
      VERBOSE(9) {
         static char compteur=0;
         if(compteur>59)
         {
            compteur=0;
            fprintf(stderr,"%s  (%s) : xPLServer thread actif\n",INFO_STR,__func__);
         }
         else
            compteur++;
      }
      
      xPL_processMessages(500);
      pthread_testcancel();
   }
   while (1);

}


pid_t start_automatorServer(void)
{
   pid_t automator_pid = -1;

   automator_pid = fork();
   
   if (automator_pid == 0) // child
   {
      // Code only executed by child process
      automator();
      exit(1);
   }
   else if (automator_pid < 0) // failed to fork
   {
      return -1;
   }

   // Code only executed by parent process
   return automator_pid;
}
