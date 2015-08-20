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
#include<stdarg.h>

#include "notify.h"
#include "consts.h"

#include "sockets_utils.h"

int _port=5600;
int _notify_socket=-1;
int _notify_enable=0;

void mea_notify_set_port(int p)
{
   _port=p;
}


void mea_notify_enable()
{
   _notify_enable=1;
}


void mea_notify_disable()
{
   _notify_enable=0;
}


int mea_notify(char *hostname, int port, char *notif_str, char notif_type)
{
   int s;
   int ret;
   
   if(_notify_enable==0)
      return 0;
   
   if(mea_socket_connect(&s, hostname, port+1)<0)
   {
      return -1;
   }

   int notif_str_l=strlen(notif_str)+6;
   char message[2048];
   sprintf(message,"$$$%c%cNOT:%c:%s###", (char)(notif_str_l%128), (char)(notif_str_l/128), notif_type, notif_str);
   ret = mea_socket_send(&s, message, notif_str_l+12);

   close(s);
   
   return ret;
}


int _notify( char *hostname, int port, char *notif_str, char notif_type)
{
   int ret;
   int s;
   
   if(_notify_enable==0)
      return 0;

   if(mea_socket_connect(&s, hostname, port)<0)
      return -1;

   int notif_str_l=strlen(notif_str)+6;
   char message[2048];
   sprintf(message,"$$$%c%cNOT:%c:%s###", (char)(notif_str_l%128), (char)(notif_str_l/128), notif_type, notif_str);
   ret = mea_socket_send(&s, message, notif_str_l+12);

   close(s);

   return ret;
}


int mea_notify_printf(int notif_type, char const* fmt, ...)
{
   char notif_str[256];
   int l_notif;
   
   if(_notify_enable==0)
      return 0;

   va_list args;
   va_start(args, fmt);
   
   l_notif = vsnprintf(notif_str, sizeof(notif_str), fmt, args);
   
   va_end(args);

   return _notify(localhost_const, _port, notif_str, notif_type);
}

