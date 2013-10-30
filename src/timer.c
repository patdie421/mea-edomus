#include <time.h>
#include <inttypes.h>

#include "timer.h"


uint16_t init_timer(timer_t *aTimer, uint32_t aDelay, uint16_t restartStatus)
/**
 * \brief     création (initialisation) d'un timer
 * \details   initialise une structure "timer_t". Le timer est "arrêté" au démarrage (pas de prise de chrono)
 * \param     aTimer   le timer.
 * \param     aDelay   délai avant expiration du timer.
 * \param     restartStatus   type de rearmement du timer.
 * \return    0 initialisation réalisée, -1 sinon
 */
{
   if(aTimer)
   {
      aTimer->stat=0;
	   aTimer->start_time=0;
	   aTimer->delay=aDelay;
	   aTimer->autorestart=restartStatus;
	  
	   return 0;
   }
   else
      return -1;
}


void start_timer(timer_t *aTimer)
/**
 * \brief     lancement d'un timer
 * \details   prise de chrono.
 * \param     aTimer   le timer.
 */
{
   time(&(aTimer->start_time));
	aTimer->stat=1;
}


void stop_timer(timer_t *aTimer)
/**
 * \brief     arrêt du timer
 * \details   Le chrono est remis à 0
 * \param     aTimer   le timer.
 */
{
   aTimer->start_time=0;
	aTimer->stat=0;
}


uint16_t test_timer(timer_t *aTimer)
/**
 * \brief     test le timer
 * \details   teste si le delai est dépassé.
 * \param     aTimer   le timer.
 * \return    0 le temps est dépassé (le temps est passé), -1 sinon
 */
{
   if(aTimer->stat==1)
   {
      time_t now;
	   double diff_time;
	  
	   time(&now);
	    
	   diff_time=difftime(aTimer->start_time,now);
      if(diff_time > (double)(aTimer->delay))
	   {
	      if(aTimer->autorestart==1)
		   {
		      time(&(aTimer->start_time));
		   }
		   else
		   {
		      aTimer->stat=0;
			   aTimer->start_time=0;
		   }
         return 0;
	   }
	   else
	      return -1;
   }
   else
      return -1;
}	  
