#ifndef __netatmo_h
#define __netatmo_h

#include <inttypes.h>

enum netatmo_setpoint_mode_e
{
   PROGRAM,
   AWAY,
   HG,
   MANUAL,
   OFF,
   MAX,
};

extern char *netatmo_therm_mode[];

struct netatmo_token_s
{
   char access[81];
   char refresh[81]; 
   time_t expire_in;
};

struct netatmo_thermostat_data_s
{
   int battery_vp;
   int therm_relay_cmd;
   double temperature;
   enum netatmo_setpoint_mode_e setpoint;
   double setpoint_temp;
};

int netatmo_get_token(char *client_id, char *client_secret, char *username, char *password, char *scope, struct netatmo_token_s *netatmo_token, char *err, int l_err);
int netatmo_refresh_token(char *client_id, char *client_secret, struct netatmo_token_s *netatmo_token, char *err, int l_err);
int netatmo_set_thermostat_setpoint(char *access_token, char *relay_id, char *thermostat_id, enum netatmo_setpoint_mode_e mode, uint32_t delay, double temp, char *err, int l_err);
int netatmo_get_thermostat_data(char *access_token, char *relay_id, char *thermostat_id, struct netatmo_thermostat_data_s *thermostat_data, char *err, int l_err);

#endif

