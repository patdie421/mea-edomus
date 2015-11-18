//
//  interface_type_006.c
//  mea-edomus
//
//  Created by Patrice Dietsch on 16/11/2015.
//
//
#include <Python.h>

#include "interface_type_006.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <termios.h>
#include <time.h>
#include <sys/time.h>
#include <signal.h>
#include <sqlite3.h>
#include <errno.h>
#include <pthread.h>

#include "globals.h"
#include "consts.h"

#include "serial.h"

#include "tokens.h"
#include "tokens_da.h"

#include "mea_verbose.h"
#include "macros.h"

#include "dbServer.h"
#include "parameters_utils.h"
#include "mea_api.h"
#include "pythonPluginServer.h"
#include "python_utils.h"

#include "processManager.h"
#include "notify.h"

#include "interfacesServer.h"
#include "interface_type_006.h"


char *interface_type_006_senttoplugin_str="SENT2PLUGIN";
char *interface_type_006_xplin_str="XPLIN";
char *interface_type_006_serialin_str="SERIALIN";


typedef void (*thread_f)(void *);

// parametres valide pour les capteurs ou actionneurs pris en compte par le type 2.
char *valid_genericserial_plugin_params[]={"S:PLUGIN","S:PARAMETERS", NULL};
#define GENERICSERIAL_PLUGIN_PARAMS_PLUGIN      0
#define GENERICSERIAL_PLUGIN_PARAMS_PARAMETERS  1


struct genericserial_callback_xpl_data_s
{
   PyThreadState  *mainThreadState;
   PyThreadState  *myThreadState;
   sqlite3        *param_db;
};


struct genericserial_thread_params_s
{
//   int                   fd;
   sqlite3              *param_db;
//   pthread_mutex_t       callback_lock;
//   pthread_cond_t        callback_cond;
   PyThreadState        *mainThreadState;
   PyThreadState        *myThreadState;
   parsed_parameters_t  *plugin_params;
   int                   nb_plugin_params;
   sqlite3_stmt         *stmt;
   interface_type_006_t *i006;
};


void set_interface_type_006_isnt_running(void *data)
{
   interface_type_006_t *i006 = (interface_type_006_t *)data;

   i006->thread_is_running=0;
}

/*
int _serial_open(char *dev, int speed)
{
   struct termios options, options_old;
   int fd;

   // ouverture du port
   int flags;

   flags=O_RDWR | O_NOCTTY | O_NDELAY | O_EXCL;
#ifdef O_CLOEXEC
   flags |= O_CLOEXEC;
#endif

   fd = open(dev, flags);
   if (fd == -1)
   {
      // ouverture du port serie impossible
      return -1;
   }

   // sauvegarde des caractéristiques du port serie
   tcgetattr(fd, &options_old);

   // initialisation à 0 de la structure des options (termios)
   memset(&options, 0, sizeof(struct termios));

   // paramétrage du débit
   if(cfsetispeed(&options, speed)<0)
   {
      // modification du debit d'entrée impossible
      return -1;
   }
   if(cfsetospeed(&options, speed)<0)
   {
      // modification du debit de sortie impossible
      return -1;
   }

   // ???
   options.c_cflag |= (CLOCAL | CREAD); // mise à 1 du CLOCAL et CREAD

   // 8 bits de données, pas de parité, 1 bit de stop (8N1):
   options.c_cflag &= ~PARENB; // pas de parité (N)
   options.c_cflag &= ~CSTOPB; // 1 bit de stop seulement (1)
   options.c_cflag &= ~CSIZE;
   options.c_cflag |= CS8; // 8 bits (8)

   // bit ICANON = 0 => port en RAW (au lieu de canonical)
   // bit ECHO =   0 => pas d'écho des caractères
   // bit ECHOE =  0 => pas de substitution du caractère d'"erase"
   // bit ISIG =   0 => interruption non autorisées
   options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);

   // pas de contrôle de parité
   options.c_iflag &= ~INPCK;

   // pas de contrôle de flux
   options.c_iflag &= ~(IXON | IXOFF | IXANY);

   // parce qu'on est en raw
   options.c_oflag &=~ OPOST;

   // VMIN : Nombre minimum de caractère à lire
   // VTIME : temps d'attentes de données (en 10eme de secondes)
   // à 0 car O_NDELAY utilisé
   options.c_cc[VMIN] = 0;
   options.c_cc[VTIME] = 0;

   // réécriture des options
   tcsetattr(fd, TCSANOW, &options);

   // préparation du descripteur

   return fd;
}
*/

int16_t _interface_type_006_xPL_callback(xPL_ServicePtr theService, xPL_MessagePtr xplMsg, xPL_ObjectPtr userValue)
{
   xPL_NameValueListPtr xplBody;
   char *device;
   int ret;
   int err;
   
   interface_type_006_t *interface=(interface_type_006_t *)userValue;
   struct genericserial_callback_xpl_data_s *params=(struct genericserial_callback_xpl_data_s *)interface->xPL_callback_data;
   
   interface->indicators.xplin++;
   
   sqlite3 *params_db = params->param_db;
   
   if(!params->mainThreadState)
   {
      params->mainThreadState=PyThreadState_Get();
   }
   if(!params->myThreadState)
      params->myThreadState = PyThreadState_New(params->mainThreadState->interp);
   
   xplBody = xPL_getMessageBody(xplMsg);
   device  = xPL_getNamedValue(xplBody, XPL_DEVICE_STR_C);
   
   if(!device)
      return -1;
   
   char sql[1024];
   sqlite3_stmt * stmt;
   
   sprintf(sql,"%s WHERE lower(sensors_actuators.name)='%s' and sensors_actuators.state='1';", sql_select_device_info, device);
   ret = sqlite3_prepare_v2(params_db, sql, strlen(sql)+1, &stmt, NULL);
   if(ret)
   {
      VERBOSE(2) mea_log_printf("%s (%s) : sqlite3_prepare_v2 - %s\n", ERROR_STR, __func__, sqlite3_errmsg (params_db));
      return -1;
   }
   
   while(1)
   {
      int s = sqlite3_step(stmt);
      if (s == SQLITE_ROW)
      {
         parsed_parameters_t *plugin_params=NULL;
         int nb_plugin_params;
        
         plugin_params=alloc_parsed_parameters((char *)sqlite3_column_text(stmt, 3), valid_genericserial_plugin_params, &nb_plugin_params, &err, 0);
         if(!plugin_params || !plugin_params->parameters[GENERICSERIAL_PLUGIN_PARAMS_PLUGIN].value.s)
         {
            if(plugin_params)
               release_parsed_parameters(&plugin_params);

            continue; // si pas de parametre (=> pas de plugin) ou pas de fonction ... pas la peine d'aller plus loin pour ce capteur
         }
         plugin_queue_elem_t *plugin_elem = (plugin_queue_elem_t *)malloc(sizeof(plugin_queue_elem_t));
         if(plugin_elem)
         {
            plugin_elem->type_elem=XPLMSG;
            
            { // appel des fonctions Python
               PyEval_AcquireLock();
               PyThreadState *tempState = PyThreadState_Swap(params->myThreadState);
               
               plugin_elem->aDict=mea_stmt_to_pydict_device(stmt);
 
               // 
               // données (capteur/actionneur et xpl) à transmettre au plugin à préparer ici
               // 

               PyThreadState_Swap(tempState);
               PyEval_ReleaseLock();
            } // fin appel des fonctions Python
            
            pythonPluginServer_add_cmd(plugin_params->parameters[GENERICSERIAL_PLUGIN_PARAMS_PLUGIN].value.s, (void *)plugin_elem, sizeof(plugin_queue_elem_t));
            interface->indicators.senttoplugin++;
            free(plugin_elem);
            plugin_elem=NULL;
         }
         
         release_parsed_parameters(&plugin_params);
         plugin_params=NULL;
      }
      else if (s == SQLITE_DONE)
      {
         sqlite3_finalize(stmt);
         break;
      }
      else
      {
         sqlite3_finalize(stmt);
         break; // voir autres erreurs possibles
      }
   }
   return 0;
}


void *_thread_interface_type_006_genericserial_data_cleanup(void *args)
{
   struct genericserial_thread_params_s *params=(struct genericserial_thread_params_s *)args;

   if(!params)
      return NULL;
   if(params->myThreadState)
   {
      PyEval_AcquireLock();
      PyThreadState_Clear(params->myThreadState);
      PyThreadState_Delete(params->myThreadState);
      PyEval_ReleaseLock();
      params->myThreadState=NULL;
   }
   
   if(params->plugin_params)
      release_parsed_parameters(&(params->plugin_params));
   
   if(params->stmt)
   {
      sqlite3_finalize(params->stmt);
      params->stmt=NULL;
   }
   
   free(params);
   params=NULL;

   return NULL;
}


void *_thread_interface_type_006_genericserial_data(void *args)
{
   struct genericserial_thread_params_s *params=(struct genericserial_thread_params_s *)args;

   pthread_cleanup_push( (void *)_thread_interface_type_006_genericserial_data_cleanup, (void *)params );
   pthread_cleanup_push( (void *)set_interface_type_006_isnt_running, (void *)params->i006 );
   
   params->i006->thread_is_running=1;
   process_heartbeat(params->i006->monitoring_id);
   
//   sqlite3 *params_db=params->param_db;
//   int ret;
   
   params->plugin_params=NULL;
   params->nb_plugin_params=0;
   params->stmt=NULL;
   
   pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
   PyEval_AcquireLock();
   params->mainThreadState = PyThreadState_Get();
   params->myThreadState = PyThreadState_New(params->mainThreadState->interp);
   PyEval_ReleaseLock();
   pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
   pthread_testcancel();

   mea_timer_t process_timer;

   mea_init_timer(&process_timer, 5, 1);
   mea_start_timer(&process_timer); 

   params->i006->fd=-1; 

   while(1)
   {
      if(mea_test_timer(&process_timer)==0)
      {
         process_heartbeat(params->i006->monitoring_id);
         process_update_indicator(params->i006->monitoring_id, interface_type_006_senttoplugin_str, params->i006->indicators.senttoplugin);
         process_update_indicator(params->i006->monitoring_id, interface_type_006_xplin_str, params->i006->indicators.xplin);
         process_update_indicator(params->i006->monitoring_id, interface_type_006_serialin_str, params->i006->indicators.serialin);
//         process_update_indicator(params->i006->monitoring_id, interface_type_006_serialout_str, params->i006->indicators.serialout);
      }

      if(params->i006->fd<0)
      {
         params->i006->fd=serial_open(params->i006->real_dev, params->i006->real_speed);
         if(params->i006->fd < 0)
         {
            VERBOSE(5) mea_log_printf("%s (%s) : can't open %s - ", ERROR_STR, __func__, params->i006->real_dev);
            perror("");
         }
      }

      if(params->i006->fd>=0)
      {

         char buffer[4096];
         int buffer_ptr=0;
         struct timeval timeout;
         fd_set input_set;
         char c;

         FD_ZERO(&input_set);
         FD_SET(params->i006->fd, &input_set);
         while(1)
         {
            timeout.tv_sec  = 0; // timeout après
            timeout.tv_usec = 100000; // 100 ms

            int ret = select(params->i006->fd+1, &input_set, NULL, NULL, &timeout);
            if (ret <= 0)
            {
               if(ret == 0)
               {
                  if(buffer_ptr>0)
                     break; // après un "blanc" de 100 ms, si on a des données, on les envoie au plugin
               }
               else
               {
                  // erreur à traiter ...
                  perror("");
                  close(params->i006->fd);
                  params->i006->fd=-1;
                  break;
               }
            }
            ret=read(params->i006->fd, &c, 1);
            if(ret!=-1)
            {
               buffer[buffer_ptr++]=c;
               if(buffer_ptr >= sizeof(buffer)-1)
                  break;
            }
         }

         if(buffer_ptr>0)
         {
            params->i006->indicators.serialin+=buffer_ptr;
         
            DEBUG_SECTION mea_log_printf("%s (%s) : data from Serial : %s\n", INFO_STR, __func__, buffer);
         
            // transmettre buffer au plugin
            buffer[buffer_ptr]=0;
            fprintf(stderr,"%s\n",buffer);

            buffer_ptr=0;

            pthread_testcancel();
         }

         pthread_testcancel();
      }
      else
      {
         sleep(5);
      }

      pthread_testcancel();
   }
   
   pthread_cleanup_pop(1);
   pthread_cleanup_pop(1);
   
   return NULL;
}


pthread_t *start_interface_type_006_genericserial_data_thread(interface_type_006_t *i006, sqlite3 *db, thread_f function)
{
   pthread_t *thread=NULL;
   struct genericserial_thread_params_s *params=NULL;
   struct genericserial_callback_data_s *genericserial_callback_data=NULL;
   
   params=malloc(sizeof(struct genericserial_thread_params_s));
   if(!params)
   {
      VERBOSE(2) {
         mea_log_printf("%s (%s) : %s - ", ERROR_STR, __func__, MALLOC_ERROR_STR);
         perror("");
      }
      goto clean_exit;
   }

   params->param_db=db;
//   pthread_mutex_init(&params->callback_lock, NULL);
//   pthread_cond_init(&params->callback_cond, NULL);
   params->i006=(void *)i006;
   params->mainThreadState = NULL;
   params->myThreadState = NULL;

   thread=(pthread_t *)malloc(sizeof(pthread_t));
   if(!thread)
   {
      VERBOSE(2) mea_log_printf("%s (%s) : %s\n", ERROR_STR, __func__, MALLOC_ERROR_STR);
      goto clean_exit;
   }
   
   if(pthread_create (thread, NULL, (void *)function, (void *)params))
      goto clean_exit;
   
   return thread;
   
clean_exit:
   if(thread)
   {
      free(thread);
      thread=NULL;
   }
   
   if(genericserial_callback_data)
   {
      free(genericserial_callback_data);
      genericserial_callback_data=NULL;
   }

   if(params)
   {
      free(params);
      params=NULL;
   }
   return NULL;
}


int clean_interface_type_006(interface_type_006_t *i006)
{
   if(i006->parameters)
   {
      free(i006->parameters);
      i006->parameters=NULL;
   }

   if(i006->xPL_callback_data)
   {
      free(i006->xPL_callback_data);
      i006->xPL_callback_data=NULL;
   }
   
   if(i006->xPL_callback)
      i006->xPL_callback=NULL;

   if(i006->thread)
   {
      free(i006->thread);
      i006->thread=NULL;
   }
      
   return 0;
}


interface_type_006_t *malloc_and_init_interface_type_006(sqlite3 *sqlite3_param_db, int id_interface, char *name, char *dev, char *parameters, char *description)
{
   interface_type_006_t *i006;
                  
   i006=(interface_type_006_t *)malloc(sizeof(interface_type_006_t));
   if(!i006)
   {
      VERBOSE(2) {
        mea_log_printf("%s (%s) : %s - ",ERROR_STR,__func__,MALLOC_ERROR_STR);
        perror("");
      }
      return NULL;
   }
   i006->thread_is_running=0;
                  
   struct interface_type_006_data_s *i006_start_stop_params=(struct interface_type_006_data_s *)malloc(sizeof(struct interface_type_006_data_s));
   if(!i006_start_stop_params)
   {
      free(i006);
      i006=NULL;
      VERBOSE(2) {
         mea_log_printf("%s (%s) : %s - ",ERROR_STR,__func__,MALLOC_ERROR_STR);
         perror("");
      }  
      return NULL;
   }
   strncpy(i006->dev, (char *)dev, sizeof(i006->dev)-1);
   strncpy(i006->name, (char *)name, sizeof(i006->name)-1);
   i006->id_interface=id_interface;
   i006->parameters=(char *)malloc(strlen((char *)parameters)+1);
   strcpy(i006->parameters,(char *)parameters);
   i006->indicators.senttoplugin=0;
   i006->indicators.xplin=0;
   i006->indicators.serialin=0;
   
   i006->thread=NULL;
   i006->xPL_callback=NULL;
   i006->xPL_callback_data=NULL;

   i006->monitoring_id=process_register((char *)name);
   i006_start_stop_params->sqlite3_param_db = sqlite3_param_db;
   i006_start_stop_params->i006=i006;
                  
   process_set_group(i006->monitoring_id, 1);
   process_set_start_stop(i006->monitoring_id, start_interface_type_006, stop_interface_type_006, (void *)i006_start_stop_params, 1);
   process_set_watchdog_recovery(i006->monitoring_id, restart_interface_type_006, (void *)i006_start_stop_params);
   process_set_description(i006->monitoring_id, (char *)description);
   process_set_heartbeat_interval(i006->monitoring_id, 60); // chien de garde au bout de 60 secondes sans heartbeat

   process_add_indicator(i006->monitoring_id, interface_type_006_senttoplugin_str, 0);
   process_add_indicator(i006->monitoring_id, interface_type_006_xplin_str, 0);
   process_add_indicator(i006->monitoring_id, interface_type_006_serialin_str, 0);

   return i006;
}


int stop_interface_type_006(int my_id, void *data, char *errmsg, int l_errmsg)
{
   if(!data)
      return -1;

   struct interface_type_006_data_s *start_stop_params=(struct interface_type_006_data_s *)data;

   VERBOSE(1) mea_log_printf("%s  (%s) : %s shutdown thread ... ", INFO_STR, __func__, start_stop_params->i006->name);

   if(start_stop_params->i006->xPL_callback_data)
   {
      free(start_stop_params->i006->xPL_callback_data);
      start_stop_params->i006->xPL_callback_data=NULL;
   }
   
   if(start_stop_params->i006->xPL_callback)
   {
      start_stop_params->i006->xPL_callback=NULL;
   }
   

   if(start_stop_params->i006->thread)
   {
      pthread_cancel(*(start_stop_params->i006->thread));
      
      int counter=100;
      int stopped=-1;
      while(counter--)
      {
         if(start_stop_params->i006->thread_is_running)
         {  // pour éviter une attente "trop" active
            usleep(100); // will sleep for 10 ms
         }
         else
         {
            stopped=0;
            break;
         }
      }
      DEBUG_SECTION mea_log_printf("%s (%s) : %s, fin après %d itération(s)\n",DEBUG_STR, __func__,start_stop_params->i006->name,100-counter);

      free(start_stop_params->i006->thread);
      start_stop_params->i006->thread=NULL;
   }
   
   mea_notify_printf('S', "%s %s", start_stop_params->i006->name, stopped_successfully_str);

   return 0;
}


int restart_interface_type_006(int my_id, void *data, char *errmsg, int l_errmsg)
{
   process_stop(my_id, NULL, 0);
   sleep(5);
   return process_start(my_id, NULL, 0);
}


int16_t check_status_interface_type_006(interface_type_006_t *i006)
{
   return 0;
}


int start_interface_type_006(int my_id, void *data, char *errmsg, int l_errmsg)
{
   char dev[81];
   char buff[80];
   speed_t speed;

   int err;
   int ret;
   
   struct genericserial_callback_xpl_data_s *xpl_callback_params=NULL;
   
   struct interface_type_006_data_s *start_stop_params=(struct interface_type_006_data_s *)data;

   start_stop_params->i006->thread=NULL;

   int interface_nb_parameters=0;
   parsed_parameters_t *interface_parameters=NULL;
   
   char err_str[128];
   
   ret=get_dev_and_speed((char *)start_stop_params->i006->dev, buff, sizeof(buff), &speed);
   if(!ret)
   {
      int n=snprintf(dev,sizeof(buff)-1,"/dev/%s",buff);
      if(n<0 || n==(sizeof(buff)-1))
      {
         strerror_r(errno, err_str, sizeof(err_str));
         VERBOSE(2) {
            mea_log_printf("%s (%s) : snprintf - %s\n", ERROR_STR, __func__, err_str);
         }
         mea_notify_printf('E', "%s can't be launched - %s.\n", start_stop_params->i006->name, err_str);
         goto clean_exit;
      }
   }
   else
   {
      VERBOSE(2) mea_log_printf("%s (%s) : incorrect device/speed interface - %s\n", ERROR_STR,__func__,start_stop_params->i006->dev);
      mea_notify_printf('E', "%s can't be launched - incorrect device/speed interface - %s.\n", start_stop_params->i006->name, start_stop_params->i006->dev);
      goto clean_exit;
   }
   strncpy(start_stop_params->i006->real_dev, dev, sizeof( start_stop_params->i006->real_dev)-1);
   start_stop_params->i006->real_speed=speed;

   /*
    * exécution du plugin de paramétrage
    */
   interface_parameters=alloc_parsed_parameters(start_stop_params->i006->parameters, valid_genericserial_plugin_params, &interface_nb_parameters, &err, 0);
   if(!interface_parameters || !interface_parameters->parameters[GENERICSERIAL_PLUGIN_PARAMS_PLUGIN].value.s)
   {
      if(interface_parameters)
      {
         // pas de plugin spécifié
         free(interface_parameters);
         interface_parameters=NULL;
         VERBOSE(9) mea_log_printf("%s  (%s) : no python plugin specified\n", INFO_STR, __func__);
      }
      else
      {
         VERBOSE(5) mea_log_printf("%s (%s) : invalid or no python plugin parameters (%s)\n", ERROR_STR, __func__, start_stop_params->i006->parameters);
      }
   }
   else
   {
      PyObject *plugin_params_dict=NULL;
      PyObject *pName, *pModule, *pFunc;
      PyObject *pArgs, *pValue=NULL;
      
      pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
      PyEval_AcquireLock();
      PyThreadState *mainThreadState=PyThreadState_Get();
      PyThreadState *myThreadState = PyThreadState_New(mainThreadState->interp);
      
      PyThreadState *tempState = PyThreadState_Swap(myThreadState);

      PyErr_Clear();
      pName = PyString_FromString(interface_parameters->parameters[GENERICSERIAL_PLUGIN_PARAMS_PLUGIN].value.s);
      pModule = PyImport_Import(pName);
      if(!pModule)
      {
         VERBOSE(5) mea_log_printf("%s (%s) : %s not found\n", ERROR_STR, __func__, interface_parameters->parameters[GENERICSERIAL_PLUGIN_PARAMS_PLUGIN].value.s);
      }
      else
      {
         pFunc = PyObject_GetAttrString(pModule, "mea_init");
         if (pFunc && PyCallable_Check(pFunc))
         {
            // préparation du parametre du module
            plugin_params_dict=PyDict_New();

            //
            // ajouter les données nécessaires ici
            //

            pArgs = PyTuple_New(1);
            Py_INCREF(plugin_params_dict); // PyTuple_SetItem va voler la référence, on en rajoute une pour pouvoir ensuite faire un Py_DECREF
            PyTuple_SetItem(pArgs, 0, plugin_params_dict);
         
            pValue = PyObject_CallObject(pFunc, pArgs); // appel du plugin
            if (pValue != NULL)
            {
               DEBUG_SECTION mea_log_printf("%s (%s) : Result of call of mea_init : %ld\n", DEBUG_STR, __func__, PyInt_AsLong(pValue));
            }
            Py_DECREF(pValue);
            Py_DECREF(pArgs);
            Py_DECREF(plugin_params_dict);
         }
         else
         {
            VERBOSE(5) mea_log_printf("%s (%s) : mea_init not fount in %s module\n", ERROR_STR, __func__, interface_parameters->parameters[GENERICSERIAL_PLUGIN_PARAMS_PLUGIN].value.s);
         }
         Py_XDECREF(pFunc);
      }
      Py_XDECREF(pModule);
      Py_DECREF(pName);
      PyErr_Clear();
      
      PyThreadState_Swap(tempState);
      PyThreadState_Clear(myThreadState);
      PyThreadState_Delete(myThreadState);
      PyEval_ReleaseLock();
      pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);

      if(interface_parameters)
         release_parsed_parameters(&interface_parameters);
   }

   //
   // lancement du thread à faire ici ...
   //
   start_stop_params->i006->thread=start_interface_type_006_genericserial_data_thread(start_stop_params->i006, start_stop_params->sqlite3_param_db, (thread_f)_thread_interface_type_006_genericserial_data);
//   start_stop_params->i003->thread=start_interface_type_003_enocean_data_thread(start_stop_params->i003, ed, start_stop_params->sqlite3_param_db, (thread_f)_thread_interface_type_003_enocean_data);

   xpl_callback_params=(struct genericserial_callback_xpl_data_s *)malloc(sizeof(struct genericserial_callback_xpl_data_s));
   if(!xpl_callback_params)
   {
      strerror_r(errno, err_str, sizeof(err_str));
      VERBOSE(2) {
         mea_log_printf("%s (%s) : %s - %s\n", ERROR_STR, __func__, MALLOC_ERROR_STR, err_str);
      }
      mea_notify_printf('E', "%s can't be launched - %s.\n", start_stop_params->i006->name, err_str);
      goto clean_exit;
   }
   xpl_callback_params->param_db=start_stop_params->sqlite3_param_db;
   xpl_callback_params->mainThreadState=NULL;
   xpl_callback_params->myThreadState=NULL;
   
   start_stop_params->i006->xPL_callback_data=xpl_callback_params;
   start_stop_params->i006->xPL_callback=_interface_type_006_xPL_callback;
   
   VERBOSE(2) mea_log_printf("%s  (%s) : %s %s.\n", INFO_STR, __func__,start_stop_params->i006->name, launched_successfully_str);
   mea_notify_printf('S', "%s %s", start_stop_params->i006->name, launched_successfully_str);
   
   return 0;
   
clean_exit:
   if(start_stop_params->i006->thread)
   {
      stop_interface_type_006(start_stop_params->i006->monitoring_id, start_stop_params, NULL, 0);
   }

   
   if(interface_parameters)
   {
      release_parsed_parameters(&interface_parameters);
      interface_nb_parameters=0;

   }
   
   if(xpl_callback_params)
   {
      free(xpl_callback_params);
      xpl_callback_params=NULL;
   }
   
   return -1;
}

/*
AT

OK
ATE0

OK
AT+CPIN=1234

ERROR
AT+CMGF=1

OK
AT+CNMI=2,2,0,0,0

OK
AT+CSDH=0

OK

+CMT: "+33661665082","","15/11/17,23:51:28+04"
Info télésurveillance 15/11/2015 15:40:20 : mise en marche via code NATHALIE du clavier ENTREE (zone 1) au 16 RUE JULES GUESDE - ROSNY SOUS

+CMT: "+33661665082","","15/11/17,23:51:52+04"
##4,A,G;7,L,L##
AT

OK
AT+CMGS="+33661665082"

> A/PIN4=688;DONE

> 
+CMGS: 94

OK

+CMT: "+33661665082","","15/11/17,23:54:28+04"
 Info télésurveillance 13/11/2015 08:40:27 : mise à l'arret via code NATHALIE du clavier ENTREE (zone 1) au 16 RUE JULES GUESDE - ROSNY SOUS
*/
