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
#include "mea_verbose.h"
#include "mea_queue.h"
#include "mea_timer.h"
#include "tokens.h"
#include "xPLServer.h"
#include "mea_string_utils.h"
#include "processManager.h"

#include "interfacesServer.h"

#include "mea_sockets_utils.h"
#include "notify.h"

#define XPL_VERSION "0.1a2"


#define XPL_EXTERNAL_WD 1
#ifdef XPL_EXTERNAL_WD
xPL_MessagePtr xplWDMsg;
mea_timer_t xPLnoMsgReceivedTimer;
#endif


char *xpl_server_name_str="XPLSERVER";
char *xpl_server_xplin_str="XPLIN";
char *xpl_server_xplout_str="XPLOUT";

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
mea_queue_t        *xplRespQueue;
pthread_cond_t  xplRespQueue_sync_cond;
pthread_mutex_t xplRespQueue_sync_lock;
int             _xPLServer_mutex_initialized=0;

/*
// declaration des fonctions xPL non exporté par la librairies
extern xPL_MessagePtr xPL_AllocMessage();
extern xPL_NameValueListPtr xPL_AllocNVList();
extern xPL_MessagePtr createSendableMessage(xPL_MessageType messageType, char *vendorID, char *deviceID, char *instanceID);
export xPL_MessagePtr createReceivedMessage(xPL_MessageType messageType);
*/

void _cmndXPLMessageHandler(xPL_ServicePtr theService, xPL_MessagePtr theMessage, xPL_ObjectPtr userValue);


void set_xPLServer_isnt_running(void *data)
{
   fprintf(stderr,"OK je m'arrete\n");
   _xPLServer_thread_is_running=0;
}


void clean_xPLServer(void *data)
{
   xPL_removeServiceListener(xPLService, _cmndXPLMessageHandler);
   xPL_removeServiceListener(xPLService, _cmndXPLMessageHandler);
#ifdef XPL_EXTERNAL_WD
   xPL_removeServiceListener(xPLService, _cmndXPLMessageHandler);
#endif
   xPL_setServiceEnabled(xPLService, FALSE);
   xPL_releaseService(xPLService);

#ifdef XPL_EXTERNAL_WD
   if(xplWDMsg)
   {
      xPL_releaseMessage(xplWDMsg);
      xplWDMsg=NULL;
   }
#endif
}

// duplication de createSendableMessage de la lib xPL qui est déclarée en static et ne peut donc
// pas normalement être utilisée. On a besoin de cette fonction pour pouvoir mettre
// créer un message sans passer par un service (utilisé par le WD externe).
xPL_MessagePtr mea_createSendableMessage(xPL_MessageType messageType, char *vendorID, char *deviceID, char *instanceID)
{
  xPL_MessagePtr theMessage;
  
  // Allocate the message
  theMessage = xPL_AllocMessage();

  // Set the version (NOT DYNAMIC)
  theMessage->messageType = messageType;
  theMessage->hopCount = 1;
  theMessage->receivedMessage = FALSE;

  theMessage->sourceVendor = vendorID;
  theMessage->sourceDeviceID = deviceID;
  theMessage->sourceInstanceID = instanceID;

  // Allocate a name/value list, if needed
  if (theMessage->messageBody == NULL) theMessage->messageBody = xPL_AllocNVList();
  
  // And we are done
  return theMessage;
}


// duplication de createReceivedMessage de la lib xPL qui est déclarée en static et ne peut donc
// pas normalement être utilisée. On a besoin de cette fonction pour pouvoir mettre
// une adresse source différente de l'adresse normale du soft (besoin pour les echanges internes).
xPL_MessagePtr mea_createReceivedMessage(xPL_MessageType messageType)
{
  xPL_MessagePtr theMessage;
  
  // Allocate the message
  theMessage = xPL_AllocMessage();

  // Set the version (NOT DYNAMIC)
  theMessage->messageType = messageType;
  theMessage->receivedMessage = TRUE;

  // Allocate a name/value list, if needed
  if (theMessage->messageBody == NULL) theMessage->messageBody = xPL_AllocNVList();

  // And we are done
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
   return mea_string_free_alloc_and_copy(&xpl_vendorID, value);
}


char *mea_setXPLDeviceID(char *value)
{
   return mea_string_free_alloc_and_copy(&xpl_deviceID, value);
}


char *mea_setXPLInstanceID(char *value)
{
   return mea_string_free_alloc_and_copy(&xpl_instanceID, value);
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

   process_update_indicator(_xplServer_monitoring_id, xpl_server_xplout_str, ++xplout_indicator);

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
      
         mea_queue_in_elem(xplRespQueue, e);
      
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
   if(mea_queue_first(xplRespQueue)==0)
   {
      xplRespQueue_elem_t *e;
      do // parcours de la liste jusqu'a trouver une reponse pour nous
      {
         if(mea_queue_current(xplRespQueue, (void **)&e)==0)
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
                  mea_queue_remove_current(xplRespQueue);
                  goto readFromQueue_return;
               }
               // theoriquement pour nous mais donnees trop vieilles, on supprime ?
               DEBUG_SECTION mea_log_printf("%s (%s) : data are too old\n", DEBUG_STR,__func__);
            }
            else
            {
               DEBUG_SECTION mea_log_printf("%s (%s) : data aren't for me (%d != %d)\n", DEBUG_STR,__func__, e->id, id);
               e=NULL;
            }
         }
      }
      while(mea_queue_next(xplRespQueue)==0);
   }

readFromQueue_return:
   pthread_mutex_unlock(&(xplRespQueue_sync_lock));
   pthread_cleanup_pop(0);
   return msg;
}


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

   mea_log_printf("%s (%s) : source = %s, destination = %s, schema = %s, body = {",DEBUG_STR,__func__,
   xpl_source, xpl_destination
   , xpl_schema);

   xPL_NameValueListPtr xpl_body = xPL_getMessageBody(theMessage);
   int n = xPL_getNamedValueCount(xpl_body);
   for (int16_t i = 0; i < n; i++)
   {
      xPL_NameValuePairPtr keyValuePtr = xPL_getNamedValuePairAt(xpl_body, i);
      if(i)
         fprintf(stderr,", ");
      fprintf(stderr,"%s = %s",keyValuePtr->itemName, keyValuePtr->itemValue);
   }
   fprintf(stderr,"}\n");
   return 0;
}


void _cmndXPLMessageHandler(xPL_ServicePtr theService, xPL_MessagePtr theMessage, xPL_ObjectPtr userValue)
{
#ifdef XPL_EXTERNAL_WD
   mea_start_timer(&xPLnoMsgReceivedTimer);
#endif
   char *schema_type, *schema_class;
   schema_class = xPL_getSchemaClass(theMessage);
   schema_type  = xPL_getSchemaType(theMessage);

   DEBUG_SECTION  displayXPLMsg(theMessage);
   
   if(mea_strcmplower(schema_class, get_token_string_by_id(XPL_WATCHDOG_ID)) == 0 &&
      mea_strcmplower(schema_type, get_token_string_by_id(XPL_BASIC_ID)) == 0)
   {
      // un message wd ... voir s'il faut faire quelque chose ...
   }
   else
   {
      process_update_indicator(_xplServer_monitoring_id, xpl_server_xplin_str, ++xplin_indicator);
      dispatchXPLMessageToInterfaces(theService, theMessage);
   }
}


void _flushExpiredXPLResponses()
{
   xplRespQueue_elem_t *e;
   uint32_t tsp=(uint32_t)time(NULL);
   
   pthread_cleanup_push( (void *)pthread_mutex_unlock, (void *)&(xplRespQueue_sync_lock) );
   pthread_mutex_lock(&xplRespQueue_sync_lock);
   
   if(xplRespQueue && mea_queue_first(xplRespQueue)==0)
   {
      while(1)
      {
         if(mea_queue_current(xplRespQueue, (void **)&e)==0)
         {
            if((tsp - e->tsp) > 5)
            {
               xPL_releaseMessage(e->msg);
               free(e);
               e=NULL;
               mea_queue_remove_current(xplRespQueue); // remove current passe sur le suivant
               DEBUG_SECTION mea_log_printf("%s (%s) : responses queue was flushed\n",DEBUG_STR,__func__);
            }
            else
            mea_queue_next(xplRespQueue);
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
   pthread_cleanup_push( (void *)clean_xPLServer, (void *)NULL);
   
   _xPLServer_thread_is_running=1;
   process_heartbeat(_xplServer_monitoring_id); // 1er heartbeat après démarrage.
   
//   xPL_setDebugging(TRUE); // xPL en mode debug
   
   xPLService = xPL_createService(xpl_vendorID, xpl_deviceID, xpl_instanceID);
   xPL_setRespondingToBroadcasts(xPLService, TRUE);
   xPL_setServiceVersion(xPLService, XPL_VERSION);
   xPL_setHeartbeatInterval(xPLService, 5000); // en milliseconde
   
   xPL_addServiceListener(xPLService, _cmndXPLMessageHandler, xPL_MESSAGE_COMMAND, "control", "basic", (xPL_ObjectPtr)data) ;
   xPL_addServiceListener(xPLService, _cmndXPLMessageHandler, xPL_MESSAGE_COMMAND, "sensor", "request", (xPL_ObjectPtr)data) ;
   
#ifdef XPL_EXTERNAL_WD
   mea_timer_t xPLWDSendMsgTimer;
   mea_init_timer(&xPLnoMsgReceivedTimer, 90, 1);
   mea_init_timer(&xPLWDSendMsgTimer, 30, 1);
   
   char xpl_instanceWDID[17];
   snprintf(xpl_instanceWDID,sizeof(xpl_instanceWDID)-1,"%s%s",xpl_instanceID,"wd");
   
   xplWDMsg=mea_createSendableMessage(xPL_MESSAGE_TRIGGER, xpl_vendorID, xpl_deviceID, xpl_instanceWDID);
   xPL_setBroadcastMessage(xplWDMsg, FALSE);
   xPL_setSchema(xplWDMsg, "watchdog", "basic");
   xPL_setTarget(xplWDMsg, xpl_vendorID, xpl_deviceID, xpl_instanceID);
   xPL_setMessageNamedValue(xplWDMsg, "interval", "5");
   mea_start_timer(&xPLnoMsgReceivedTimer);
   mea_start_timer(&xPLWDSendMsgTimer);
   xPL_addServiceListener(xPLService, _cmndXPLMessageHandler, xPL_MESSAGE_TRIGGER, "watchdog", "basic", (xPL_ObjectPtr)data);
#endif

   xPL_setServiceEnabled(xPLService, TRUE);

   do
   {
      pthread_testcancel();
      process_heartbeat(_xplServer_monitoring_id); // heartbeat après chaque boucle
      
#ifdef XPL_EXTERNAL_WD
      if(mea_test_timer(&xPLnoMsgReceivedTimer)==0)
      {
         // pas de message depuis 10 secondes
         VERBOSE(2) {
            mea_log_printf("%s (%s) : no xPL message since 10 seconds, but we should have one ... I send me one !\n", ERROR_STR, __func__);
            mea_log_printf("%s (%s) : watchdog_recovery forced ...\n", ERROR_STR, __func__);
         }
         process_forced_watchdog_recovery(_xplServer_monitoring_id);
      }
      
      if(mea_test_timer(&xPLWDSendMsgTimer)==0) // envoie d'un message toutes les 5 secondes
         xPL_sendMessage(xplWDMsg);
#endif

      VERBOSE(9) {
         static char compteur=0;
         if(compteur>59)
         {
            compteur=0;
            mea_log_printf("%s  (%s) : %s thread is running\n", INFO_STR, __func__, xpl_server_name_str);
         }
         else
            compteur++;
      }

      xPL_processMessages(500);

      _flushExpiredXPLResponses();
   }
   while (1);

   pthread_cleanup_pop(1);
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
   xplRespQueue=(mea_queue_t *)malloc(sizeof(mea_queue_t));
   if(!xplRespQueue)
   {
      VERBOSE(1) {
         mea_log_printf("%s (%s) : %s - ",ERROR_STR,__func__,MALLOC_ERROR_STR);
         perror("");
      }
      return NULL;
   }
   mea_queue_init(xplRespQueue); // initialisation de la file
   pthread_mutex_unlock(&(xplRespQueue_sync_lock));
   pthread_cleanup_pop(0);

   _xPLServer_thread_id=(pthread_t *)malloc(sizeof(pthread_t));
   if(!_xPLServer_thread_id)
   {
      VERBOSE(1) {
         mea_log_printf("%s (%s) : %s - ",ERROR_STR,__func__, MALLOC_ERROR_STR);
         perror("");
      }
      free(xplRespQueue);
      xplRespQueue=NULL;
      return NULL;
   }

   if(pthread_create (_xPLServer_thread_id, NULL, xPLServer_thread, NULL))
   {
      VERBOSE(1) mea_log_printf("%s (%s) : pthread_create - can't start thread\n",ERROR_STR,__func__);
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

      DEBUG_SECTION mea_log_printf("%s (%s) : %s, fin après %d itération\n",DEBUG_STR, __func__,xpl_server_name_str,100-counter);
      ret=stopped;
   }

   pthread_cleanup_push( (void *)pthread_mutex_unlock, (void *)&(xplRespQueue_sync_lock) );
   pthread_mutex_lock(&(xplRespQueue_sync_lock));
   if(xplRespQueue)
   {
      mea_queue_cleanup(xplRespQueue,_xplRespQueue_free_queue_elem);
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
      VERBOSE(2) mea_log_printf("%s  (%s) : %s %s.\n", INFO_STR, __func__, xpl_server_name_str, stopped_successfully_str);
      mea_notify_printf('S', "%s %s", xpl_server_name_str, stopped_successfully_str);
   }
   else
   {
      VERBOSE(2) mea_log_printf("%s  (%s) : %s can't cancel thread.\n", INFO_STR, __func__, xpl_server_name_str);
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


      xPL_setBroadcastInterface(xplServer_start_stop_params->params_list[INTERFACE]);

      if ( !xPL_initialize(xPL_getParsedConnectionType()) )
//      if ( !xPL_initialize(xcStandAlone) )
//      if ( !xPL_initialize(xcViaHub) )
      {
         VERBOSE(1) {
            mea_log_printf("%s (%s) : xPL_initialize - error\n",ERROR_STR,__func__);
         }
         mea_notify_printf('E', "%s Can't be launched - xPL_initialize error.\n", xpl_server_name_str);
         return -1;
      }

      _xPLServer_thread_id=xPLServer();

      if(_xPLServer_thread_id==NULL)
      {
         strerror_r(errno, err_str, sizeof(err_str));
         VERBOSE(1) {
            mea_log_printf("%s (%s) : can't start xpl server - %s\n",ERROR_STR,__func__,err_str);
         }
         mea_notify_printf('E', "%s Can't be launched - %s.", xpl_server_name_str, err_str);

         return -1;
      }
      else
      {
         pthread_detach(*_xPLServer_thread_id);
         VERBOSE(2) mea_log_printf("%s  (%s) : %s launched successfully.\n", INFO_STR, __func__, xpl_server_name_str);
         mea_notify_printf('S', "%s launched successfully", xpl_server_name_str);

         return 0;
      }
   }
   else
   {
      VERBOSE(1) mea_log_printf("%s (%s) : no valid xPL address.\n", ERROR_STR, __func__);
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

