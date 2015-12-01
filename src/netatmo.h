#ifndef __netatmo_h
#define __netatmo_h

#include <inttypes.h>

enum netatmo_setpoint_mode_e
{
   NONE=-1,
   PROGRAM=0,
   AWAY,
   HG,
   MANUAL,
   OFF,
   MAX,
};

extern char *netatmo_thermostat_mode[];

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


struct netatmo_module_data_s
{
   char id[18];
   char name[40];
   char dataType;
   double temperature;
   double humidity;
};

struct netatmo_station_data_s
{
   double temperature;
   struct netatmo_module_data_s modules_data[5];
};


int netatmo_get_token(char *client_id, char *client_secret, char *username, char *password, char *scope, struct netatmo_token_s *netatmo_token, char *err, int l_err);
int netatmo_refresh_token(char *client_id, char *client_secret, struct netatmo_token_s *netatmo_token, char *err, int l_err);
int netatmo_set_thermostat_setpoint(char *access_token, char *relay_id, char *thermostat_id, enum netatmo_setpoint_mode_e mode, uint32_t delay, double temp, char *err, int l_err);
int netatmo_get_thermostat_data(char *access_token, char *relay_id, char *thermostat_id, struct netatmo_thermostat_data_s *thermostat_data, char *err, int l_err);

#endif

