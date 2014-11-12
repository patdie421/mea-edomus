/*
 *  debug.c
 *
 *  Created by Patrice Dietsch on 14/08/12.
 *  Copyright 2012 -. All rights reserved.
 *
 */
#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <inttypes.h>
#include <stdarg.h>
#include <time.h>

#include "debug.h"

int debug_msg=1;
int verbose_level=0;


void set_verbose_level(int level)
/**
 * \brief     positionne le niveau de logging.
 * \details   le niveau n'est pas gérer directement (pas de min et pas de max). C'est au programmeur d'en faire la gestion.
 * \param     level   valeur à positionner.
 */
{
   verbose_level=level;
}


void debug_on()
/**
 * \brief     active le mode message de debug
 */
{
   debug_msg=1;
}


void debug_off()
{
/**
 * \brief     désactive le mode debug message de debug
 */
   debug_msg=0;
}


int16_t debug_status()
/**
 * \brief     return l'état du mode de message de debug
 * \return    0 debug desactivé, 1 debug activé
 */
{
   return debug_msg;
}


uint32_t start_chrono(uint32_t *_now)
/**
 * \brief     gestion d'un chronomètre : lancement du chronomètre
 * \param     _now   la variable contiendra la valeur courrante du chrono. _now peut être égal à NULL
 * \return    retour la valeur du chrono (=_now)
 */
{
   uint32_t t;
   struct timeval tv;
   
   gettimeofday(&tv, NULL);
   t=(uint32_t)tv.tv_sec * 1000 + tv.tv_usec / 1000;
   if(_now)
      *_now=t;
   return t;
}


uint32_t take_chrono(uint32_t *_last_time)
/**
 * \brief     gestion d'un chronomètre : lecture du chrono
 * \param     _last_time   valeur retournée par start_chrono
 * \return    différence entre "maintenant" et _last_chrono
 */
{
   struct timeval tv;
   uint32_t ret;
   
   gettimeofday(&tv, NULL);
   
   uint32_t now=(uint32_t)tv.tv_sec * 1000 + tv.tv_usec / 1000;
   
   if(now>*_last_time)
	   ret=now - *_last_time;
   else
	   ret=0xFFFFFFFFF - *_last_time + now + 1;
   *_last_time = now;

   return ret;
}


void mea_log_printf(char const* fmt, ...)
{
   va_list args;
   char *date_format="[%Y-%m-%d %H:%M:%S]";

   char date_str[40];
   time_t t;
   struct tm t_tm;

   t=time(NULL);

   if (localtime_r(&t, &t_tm) == NULL)
   {
      perror("");
      return;
   }

   if (strftime(date_str, sizeof(date_str), date_format, &t_tm) == 0)
   {
      fprintf(stderr, "strftime returned 0");
      return;
   }

   fprintf(stderr,"%s ",date_str);
   
   va_start(args, fmt);
   vfprintf(stderr, fmt, args);
   va_end(args);
}

