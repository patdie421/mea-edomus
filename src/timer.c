#include <time.h>
#include <inttypes.h>

#include "timer.h"


uint16_t init_timer(timer_t *aTimer, uint32_t aDelay, uint16_t restartStatus)
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
{
   time(&(aTimer->start_time));
	aTimer->stat=1;
}


void stop_timer(timer_t *aTimer)
{
   aTimer->start_time=0;
	aTimer->stat=0;
}


uint16_t test_timer(timer_t *aTimer)
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
