//
//  sockets_utils.c
//  mea-edomus
//
//  Created by Patrice Dietsch on 16/10/2014.
//
//
#include <stdio.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h> // close
#include <netdb.h> // gethostbyname
#include <string.h>

#include "globals.h"
#include "debug.h"

/* pour timeout sur lecture de socket. Utilisable pour une fonction mea_socket_recv à écrire
struct timeval tv;

tv.tv_sec = 30;  // 30 Secs Timeout
tv.tv_usec = 0;  // Not init'ing this can cause strange errors

setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv,sizeof(struct timeval));
*/

int mea_socket_connect(int *s, char *hostname, int port)
{
   int sock;
   struct sockaddr_in serv_addr;
   struct hostent *serv_info = NULL;

   sock = socket(AF_INET, SOCK_STREAM, 0);
   if(sock < 0)
   {
      DEBUG_SECTION {
         fprintf(stderr, "%s (%s) :  socket - can't create : ",ERROR_STR,__func__);
         perror("");
      }
      return -1;
   }

   serv_info = gethostbyname(hostname); // on récupère les informations de l'hôte auquel on veut se connecter
   if(serv_info == NULL)
   {
      DEBUG_SECTION {
         fprintf(stderr, "%s (%s) :  gethostbyname - can't get information : ",ERROR_STR,__func__);
         perror("");
      }
      return -1;
   }

   bzero((char *)&serv_addr, sizeof(serv_addr));
   serv_addr.sin_family = AF_INET;
   serv_addr.sin_port   = htons(port);
   bcopy((char *)serv_info->h_addr, (char *)&serv_addr.sin_addr.s_addr, serv_info->h_length);
   
   if(connect(sock, (const struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
   {
      DEBUG_SECTION {
         fprintf(stderr, "%s (%s) :  connect - can't connect : ",ERROR_STR,__func__);
         perror("");
      }
      return -1;
   }

   *s=sock;
   return 0;
}

pthread_mutex_t mea_socket_send_lock;
int mea_socket_send_lock_is_init=0;

int mea_socket_send(int *s, char *message, int l_message)
{
   int ret;
   
   if(mea_socket_send_lock_is_init==0)
   {
      pthread_mutex_init(&mea_socket_send_lock, NULL);
      mea_socket_send_lock_is_init=1;
   }
   
   pthread_cleanup_push( (void *)pthread_mutex_unlock, (void *)&mea_socket_send_lock );
   pthread_mutex_lock(&mea_socket_send_lock);
   
   ret=send(*s, message, l_message, 0);

   pthread_mutex_unlock(&mea_socket_send_lock);
   pthread_cleanup_pop(0);

   if(ret < 0)
   {
      DEBUG_SECTION {
         fprintf(stderr, "%s (%s) :  send - can't send : ",ERROR_STR,__func__);
         perror("");
      }
      return -1;
   }
   
   return 0;
}

