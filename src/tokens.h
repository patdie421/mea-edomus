/**
 * \file tokens.h
 * \brief gestion de token (association identifiant/chaine de charactères).
 * \author Patrice DIETSCH
 * \date 29/10/12
 */

#ifndef __tokens_strings_h
#define __tokens_strings_h

#include <inttypes.h>

//
// paramétrage du module
//

// Type d'algorithme utilisé pour l'acces aux données
#define TOKEN_BY_SEQUENTIAL_READ 0     // par lecture séquentielle des données
#define TOKEN_BY_INDEX           1     // par index trié (recherche dichotomique) => à revoir ...
#define TOKEN_BY_HASH_TABLE      0     // par table de hashage

#define TOKENS_AUTOINIT          1     // nécessité (TOKENS_AUTOINIT 0) ou non (TOKENS_AUTOINIT 1) d'initialiser explicitement les tokens (init_tokens()) avant de les utiliser.

enum token_id_e {
_UNKNOWN = -1,
_ZERO = 0,

ACTION_ID,
ADDR_H_ID,
ADDR_L_ID,
ANALOG_ID,
ANALOG_IN_ID,
ANALOG_OUT_ID,
COLOR_ID,
DEC_ID,
DEVICE_ID_ID,
DEVICE_INTERFACE_NAME_ID,
DEVICE_INTERFACE_TYPE_NAME_ID,
DEVICE_LOCATION_ID_ID,
DEVICE_NAME_ID,
DEVICE_PARAMETERS_ID,
DEVICE_STATE_ID,
DEVICE_TYPE_ID_ID,
DEVICE_TYPE_PARAMETERS_ID,
DIGITAL_ID,
DIGITAL_IN_ID,
DIGITAL_OUT_ID,
ENOCEAN_ADDR_ID,
FALSE_ID,
GENERIC_ID,
HIGH_ID,
ID_ENOCEAN_ID,
ID_XBEE_ID,
INC_ID,
INTERFACE_ID_ID,
INTERFACE_LOCATION_ID_ID,
INTERFACE_NAME_ID,
INTERFACE_PARAMETERS_ID,
INTERFACE_STATE_ID,
INTERFACE_TYPE_ID_ID,
LOW_ID,
NAME_ID,
ON_ID,
ONOFF_ID,
PWM_ID,
RAW_ID,
REACHABLE_ID,
STATE_ID,
TODBFLAG_ID,
TRUE_ID,
UNIT_ID,
VARIABLE_ID,
XPL_ALGO_ID,
XPL_AREF11_ID,
XPL_AREF5_ID,
XPL_BASIC_ID,
XPL_COMMAND_ID,
XPL_CONTROL_ID,
XPL_CURRENT_ID,
XPL_DATA1_ID,
XPL_DEVICE_ID,
XPL_ENERGY_ID,
XPL_INPUT_ID,
XPL_LAST_ID,
XPL_OUTPUT_ID,
XPL_POWER_ID,
XPL_PULSE_ID,
XPL_REQUEST_ID,
XPL_SENSOR_ID,
XPL_TEMP_ID,
XPL_TMP36_ID,
XPL_TYPE_ID,
XPL_VOLTAGE_ID,
XPL_WATCHDOG_ID,
TYPEOFTYPE_ID,
INTERNAL_ID,
XPLMSG_ID,
XPLSOURCE_ID,
XPLTARGET_ID,
XPLSCHEMA_ID,
XPLMSGTYPE_ID,
XPL_TRIG_ID,
XPL_STAT_ID,
XPL_CMND_ID,
XPL_CONTROLBASIC_ID,
XPL_SENSORREQUEST_ID,
LOCAL_XBEE_ADDR_H_ID,
LOCAL_XBEE_ADDR_L_ID,

_END = LOCAL_XBEE_ADDR_L_ID,
_START = ACTION_ID,
};

void init_tokens(void);
void release_tokens(void);
char *get_token_string_by_id(enum token_id_e id);
enum token_id_e get_token_id_by_string(char *str);

#endif
