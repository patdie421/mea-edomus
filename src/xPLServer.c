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

#include "monitoringServer.h"

#include "interfacesServer.h"
//#include "interface_type_001.h"
//#include "interface_type_002.h"


#define XPL_VERSION "0.1a2"


xPL_ServicePtr xPLService = NULL;

char *xpl_vendorID=NULL;
char *xpl_deviceID=NULL;
char *xpl_instanceID=NULL;

pthread_t *_xPLServer_thread;
// pthread_mutex_t xplRespSend_lock;


// gestion du thread et des indicateurs
int _xplServer_monitoring_id = -1;
long xplin_indicator = 0;
long xplout_indicator = 0;


// gestion de des messages xpl internes
uint32_t requestId = 1;
pthread_mutex_t requestId_lock;
pthread_cond_t  xplRespQueue_sync_cond;
pthread_mutex_t xplRespQueue_sync_lock;
queue_t         *xplRespQueue;


// declaration des fonctions xPL non exporté par la librairies
extern xPL_MessagePtr xPL_AllocMessage();
extern xPL_NameValueListPtr xPL_AllocNVList();


// duplication de createReceivedMessage de la lib xPL qui est déclarée en static et ne peut donc
// pas normalement être utilisée. On a besoin de cette fonction pour pouvoir utiliser mettre
// une adresse source différente de l'adresse normale du soft.
xPL_MessagePtr mea_createReceivedMessage(xPL_MessageType messageType)
{
  xPL_MessagePtr theMessage;
  
  /* Allocate the message */
  theMessage = xPL_AllocMessage();

  /* Set the version (NOT DYNAMIC) */
  theMessage->messageType = messageType;
  theMessage->receivedMessage = TRUE;

  /* Allocate a name/value list, if needed */
  if (theMessage->messageBody == NULL) theMessage->messageBody = xPL_AllocNVList();

  /* And we are done */
  return theMessage;
}


uint32_t mea_getXplRequestId() // rajouter un verrou ...
{
   uint32_t id=0;

   pthread_cleanup_push( (void *)pthread_mutex_unlock, (void *)&(requestId_lock) );
   pthread_mutex_lock(&requestId_lock);

   id=requestId;

   requestId++;
   if(requestId>20000)
   requestId=1;

   pthread_mutex_unlock(&requestId_lock);
   pthread_cleanup_pop(0);
 
   return id;
}


char *mea_setXPLVendorID(char *value)
{
   return string_free_malloc_and_copy(&xpl_vendorID, value, 1);
}


char *mea_setXPLDeviceID(char *value)
{
   return string_free_malloc_and_copy(&xpl_deviceID, value, 1);
}


char *mea_setXPLInstanceID(char *value)
{
   return string_free_malloc_and_copy(&xpl_instanceID, value, 1);
}


char *mea_getXPLInstanceID()
{
   return xpl_instanceID;
}


char *mea_getXPLDeviceID()
{
   return xpl_deviceID;
}


char *mea_getXPLVendorID()
{
   return xpl_vendorID;
}


xPL_ServicePtr mea_getXPLServicePtr()
{
   return xPLService;
}

/*
void _dispatchXPLMessage(xPL_ServicePtr theService, xPL_MessagePtr theMessage, xPL_ObjectPtr userValue) // à mettre dans interfaces.h ?
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
*/

uint16_t mea_sendXPLMessage(xPL_MessagePtr xPLMsg)
{
   char *addr;
   xPL_MessagePtr newXPLMsg = NULL;

   xplout_indicator++;

   addr = xPL_getSourceDeviceID(xPLMsg);
   if(addr && strcmp(addr,"internal")==0) // source interne => dispatching sans passer par le réseau
   {
      dispatchXPLMessageToInterfaces(xPLService, xPLMsg, (xPL_ObjectPtr)get_interfaces());
      return 0;
   }
   
   addr = xPL_getTargetDeviceID(xPLMsg);
   if(addr && strcmp(addr,"internal")==0) // destination interne, retour à mettre dans une file (avec timestamp) ...
   {
      int id;
      
      sscanf(xPL_getTargetInstanceID(xPLMsg), "%d", &id);

//      DEBUG_SECTION fprintf(stderr,"Retour de la demande interne à mettre dans la file (id demande = %d)\n",id);

      // duplication du message xPL
      newXPLMsg = xPL_createBroadcastMessage(xPLService, xPL_getMessageType(xPLMsg));

      xPL_setSchema(newXPLMsg, xPL_getSchemaClass(xPLMsg), xPL_getSchemaType(xPLMsg));
      xPL_setTarget(newXPLMsg, xPL_getTargetVendor(xPLMsg), xPL_getTargetDeviceID(xPLMsg), xPL_getTargetInstanceID(xPLMsg));
      xPL_NameValueListPtr body = xPL_getMessageBody(xPLMsg);

      int n = xPL_getNamedValueCount(body);
      for (int i=0; i<n; i++)
      {
         xPL_NameValuePairPtr keyValuePtr = xPL_getNamedValuePairAt(body, i);
         xPL_setMessageNamedValue(newXPLMsg, keyValuePtr->itemName, keyValuePtr->itemValue);
      }

      // ajout de la copie du message dans la file
      pthread_cleanup_push( (void *)pthread_mutex_unlock, (void *)&(xplRespQueue_sync_lock) );
      pthread_mutex_lock(&xplRespQueue_sync_lock);

      if(xplRespQueue)
      {
         xplRespQueue_elem_t *e = malloc(sizeof(xplRespQueue_elem_t));
         e->msg = newXPLMsg;
         e->id = id;
         e->tsp = (uint32_t)time(NULL);
      
         in_queue_elem(xplRespQueue, e);
      
         if(xplRespQueue->nb_elem>=1)
            pthread_cond_broadcast(&xplRespQueue_sync_cond);
      }
      
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


xPL_MessagePtr mea_readXPLResponse(int id)
{
   int16_t ret;
//   int16_t boucle=5; // 5 tentatives de 1 secondes
   uint16_t notfound=0;
   xPL_MessagePtr msg=NULL;

   // on va attendre le retour dans la file des reponses
   pthread_cleanup_push( (void *)pthread_mutex_unlock, (void *)&(xplRespQueue_sync_lock) );
   pthread_mutex_lock(&(xplRespQueue_sync_lock));

   if((xplRespQueue && xplRespQueue->nb_elem==0) || notfound==1)
   {
      // rien a lire => on va attendre que quelque chose soit mis dans la file
      struct timeval tv;
      struct timespec ts;
      gettimeofday(&tv, NULL);
      ts.tv_sec = tv.tv_sec + 2; // timeout de deux secondes
      ts.tv_nsec = 0;
      ret=pthread_cond_timedwait(&xplRespQueue_sync_cond, &xplRespQueue_sync_lock, &ts);
      if(ret)
      {
         if(ret!=ETIMEDOUT)
            goto readFromQueue_return;
      } 
   }
   else
   {
      goto readFromQueue_return;
   }

   // a ce point il devrait y avoir quelque chose dans la file.
   if(first_queue(xplRespQueue)==0)
   {
      xplRespQueue_elem_t *e;
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


void _cmndXPLMessageHandler(xPL_ServicePtr theService, xPL_MessagePtr theMessage, xPL_ObjectPtr userValue)
{
   xplin_indicator++;
   dispatchXPLMessageToInterfaces(theService, theMessage, userValue);
}


void _flushExpiredXPLResponses()
{
   xplRespQueue_elem_t *e;
   uint32_t tsp=(uint32_t)time(NULL);
   
   pthread_cleanup_push( (void *)pthread_mutex_unlock, (void *)&(xplRespQueue_sync_lock) );
   pthread_mutex_lock(&xplRespQueue_sync_lock);
   
   if(xplRespQueue && first_queue(xplRespQueue)==0)
   {
      while(1)
      {
         if(current_queue(xplRespQueue, (void **)&e)==0)
         {
            if((tsp - e->tsp) > 5)
            {
               xPL_releaseMessage(e->msg);
               DEBUG_SECTION fprintf(stderr,"%s (%s) : Je flush\n",DEBUG_STR,__func__);
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


void _xplRespQueue_free_queue_elem(void *d)
{
   xplRespQueue_elem_t *e=(xplRespQueue_elem_t *)d;
   xPL_releaseMessage(e->msg);
   e->msg=NULL;
}


void *_xPL_thread(void *data)
{
//   xPL_setDebugging(TRUE); // xPL en mode debug

   process_add_indicator(get_monitored_processes_descriptor(), _xplServer_monitoring_id, "XPLIN", xplin_indicator);
   process_add_indicator(get_monitored_processes_descriptor(), _xplServer_monitoring_id, "XPLOUT", xplout_indicator);

   if ( !xPL_initialize(xPL_getParsedConnectionType()) ) return 0 ;
   
   xPLService = xPL_createService(xpl_vendorID, xpl_deviceID, xpl_instanceID);
   xPL_setServiceVersion(xPLService, XPL_VERSION);
   
   xPL_setRespondingToBroadcasts(xPLService, TRUE);
   
   xPL_setHeartbeatInterval(xPLService, 5000); // en milliseconde    
   // xPL_MESSAGE_ANY, xPL_MESSAGE_COMMAND, xPL_MESSAGE_STATUS, xPL_MESSAGE_TRIGGER
   xPL_addServiceListener(xPLService, _cmndXPLMessageHandler, xPL_MESSAGE_COMMAND, "control", "basic", (xPL_ObjectPtr)data) ;
   xPL_addServiceListener(xPLService, _cmndXPLMessageHandler, xPL_MESSAGE_COMMAND, "sensor", "request", (xPL_ObjectPtr)data) ;
   
   xPL_setServiceEnabled(xPLService, TRUE);

   do
   {
      process_heartbeat(get_monitored_processes_descriptor(), _xplServer_monitoring_id);
   
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
      
      _flushExpiredXPLResponses();

      pthread_testcancel();
   }
   while (1);
}


//pthread_t *xPLServer(queue_t *interfaces)
pthread_t *xPLServer()
{
   pthread_t *xPL_thread=NULL;

   if(!xpl_deviceID || !xpl_instanceID || !xpl_vendorID)
   {
      return NULL;
   }
   
   // initialisation
//   pthread_mutex_init(&xplRespSend_lock, NULL);

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
      free(xplRespQueue);
      xplRespQueue=NULL;
      return NULL;
   }

//   if(pthread_create (xPL_thread, NULL, _xPL_thread, (void *)interfaces))
   if(pthread_create (xPL_thread, NULL, _xPL_thread, NULL))
   {
      VERBOSE(1) fprintf(stderr, "%s (%s) : pthread_create - can't start thread\n",ERROR_STR,__func__);
      return NULL;
   }
      
   return xPL_thread;
}


int16_t set_xpl_address(char **params_list)
/**
 * \brief     initialise les données pour l'adresse xPL
 * \details   positionne vendorID, deviceID et instanceID pour xPLServer
 * \param     params_liste  liste des parametres.
 * \return   -1 en cas d'erreur, 0 sinon
 */
{
   mea_setXPLVendorID(params_list[VENDOR_ID]);
   mea_setXPLDeviceID(params_list[DEVICE_ID]);
   mea_setXPLInstanceID(params_list[INSTANCE_ID]);
   
   return 0;
}


int stop_xPLServer(int my_id, void *data)
{
   if(_xPLServer_thread)
   {
      xPL_shutdown();
      pthread_cancel(*_xPLServer_thread);
      pthread_join(*_xPLServer_thread, NULL);
      free(_xPLServer_thread);
      _xPLServer_thread=NULL;
   }
      
   pthread_cleanup_push( (void *)pthread_mutex_unlock, (void *)&(xplRespQueue_sync_lock) );
   pthread_mutex_lock(&(xplRespQueue_sync_lock));
   if(xplRespQueue)
   {
      clear_queue(xplRespQueue,_xplRespQueue_free_queue_elem);
      free(xplRespQueue);
      xplRespQueue=NULL;
   }
   pthread_mutex_unlock(&(xplRespQueue_sync_lock));
   pthread_cleanup_pop(0);

   _xplServer_monitoring_id=-1;
   
   return 0;
}


int start_xPLServer(int my_id, void *data)
{
   struct xplServerData_s *xplServerData = (struct xplServerData_s *)data;
   
   if(!set_xpl_address(xplServerData->params_list))
   {
      _xplServer_monitoring_id=my_id;

      _xPLServer_thread=xPLServer();
      //_xPLServer_thread=xPLServer(xplServerData->interfaces);

      if(_xPLServer_thread==NULL)
      {
         VERBOSE(2) {
            fprintf(stderr,"%s (%s) : can't start xpl server -\n",ERROR_STR,__func__);
            perror("");
         }
         return -1;
      }
      else
         return 0;
   }
   else
   {
      VERBOSE(2) fprintf(stderr,"%s (%s) : no valid xPL address.\n",ERROR_STR,__func__);
      return -1;
   }
}




