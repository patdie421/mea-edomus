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
#include "consts.h"
#include "xPL.h"
#include "debug.h"
#include "queue.h"
#include "memory.h"

#include "xPLServer.h"

#include "processManager.h"

#include "interfacesServer.h"

#include "sockets_utils.h"
#include "notify.h"

#define XPL_VERSION "0.1a2"


char *xpl_server_name_str="XPLSERVER";


xPL_ServicePtr xPLService = NULL;
char *xpl_vendorID=NULL;
char *xpl_deviceID=NULL;
char *xpl_instanceID=NULL;


// gestion du thread et des indicateurs
pthread_t *_xPLServer_thread_id;
int _xplServer_monitoring_id = -1;
volatile sig_atomic_t _xPLServer_thread_is_running=0;

long xplin_indicator = 0;
long xplout_indicator = 0;


// gestion de des messages xpl internes
uint32_t requestId = 1;
pthread_mutex_t requestId_lock;
queue_t        *xplRespQueue;
pthread_cond_t  xplRespQueue_sync_cond;
pthread_mutex_t xplRespQueue_sync_lock;
int             _xPLServer_mutex_initialized=0;


// declaration des fonctions xPL non exporté par la librairies
extern xPL_MessagePtr xPL_AllocMessage();
extern xPL_NameValueListPtr xPL_AllocNVList();


void set_xPLServer_isnt_running(void *data)
{
   _xPLServer_thread_is_running=0;
}


// duplication de createReceivedMessage de la lib xPL qui est déclarée en static et ne peut donc
// pas normalement être utilisée. On a besoin de cette fonction pour pouvoir mettre
// une adresse source différente de l'adresse normale du soft (besoin pour les echanges internes).
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


uint16_t mea_sendXPLMessage(xPL_MessagePtr xPLMsg)
{
   char *addr;
   xPL_MessagePtr newXPLMsg = NULL;
   xplRespQueue_elem_t *e;

   process_update_indicator(_xplServer_monitoring_id, "XPLOUT", ++xplout_indicator);

   addr = xPL_getSourceDeviceID(xPLMsg);
   if(addr && strcmp(addr,"internal")==0) // source interne => dispatching sans passer par le réseau
   {
      dispatchXPLMessageToInterfaces(xPLService, xPLMsg);
      return 0;
   }
   
   addr = xPL_getTargetDeviceID(xPLMsg);
   if(addr && strcmp(addr,"internal")==0) // destination interne, retour à mettre dans une file (avec timestamp) ...
   {
      int id;
      
      sscanf(xPL_getTargetInstanceID(xPLMsg), "%d", &id);

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
         e = malloc(sizeof(xplRespQueue_elem_t));
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
                  e=NULL;
                  xplRespQueue->current->d=NULL; // pour evite le bug
                  remove_current_queue(xplRespQueue);
                  goto readFromQueue_return;
               }
               // theoriquement pour nous mais donnees trop vieilles, on supprime ?
//               DEBUG_SECTION fprintf(stderr,"%s (%s) : data are too old\n", DEBUG_STR,__func__);
               DEBUG_SECTION mea_logprintf("%s (%s) : data are too old\n", DEBUG_STR,__func__);
            }
            else
            {
//               DEBUG_SECTION fprintf(stderr,"%s (%s) : data aren't for me (%d != %d)\n", DEBUG_STR,__func__, e->id, id);
               DEBUG_SECTION mea_logprintf("%s (%s) : data aren't for me (%d != %d)\n", DEBUG_STR,__func__, e->id, id);
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
   process_update_indicator(_xplServer_monitoring_id, "XPLIN", ++xplin_indicator);

   dispatchXPLMessageToInterfaces(theService, theMessage);
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
               free(e);
               e=NULL;
               remove_current_queue(xplRespQueue); // remove current passe sur le suivant
//               DEBUG_SECTION fprintf(stderr,"%s (%s) : responses queue was flushed\n",DEBUG_STR,__func__);
               DEBUG_SECTION mea_logprintf("%s (%s) : responses queue was flushed\n",DEBUG_STR,__func__);
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


void _xplRespQueue_free_queue_elem(void *d)
{
   xplRespQueue_elem_t *e=(xplRespQueue_elem_t *)d;
   xPL_releaseMessage(e->msg);
   e->msg=NULL;
}


void *xPLServer_thread(void *data)
{
   pthread_cleanup_push( (void *)set_xPLServer_isnt_running, (void *)NULL );
   _xPLServer_thread_is_running=1;
   process_heartbeat(_xplServer_monitoring_id); // 1er heartbeat après démarrage.
   
//   xPL_setDebugging(TRUE); // xPL en mode debug

   xPLService = xPL_createService(xpl_vendorID, xpl_deviceID, xpl_instanceID);
   xPL_setServiceVersion(xPLService, XPL_VERSION);
   
   xPL_setRespondingToBroadcasts(xPLService, TRUE);
   
   xPL_setHeartbeatInterval(xPLService, 5000); // en milliseconde    
   xPL_addServiceListener(xPLService, _cmndXPLMessageHandler, xPL_MESSAGE_COMMAND, "control", "basic", (xPL_ObjectPtr)data) ;
   xPL_addServiceListener(xPLService, _cmndXPLMessageHandler, xPL_MESSAGE_COMMAND, "sensor", "request", (xPL_ObjectPtr)data) ;
   
   xPL_setServiceEnabled(xPLService, TRUE);

   do
   {
      pthread_testcancel();
      process_heartbeat(_xplServer_monitoring_id); // heartbeat après chaque boucle
      VERBOSE(9) {
         static char compteur=0;
         if(compteur>59)
         {
            compteur=0;
//            fprintf(stderr,"%s  (%s) : %s thread is running\n", INFO_STR, __func__, xpl_server_name_str);
            mea_logprintf("%s  (%s) : %s thread is running\n", INFO_STR, __func__, xpl_server_name_str);
         }
         else
            compteur++;
      }

      xPL_processMessages(500);

      _flushExpiredXPLResponses();
   }
   while (1);

   pthread_cleanup_pop(1);
}


pthread_t *xPLServer()
{
//   pthread_t *xPL_thread=NULL;

   if(!xpl_deviceID || !xpl_instanceID || !xpl_vendorID)
   {
      return NULL;
   }
   
   // préparation synchro consommateur / producteur si nécessaire
   if(_xPLServer_mutex_initialized==0)
   {
      pthread_cond_init(&xplRespQueue_sync_cond, NULL);
      pthread_mutex_init(&xplRespQueue_sync_lock, NULL);
      pthread_mutex_init(&requestId_lock, NULL);
      _xPLServer_mutex_initialized=1;
   }

   pthread_cleanup_push( (void *)pthread_mutex_unlock, (void *)&(xplRespQueue_sync_lock) );
   pthread_mutex_lock(&(xplRespQueue_sync_lock));
   xplRespQueue=(queue_t *)malloc(sizeof(queue_t));
   if(!xplRespQueue)
   {
      VERBOSE(1) {
//         fprintf (stderr, "%s (%s) : %s - ",ERROR_STR,__func__,MALLOC_ERROR_STR);
         mea_logprintf("%s (%s) : %s - ",ERROR_STR,__func__,MALLOC_ERROR_STR);
         perror("");
      }
      return NULL;
   }
   init_queue(xplRespQueue); // initialisation de la file
   pthread_mutex_unlock(&(xplRespQueue_sync_lock));
   pthread_cleanup_pop(0);

   _xPLServer_thread_id=(pthread_t *)malloc(sizeof(pthread_t));
   if(!_xPLServer_thread_id)
   {
      VERBOSE(1) {
//         fprintf (stderr, "%s (%s) : %s - ",ERROR_STR,__func__, MALLOC_ERROR_STR);
         mea_logprintf("%s (%s) : %s - ",ERROR_STR,__func__, MALLOC_ERROR_STR);
         perror("");
      }
      free(xplRespQueue);
      xplRespQueue=NULL;
      return NULL;
   }

   if(pthread_create (_xPLServer_thread_id, NULL, xPLServer_thread, NULL))
   {
//      VERBOSE(1) fprintf(stderr, "%s (%s) : pthread_create - can't start thread\n",ERROR_STR,__func__);
      VERBOSE(1) mea_logprintf("%s (%s) : pthread_create - can't start thread\n",ERROR_STR,__func__);
      return NULL;
   }
      
   return _xPLServer_thread_id;
}


int stop_xPLServer(int my_id, void *data,  char *errmsg, int l_errmsg)
{
   int ret=0;
   
   if(_xPLServer_thread_id)
   {
      pthread_cancel(*_xPLServer_thread_id);
      int counter=100;
      int stopped=-1;
      while(counter--)
      {
         if(_xPLServer_thread_is_running)
         {  // pour éviter une attente "trop" active
            usleep(100); // will sleep for 1 ms
         }
         else
         {
            stopped=0;
            break;
         }
      }

//      DEBUG_SECTION fprintf(stderr,"%s (%s) : %s, fin après %d itération\n",DEBUG_STR, __func__,xpl_server_name_str,100-counter);
      DEBUG_SECTION mea_logprintf("%s (%s) : %s, fin après %d itération\n",DEBUG_STR, __func__,xpl_server_name_str,100-counter);
      ret=stopped;
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

   if(_xPLServer_thread_id)
   {
      free(_xPLServer_thread_id);
      _xPLServer_thread_id=NULL;
   }   

   xPL_shutdown();

   if(ret==0)
   {
//      VERBOSE(2) fprintf(stderr,"%s  (%s) : %s %s.\n", INFO_STR, __func__, xpl_server_name_str, stopped_successfully_str);
      VERBOSE(2) mea_logprintf("%s  (%s) : %s %s.\n", INFO_STR, __func__, xpl_server_name_str, stopped_successfully_str);
      mea_notify_printf('S', "%s %s", xpl_server_name_str, stopped_successfully_str);
   }
   else
   {
//      VERBOSE(2) fprintf(stderr,"%s  (%s) : %s can't cancel thread.\n", INFO_STR, __func__, xpl_server_name_str);
      VERBOSE(2) mea_logprintf("%s  (%s) : %s can't cancel thread.\n", INFO_STR, __func__, xpl_server_name_str);
      mea_notify_printf('S', "%s can't cancel thread", xpl_server_name_str);
   }
   return ret;
}


int start_xPLServer(int my_id, void *data, char *errmsg, int l_errmsg)
{
   struct xplServer_start_stop_params_s *xplServer_start_stop_params = (struct xplServer_start_stop_params_s *)data;
   char err_str[256];
   
   if(!set_xpl_address(xplServer_start_stop_params->params_list))
   {
      _xplServer_monitoring_id=my_id;

#ifdef __APPLE__
      xPL_setBroadcastInterface("lo0"); // comprendre pourquoi ca ne marche plus sans ...
#endif
      if ( !xPL_initialize(xPL_getParsedConnectionType()) )
      {
         VERBOSE(1) {
//            fprintf (stderr, "%s (%s) : xPL_initialize - error\n",ERROR_STR,__func__);
            mea_logprintf("%s (%s) : xPL_initialize - error\n",ERROR_STR,__func__);
         }
         mea_notify_printf('E', "%s Can't be launched - xPL_initialize error.\n", xpl_server_name_str);
         return -1;
      }

      _xPLServer_thread_id=xPLServer();

      if(_xPLServer_thread_id==NULL)
      {
         strerror_r(errno, err_str, sizeof(err_str));
         VERBOSE(2) {
//            fprintf(stderr,"%s (%s) : can't start xpl server - %s\n",ERROR_STR,__func__,err_str);
            mea_logprintf("%s (%s) : can't start xpl server - %s\n",ERROR_STR,__func__,err_str);
         }
         mea_notify_printf('E', "%s Can't be launched - %s.", xpl_server_name_str, err_str);

         return -1;
      }
      else
      {
         pthread_detach(*_xPLServer_thread_id);
//         VERBOSE(2) fprintf(stderr,"%s  (%s) : %s launched successfully.\n", INFO_STR, __func__, xpl_server_name_str);
         VERBOSE(2) mea_logprintf("%s  (%s) : %s launched successfully.\n", INFO_STR, __func__, xpl_server_name_str);
         mea_notify_printf('S', "%s launched successfully", xpl_server_name_str);

         return 0;
      }
   }
   else
   {
//      VERBOSE(2) fprintf(stderr,"%s (%s) : no valid xPL address.\n", ERROR_STR, __func__);
      VERBOSE(2) mea_logprintf("%s (%s) : no valid xPL address.\n", ERROR_STR, __func__);
      mea_notify_printf('E', "%s Can't be launched - no valid xPL address.", xpl_server_name_str);
      return -1;
   }
}


int restart_xPLServer(int my_id, void *data, char *errmsg, int l_errmsg)
{
   int ret=0;
   ret=stop_xPLServer(my_id, data, errmsg, l_errmsg);
   if(ret==0)
   {
      return start_xPLServer(my_id, data, errmsg, l_errmsg);
   }
   return ret;
}

