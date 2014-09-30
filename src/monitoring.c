#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h> // close
#include <netdb.h> // gethostbyname
#include <errno.h>
#include <signal.h>
#include <sys/wait.h>

#define PORT 5600

#include "debug.h"
#include "monitoring.h"

const char *hostname = "localhost";
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


int seek_to_prevs_lines(FILE *fp, int nb_lines)
{
   char buff[512];
   long pos=0;
   int n;

   if(nb_lines < 1)
      return -1;


   fseek(fp, 0, SEEK_END);

   long max=ftell(fp);
   long pos=max;

   do
   {
      int nb_read=0;

      pos=pos-sizeof(buff);
      if(pos<0)
      {
         nb_read=sizeof(buff)+pos;
         pos=0;
      }
      else
      {
         nb_read=sizeof(buff);
      }

      fseek(fp, pos, SEEK_SET);
      fread(fp, buff, nb_read);

      for(i=nb_read;i;i--)
      {
         if( (buff[i-1]=='\r') && ((pos+i) != 0) )
         {
            n++;
            if(n==nb_line)
            {
               fseek(fp,pos+i,SEEK_SET);
               return 0;
            }
        }
     }
  }
  while(pos>0);

  fseek(fp, pos, SEEK_END);
  return 0;
}


int readAndSendLine(int nodejs_socket, char *file, long *pos)
{
   FILE *fp=NULL;
   char line[512];
   struct stat fileStat;
   static time_t st_mtime=0;

   fp = fopen(file, "r");
   if(fp == NULL)
   {
      //perror("");
      *pos=0; // le fichier n'existe pas. lorsqu'il sera créé on le lira depuis le debut
      st_mtime=0;
      return 0;
   }

   fstat(fp, &fileStat);
   if(fileStat.st_mtime == st_mtime)
   { // aucun changement
      fclose(fp);
      return 0;
   }
   st_mtime=fileStat.st_mtime;


   fseek(fp,0,SEEK_END);
   if(*pos>=0)
   {
      long current=ftell(fp);

      if(*pos > current) // le fichier a diminué, on le relie depuis le debut
      {
         fseek(fp, 0, SEEK_SET);
         *pos=0;
      }
      else // le fichier a grossie ou est resté identique (en taille).
      {
         if(current != *pos) // il a grossie
            fseek(fp, *pos, SEEK_SET);
         else // il est identique (en taille), mais il a été modifié.
         {
            fseek(fp, 0, SEEK_SET);
            *pos=0;
         }
      }
   }

   int ret=0;
   while (1)
   {
      if (fgets(line, sizeof(line), fp) == NULL)
      {
         // plus de ligne à lire, on mémorise la dernière position connue
         *pos=ftell(fp);
         // et on s'arrete pour l'instant
         break; 
      }
      else
      {
         char message[512];

         sprintf(message,"LOG:%s",line);
         fprintf(stderr,"%d %s",*pos,message);
         int ret = envoie(&nodejs_socket, message);
         if(ret<0)
         {
            ret=-1;
            break;
         }
      }
   }
   if(fp)
      fclose(fp);

   return ret;
}


void monitoringServer(char *logfile)
{
   int exit=0;
   int nodejs_socket=-1;
   char message[1024];
   long pos = -1;
   
   do
   {
     if(connexion(&nodejs_socket, (char *)hostname, PORT)==0)
     {
       int ret;
       do
       {
         if(readAndSendLine(nodejs_socket, logfile, &pos)==-1)
            break;
         sleep(1);
       }
       while(1);

       close(nodejs_socket);
     }
     else
     {
       fprintf(stderr,"Retry next time ...\n");
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
      char str_port_socketio[16];
      char str_port_socketdata[16];
      char *params[5];

      sprintf(str_port_socketio,"%d",port_socketio);
      sprintf(str_port_socketdata,"%d",port_socketdata);

      params[0]="nodejs";
      params[1]=eventServer_path;
      params[2]=str_port_socketio;
      params[3]=str_port_socketio;
      params[4]=0;

      execvp(nodejs_path, params);

      perror("");

      exit(1);
   }

   else if (nodejs_pid < 0)
   { // failed to fork
      perror("");
      return -1;
   }
   // Code only executed by parent process
   return nodejs_pid;
} 


void startMonitoringServer(char *nodejs_path, char *eventServer_path, int port_socketio, int port_socketdata, char *log_path)
{
   pid_t pid;

   pid=start_nodejs(nodejs_path, eventServer_path, port_socketio, port_socketdata);
   if(pid>0)
   {
      pid_nodejs=pid;
      monitoringServer(log_path);
   }
}


void stop_nodejs()
{
   int status;

   if(pid_nodejs>1)
   {
      kill(pid_nodejs, SIGTERM);
      wait(&status);
      pid_nodejs=-1;
   }
}
