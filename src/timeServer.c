#include <stdio.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#define __USE_XOPEN
#include <time.h>
#undef __USE_XOPEN
#include <sys/time.h>
#include <signal.h>
#include <string.h>
#include <pthread.h>

#define DEBUGFLAG 1
#include "mea_verbose.h"

#include "uthash.h"
#include "sunriset.h"
#include "mea_timer.h"

pthread_t *_timerServer_thread_id=NULL;

pthread_rwlock_t _timeServer_rwlock = PTHREAD_RWLOCK_INITIALIZER;

time_t mea_time_value = 0;
time_t mea_sunrise_value = 0;
time_t mea_sunset_value = 0;
time_t mea_twilightstart_value = 0;
time_t mea_twilightend_value = 0;

struct tm mea_tm;

enum datetime_type_e { DATETIME_TIME, DATETIME_DATE };

struct mea_datetime_value_s
{
   char dateTimeStr[20]; // format "AAAA/MM/JJ hh:mm:ss" ou "hh:mm:ss"
   enum datetime_type_e type;
   struct tm tm;
   time_t time_value;
   time_t last_access;

   UT_hash_handle hh;
};

struct mea_datetime_value_s *mea_datetime_values_cache = NULL;


#define toMillis(s,u) (double)(x)*1000.0 + (double)(u)/1000.0

time_t mea_sunrise()
{
   return mea_sunrise_value;
}


time_t mea_sunset()
{
   return mea_sunset_value;
}


time_t mea_twilightstart()
{
   return mea_twilightstart_value;
}


time_t mea_twilightend()
{
   return mea_twilightend_value;
}


time_t mea_time(time_t *v)
{
   if(v!=NULL)
      *v=mea_time_value;
   return mea_time_value;
}


struct tm *mea_localtime_r(const time_t *timep, struct tm *result)
{
   if(result)
   {
      memcpy(result, &mea_tm, sizeof(struct tm));
   }

   return &mea_tm;
}


int mea_clean_datetime_values_cache()
{
   if(mea_datetime_values_cache)
   {
      struct mea_datetime_value_s  *current, *tmp;

      HASH_ITER(hh, mea_datetime_values_cache, current, tmp)
      {
         if((mea_time_value - current->last_access) > 3600)
         {
            HASH_DEL(mea_datetime_values_cache, current);
            free(current);
         }
      }
   }

   return 0;
}


int update_datetime_values_cache()
{
   struct tm tm;
   struct mea_datetime_value_s *e = NULL;

   if(mea_datetime_values_cache)
   {
      struct mea_datetime_value_s  *current, *tmp;

      memcpy(&tm, &mea_tm, sizeof(struct tm));

      HASH_ITER(hh, mea_datetime_values_cache, current, tmp)
      {
         if(current->type == DATETIME_TIME)
         {
            strptime(e->dateTimeStr, "%H:%M:%S", &tm);
            memcpy(&(e->tm), &tm, sizeof(struct tm));
            e->time_value = mktime(&tm);
         }
      }
   }

   return 0;
}


int mea_timeFromStr(char *str, time_t *t)
{
   struct tm tm;
   int ret;
   struct mea_datetime_value_s *e = NULL;

   HASH_FIND_STR(mea_datetime_values_cache, str, e);

   if(e)
   {
      e->last_access = mea_time_value;
      *t = e->time_value;
      return 0;
   }
   else
   {
      memcpy(&tm, &mea_tm, sizeof(struct tm));
      char *p=strptime(str, "%H:%M:%S", &tm); // va juste remplacer les heures, minutes et secondes dans tm
      if(p!=NULL && *p==0)
      {
         struct mea_datetime_value_s *newTime=(struct mea_datetime_value_s *)malloc(sizeof(struct mea_datetime_value_s));
         if(newTime==NULL)
            return -1;

         strcpy(newTime->dateTimeStr, str);
         memcpy(&(newTime->tm), &tm, sizeof(struct tm));
         newTime->last_access = mea_time_value;
         newTime->time_value = mktime(&tm);
         newTime->type = DATETIME_TIME;
         *t =  newTime->time_value;
          
         HASH_ADD_STR(mea_datetime_values_cache, dateTimeStr, newTime); 

         return 0;
      }
      return -1;
   }
}


int getSunRiseSetOrTwilingStartEnd(double lon, double lat, time_t *_start, time_t *_end, int twilight)
{
   int year,month,day;
   double start, end;
   int  rs;

   // Les coordonnées de Paris en degrés décimaux
//   lat = 48.8534100;
//   lon = 2.3488000;

   struct timeval te;
   struct tm tm_gmt;

   time_t t = mea_time_value;
   localtime_r(&t, &tm_gmt); // pour récupérer la date du jour (les h, m et s ne nous intéressent pas);

   // mise au format pour sunriset
   year  = tm_gmt.tm_year+1900;
   month = tm_gmt.tm_mon+1;
   day   = tm_gmt.tm_mday;

   if(twilight==0)
      rs = sun_rise_set(year, month, day, lon, lat, &start, &end);
   else
      rs = civil_twilight(year, month, day, lon, lat, &start, &end);

   int rh=0,rm=0,sh=0,sm=0;

   switch(rs)
   {
      case 0:
      {
         // conversion en heure/minute
         rh=(int)start;
         rm=(int)((start - (double)rh)*60);

         // conversion en time_t
         tm_gmt.tm_sec = 0;
         tm_gmt.tm_hour = rh;
         tm_gmt.tm_min = rm;
         t = timegm(&tm_gmt);
         *_start = t;
 
         DEBUG_SECTION {
            struct tm tm_local;
            localtime_r(&t, &tm_local);
            fprintf(stderr,"start(%d) : %02d:%02d\n", twilight, tm_local.tm_hour, tm_local.tm_min);
         }

         // conversion en heure/minute
         sh=(int)end;
         sm=(int)((end - (double)sh)*60);

         // conversion en time_t
         tm_gmt.tm_sec = 0;
         tm_gmt.tm_hour = sh;
         tm_gmt.tm_min = sm;
         t = timegm(&tm_gmt);
         *_end = t;
         
         DEBUG_SECTION {
            struct tm tm_local;
            localtime_r(&t, &tm_local);
            fprintf(stderr,"end(%d) :  %02d:%02d\n", twilight, tm_local.tm_hour, tm_local.tm_min);
         }
      }
      break;

      case +1:
         fprintf(stderr, "Sun above horizon\n");
         return -1;
         break;

      case -1:
         fprintf(stderr, "Sun below horizon\n");
         return -1;
         break;
   }

   return 0;
}


void *_timeServer_thread(void *data)
{
   struct timeval te;
   struct timespec req,res;
   int cntr=0;
   mea_time_value = time(NULL);
   int current_day = -1;

   gettimeofday(&te, NULL); // get current time
   mea_time_value = te.tv_sec;
   localtime_r(&mea_time_value, &mea_tm); // conversion en tm

   while(1)
   {
      if((te.tv_sec % 60) == 0) // toutes les minutes
      {
         localtime_r(&(mea_time_value), &mea_tm); // conversion en tm
      }

      if((te.tv_sec % 3600) == 0) // toutes les heures;
      {
         mea_clean_datetime_values_cache(); // on fait le ménage dans le cache
      }

      if(mea_tm.tm_wday != current_day) // tous les jours
      {
         current_day = mea_tm.tm_wday;

         update_datetime_values_cache(); // on remet a jour les heures pour la nouvelle journée

         // calcul des heures de couché et levé du soleil et +/- pénombre
         // Les coordonnées de Paris en degrés décimaux (constante à remplacer ...)
         double lat = 48.8534100;
         double lon = 2.3488000;
         time_t s=0, e=0;
         if(getSunRiseSetOrTwilingStartEnd(lon, lat, &s, &e,0)==0)
         {
            mea_sunrise_value = s;
            mea_sunset_value = e;
         }
         if(getSunRiseSetOrTwilingStartEnd(lon, lat, &s, &e,1)==0)
         {
            mea_twilightstart_value = s;
            mea_twilightend_value = e;
         }
      }

      // on se prépare à dormir 100 ms
      gettimeofday(&te, NULL);
      req.tv_nsec=1000L*(100000L-(te.tv_usec % 100000L)); // rattrapage temps écoulé pour les traitements
      req.tv_sec=0;
      if(req.tv_nsec>1000000000L) // 1.000.000.000 ns = 1 seconde
      {
         req.tv_sec++;
         req.tv_nsec=req.tv_nsec - 1000000000L;
      }
      while ( nanosleep(&req,&res) == -1 )
      {
         req.tv_sec  = res.tv_sec;
         req.tv_nsec = res.tv_nsec;
      }

      gettimeofday(&te, NULL); // get current time
      mea_time_value = te.tv_sec;

      pthread_testcancel();
   }
   
   pthread_exit(NULL);

   return NULL;
}


pthread_t *timeServer()
{
   pthread_t *timeServer_thread=NULL;

   timeServer_thread=(pthread_t *)malloc(sizeof(pthread_t));
   if(!timeServer_thread)
   {
      VERBOSE(2) {
         mea_log_printf("%s (%s) : %s - ",FATAL_ERROR_STR, __func__, MALLOC_ERROR_STR);
         perror("");
      }
      goto timeServer_clean_exit;
   }
   
   if(pthread_create (timeServer_thread, NULL, _timeServer_thread, NULL))
   {
      VERBOSE(2) {
         mea_log_printf("%s (%s) : pthread_create - can't start thread - ", FATAL_ERROR_STR, __func__);
         perror("");
      }
      goto timeServer_clean_exit;
   }
   pthread_detach(*timeServer_thread);
   
   if(timeServer_thread)
      return timeServer_thread;

timeServer_clean_exit:
   if(timeServer_thread)
   {
      free(timeServer_thread);
      timeServer_thread=NULL;
   }

   return NULL;
}


int start_timeServer()
{
   _timerServer_thread_id=timeServer();
   
   if(_timerServer_thread_id==NULL)
      return -1;

   return 0;
}
