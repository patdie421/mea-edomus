//
//  xPL_strings.c
//
//  Created by Patrice DIETSCH on 29/10/12.
//
//

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>

#include "string_utils.h"
#include "token_strings.h"

struct token_s tokens_list[]={
   {"control",                    XPL_CONTROL_ID},
   {"basic",                      XPL_BASIC_ID},
   {"device",                     XPL_DEVICE_ID},
   {"type",                       XPL_TYPE_ID},
   {"output",                     XPL_OUTPUT_ID},
   {"current",                    XPL_CURRENT_ID},
   {"sensor",                     XPL_SENSOR_ID},
   {"pulse",                      XPL_PULSE_ID},
   {"command",                    XPL_COMMAND_ID},
   {"data1",                      XPL_DATA1_ID},
   {"request",                    XPL_REQUEST_ID},
   {"energy",                     XPL_ENERGY_ID},
   {"power",                      XPL_POWER_ID},
   {"temp",                       XPL_TEMP_ID},
   {"input",                      XPL_INPUT_ID},
   {"digital_in",                 DIGITAL_IN_ID},
   {"digital_out",                DIGITAL_OUT_ID},
   {"analog_in",                  ANALOG_IN_ID},
   {"analog_out",                 ANALOG_OUT_ID},
   {"voltage",                    XPL_VOLTAGE_ID},
   {"tmp36",                      XPL_TMP36_ID},
   {"aref5",                      XPL_AREF5_ID},
   {"aref11",                     XPL_AREF11_ID},
   {"algo",                       XPL_ALGO_ID},
   {"action",                     ACTION_ID},
   {"pwm",                        PWM_ID},
   {"onoff",                      ONOFF_ID},
   {"variable",                   VARIABLE_ID},
   {"inc",                        INC_ID},
   {"dec",                        DEC_ID},
   {"device_id",                  DEVICE_ID_ID},
   {"device_name",                DEVICE_NAME_ID},
   {"device_type_id",             DEVICE_TYPE_ID_ID},
   {"device_location_id",         DEVICE_LOCATION_ID_ID},
   {"device_interface_name",      DEVICE_INTERFACE_NAME_ID},
   {"device_interface_type_name", DEVICE_INTERFACE_TYPE_NAME_ID},
   {"device_state",               DEVICE_STATE_ID},
   {"device_type_parameters",     DEVICE_TYPE_PARAMETERS_ID},
   {"interface_id",               INTERFACE_ID_ID},
   {"interface_name",             INTERFACE_NAME_ID},
   {"interface_type_id",          INTERFACE_TYPE_ID_ID},
   {"interface_location_id",      INTERFACE_LOCATION_ID_ID},
   {"interface_state",            INTERFACE_STATE_ID},
   {"interface_parameters",       INTERFACE_PARAMETERS_ID},
   {"device_parameters",          DEVICE_PARAMETERS_ID},
   {"ID_XBEE",                    ID_XBEE_ID},
   {"ADDR_H",                     ADDR_H_ID},
   {"ADDR_L",                     ADDR_L_ID},
   {"digital",                    DIGITAL_ID},
   {"analog",                     ANALOG_ID},
   {NULL,0}
};


char *get_token_by_id(int id)
{
   for(int i=0;tokens_list[i].str;i++)
   {
      if(tokens_list[i].id == id)
         return tokens_list[i].str;
   }
   return NULL;
}


int16_t get_id_by_string(char *str)
{
   if(!str)
      return -1;
   
   for(int i=0;tokens_list[i].str;i++)
   {
      if(strcmplower(tokens_list[i].str, str) == 0)
         return tokens_list[i].id;
   }
   return -1;
}

/*
int16_t strcmplower(char *str1, char *str2)
{
   int i;
   for(i=0;str1[i];i++)
      if(tolower(str1[i])!=tolower(str2[i]))
         return 1;
   if(str1[i]!=str2[i])
      return 1;
   return 0;
}
*/

int16_t int_isin(int val, int list[])
{
   for(int i=0; list[i]!=-1; i++)
      if(val==list[i])
         return 1;
   return 0;
}
