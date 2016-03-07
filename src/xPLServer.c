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
#include <setjmp.h>
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
#include "automator.h"
#include "interfacesServer.h"
#include "automatorServer.h"
#include "mea_sockets_utils.h"
#include "notify.h"
#include "cJSON.h"

#define XPL_VERSION "0.1a2"

extern Bool xPL_sendRawMessage(String, int);

#define XPL_WD 1
#ifdef XPL_WD
char *xplWDMsg = NULL;
int xplWDMsg_l = -1;

mea_timer_t xPLnoMsgReceivedTimer;
#endif


char *xpl_server_name_str="XPLSERVER";
char *xpl_server_xplin_str="XPLIN";
char *xpl_server_xplout_str="XPLOUT";

// gestion du service xPL
xPL_ServicePtr xPLService = NULL;
char *xpl_vendorID=NULL;
char *xpl_deviceID=NULL;
char *xpl_instanceID=NULL;
char xpl_source[36]="";

// gestion du thread et des indicateurs
pthread_t *_xPLServer_thread_id;
jmp_buf xPLServer_JumpBuffer;
int xPLServer_longjmp_flag = 0;
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


void _cmndXPLMessageHandler(xPL_MessagePtr theMessage, xPL_ObjectPtr userValue);


void set_xPLServer_isnt_running(void *data)
{
   _xPLServer_thread_is_running=0;
}


void clean_xPLServer(void *data)
{
   xPL_ServicePtr tmp = xPLService;
   xPLService = NULL;
 
   xPL_removeMessageListener(_cmndXPLMessageHandler);

   xPL_setServiceEnabled(tmp, FALSE);
   xPL_releaseService(tmp);

#ifdef XPL_WD
   if(xplWDMsg)
   {
      free(xplWDMsg);
      xplWDMsg=NULL;
   }
#endif
}


cJSON *mea_xPL2JSON(xPL_MessagePtr msg)
{
   char str[256];
   char *msgtype="";

   cJSON *msg_json = cJSON_CreateObject();

   switch(xPL_getMessageType(msg))
   {
      case xPL_MESSAGE_COMMAND:
         msgtype="xpl-cmnd";
         break;
      case xPL_MESSAGE_STATUS:
         msgtype="xpl-stat";
         break;
      case xPL_MESSAGE_TRIGGER:
         msgtype="xpl-trig";
         break;
   }
   cJSON_AddItemToObject(msg_json, XPLMSGTYPE_STR_C, cJSON_CreateString(msgtype));

   cJSON_AddItemToObject(msg_json, XPLSOURCE_STR_C, cJSON_CreateString(xpl_source));

   strcpy(str,"*");
   if (!xPL_isBroadcastMessage(msg))
      sprintf(str,"%s-%s.%s", xPL_getTargetVendor(msg), xPL_getTargetDeviceID(msg), xPL_getTargetInstanceID(msg));
   cJSON_AddItemToObject(msg_json, XPLTARGET_STR_C, cJSON_CreateString("*"));

   sprintf(str,"%s.%s", xPL_getSchemaClass(msg), xPL_getSchemaType(msg));
   cJSON_AddItemToObject(msg_json, XPLSCHEMA_STR_C, cJSON_CreateString(str));

   xPL_NameValueListPtr body = xPL_getMessageBody(msg);
   int n = xPL_getNamedValueCount(body);
   for (int16_t i = 0; i < n; i++)
   {
      xPL_NameValuePairPtr keyValuePtr = xPL_getNamedValuePairAt(body, i);
      if (keyValuePtr->itemValue != NULL && keyValuePtr->itemName != NULL)
          cJSON_AddItemToObject(msg_json,  keyValuePtr->itemName, cJSON_CreateString(keyValuePtr->itemValue));
   }

   return msg_json;
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


char *mea_getXPLSource()
{
   return xpl_source;
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


int mea_sendXplMsgJson(cJSON *xplMsgJson)
{
   if(xplMsgJson==NULL)
      return -1;

   cJSON *e=xplMsgJson->child;
   if(e==NULL)
      return -1;

   int retour=0;

   char *schema=NULL;
   char *target=NULL;
   char *source=NULL;
   char *type=NULL;

   char xplBodyStr[2048] = "";
   int xplBodyStrPtr = 0;

   source = xpl_source;
   target="*";

   while(e)
   {
      // type de message
      if(strcmp(e->string, XPLMSGTYPE_STR_C)==0)
         type=e->valuestring;
      else if(strcmp(e->string, XPLSCHEMA_STR_C)==0)
         schema=e->valuestring;
      else if(strcmp(e->string, XPLSOURCE_STR_C)==0)
         source=e->valuestring;
      else if(strcmp(e->string, XPLTARGET_STR_C)==0)
         target=e->valuestring;
      else
      {
         int n=sprintf(&(xplBodyStr[xplBodyStrPtr]),"%s=%s\n",e->string,e->valuestring);

         xplBodyStrPtr+=n;
      }
      e=e->next;
   }

   if(!type || !schema || !source || !target || !xplBodyStrPtr)
      return -1;

   char *msg = (char *)alloca(2048);

   int n=sprintf(msg,"%s\n{\nhop=1\nsource=%s\ntarget=%s\n}\n%s\n{\n%s}\n",type,source,target,schema,xplBodyStr);

   if(n>0)
   {
      xPL_sendRawMessage(msg, n);

      return 0;
   }
   return -1;
}


uint16_t mea_sendXPLMessage2(cJSON *xplMsgJson)
{
   char *str = NULL;
   char deviceID[17]="";
   xPL_MessagePtr newXPLMsg = NULL;
   xplRespQueue_elem_t *e;
   cJSON *j = NULL;

   process_update_indicator(_xplServer_monitoring_id, xpl_server_xplout_str, ++xplout_indicator);

   str=NULL;
   deviceID[0]=0;
   j=cJSON_GetObjectItem(xplMsgJson, XPLSOURCE_STR_C);
   if(j)
   {
      str=j->valuestring;
      if(str)
         sscanf(str, "%*[^-]-%[^.].%*s", deviceID);
   }

   if(deviceID[0]!=0 && strcmp(deviceID,INTERNAL_STR_C)==0) // source interne => dispatching sans passer par le réseau
   {
      dispatchXPLMessageToInterfaces2(xplMsgJson);
      return 0;
   }

   str=NULL;
   deviceID[0]=0;
   j=cJSON_GetObjectItem(xplMsgJson, XPLTARGET_STR_C);
   if(j)
   {
      str=j->valuestring;
      if(str)
         sscanf(str, "%*[^-]-%[^.].%*s", deviceID);
   }

   if(deviceID[0]!=0 && strcmp(str,INTERNAL_STR_C)==0) // destination interne, retour à mettre dans une file (avec timestamp) ...
   {
      int id;

      sscanf(str, "%*[^.].%d", &id);
      cJSON *xplMsgJson_new = cJSON_Duplicate(xplMsgJson, 1);

      pthread_cleanup_push( (void *)pthread_mutex_unlock, (void *)&(xplRespQueue_sync_lock) );
      pthread_mutex_lock(&xplRespQueue_sync_lock);

      if(xplRespQueue)
      {
         e = malloc(sizeof(xplRespQueue_elem_t));
         e->msgjson = xplMsgJson_new;
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
      mea_sendXplMsgJson(xplMsgJson);

      return 0;
   }
}


cJSON *mea_readXPLResponse2(int id)
{
   int16_t ret;
   uint16_t notfound=0;
   cJSON *xplMsgJson = NULL;

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
      goto readFromQueue_return;

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
                  xplMsgJson=e->msgjson;
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

   return xplMsgJson;
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


void _cmndXPLMessageHandler(xPL_MessagePtr theMessage, xPL_ObjectPtr userValue)
{
   process_update_indicator(_xplServer_monitoring_id, xpl_server_xplin_str, ++xplin_indicator);

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

   // on envoie tous les messages à l'automate (à lui de filtrer ...)
   cJSON *xplMsgJson_interfaces = mea_xPL2JSON(theMessage);
   cJSON *xplMsgJson_automator = cJSON_Duplicate(xplMsgJson_interfaces, 1);

   if(xplMsgJson_automator)
   {
      if(automatorServer_add_msg(xplMsgJson_automator)==ERROR)
      {
         DEBUG_SECTION mea_log_printf("%s (%s) : to automator error\n", DEBUG_STR, __func__);
      }
   }

   // pour les autres on filtre un peu avant de transmettre pour traitement
   // on ne traite que les cmnd au niveau des interfaces
   if(xPL_getMessageType(theMessage)!=xPL_MESSAGE_COMMAND)
      return;


   dispatchXPLMessageToInterfaces2(xplMsgJson_interfaces);
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
               cJSON_Delete(e->msgjson);
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
   cJSON_Delete(e->msgjson);
   e->msgjson=NULL;
}


void *xPLServer_thread(void *data)
{
   pthread_cleanup_push( (void *)set_xPLServer_isnt_running, (void *)NULL );
   pthread_cleanup_push( (void *)clean_xPLServer, (void *)NULL);
   
   xPLServer_longjmp_flag = 0;
   _xPLServer_thread_is_running=1;
   process_heartbeat(_xplServer_monitoring_id); // 1er heartbeat après démarrage.
   
//   xPL_setDebugging(TRUE); // xPL en mode debug
   
   xPLService = xPL_createService(xpl_vendorID, xpl_deviceID, xpl_instanceID);
   if(!xPLService)
      return NULL;
   sprintf(xpl_source,"%s-%s.%s", xpl_vendorID, xpl_deviceID, xpl_instanceID);

   xPL_setRespondingToBroadcasts(xPLService, TRUE);
   xPL_setServiceVersion(xPLService, XPL_VERSION);
   xPL_setHeartbeatInterval(xPLService, 5000); // en milliseconde

   xPL_addMessageListener(_cmndXPLMessageHandler, NULL);

#ifdef XPL_WD
   mea_timer_t xPLWDSendMsgTimer;
   mea_init_timer(&xPLnoMsgReceivedTimer, 30, 1);
   mea_init_timer(&xPLWDSendMsgTimer, 10, 1);

   char *_xplWDMsg = "xpl-trig\n{\nhop=1\nsource=%s\ntarget=%s\n}\nwatchdog.basic\n{\ninterval=10\n}\n";

   xplWDMsg=malloc(strlen(_xplWDMsg)-4 + 2*strlen(xpl_source) + 1);
   xplWDMsg_l = sprintf(xplWDMsg,_xplWDMsg,xpl_source,xpl_source);

   mea_start_timer(&xPLnoMsgReceivedTimer);
   mea_start_timer(&xPLWDSendMsgTimer);
#endif

   xPL_setServiceEnabled(xPLService, TRUE);

   do
   {
      if(setjmp(xPLServer_JumpBuffer) != 0)
      {
         xPLServer_longjmp_flag++;
         break;
      }

      pthread_testcancel();
      process_heartbeat(_xplServer_monitoring_id); // heartbeat après chaque boucle
      
#ifdef XPL_WD
      if(mea_test_timer(&xPLnoMsgReceivedTimer)==0)
      {
         // pas de message depuis X secondes
         VERBOSE(2) {
            mea_log_printf("%s (%s) : no xPL message since 30 seconds, but we should have one ... I send me three !\n", ERROR_STR, __func__);
            mea_log_printf("%s (%s) : watchdog_recovery forced ...\n", ERROR_STR, __func__);
         }
         process_forced_watchdog_recovery(_xplServer_monitoring_id);
      }
      
      if(mea_test_timer(&xPLWDSendMsgTimer)==0) // envoie d'un message toutes les X secondes
      {
         xPL_sendRawMessage(xplWDMsg, xplWDMsg_l);
      }
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

   if(xPLServer_longjmp_flag > 1)
      abort();

   pthread_cleanup_pop(1);
   pthread_cleanup_pop(1);
   
   return NULL;
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
   fprintf(stderr, "xPL_shutdown\n");

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
