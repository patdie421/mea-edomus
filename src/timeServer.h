#ifndef __timeServer_h
#define __timeServer_h

int start_timeServer();

time_t mea_time(time_t *v);
time_t mea_sunrise();
time_t mea_sunset();
time_t mea_twilightstart();
time_t mea_twilightend();

struct tm *mea_localtime_r(const time_t *timep, struct tm *result);

int mea_timeFromStr(char *str, time_t *t);

#endif
