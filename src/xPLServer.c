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

#include "globals.h"

#include "xPL.h"
#include "debug.h"
#include "queue.h"
#include "memory.h"

#include "interfaces.h"
#include "interface_type_001.h"
#include "interface_type_002.h"

#define XPL_VERSION "0.1a2"


xPL_ServicePtr xPLService = NULL;


char *xpl_vendorID=NULL;
char *xpl_deviceID=NULL;
char *xpl_instanceID=NULL;

pthread_cond_t  xplRespQueue_sync_cond;
pthread_mutex_t xplRespQueue_sync_lock;
queue_t         *xplRespQueue;
pthread_mutex_t xplRespSend_lock;
pthread_mutex_t requestId_lock;

uint32_t requestId = 1;

uint32_t getRequestId() // rajouter un verrou ...
{
   uint32_t id=0;

   pthread_cleanup_push( (void *)pthread_mutex_unlock, (void *)&(requestId_lock) );
   pthread_mutex_lock(&requestId_lock);

   id=requestId;

   requestId++;
   if(requestId>20000)
   requestID=1;

   pthread_mutex_unlock(&requestId_lock);
   pthread_cleanup_pop(0);
 
   return id;
}

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


char *get_xPL_instanceID()
{
   return xpl_instanceID;
}


char *get_xPL_deviceID()
{
   return xpl_deviceID;
}


char *get_xPL_vendorID()
{
   return xpl_vendorID;
}


xPL_ServicePtr get_xPL_ServicePtr()
{
   return xPLService;
}


void dispatchXplMessage(xPL_ServicePtr theService, xPL_MessagePtr theMessage, xPL_ObjectPtr userValue)
{
   int ret;

   queue_t *interfaces;
   interfaces_queue_elem_t *iq;

   interfaces=(queue_t *)userValue;
   
   VERBOSE(9) fprintf(stderr,"%s  (%s) : Reception message xPL\n",INFO_STR,__func__);
   
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
            if(i001->xPL_callback)
               i001->xPL_callback(theService, theMessage, (xPL_ObjectPtr)i001);
            break;
         }

         case INTERFACE_TYPE_002:
         {
            interface_type_002_t *i002 = (interface_type_002_t *)(iq->context);
            if(i002->xPL_callback)
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


uint16_t sendXplMessage(xPL_MessagePtr xPLMsg)
{
   char *addr;
   xPL_MessagePtr newXPLMsg = NULL;

   addr = xPL_getSourceDeviceID(xPLMsg);
   if(strcmp(addr,"internal")==0) // source interne => dispatching sans passer par le réseau
   {
      dispatchXplMessage(xPLService, xPLMsg, (xPL_ObjectPtr)get_interfaces());
      return 0;
   }
   
   addr = xPL_getTargetDeviceID(xPLMsg);
   if(addr && strcmp(addr,"internal")==0) // destination interne, retour à mettre dans une file (avec timestamp) ...
   {
      int id;
      
      sscanf(xPL_getTargetInstanceID(xPLMsg), "%d", &id);

      DEBUG_SECTION fprintf(stderr,"Retour de la demande interne à mettre dans la file (id demande = %d)\n",id);

      // duplication du message xPL
      newXPLMsg = xPL_createBroadcastMessage(xPLService, xPL_getMessageType(xPLMsg));

      xPL_setSchema(newXPLMsg, xPL_getSchemaClass(xPLMsg), xPL_getSchemaType(xPLMsg));
      xPL_setTarget(newXPLMsg, xPL_getTargetVendor(xPLMsg), xPL_getTargetDeviceID(xPLMsg), xPL_getTargetInstanceID(xPLMsg));
      xPL_NameValueListPtr body = xPL_getMessageBody(xPLMsg);

      int n = xPL_getNamedValueCount(body);
      for (int i=0; i<n; i++)
      {
         xPL_NameValuePairPtr keyValuePtr = xPL_getNamedValuePairAt(body, i);
/*
         char key[41],value[81];
         key[40]=0;
         value[80]=0;
         xPL_NameValuePairPtr keyValuePtr = xPL_getNamedValuePairAt(body, i);
         strncpy(key, keyValuePtr->itemName,40);
         strncpy(value, keyValuePtr->itemValue,80);         
         xPL_setMessageNamedValue(newXPLMsg, key, value);
*/
         xPL_setMessageNamedValue(newXPLMsg, keyValuePtr->itemName, keyValuePtr->itemValue);
      }

      // ajout de la copie du message dans la file
      xplRespQueue_elem_t *e = malloc(sizeof(xplRespQueue_elem_t));
      e->msg = newXPLMsg;
      e->id = id;
      e->tsp = (uint32_t)time(NULL);
      
      pthread_cleanup_push( (void *)pthread_mutex_unlock, (void *)&(xplRespQueue_sync_lock) );
      pthread_mutex_lock(&xplRespQueue_sync_lock);
      
      in_queue_elem(xplRespQueue, e);
      
      if(xplRespQueue->nb_elem>=1)
         pthread_cond_broadcast(&xplRespQueue_sync_cond);
      
      pthread_mutex_unlock(&xplRespQueue_sync_lock);
      pthread_cleanup_pop(0);

      return 0;
   }
   else
   {
      xPL_sendMessage(xPLMsg);
      return 0;
   }
}


xPL_MessagePtr readResponseFromQueue(int id)
{
   int16_t ret;
   int16_t boucle=5; // 5 tentatives de 1 secondes
   uint16_t notfound=0;
   xPL_MessagePtr msg=NULL;

   // on va attendre le retour dans la file des reponses
   pthread_cleanup_push( (void *)pthread_mutex_unlock, (void *)&(xplRespQueue_sync_lock) );

   pthread_mutex_lock(&(xplRespQueue_sync_lock));
   if(xplRespQueue->nb_elem==0 || notfound==1)
   {
      // rien a lire => on va attendre que quelque chose soit mis dans la file
      struct timeval tv;
      struct timespec ts;
      gettimeofday(&tv, NULL);
      ts.tv_sec = tv.tv_sec + 1; // timeout de une seconde
      ts.tv_nsec = 0;
      ret=pthread_cond_timedwait(&xplRespQueue_sync_cond, &xplRespQueue_sync_lock, &ts);
      if(ret)
      {
         if(ret!=ETIMEDOUT)
            goto readFromQueue_return;
      } 
   }  

   // a ce point il devrait y avoir quelque chose dans la file.
   if(first_queue(xplRespQueue)==0)
   {
      do // parcours de la liste jusqu'a trouver une reponse pour nous
      {
         if(current_queue(xplRespQueue, (void **)&e)==0)
         { 
            if(e->id==id)
            {
               uint32_t tsp=(uint32_t)time(NULL);

               if((tsp - e->tsp)<=10) // la reponse est pour nous et dans les temps (retour dans les 10 secondes)
               {
                  // recuperation des donnees
                  msg=e->msg;
                  // et on fait le menage avant de sortir
                  free(e);
                  xplRespQueue->current->d=NULL; // pour evite le bug
                  remove_current_queue(xplRespQueue);
                  e=NULL;
                  goto readFromQueue_return;
               }
               // theoriquement pour nous mais donnees trop vieilles, on supprime ?
               DEBUG_SECTION fprintf(stderr,"%s (%s) : data too old\n", DEBUG_STR,__func__);
            }
            else
            {
               DEBUG_SECTION fprintf(stderr,"%s (%s) : not for me (%d != %d)\n", DEBUG_STR,__func__, e->id, id);
               e=NULL;
            }
         }
      }
      while(next_queue(xplRespQueue)==0);
   }

readFromQueue_return:
   pthread_mutex_unlock(&(xplRespQueue_sync_lock));
   pthread_cleanup_pop(0);
   return msg;
}


void cmndMsgHandler(xPL_ServicePtr theService, xPL_MessagePtr theMessage, xPL_ObjectPtr userValue)
{
   dispatchXplMessage(theService, theMessage, userValue);
}


void _xPL_server_flush_old_responses_queue()
{
   xplRespQueue_elem_t *e;
   uint32_t tsp=(uint32_t)time(NULL);
   
   pthread_cleanup_push( (void *)pthread_mutex_unlock, (void *)&(xplRespQueue_sync_lock) );
   pthread_mutex_lock(&xplRespQueue_sync_lock);
   
   if(first_queue(xplRespQueue)==0)
   {
      while(1)
      {
         if(current_queue(xplRespQueue, (void **)&e)==0)
         {
            if((tsp - e->tsp) > 5)
            {
               xPL_releaseMessage(e->msg);
               DEBUG_SECTION fprintf(stderr,"Je flush\n");
               free(e);
               remove_current_queue(xplRespQueue); // remove current passe sur le suivant
            }
            else
            next_queue(xplRespQueue);
         }
         else
            break;
      }
   }
   
   pthread_mutex_unlock(&xplRespQueue_sync_lock);
   pthread_cleanup_pop(0);
}


void *_xPL_server_thread(void *data)
{
   xPL_setDebugging(TRUE); // xPL en mode debug

   if ( !xPL_initialize(xPL_getParsedConnectionType()) ) return 0 ;
   
   xPLService = xPL_createService(xpl_vendorID, xpl_deviceID, xpl_instanceID);
   xPL_setServiceVersion(xPLService, XPL_VERSION);
   
   xPL_setRespondingToBroadcasts(xPLService, TRUE);
   
   xPL_setHeartbeatInterval(xPLService, 5000); // en milliseconde    
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
            fprintf(stderr,"%s  (%s) : xPLServer thread actif\n",INFO_STR,__func__);
         }
         else
            compteur++;
      }
      xPL_processMessages(500);
      
      _xPL_server_flush_old_responses_queue();
      
      pthread_testcancel();
   }
   while (1);
}


pthread_t *xPLServer(queue_t *interfaces)
{
   pthread_t *xPL_thread=NULL;

   if(!xpl_deviceID || !xpl_instanceID || !xpl_vendorID)
   {
      return NULL;
   }
   
   // initialisation
   xplpthread_cond_init(&RespSend_lock, NULL);

      // préparation synchro consommateur / producteur
   pthread_cond_init(&xplRespQueue_sync_cond, NULL);
   pthread_mutex_init(&xplRespQueue_sync_lock, NULL);
   pthread_mutex_init(&requestId_lock, NULL);

   xplRespQueue=(queue_t *)malloc(sizeof(queue_t));
   if(!xplRespQueue)
   {
      VERBOSE(1) {
         fprintf (stderr, "%s (%s) : %s - ",ERROR_STR,__func__,MALLOC_ERROR_STR);
         perror("");
      }
      return NULL;
   }
   init_queue(xplRespQueue); // initialisation de la file

   xPL_thread=(pthread_t *)malloc(sizeof(pthread_t));
   if(!xPL_thread)
   {
      VERBOSE(1) {
         fprintf (stderr, "%s (%s) : malloc - ",ERROR_STR,__func__);
         perror("");
      }
      return NULL;
   }

   if(pthread_create (xPL_thread, NULL, _xPL_server_thread, (void *)interfaces))
   {
      VERBOSE(1) fprintf(stderr, "%s (%s) : pthread_create - can't start thread\n",ERROR_STR,__func__);
      return NULL;
   }
      
   return xPL_thread;
}
