#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
#include <unistd.h> // close
#include <signal.h>
#include <sys/wait.h>

#include "globals.h"
#include "consts.h"
#include "debug.h"
#include "error.h"
#include "string_utils.h"
#include "sockets_utils.h"

#include "processManager.h"


pid_t _pid_nodejs=0;
int  _socketio_port=0;
int  _socketdata_port=0;


int get_nodejsServer_socketio_port()
{
   return _socketio_port;
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
               fprintf(stderr,"%s (%s) : nodejs not responding.\n", ERROR_STR, __func__);
            }
         }
         close(s);
      }
      else
      {
         close(s);
         return -1;
      }
   }
   return -1;
}


void stop_nodejsServer()
{
   int status;
   
   if(_pid_nodejs>0)
   {
      kill(_pid_nodejs, SIGKILL);
      wait(&status);
      _pid_nodejs=-1;
   }
   
   VERBOSE(1) fprintf(stderr,"%s  (%s) : nodejs %s.\n", INFO_STR, __func__,stopped_successfully_str);
   mea_notify_printf('S', "nodejs %s.",stopped_successfully_str);

}


pid_t start_nodejsServer(char *nodejs_path, char *eventServer_path, int port_socketio, int port_socketdata, char *phpsession_path)
{
   pid_t nodejs_pid = -1;
   nodejs_pid = fork();
   
   if (nodejs_pid == 0) // child
   {
      // Code only executed by child process
      char str_port_socketio[20];
      char str_port_socketdata[20];
      char str_phpsession_path[256];
     
      char *params[6];
     
      sprintf(str_port_socketio,"--iosocketport=%d",port_socketio);
      sprintf(str_port_socketdata,"--dataport=%d",port_socketdata);
      sprintf(str_phpsession_path,"--phpsession_path=%s",phpsession_path);
     
      params[0]="mea-edomus[nodejs]";
      params[1]=eventServer_path;
      params[2]=str_port_socketio;
      params[3]=str_port_socketdata;
      params[4]=str_phpsession_path;
      params[5]=0;
     
      execvp(nodejs_path, params);

      VERBOSE(1) fprintf(stderr,"%s (%s) : can't start nodejs Server (execvp).\n", ERROR_STR, __func__);
      perror("");
     
      exit(1);
   }
   else if (nodejs_pid < 0)
   { // failed to fork
      VERBOSE(1) fprintf(stderr,"%s (%s) : can't start nodejs server (fork).\n", ERROR_STR, __func__);
      perror("");
      return -1;
   }
   // Code only executed by parent process
   VERBOSE(1) fprintf(stderr,"%s  (%s) : nodejs server %s.\n", INFO_STR, __func__, launched_successfully_str);
   mea_notify_printf('S', "nodejs server %sy.", launched_successfully_str);
   return nodejs_pid;
}


