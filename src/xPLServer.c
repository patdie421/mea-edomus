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
#include "tokens_da.h"
#include "xPLServer.h"
#include "mea_string_utils.h"
#include "processManager.h"

#include "interfacesServer.h"

#include "mea_sockets_utils.h"
#include "notify.h"

#define XPL_VERSION "0.1a2"


#include "cJSON.h"
extern char *inputs_rules;
extern char *outputs_rules;

cJSON *_inputs_rules;
cJSON *_outputs_rules;

cJSON *automator_load_rules(char *rules);
int automator_match_inputs_rules(cJSON *rules, xPL_MessagePtr message);
int automator_play_output_rules(cJSON *rules);
int automator_reset_inputs_change_flags();


#define XPL_WD 1
#ifdef XPL_WD
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
mea_queue_t    *xplRespQueue;
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

//void _cmndXPLMessageHandler(xPL_ServicePtr theService, xPL_MessagePtr theMessage, xPL_ObjectPtr userValue);
void _cmndXPLMessageHandler(xPL_MessagePtr theMessage, xPL_ObjectPtr userValue);


void set_xPLServer_isnt_running(void *data)
{
   _xPLServer_thread_is_running=0;
}


void clean_xPLServer(void *data)
{
   xPL_removeMessageListener(_cmndXPLMessageHandler);

//   xPL_removeServiceListener(xPLService, _cmndXPLMessageHandler);
//   xPL_removeServiceListener(xPLService, _cmndXPLMessageHandler);
#ifdef XPL_WD
//   xPL_removeServiceListener(xPLService, _cmndXPLMessageHandler);
#endif
   xPL_setServiceEnabled(xPLService, FALSE);
   xPL_releaseService(xPLService);

#ifdef XPL_WD
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
   if(addr && strcmp(addr,INTERNAL_STR_C)==0) // source interne => dispatching sans passer par le réseau
   {
      dispatchXPLMessageToInterfaces(xPLService, xPLMsg);
      return 0;
   }
   
   addr = xPL_getTargetDeviceID(xPLMsg);
   if(addr && strcmp(addr,INTERNAL_STR_C)==0) // destination interne, retour à mettre dans une file (avec timestamp) ...
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

   mea_log_printf("%s (%s) : source = %s, destination = %s, schema = %s, body = {", DEBUG_STR, __func__, xpl_source, xpl_destination, xpl_schema);

   xPL_NameValueListPtr xpl_body = xPL_getMessageBody(theMessage);
   int n = xPL_getNamedValueCount(xpl_body);
   for (int16_t i = 0; i < n; i++)
   {
      xPL_NameValuePairPtr keyValuePtr = xPL_getNamedValuePairAt(xpl_body, i);
      if(i)
         fprintf(MEA_STDERR,", ");
      fprintf(MEA_STDERR, "%s = %s",keyValuePtr->itemName, keyValuePtr->itemValue);
   }
   fprintf(MEA_STDERR, "}\n");
   return 0;
}


//void _cmndXPLMessageHandler(xPL_ServicePtr theService, xPL_MessagePtr theMessage, xPL_ObjectPtr userValue)
void _cmndXPLMessageHandler(xPL_MessagePtr theMessage, xPL_ObjectPtr userValue)
{
#ifdef XPL_WD
   mea_start_timer(&xPLnoMsgReceivedTimer);
#endif
   char *schema_type, *schema_class, *vendor, *deviceID, *instanceID;
   schema_class = xPL_getSchemaClass(theMessage);
   schema_type  = xPL_getSchemaType(theMessage);
   vendor       = xPL_getSourceVendor(theMessage);
   deviceID     = xPL_getSourceDeviceID(theMessage);
   instanceID   = xPL_getSourceInstanceID(theMessage);

   int fromMe=-1;
   if( strcmp(vendor,     xpl_vendorID)   ==  0 &&
       strcmp(deviceID,   xpl_deviceID)   == 0 &&
       strcmp(instanceID, xpl_instanceID) == 0)
      fromMe=0;

//   DEBUG_SECTION  displayXPLMsg(theMessage);
   
   if(mea_strcmplower(schema_class, get_token_string_by_id(XPL_WATCHDOG_ID)) == 0 &&
      mea_strcmplower(schema_type, get_token_string_by_id(XPL_BASIC_ID)) == 0 &&
      fromMe==0)
   {
//      DEBUG_SECTION mea_log_printf("%s (%s) : watchdog xpl\n", DEBUG_STR, __func__);
   }

   // on filtre un peu avant de transmettre pour traitement

   automator_match_inputs_rules(_inputs_rules, theMessage);
   automator_play_output_rules(_outputs_rules);
   automator_reset_inputs_change_flags();

   if(fromMe==0) // c'est de moi, pas la peine de traiter
      return;

   if(xPL_getMessageType(theMessage)!=xPL_MESSAGE_COMMAND)
   {
      // faire quelque chose ici si nécessaire (ex : automate)
      // automator_match_rules(_rules, theMessage);
      return;
   }
/*
   if(mea_strcmplower(schema_class, "hbeat") == 0 &&
      mea_strcmplower(schema_type, "app")    == 0) // hbeat.app : pas la peine de traiter
      return;
*/
   process_update_indicator(_xplServer_monitoring_id, xpl_server_xplin_str, ++xplin_indicator);
   dispatchXPLMessageToInterfaces(xPLService, theMessage);
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
   _inputs_rules=automator_load_rules(inputs_rules);
   _outputs_rules=automator_load_rules(outputs_rules);

   pthread_cleanup_push( (void *)set_xPLServer_isnt_running, (void *)NULL );
   pthread_cleanup_push( (void *)clean_xPLServer, (void *)NULL);
   
   _xPLServer_thread_is_running=1;
   process_heartbeat(_xplServer_monitoring_id); // 1er heartbeat après démarrage.
   
//   xPL_setDebugging(TRUE); // xPL en mode debug
   
   xPLService = xPL_createService(xpl_vendorID, xpl_deviceID, xpl_instanceID);
   xPL_setRespondingToBroadcasts(xPLService, TRUE);
   xPL_setServiceVersion(xPLService, XPL_VERSION);
   xPL_setHeartbeatInterval(xPLService, 5000); // en milliseconde

   xPL_addMessageListener(_cmndXPLMessageHandler, NULL);
/*   
   xPL_addServiceListener(xPLService, _cmndXPLMessageHandler, xPL_MESSAGE_COMMAND, "control", "basic", (xPL_ObjectPtr)data) ;
   xPL_addServiceListener(xPLService, _cmndXPLMessageHandler, xPL_MESSAGE_COMMAND, "sensor", "request", (xPL_ObjectPtr)data) ;
   xPL_addServiceListener(xPLService, _cmndXPLMessageHandler, xPL_MESSAGE_COMMAND, "sendmsg", "basic", (xPL_ObjectPtr)data) ;
*/   
#ifdef XPL_WD
   mea_timer_t xPLWDSendMsgTimer;
   mea_init_timer(&xPLnoMsgReceivedTimer, 30, 1);
   mea_init_timer(&xPLWDSendMsgTimer, 10, 1);

/*   
   char xpl_instanceWDID[17];
   snprintf(xpl_instanceWDID,sizeof(xpl_instanceWDID)-1,"%s%s",xpl_instanceID,"wd");
   xplWDMsg=mea_createSendableMessage(xPL_MESSAGE_TRIGGER, xpl_vendorID, xpl_deviceID, xpl_instanceWDID);
*/
   xplWDMsg=mea_createSendableMessage(xPL_MESSAGE_TRIGGER, xpl_vendorID, xpl_deviceID, xpl_instanceID);
   xPL_setBroadcastMessage(xplWDMsg, FALSE);
   xPL_setSchema(xplWDMsg, "watchdog", "basic");
   xPL_setTarget(xplWDMsg, xpl_vendorID, xpl_deviceID, xpl_instanceID);
   xPL_setMessageNamedValue(xplWDMsg, "interval", "10");
   mea_start_timer(&xPLnoMsgReceivedTimer);
   mea_start_timer(&xPLWDSendMsgTimer);
//   xPL_addServiceListener(xPLService, _cmndXPLMessageHandler, xPL_MESSAGE_TRIGGER, "watchdog", "basic", (xPL_ObjectPtr)data);
#endif

   xPL_setServiceEnabled(xPLService, TRUE);

   do
   {
      pthread_testcancel();
      process_heartbeat(_xplServer_monitoring_id); // heartbeat après chaque boucle
      
#ifdef XPL_WD
      if(mea_test_timer(&xPLnoMsgReceivedTimer)==0)
      {
         // pas de message depuis 90 secondes
         VERBOSE(2) {
            mea_log_printf("%s (%s) : no xPL message since 30 seconds, but we should have one ... I send me three !\n", ERROR_STR, __func__);
            mea_log_printf("%s (%s) : watchdog_recovery forced ...\n", ERROR_STR, __func__);
         }
         process_forced_watchdog_recovery(_xplServer_monitoring_id);
      }
      
      if(mea_test_timer(&xPLWDSendMsgTimer)==0) // envoie d'un message toutes les 30 secondes
         xPL_sendMessage(xplWDMsg);
#endif

      DEBUG_SECTION {
         static char compteur=0;
         if(compteur>59)
         {
            compteur=0;
            mea_log_printf("%s (%s) : %s thread is running\n", INFO_STR, __func__, xpl_server_name_str);
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
         mea_log_printf("%s (%s) : %s - ", ERROR_STR, __func__, MALLOC_ERROR_STR);
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
         mea_log_printf("%s (%s) : %s - ", ERROR_STR, __func__, MALLOC_ERROR_STR);
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

      DEBUG_SECTION mea_log_printf("%s (%s) : %s, fin après %d itération(s)\n",DEBUG_STR, __func__,xpl_server_name_str,100-counter);
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
      VERBOSE(2) mea_log_printf("%s (%s) : %s %s.\n", INFO_STR, __func__, xpl_server_name_str, stopped_successfully_str);
      mea_notify_printf('S', "%s %s", xpl_server_name_str, stopped_successfully_str);
   }
   else
   {
      VERBOSE(2) mea_log_printf("%s (%s) : %s can't cancel thread.\n", INFO_STR, __func__, xpl_server_name_str);
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
         VERBOSE(2) mea_log_printf("%s (%s) : %s launched successfully.\n", INFO_STR, __func__, xpl_server_name_str);
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

// RULES :
// xpl-trig: source = mea-edomus.home, destination = *, schema = sensor.basic, body = [device = conso, type = power, current = 274.591704]
// xpl-trig: source = mea-edomus.home, destination = *, schema = sensor.basic, body = [device = epgstat01, current = 1]
char *outputs_rules="[ \
   { \
      \"name\": \"A1\", \
      \"action\" : \"xPLSend\", \
      \"parameters\" : [ \
         { \"schema\"  : \"'control.basic'\" }, \
         { \"device\"  : \"'toto'\" }, \
         { \"current\" : \"{E3}\" } \
      ], \
      \"condition\" : { \"'E3'\" : \"raise\"} \
   }, \
   { \
      \"name\": \"A2\", \
      \"action\" : \"xPLSend\", \
      \"parameters\" : [ \
         { \"schema\"  : \"'control.basic'\" }, \
         { \"device\"  : \"'toto'\" }, \
         { \"current\" : \"{E3}\" } \
      ], \
      \"condition\" : { \"'E3'\" : \"fall\"} \
   }, \
   { \
      \"name\": \"A3\", \
      \"action\" : \"xPLSend\", \
      \"parameters\" : [ \
         { \"schema\"  : \"'control.basic'\" }, \
         { \"target\"  : \"'mea-edomus.test'\" }, \
         { \"device\"  : \"'tata'\" }, \
         { \"current\" : \"{E3}\" } \
      ], \
      \"condition\" : { \"'E3'\" : \"change\"} \
   } \
]";

char *inputs_rules="[ \
   { \
      \"name\": \"V1\", \
      \"value\" : \"$now()\", \
      \"onmatch\": \"continue\" \
   }, \
   { \
      \"name\": \"<NOP>\", \
      \"value\" : \"#0\", \
      \"conditions\" : { \
         \"schema\" : {\"op\" : \"==\", \"value\" : \"'hbeat.app'\" } \
      }, \
      \"onmatch\": \"break\" \
   }, \
   { \
      \"name\": \"<NOP>\", \
      \"value\" : \"#0\", \
      \"conditions\" : { \
         \"schema\" : {\"op\" : \"==\", \"value\" : \"'watchdog.basic'\" } \
      }, \
      \"onmatch\": \"break\" \
   }, \
   { \
      \"name\": \"STOPEVAL\", \
      \"value\" : \"current\", \
      \"conditions\" : { \
         \"source\" : {\"op\" : \"==\", \"value\" : \"'mea-edomus.home'\" }, \
         \"schema\" : {\"op\" : \"==\", \"value\" : \"'sensor.basic'\"}, \
         \"device\" : {\"op\" : \"==\", \"value\" : \"'stopeval01'\"} \
      }, \
      \"onmatch\": \"continue\" \
   }, \
   { \
      \"name\": \"BREAK\", \
      \"value\" : \"#0\", \
      \"conditions\" : { \
         \"{STOPEVAL}\" : {\"op\" : \"==\", \"value\" : \"&true\" } \
      }, \
      \"onmatch\": \"break\" \
   }, \
   { \
      \"name\": \"T1\", \
      \"value\" : \"#1\", \
      \"conditions\" : { \
         \"$exist('T1')\" : {\"op\" : \"==\", \"value\" : \"&false\" } \
      }, \
      \"onmatch\": \"continue\" \
   }, \
   { \
      \"name\": \"V2\", \
      \"value\" : \"{E1}\", \
      \"onmatch\": \"continue\" \
   }, \
   { \
      \"name\": \"E1\", \
      \"value\" : \"current\", \
      \"conditions\" : { \
         \"device\" : {\"op\" : \"==\", \"value\" : \"'BUTTON1'\"}, \
         \"source\" : {\"op\" : \"==\", \"value\" : \"'mea-edomus.home'\" }, \
         \"schema\" : {\"op\" : \"==\", \"value\" : \"'sensor.basic'\"}, \
         \"type\"   : {\"op\" : \"==\", \"value\" : \"'input'\"} \
      }, \
      \"onmatch\": \"break\" \
   }, \
   { \
      \"name\": \"E2\", \
      \"value\" : \"current\", \
      \"conditions\" : { \
         \"device\" : {\"op\" : \"==\", \"value\" : \"'BUTTON2'\"}, \
         \"source\" : {\"op\" : \"==\", \"value\" : \"'mea-edomus.home'\" }, \
         \"schema\" : {\"op\" : \"==\", \"value\" : \"'sensor.basic'\"}, \
         \"type\"   : {\"op\" : \"==\", \"value\" : \"'input'\"} \
      }, \
      \"onmatch\": \"break\" \
   }, \
   { \
      \"name\": \"E3\", \
      \"value\" : \"current\", \
      \"conditions\" : { \
         \"device\" : {\"op\" : \"==\", \"value\" : \"'conso'\"}, \
         \"source\" : {\"op\" : \"==\", \"value\" : \"'mea-edomus.home'\" }, \
         \"schema\" : {\"op\" : \"==\", \"value\" : \"'sensor.basic'\"}, \
         \"type\"   : {\"op\" : \"==\", \"value\" : \"'power'\"} \
      }, \
      \"onmatch\": \"break\" \
   }, \
   { \
      \"name\": \"TEMP1\", \
      \"value\" : \"current\", \
      \"conditions\" : { \
         \"device\" : {\"op\" : \"==\", \"value\" : \"'temp01'\"}, \
         \"source\" : {\"op\" : \"==\", \"value\" : \"'mea-edomus.home'\" }, \
         \"schema\" : {\"op\" : \"==\", \"value\" : \"'sensor.basic'\"} \
      }, \
      \"onmatch\": \"break\" \
   }, \
   { \
      \"name\": \"E4\", \
      \"value\" : \"current\", \
      \"conditions\" : { \
         \"device\" : {\"op\" : \"==\", \"value\" : \"'epgstat01'\"}, \
         \"source\" : {\"op\" : \"==\", \"value\" : \"'mea-edomus.home'\" }, \
         \"schema\" : {\"op\" : \"==\", \"value\" : \"'sensor.basic'\"} \
      }, \
      \"onmatch\": \"break\" \
   } \
]";
/*
# les règles sont évaluées dans l'ordre
# par defaut toutes les règles sont évaluées
# le comportement par défaut est modifiée par "onmatch" : break : l'évaluation s'arrête, continue : l'évaluation se poursuit
# mon langage de regles
# 
# D1 is: <NOP>     if: (schema == 'hbeat.app') onmatch: break 
# V1 is: #1
# V2 is: #2.1      if: (source == 'mea-edomus.home', schema == 'sensor.basic', device == 'BUTTON3', current == #2) onmatch: break
# V2 is: #10       if: (source == 'mea-edomus.home', schema == 'sensor.basic', device == 'BUTTON3', current > #3, current < #5) onmatch: continue
# E1 is: current   if: (source == 'mea-edomus.home', schema == 'sensor.basic', device == 'BUTTON1', type == 'input') onmatch: break
# E2 is: last      if: (source == 'mea-edomus.home', schema == 'sensor.basic', device == 'BUTTON2') onmatch: continue
# T2 is: $now()    if: (source == 'mea-edomus.home', schema == 'sensor.basic', device == 'BUTTON2') onmatch: continue
# P1 is: current   if: (source == 'mea-edomus.home', schema == 'sensor.basic', device == 'CONSO', type == 'power') onmatch: continue
# P2 is: current   if: (source == 'mea-edomus.home', schema == 'sensor.basic', device == 'CONSO', type == 'power', current != #0) onmatch: continue
# P3 is: current   if: (source == 'mea-edomus.home', schema == 'sensor.basic', device == 'PROD',  type == 'power', current != {V1}) onmatch: continue
# C1 is: current   if: (source == 'mea-edomus.home', schema == 'sensor.basic', device == 'PROD',  type == 'power', {T2}>0 ) onmatch: moveforward S1
# B1 is: &false    if: (source == 'mea-edomus.home', schema == 'sensor.basic', device == 'TEMP',  type == 'temp',  current > 0)   onmatch: continue
# S1 is: 'toto'    if: (source == 'mea-edomus.home', schema == 'sensor.basic', device == 'TEMP',  type == 'temp',  current <= #0) onmatch: continue
# S2 is: &true     if: ($timer('timer1')==&true) onmatch: continue

# des exemples de règles "compliqués"
# T1_last is: {T1} if: (source == mea-edomus.home, schema == sensor.basic, device == "BUTTON2", current == "high")
# T1 is: $now() if: (source == mea-edomus.home, schema == sensor.basic, device == "BUTTON2", current == "high")
# DIFF is: $eval({T2} - {T2_last}) if: (source == mea-edomus.home, schema == sensor.basic, device == "BUTTON2", current == 'high')
# P1 is: &high if: (source == mea-edomus.home, schema == sensor.basic, device == "BUTTON2", current == &high, {DIFF} > #1000)
# P1 is: &low  if: (source == mea-edomus.home, schema == sensor.basic, device == "BUTTON2", current == &high, {DIFF} <= #1000)
# exemple de compteur
# C1 is: #0 if: $exist('C1') == &false // initialisation du compteur
# C1 is: $eval({C1}+#1)
#

# Pour les actions :
#
# A1 do: xPLSend with: (schema='control.basic', device='toto', current={E1}) when: 'C1' raise
# A2 do: xPLSend with: (schema='control.basic', device='toto', current={E1}) when: 'C1' fall
# A3 do: xPLSend with: (schema='control.basic', device='tata', current={E1}) when: 'C1' change
# A4 do: xPLSend with: (schema='control.basic', device='tata', current=$eval(!{E1})) when: 'C1' change
# A5 do: timerstart with: (name='timer1', value=10, unit='s', autorestart=&false) when: 'E1' raise
*/

struct value_s {
   char type;
   union {
      char strval[41];
      double floatval;
      char booleanval;
   } val;
};


#include "uthash.h"

enum input_state_e { NEW, RAISE, FALL, STAY, CHANGE, TYPECHANGE, UNKNOWN };

struct inputs_table_s
{
   char name[20];
   enum input_state_e state;
   struct value_s v;

   UT_hash_handle hh;
};


int automator_add_to_inputs_table(char *name, struct value_s *v);
int automator_print_inputs_table();
int reset_inputs_table_change_flag();
static int eval_string(char *str, struct value_s *v, xPL_NameValueListPtr ListNomsValeursPtr);


struct inputs_table_s *inputs_table = NULL;

static int printVal(struct value_s *v)
{
   if(v->type==0)
      fprintf(stderr,"(float)%f",v->val.floatval);
   else if(v->type==2)
      fprintf(stderr,"(boolean)%d",v->val.booleanval);
   else
      fprintf(stderr,"(string)%s",v->val.strval);
   return 0;
}


static int getBoolean(char *s, char *b)
{
   *b=-1;
   if(mea_strcmplower(s,"true")==0 || mea_strcmplower(s, "high")==0)
   {
      *b=1;
      return 0;
   }
   else if(mea_strcmplower(s,"false")==0 || mea_strcmplower(s, "low")==0)
   {
      *b=0;
      return 0;
   }
   return -1;
}


static int getNumber(char *s, double *v)
{
   char *n;
 
   *v=strtod(s, &n);

   if(*n!=0)
      return -1;
   else
      return 0;
}


static int setValueFromStr(struct value_s *v, char *str)
{
   double f;
   char b;

   if(getNumber(str, &f)==0)
   {
      v->type=0;
      v->val.floatval=f; 
   }
   else if(getBoolean(str, &b)==0)
   {
      v->type=2;
      v->val.booleanval=b;
   }
   else
   {
      v->type=1;
      strncpy(v->val.strval, str, sizeof(v->val.strval)); 
      v->val.strval[sizeof(v->val.strval)-1]=0;
   }
   return 0;
}


static int operation(struct value_s *v1, char *op, struct value_s *v2)
{
   if(op[2]!=0)
      return -1;

   if(v1->type == 1 || v2->type == 1) // au moins une chaine de caractères
   {
      if(op[0]=='=' && op[1]=='=')
      {
         if(v1->type != v2->type)
            return 0;
         else
            return (mea_strcmplower(v1->val.strval, v2->val.strval)==0);
      }
      else if(op[0]=='!' && op[1]=='=')
      {
         if(v1->type != v2->type)
            return 1;
         else
            return !(mea_strcmplower(v1->val.strval, v2->val.strval)==0);
      }
      else
         return 0;
   }
   else if(v1->type == 0 || v2->type == 0)
   {
      if(op[0]=='=' && op[1]=='=')
      {
         if(v1->type != v2->type)
            return 0;
         else
            return (v1->val.floatval == v2->val.floatval);
      }
      else if(op[0]=='!' && op[1]=='=')
      {
         if(v1->type != v2->type)
            return 1;
         else
            return (v1->val.floatval != v2->val.floatval);
      }
      else
         return 0;
   }
   else // normalement 2 boolean ici
   {
      if(op[0]=='=' && op[1]=='=')
      {
         return ((int)v1->val.booleanval == (int)v2->val.booleanval);
      }
      else if(op[0]=='!' && op[1]=='=')
      {
         return ((int)v1->val.booleanval != (int)v2->val.booleanval);
      }
   }

   return 0;
}


static int eval_function(char *str, struct value_s *v, xPL_NameValueListPtr ListNomsValeursPtr)
{
   int retour=-1;
   int l=strlen(str);
   char *f=(char *)malloc(l+1);
   if(f==NULL)
      return -1;
   strcpy(f, str);
   
   // fonction now
   if(strcmp(f,"now()")==0)
   {
      time_t t=time(NULL);
      v->type=0;
      v->val.floatval=(double)t;
      
      retour=0;
      goto eval_function_clean_exit;
   }
   // fonction exist
   else if(ListNomsValeursPtr && strstr(f,"exist(")==f && f[l-1]==')')
   {
      struct inputs_table_s *e = NULL;
      f[l-1]=0; 
      char *p=&(f[6]);
      struct value_s res;

      int ret=eval_string(p,&res,ListNomsValeursPtr);
//      fprintf(stderr,"EVAL de %s : ", p); fprintf(stderr, "%d => RES : ", ret); printVal(&res); fprintf(stderr,"\n");
      if(ret==0 && res.type==1)
      {
         f[l-1]=0;
         v->type=2;
         HASH_FIND_STR(inputs_table, res.val.strval, e);
         if(e)
            v->val.booleanval=1;
         else
            v->val.booleanval=0;
         retour=0;
         goto eval_function_clean_exit;
      }
      else
      {
         goto eval_function_clean_exit;
      }
   }
   else if(ListNomsValeursPtr && strstr(f,"raise(")==f && f[l-1]==')')
   {
      struct inputs_table_s *e = NULL;
      f[l-1]=0; 
      char *p=&(f[6]);
      struct value_s res;

      int ret=eval_string(p,&res,ListNomsValeursPtr);
//      fprintf(stderr,"EVAL de %s : ", p); fprintf(stderr, "%d => RES : ", ret); printVal(&res); fprintf(stderr,"\n");
      if(ret==0 && res.type==1)
      {
         f[l-1]=0;
         v->type=2;
         HASH_FIND_STR(inputs_table, res.val.strval, e);
         if(e)
         {
            if(e->state==RAISE)
               v->val.booleanval=1;
            else
               v->val.booleanval=0;
         }
         retour=0;
         goto eval_function_clean_exit;
      }
      else
      {
         goto eval_function_clean_exit;
      }
   }
   else
   {
      goto eval_function_clean_exit;
   }
   
eval_function_clean_exit:
   if(f)
     free(f);
   return retour;
}


static int eval_string(char *str, struct value_s *v, xPL_NameValueListPtr ListNomsValeursPtr)
{
   if(str[0]=='#') // une constante numérique
   {
      double f=0;
      if(getNumber(&str[1], &f)==0)
      {
         v->type=0;
         v->val.floatval=f;
      }
      else
         return -1;
   }
   else if(str[0]=='\'') // une constante chaine de caractères
   {
      int l=strlen(str);
      if(str[l-1]!='\'')
         return -1;

      l=l-2;
      if(l>sizeof(v->val.strval)-1)
         l=sizeof(v->val.strval)-1;
      strncpy(v->val.strval, &(str[1]), l);
      v->val.strval[l]=0; // pour supprimer le "'" en fin de chaine
      v->type=1;
      return 0;
   }
   else if(str[0]=='&') // une constante booleen
   {
      char b=0;
      if(getBoolean(&str[1], &b)==0)
      {
         v->type=2;
         v->val.booleanval=b;
      }
      else
         return -1;
      return 0;
   }
   else if(str[0]=='$')
   {
      int ret=eval_function(&(str[1]), v, ListNomsValeursPtr);
      return ret;
   }
   else if(str[0]=='{')
   {
      int l=strlen(str);
      if(str[l-1]!='}')
         return -1;

      char name[20];
      struct inputs_table_s *e = NULL;

      if(l>sizeof(name))
         l=sizeof(name);
      strncpy(name, &(str[1]), l-2);
      name[l-2]=0;

      HASH_FIND_STR(inputs_table, name, e);
     if(e)
        memcpy(v, &(e->v), sizeof(struct value_s));
     else
        return -1;
     return 0;
   }
   else
   {
      char *_value=xPL_getNamedValue(ListNomsValeursPtr, str);
      if(_value!=NULL)
         setValueFromStr(v, _value);
      else
         return -1;
      return 0;
   }
   return 0;
}


int sendxpl_from_rule(cJSON *parameters)
{
   if(parameters==NULL || parameters->child==NULL)
      return -1;
   
   cJSON *e=parameters->child;

   char schema[40]="control.basic";
   char target[40]="*";
   
   while(e)
   {
      struct value_s v;
      
      if(eval_string(e->child->valuestring, &v, NULL)==0)
      {
         if(strcmp(e->child->string,"schema")==0)
         {
            strncpy(schema, e->child->valuestring, sizeof(schema)-1);
            schema[sizeof(schema)-1]=0;
         }
         else if(strcmp(e->child->string,"source")==0)
         {
            fprintf(stderr,"   source no allowed\n");
            return -1;
         }
         else if(strcmp(e->child->string,"target")==0)
         {
            strncpy(target, e->child->valuestring, sizeof(target)-1);
            schema[sizeof(target)-1]=0;
         }
         else
         {
            fprintf(stderr,"   %s : ", e->child->string);
            printVal(&v);
            fprintf(stderr,"\n");
         }
      }
      else
      {
         fprintf(stderr,"   %s : ????\n", e->child->string);
         return -1;
      }
      e=e->next;
   }
   
   fprintf(stderr,"   schema=%s\n",schema);
   fprintf(stderr,"   target=%s\n",target);
   
   return 0;
}


int automator_play_output_rules(cJSON *rules)
{
   if(rules==NULL)
   {
      fprintf(stderr,"NO OUTPUT RULE\n");
      return -1;
   }

   double start=mea_now();

   cJSON *e=rules->child;
   while(e) // balayage des règles
   {
/*
      \"name\": \"A1\", \
      \"action\" : \"send\", \
      \"parameters\" : [ \
         { \"schema\"  : \"'control.basic'\" }, \
         { \"device\"  : \"'toto'\" }, \
         { \"current\" : \"{E1}\" } \
      ], \
      \"condition\" : { \"'C1'\" : \"raise\"} \

*/
      cJSON *name       = cJSON_GetObjectItem(e,"name");
      cJSON *action     = cJSON_GetObjectItem(e,"action");
      cJSON *parameters = cJSON_GetObjectItem(e,"parameters");
      cJSON *condition  = cJSON_GetObjectItem(e,"condition");
      
      fprintf(stderr,"%s\n",name->valuestring);
      if(strcmp(action->valuestring,"xPLSend")==0)
      {
         fprintf(stderr,"xPLSend :\n");
         sendxpl_from_rule(parameters);
      }
      else
      {
         fprintf(stderr,"unknown action\n");
         goto next_rule;
      }
      
      struct value_s v;
      if(eval_string(condition->child->string, &v, NULL)!=0)
      {
         fprintf(stderr,"%s : can't eval\n", condition->child->string);
         goto next_rule;
      }
      if(v.type!=1)
      {
         fprintf(stderr,"%s : evaluation result not a string\n", condition->child->string);
         goto next_rule;
      }
      
      struct inputs_table_s *i = NULL;
      HASH_FIND_STR(inputs_table, v.val.strval, i);
      if(i==NULL)
      {
         fprintf(stderr,"Input rule (%s) not found\n", v.val.strval);
         goto next_rule;
      }
      
      if(strcmp(condition->child->valuestring, "raise")==0)
      {
         if(i->state==RAISE)
            fprintf(stderr, "OK raise => send\n");
         else
            fprintf(stderr, "don't raise\n");
      }
      else if(strcmp(condition->child->valuestring, "fall")==0)
      {
         if(i->state==FALL)
            fprintf(stderr, "OK fall => send\n");
         else
            fprintf(stderr, "don't fall\n");
      }
      else if(strcmp(condition->child->valuestring, "change")==0)
      {
         if(i->state==CHANGE || i->state==RAISE || i->state==FALL)
            fprintf(stderr, "OK change => send\n");
         else
            fprintf(stderr, "don't change\n");
      }
      else
      {
         fprintf(stderr,"condition error\n");
         goto next_rule;
      }
next_rule:
      e=e->next;
   }
   
   double now=mea_now();
   fprintf(stderr,"\nrule processing time=%f\n", now-start);

   return 0;
}

int automator_match_inputs_rules(cJSON *rules, xPL_MessagePtr message)
{
   if(rules==NULL)
   {
      fprintf(stderr,"NO INPUT RULE\n");
      return -1;
   }

   double start=mea_now();

   xPL_NameValueListPtr ListNomsValeursPtr = NULL;
   char *schema_type = NULL, *schema_class = NULL, *vendor = NULL, *deviceID = NULL, *instanceID = NULL;
   char source[80]="";
   char schema[80]="";
   
   if(message)
   {
      schema_class = xPL_getSchemaClass(message);
      schema_type  = xPL_getSchemaType(message);
      vendor       = xPL_getSourceVendor(message);
      deviceID     = xPL_getSourceDeviceID(message);
      instanceID   = xPL_getSourceInstanceID(message);

      ListNomsValeursPtr = xPL_getMessageBody(message);
      
      sprintf(source,"%s-%s.%s", vendor, deviceID, instanceID);
      sprintf(schema,"%s.%s", schema_class, schema_type);
   }

   cJSON *e=rules->child;
   while(e) // balayage des règles
   {
      int match=1;
      
      struct value_s res, val1, val2;
      
      cJSON *name    = cJSON_GetObjectItem(e,"name");
      cJSON *value   = cJSON_GetObjectItem(e,"value");
      cJSON *onmatch = cJSON_GetObjectItem(e,"onmatch");
      
      if(!name || !value || !onmatch)
         continue;

//      fprintf(stderr,"\nRULE : %s\n", name->valuestring);
      int ret=eval_string(value->valuestring, &res, ListNomsValeursPtr);
      if(ret<0)
      {
//         fprintf(stderr,"   [%s] not found\n", value->valuestring);
         match=0;
         goto next_rule;
      }
//      fprintf(stderr,"   RES = "); printVal(&res); fprintf(stderr," (%s)\n",  value->valuestring);
      // évaluation des conditions
      cJSON *conditions=cJSON_GetObjectItem(e,"conditions");
      if(conditions!=NULL)
      {
//         fprintf(stderr,"   CONDITIONS : \n");
         cJSON *c=conditions->child;
         while(c)
         {
            cJSON *op = cJSON_GetObjectItem(c, "op");
            if(!op)
            {
               match=0;
               goto next_rule;
            }
            cJSON *value2 = cJSON_GetObjectItem(c, "value");
            if( !value2 || (eval_string(value2->valuestring, &val2, ListNomsValeursPtr)<0) )
            {
               match=0;
               goto next_rule;
            }

            // cas spécifique pour source et schema
            if(strcmp(c->string, "source")==0)
            {
               setValueFromStr(&val1, source);
               int ret=operation(&val1, op->valuestring, &val2);
//               fprintf(stderr,"   "); printVal(&val1); fprintf(stderr," %s ",  op->valuestring); printVal(&val2); fprintf(stderr," ret=%d\n",ret);
               if(!ret)
               {
                  match=0;
                  break;
               }
            }
            else if(strcmp(c->string, "schema")==0)
            {
               setValueFromStr(&val1, schema);
               int ret=operation(&val1, op->valuestring, &val2);
//               fprintf(stderr,"   "); printVal(&val1); fprintf(stderr," %s ",  op->valuestring); printVal(&val2); fprintf(stderr," ret=%d\n",ret);
               if(!ret)
               {
                  match=0;
                  break;
               }
            }
            // cas général pour tous le reste
            else
            {
               int ret=eval_string(c->string, &val1, ListNomsValeursPtr);
               if(ret<0)
               {
                  match=0;
                  goto next_rule;
               }
               ret=operation(&val1, op->valuestring, &val2);
//               fprintf(stderr,"   "); printVal(&val1); fprintf(stderr," %s ",  op->valuestring); printVal(&val2); fprintf(stderr," ret=%d\n",ret);
               if(!ret)
               {
                  match=0;
                  break;
               }
            }
            c=c->next;
         }
      }
      else
      {
//         fprintf(stderr,"   NO CONDITION\n");
      }

   next_rule:
      if(match==1)
      {
//         fprintf(stderr,"   MATCH !\n");
         if(strcmp(name->valuestring, "<NOP>")!=0)
            automator_add_to_inputs_table(name->valuestring, &res);
         else
         {
//            fprintf(stderr, "   Result discarded\n");
         }
         if(onmatch==NULL)
         {
            break;
         }
         else
         {
            if(strcmp(onmatch->valuestring,"break")==0)
            {
               break;
            }
            else if(strcmp(onmatch->valuestring,"continue")==0)
            {
            }
            else
            {
               // jump to rule à écrire
            } 
         }
      }
      else
      {
//         fprintf(stderr,"   NOT MATCH !\n");
      }
      e=e->next;
   }
   double now=mea_now();
   fprintf(stderr,"\nrule processing time=%f\n", now-start);

   automator_print_inputs_table();
   
   return 0;
}


cJSON *automator_load_rules(char *rules)
{
   cJSON *rules_json = cJSON_Parse(rules);

   return rules_json;
}


int automator_add_to_inputs_table(char *_name, struct value_s *v)
{
   struct inputs_table_s *e = NULL;

   HASH_FIND_STR(inputs_table, _name, e);
   if(e)
   {
      enum input_state_e state = UNKNOWN;
      
      if((e->v.type == v->type) && v->type!=1)
      {
         if(e->v.type == 0) // double
         {
            if(e->v.val.floatval == v->val.floatval)
               state=STAY;
            else if(e->v.val.floatval < v->val.floatval)
               state=RAISE;
            else
               state=FALL;
         }
         else // boolean
         {
            if(e->v.val.booleanval == v->val.booleanval)
               state=STAY;
            else if(e->v.val.booleanval < v->val.booleanval)
               state=RAISE;
            else
               state=FALL;
         }
      }
      else if((e->v.type == v->type) && v->type==1)
      {
         if(strcmp(e->v.val.strval, v->val.strval)==0)
            state=STAY;
         else
            state=CHANGE;
      }
      else
         state=TYPECHANGE;

      memcpy(&(e->v), v,sizeof(struct value_s));
      e->state = state;
   }
   else
   {
      struct inputs_table_s *s=(struct inputs_table_s *)malloc(sizeof(struct inputs_table_s));
      strncpy(s->name, _name, sizeof(s->name));
      s->name[sizeof(s->name)-1]=0;
      s->state=NEW;
      memcpy(&(s->v), v, sizeof(struct value_s));

      HASH_ADD_STR(inputs_table, name, s);
   }
   return 0;
}


int automator_reset_inputs_change_flags()
{
   struct inputs_table_s *s;

   for(s=inputs_table; s != NULL; s=s->hh.next)
      s->state=STAY
      ;
   
   return 0;
}


int automator_print_inputs_table()
{
   struct inputs_table_s *s;

   fprintf(stderr,"INPUTS TABLE :\n");
   fprintf(stderr,"--------------\n");
   for(s=inputs_table; s != NULL; s=s->hh.next)
   {
      fprintf(stderr,"rule %s: ", s->name);
      printVal(&(s->v));
      fprintf(stderr," (%d)\n", s->state);
   }
   fprintf(stderr,"--------------\n");
   
   return 0;
}


int automator_init()
{
   inputs_table = NULL;

   return 0;
}

