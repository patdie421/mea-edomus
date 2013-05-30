//
//  xPL_strings.h
//
//  Created by Patrice DIETSCH on 29/10/12.
//
//

#ifndef __xPL_strings_h
#define __xPL_strings_h


struct token_s
{
   char *str;
   char id;
};

extern struct token_s tokens_list[];

/*
extern char *xpl_control_str;
extern char *xpl_basic_str;
extern char *xpl_device_str;
extern char *xpl_type_str;
extern char *xpl_output_str;
extern char *xpl_current_str;
extern char *xpl_sensor_str;
extern char *xpl_pulse_str;
extern char *xpl_data1_str;
extern char *xpl_request_str;
extern char *xpl_energy_str;
extern char *xpl_power_str;
extern char *xpl_temp_str;
extern char *xpl_command_str;
extern char *xpl_input_str;
*/

#define XPL_CONTROL_ID  1
#define XPL_BASIC_ID    2
#define XPL_DEVICE_ID   3
#define XPL_TYPE_ID     4
#define XPL_OUTPUT_ID   5
#define XPL_CURRENT_ID  6
#define XPL_SENSOR_ID   7
#define XPL_PULSE_ID    8
#define XPL_COMMAND_ID  9
#define XPL_DATA1_ID   10
#define XPL_REQUEST_ID 11
#define XPL_ENERGY_ID  12
#define XPL_POWER_ID   13
#define XPL_TEMP_ID    14
#define XPL_INPUT_ID   15
#define DIGITAL_IN_ID  16
#define DIGITAL_OUT_ID 17
#define ANALOG_IN_ID   18
#define ANALOG_OUT_ID  19
#define XPL_VOLTAGE_ID 20
#define XPL_TMP36_ID   21
#define XPL_AREF5_ID   22
#define XPL_AREF11_ID  23
#define XPL_ALGO_ID    24
#define ACTION_ID      25
#define PWM_ID         26
#define ONOFF_ID       27
#define VARIABLE_ID    28
#define INC_ID         29
#define DEC_ID         30

#define DEVICE_ID_ID                  31
#define DEVICE_NAME_ID                32
#define DEVICE_TYPE_ID_ID             33
#define DEVICE_LOCATION_ID_ID         34
#define DEVICE_INTERFACE_NAME_ID      35
#define DEVICE_INTERFACE_TYPE_NAME_ID 36
#define DEVICE_STATE_ID               37
#define DEVICE_TYPE_PARAMETERS_ID     38

/*
#define XPL_CONTROL_STR xpl_control_str
#define XPL_BASIC_STR   xpl_basic_str
#define XPL_DEVICE_STR  xpl_device_str
#define XPL_TYPE_STR    xpl_type_str
#define XPL_OUTPUT_STR  xpl_output_str
#define XPL_CURRENT_STR xpl_current_str
#define XPL_SENSOR_STR  xpl_sensor_str
#define XPL_PULSE_STR   xpl_pulse_str
#define XPL_COMMAND_STR xpl_command_str
#define XPL_DATA1_STR   xpl_data1_str
#define XPL_REQUEST_STR xpl_request_str
#define XPL_ENERGY_STR  xpl_energy_str
#define XPL_POWER_STR   xpl_power_str
#define XPL_TEMP_STR    xpl_temp_str
#define XPL_INPUT_STR   xpl_input_str
*/

char *get_token_by_id(int id);
int get_id_by_string(char *str);
int strcmplower(char *str1, char *str2);
int int_isin(int val, int list[]);

#endif
