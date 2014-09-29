#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h> // close
#include <netdb.h> // gethostbyname
#include <errno.h>

#include "debug.h"
#include "monitoring.h"

const char *hostname = "localhost";
#define PORT 4756

pid_t pid_nodejs=0;

int connexion(int *s, char *hostname, int port)
{
   int sock;
   struct sockaddr_in serv_addr;
   struct hostent *serv_info = NULL;

   sock = socket(AF_INET, SOCK_STREAM, 0);
   if(sock < 0)
   {
      perror("socket()");
      return -1;
   }

   serv_info = gethostbyname(hostname); // on récupère les informations de l'hôte auquel on veut se connecter
   if(serv_info == NULL)
   {
      perror("gethostbyname()");
      return -1;
   }

   bzero((char *)&serv_addr, sizeof(serv_addr));
   serv_addr.sin_family = AF_INET;
   serv_addr.sin_port   = htons(port);
   bcopy((char *)serv_info->h_addr, (char *)&serv_addr.sin_addr.s_addr, serv_info->h_length);
   
   if(connect(sock, (const struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
   {
      perror("connect()");
      return -1;
   }
   
   *s=sock;
   return 0;
}


int envoie(int *s, char *message)
{
   if(send(*s, message, strlen(message), 0) < 0)
   {
      perror("send()");
      return -1;
   }
   return 0;
}


void readAndSendLine(int nodejs_socket, char *file, long *pos)
{
   FILE *fp=NULL;
   char line[512];

   do
   {
      int ret=0;

      fp = fopen(file, "r");
      if(fp<0)
      {
         VERBOSE(1) perror("");
         return -1;
      }

      if(*pos == -1) // premiere execution, on va à la fin du fichier
      {
         fseek(fp,0,SEEK_END);
      }
      else
      { // sinon on va à la dernière position connue
         if(fseek(fp,*pos,SEEK_SET)==-1)
         {
            // le fichier a été racoursi ... à va à la fin du fichier
            fseek(fp,0,SEEK_END);
         }
      }

      while (1)
      {
         if (fgets(line, sizeof(line), fp) == NULL)
         {
            // plus de ligne à lire, on mémorise la dernière position connue
            *pos=ftell(fp);
            // et on ferme le fichier
            fclose(fp);
            fp=NULL;
            break; 
         }
         else
         {
            fprintf(stderr,"LIGNE : %s\n",ligne);
            ret = envoie(&nodejs_socket, message);
            if(ret<0)
               break;
         }
         // on retournera un peu plus tard
      }
      if(ret<0)
         break;
      sleep(1);
   }
   if(fp)
      close(fp);
   return -1;
}


void monitoringServer(char *logfile)
{
   int exit=0;
   int nodejs_socket=-1;
   char message[1024];
   
   do
   {
     if(connexion(&nodejs_socket, (char *)hostname, PORT)==0)
     {
       int ret;
       long pos = -1;
       do
       {
         ret=readAndSendLine(nodejs_socket, logfile, &pos);
       }
       while(ret==0);
       close(nodejs_socket);
     }
     else
     {
       VERBOSE(5) fprintf(stderr,"Retry next time ...\n");
       sleep(5); // on essayera de se reconnecter dans 5 secondes
     }
   }
   while(exit==0);
}


pid_t start_nodejs(char *nodejs_path, char *eventServer_path, int port_socketio, int port_socketdata)
{
   pid_t nodejs_pid = -1;
   nodejs_pid = fork();

    if (nodejs_pid == 0) // child
    {
       // Code only executed by child process
       char str_port_socketio[32];
       char str_port_socketdata[32];
       char *params[4];

       sprintf(str_port_socketio,"%d",port_socketio);
       sprintf(str_port_socketdata,"%d",port_socketdata);

       params[0]=eventServer_path;
       params[1]=port_socketio;
       params[2]=port_socketio;
       params[3]=0;

       exevp(nodejs_path, params);

       VERBOSE(1) perror("");

       exit(1);
    }

    else if (automator_pid < 0)
    { // failed to fork
       return -1;
    }
    // Code only executed by parent process
    return nodejs_pid;
} 


void startMonitoringServer(char *nodejs_path, char *eventServer_path, int port_socketio, int port_socketdata);
{
   pid_t pid;

   if(pid=start_nodejs(nodejs_path, eventServer_path, port_socketio, port_socketdata))>0)
   {
      pid_nodejs=pid;
      monitoringServer();
   }
}


void stop_nodejs()
{
   int status;

   if(pid_nodejs>1)
   {
      kill(pid_nodejs, SIGTERM);
      waitpit(stpid_nodej, &satus);
      pid_nodejs=-1;
   }
}


/*
void test(int argc, const char *argv[])
{
 int s;
 
   if(connexion(&s, "localhost", 5600)==0)
   {
      envoie(&s, "TEST");
      close(s);
   }
}
*/