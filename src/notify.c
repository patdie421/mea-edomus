//
//  notify.c
//  mea-edomus
//
//  Created by Patrice Dietsch on 27/10/2014.
//
//
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "notify.h"
#include "consts.h"

#include "sockets_utils.h"

int _port=5600;


void mea_notify_set_port(int p)
{
   _port=p;
}


int mea_notify(char *hostname, int port, char *notif_str, char notif_type)
{
   int s;
   int ret;
   
   if(mea_socket_connect(&s, hostname, port)<0)
   {
      return -1;
   }

   int notif_str_l=strlen(notif_str)+6;
   char message[2048];
   sprintf(message,"$$$%c%cNOT:%c:%s###", (char)(notif_str_l%256), (char)(notif_str_l/256), notif_type, notif_str);
   ret = mea_socket_send(&s, message, notif_str_l+12);

   close(s);
   
   return ret;
}


int mea_notify2(char *notif_str, char notif_type)
{
   return mea_notify(localhost_const, _port, notif_str, notif_type);
}
