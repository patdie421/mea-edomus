#ifndef __timeServer_h
#define __timeServer_h

int start_timeServer();

time_t mea_time(time_t *v);
struct tm *mea_localtime_r(const time_t *timep, struct tm *result);

int mea_timeFromStr(char *str, time_t *t);

#endif
