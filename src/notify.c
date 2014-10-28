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
int _notify_socket=-1;

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


int _notify(char *notif_str, char notif_type)
{
   if(_notify_socket==-1)
   {
      if(mea_socket_connect(&_notify_socket, hostname, port)<0)
      {
        _notify_socket==-1;
        return -1;
      }

      int notif_str_l=strlen(notif_str)+6;
      char message[2048];
      sprintf(message,"$$$%c%cNOT:%c:%s###", (char)(notif_str_l%256), (char)(notif_str_l/256), notif_type, notif_str);
      ret = mea_socket_send(&_notify_socket, message, notif_str_l+12);
      if(ret<0)
      {
         close(_notify_socket);
         _notify_socket=-1;
         return -1;
      }
   }
   return 0;
}


int mea_notify2(char *notif_str, char notif_type)
{
   return _notify(localhost_const, _port, notif_str, notif_type);
}


size_t mea_notify_sprintf(int notify_type, char const* fmt, ...)
{
   char notif_str[256];
   int l_notif;
   
   va_list args;
   va_start(args, fmt);
   
   l_notif = vsnprintf(notif_str, sizeof(notif_str), fmt, args);
   
   va_end(args);

   return _notify(localhost_const, _port, notif_str, notif_type);
}

