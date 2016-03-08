#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>

#include "nodejsServer.h"

#include "globals.h"
#include "consts.h"
//#include "debug.h"
//#include "error.h"
#include "mea_verbose.h"
#include "mea_string_utils.h"
#include "mea_sockets_utils.h"

#include "processManager.h"
#include "notify.h"
#include "nodejsServer.h"


pid_t _pid_nodejs=0;
int  _socketio_port=0;
int  _socketdata_port=0;
int  _nodejsServer_monitoring_id=-1;


int nodejsServer_ping()
{
   // heartbeat du nodejs ...
   if(_socketio_port<0)
      return -1;
   if(nodejsServer_send_cmnd(localhost_const, _socketio_port, 'P', "PING")<0)
   {
      return -1;
   }
   process_heartbeat(_nodejsServer_monitoring_id); // le heartbeat est fait de l'extérieur ...
   return 0;
}


int get_nodejsServer_socketio_port()
{
   return _socketio_port;
}


int get_nodejsServer_socketdata_port()
{
   return _socketdata_port;
}


int nodejsServer_send_cmnd(char *hostname, int port, char cmnd, char *str)
{
   int s;
   if( mea_socket_connect(&s, hostname, port)==0 )
   {
      int str_l=strlen(str)+6;
      char message[256];
      snprintf(message,sizeof(message)-8,"$$$%c%cINT:%c:%s###", (char)(str_l%128), (char)(str_l/128), cmnd, str);
      int ret = mea_socket_send(&s, message, str_l+8);
      if(ret==0)
      {
         int l=mea_socket_read(&s, message, sizeof(message), 1); // avec timeout de 1s
         message[l]=0;
         if(l<0)
         {
            VERBOSE(1) {
               mea_log_printf("%s (%s) : nodejs not responding.\n", ERROR_STR, __func__);
            }
         }
         close(s);
         return 0;
      }
      else
      {
         close(s);
         return -1;
      }
   }
   return -1;
}


// void stop_nodejsServer()
int stop_nodejsServer(int my_id, void *data, char *errmsg, int l_errmsg)
{
   int status;
   
   if(_pid_nodejs>0)
   {
      kill(_pid_nodejs, SIGKILL);
      sleep(1);
      wait(&status);

      fprintf(stderr,"WIFEXITED   = %d\n", WIFEXITED(status));
      fprintf(stderr,"WEXITSTATUS = %d\n", WEXITSTATUS(status));
      fprintf(stderr,"WIFSIGNALED = %d\n", WIFSIGNALED(status));
      _pid_nodejs=-1;
   }
   
   VERBOSE(1) mea_log_printf("%s (%s) : nodejsServer %s.\n", INFO_STR, __func__,stopped_successfully_str);
   mea_notify_printf('S', "nodejsServer %s.",stopped_successfully_str);
   
   return 0;
}


// pid_t start_nodejsServer(char *nodejs_path, char *eventServer_path, int port_socketio, int port_socketdata, char *phpsession_path)
int start_nodejsServer(int my_id, void *data, char *errmsg, int l_errmsg)
{
   struct nodejsServerData_s *nodejsServerData = (struct nodejsServerData_s *)data;
   char *nodejs_path;
   char *phpsession_path;
   char serverjs_path[256];

   if(nodejsServerData->params_list[NODEJSIOSOCKET_PORT] == NULL ||
      nodejsServerData->params_list[NODEJSDATA_PORT] == NULL     ||
      nodejsServerData->params_list[NODEJS_PATH] == NULL         ||
      nodejsServerData->params_list[LOG_PATH] == NULL)
   {
      VERBOSE(3) {
         mea_log_printf("%s (%s) : parameters error ...",ERROR_STR,__func__);
      }
      return -1;
   }
   else
   {
      _socketio_port = atoi(nodejsServerData->params_list[NODEJSIOSOCKET_PORT]);
      _socketdata_port = atoi(nodejsServerData->params_list[NODEJSDATA_PORT]);
      nodejs_path = nodejsServerData->params_list[NODEJS_PATH];
      phpsession_path = nodejsServerData->params_list[PHPSESSIONS_PATH];

      int n=snprintf(serverjs_path, sizeof(serverjs_path), "%s/nodeJS/server/server", nodejsServerData->params_list[GUI_PATH]);
      if(n<0 || n==sizeof(serverjs_path))
      {
         VERBOSE(3) {
            mea_log_printf("%s (%s) : snprintf - ", ERROR_STR,__func__);
            perror("");
         }
         return -1;
      }
   }

   _pid_nodejs = fork();
   
   if (_pid_nodejs == 0) // child
   {
      // Code only executed by child process
      char str_port_socketio[40];
      char str_port_socketdata[40];
      char str_phpsession_path[256];
     
      char *cmd_line_params[6];
     
      sprintf(str_port_socketio,"--iosocketport=%d",_socketio_port);
      sprintf(str_port_socketdata,"--dataport=%d",_socketdata_port);
      sprintf(str_phpsession_path,"--phpsession_path=%s",phpsession_path);
     
//      cmd_line_params[0]="mea-edomus[nodejs]";
      cmd_line_params[0]="mea-edomus";
      cmd_line_params[1]=serverjs_path;
      cmd_line_params[2]=str_port_socketio;
      cmd_line_params[3]=str_port_socketdata;
      cmd_line_params[4]=str_phpsession_path;
      cmd_line_params[5]=0;
     
      execvp(nodejs_path, cmd_line_params);

      VERBOSE(1) mea_log_printf("%s (%s) : can't start nodejs Server (execvp).\n", ERROR_STR, __func__);
      perror("");
     
      exit(1);
   }
   else if (_pid_nodejs < 0)
   { // failed to fork
      VERBOSE(1) mea_log_printf("%s (%s) : can't start nodejs server (fork).\n", ERROR_STR, __func__);
      perror("");
      return -1;
   }
   
   // Code only executed by parent process
   // attendre le démarrage de nodejs (port de data ouvert)
   int s;
   int nb=0;
   do
   {
      sleep(1);
      nb++;
      if(nb>30) // 30 secondes pour demarrer nodejs
         return -1;
   }
   while(mea_socket_connect(&s, localhost_const, _socketdata_port));
   close(s);

   _nodejsServer_monitoring_id=my_id;
   
   VERBOSE(2) mea_log_printf("%s (%s) : nodejs server %s.\n", INFO_STR, __func__, launched_successfully_str);
   mea_notify_printf('S', "nodejs server %s.", launched_successfully_str);
   
   return 0;
}


int restart_nodejsServer(int my_id, void *data, char *errmsg, int l_errmsg)
{
   int ret=0;
   ret=stop_nodejsServer(my_id, data, errmsg, l_errmsg);
   if(ret==0)
   {
      return start_nodejsServer(my_id, data, errmsg, l_errmsg);
   }
   return ret;
}
