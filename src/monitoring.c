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

int connexion(int *s, char *hostname, int port)
{
   int sock;
   struct sockaddr_in serv_addr;
   struct hostent *serv_info = NULL;

   sock = socket(AF_INET, SOCK_STREAM, 0);
   if(sock < 0)
   {
      perror("socket()");
      return 1;
   }

   serv_info = gethostbyname(hostname); // on récupère les informations de l'hôte auquel on veut se connecter
   if(serv_info == NULL)
   {
      perror("gethostbyname()");
      return 1;
   }

   bzero((char *)&serv_addr, sizeof(serv_addr));
   serv_addr.sin_family = AF_INET;
   serv_addr.sin_port   = htons(port);
   bcopy((char *)serv_info->h_addr, (char *)&serv_addr.sin_addr.s_addr, serv_info->h_length);
   
   if(connect(sock, (const struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
   {
      perror("connect()");
      return 1;
   }
   
   *s=sock;
   return 0;
}


int envoie(int *s, char *message)
{
   if(send(*s, message, strlen(message), 0) < 0)
   {
      perror("send()");
      return errno;
   }
   return 0;
}


void monitor(void)
{
   int exit=0;
   int nodejs_socket=-1;
   char message[1024];
   
   do
   {
     if(connexion(&nodejs_socket, (char *)hostname, PORT)==0)
     {
       int ret;
       do
       {
         sleep(5); // un check toutes les 5 secondes
         
         // récupération des données
         
         // formatage des données
         
         ret = envoie(&nodejs_socket, message);
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


void main_test(int argc, const char *argv[])
{
 int s;
 
   if(connexion(&s, "localhost", 5600)==0)
   {
      envoie(&s, "TEST");
      close(s);
   }
}
