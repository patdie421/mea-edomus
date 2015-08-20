//
//  xPL_strings.h
//
//  Created by Patrice DIETSCH on 29/10/12.
//
//

#ifndef __tokens_strings_h
#define __tokens_strings_h

#include <inttypes.h>

#include "uthash.h"


#define XPL_CONTROL_ID                 1
#define XPL_BASIC_ID                   2
#define XPL_DEVICE_ID                  3
#define XPL_TYPE_ID                    4
#define XPL_OUTPUT_ID                  5
#define XPL_CURRENT_ID                 6
#define XPL_SENSOR_ID                  7
#define XPL_PULSE_ID                   8
#define XPL_COMMAND_ID                 9
#define XPL_DATA1_ID                  10
#define XPL_REQUEST_ID                11
#define XPL_ENERGY_ID                 12
#define XPL_POWER_ID                  13
#define XPL_TEMP_ID                   14
#define XPL_INPUT_ID                  15
#define DIGITAL_IN_ID                 16
#define DIGITAL_OUT_ID                17
#define ANALOG_IN_ID                  18
#define ANALOG_OUT_ID                 19
#define XPL_VOLTAGE_ID                20
#define XPL_TMP36_ID                  21
#define XPL_AREF5_ID                  22
#define XPL_AREF11_ID                 23
#define XPL_ALGO_ID                   24
#define ACTION_ID                     25
#define PWM_ID                        26
#define ONOFF_ID                      27
#define VARIABLE_ID                   28
#define INC_ID                        29
#define DEC_ID                        30
#define DEVICE_ID_ID                  31
#define DEVICE_NAME_ID                32
#define DEVICE_TYPE_ID_ID             33
#define DEVICE_LOCATION_ID_ID         34
#define DEVICE_INTERFACE_NAME_ID      35
#define DEVICE_INTERFACE_TYPE_NAME_ID 36
#define DEVICE_STATE_ID               37
#define DEVICE_TYPE_PARAMETERS_ID     38
#define INTERFACE_ID_ID               39
#define INTERFACE_NAME_ID             40
#define INTERFACE_TYPE_ID_ID          41
#define INTERFACE_LOCATION_ID_ID      42
#define INTERFACE_STATE_ID            43
#define INTERFACE_PARAMETERS_ID       44
#define DEVICE_PARAMETERS_ID          45
#define ID_XBEE_ID                    46
#define ADDR_H_ID                     47
#define ADDR_L_ID                     48
#define DIGITAL_ID                    49
#define ANALOG_ID                     50
#define XPL_LAST_ID                   51
#define RAW_ID                        52
#define HIGH_ID                       53
#define LOW_ID                        54
#define GENERIC_ID                    55
#define UNIT_ID                       56
#define TODBFLAG_ID                   57
#define ENOCEAN_ADDR_ID               58
#define ID_ENOCEAN_ID                 59
#define XPL_WATCHDOG_ID               60
#define XPL_REACHABLE_ID              61
#define XPL_COLOR_ID                  62
#define TRUE_ID                       63
#define FALSE_ID                      64

/*
char *get_token_by_id(int id);
int16_t get_id_by_string(char *str);

void init_tokens_hashs();
int16_t get_token_string_by_id(int16_t id);
int16_t get_token_id_by_string(char *str);
*/

void init_tokens();
char *get_token_string_by_id(int16_t id);
int16_t get_token_id_by_string(char *str);


//int16_t int_isin(int val, int list[]);

#endif
