/**
 * \file tokens_da.h
 * \brief acces direct aux chaines des tokens.
 * \author Patrice DIETSCH
 * \date 02/09/15
 */

#ifndef __tokens_da_h
#define __tokens_da_h

#include <inttypes.h>

#include "tokens.h"

// Activation du mode "direct access" (DA).
#define TOKENS_STRING_DA 1

#if TOKENS_STRING_DA == 1
struct tokens_strings_da_s
{
   char *device_parameters_str_c;
   char *device_type_id_str_c;
   char *enocean_addr_str_c;
   char *false_str_c;
   char *id_enocean_str_c;
   char *interface_id_str_c;
   char *interface_parameters_str_c;
   char *low_str_c;
   char *name_str_c;
   char *on_str_c;
   char *recheable_str_c;
   char *true_str_c;
   char *xpl_basic_str_c;
   char *xpl_current_str_c;
   char *xpl_control_str_c;
   char *xpl_last_str_c;
   char *xpl_output_str_c;
   char *xpl_request_str_c;
   char *xpl_sensor_str_c;
   char *xpl_type_str_c;
   char *xpl_device_str_c;
   char *high_str_c;
   char *device_id_str_c;
   char *device_name_str_c;
   char *device_localtion_id_str_c;
   char *device_interface_name_str_c;
   char *device_interface_type_name_str_c;
   char *device_state_str_c;
   char *device_type_parameters_str_c;
   char *todbflag_str_c;
   char *addr_l_str_c;
   char *addr_h_str_c;
   char *interface_type_id_str_c;
   char *interface_name_str_c;
   char *interface_state_str_c;
   char *local_xbee_addr_h_str_c;
   char *local_xbee_addr_l_str_c;
   char *reachable_str_c;
   char *state_str_c;
   char *internal_str_c;
   char *xplmsg_str_c;
   char *xplsource_str_c;
   char *xpltarget_str_c;
   char *xplschema_str_c;
   char *xplmsgtype_str_c;
   char *xpl_trig_str_c;
   char *xpl_stat_str_c;
   char *xpl_cmnd_str_c;
   char *xpl_pulse_str_c;
   char *xpl_controlbasic_str_c;
   char *xpl_sensorrequest_str_c;
};

extern struct tokens_strings_da_s *tokens_string_da;

// Macro pour les accès direct à la chaine d'un token. Tous les tokens ne sont pas repris pour l'instant
#define DEVICE_PARAMETERS_STR_C          tokens_string_da->device_parameters_str_c
#define DEVICE_TYPE_ID_STR_C             tokens_string_da->device_type_id_str_c
#define FALSE_STR_C                      tokens_string_da->false_str_c
#define HIGH_STR_C                       tokens_string_da->high_str_c
#define ID_ENOCEAN_STR_C                 tokens_string_da->id_enocean_str_c
#define LOW_STR_C                        tokens_string_da->low_str_c
#define NAME_STR_C                       tokens_string_da->name_str_c
#define ON_STR_C                         tokens_string_da->on_str_c
#define REACHABLE_STR_C                  tokens_string_da->reachable_str_c
#define STATE_STR_C                      tokens_string_da->state_str_c
#define TRUE_STR_C                       tokens_string_da->true_str_c
#define XPL_BASIC_STR_C                  tokens_string_da->xpl_basic_str_c
#define XPL_CURRENT_STR_C                tokens_string_da->xpl_current_str_c
#define XPL_CONTROL_STR_C                tokens_string_da->xpl_control_str_c
#define XPL_DEVICE_STR_C                 tokens_string_da->xpl_device_str_c
#define XPL_ENOCEAN_ADDR_STR_C           tokens_string_da->enocean_addr_str_c
#define XPL_LAST_STR_C                   tokens_string_da->xpl_last_str_c
#define XPL_OUTPUT_STR_C                 tokens_string_da->xpl_output_str_c
#define XPL_REQUEST_STR_C                tokens_string_da->xpl_request_str_c
#define XPL_SENSOR_STR_C                 tokens_string_da->xpl_sensor_str_c
#define XPL_TYPE_STR_C                   tokens_string_da->xpl_type_str_c
#define DEVICE_ID_STR_C                  tokens_string_da->device_id_str_c
#define DEVICE_NAME_STR_C                tokens_string_da->device_name_str_c
#define DEVICE_TYPE_ID_STR_C             tokens_string_da->device_type_id_str_c
#define DEVICE_LOCATION_ID_STR_C         tokens_string_da->device_localtion_id_str_c
#define DEVICE_INTERFACE_NAME_STR_C      tokens_string_da->device_interface_name_str_c
#define DEVICE_INTERFACE_TYPE_NAME_STR_C tokens_string_da->device_interface_type_name_str_c
#define INTERFACE_PARAMETERS_STR_C       tokens_string_da->interface_parameters_str_c
#define DEVICE_STATE_STR_C               tokens_string_da->device_state_str_c
#define DEVICE_TYPE_PARAMETERS_STR_C     tokens_string_da->device_type_parameters_str_c
#define DEVICE_TODBFLAG_STR_C            tokens_string_da->todbflag_str_c
#define DEVICE_ADDR_H_STR_C              tokens_string_da->addr_h_str_c
#define DEVICE_ADDR_L_STR_C              tokens_string_da->addr_l_str_c
#define INTERFACE_ID_STR_C               tokens_string_da->interface_id_str_c
#define INTERFACE_TYPE_ID_STR_C          tokens_string_da->interface_type_id_str_c
#define INTERFACE_NAME_STR_C             tokens_string_da->interface_name_str_c
#define INTERFACE_STATE_STR_C            tokens_string_da->interface_state_str_c
#define LOCAL_XBEE_ADDR_H_STR_C          tokens_string_da->local_xbee_addr_h_str_c
#define LOCAL_XBEE_ADDR_L_STR_C          tokens_string_da->local_xbee_addr_l_str_c
#define INTERNAL_STR_C                   tokens_string_da->internal_str_c
#define XPLMSG_STR_C                     tokens_string_da->xplmsg_str_c
#define XPLSOURCE_STR_C                  tokens_string_da->xplsource_str_c
#define XPLTARGET_STR_C                  tokens_string_da->xpltarget_str_c
#define XPLMSGTYPE_STR_C                 tokens_string_da->xplmsgtype_str_c
#define XPLSCHEMA_STR_C                  tokens_string_da->xplschema_str_c
#define XPL_TRIG_STR_C                   tokens_string_da->xpl_trig_str_c
#define XPL_STAT_STR_C                   tokens_string_da->xpl_stat_str_c
#define XPL_CMND_STR_C                   tokens_string_da->xpl_cmnd_str_c
#define XPL_PULSE_STR_C                  tokens_string_da->xpl_pulse_str_c
#define XPL_CONTROLBASIC_STR_C           tokens_string_da->xpl_controlbasic_str_c
#define XPL_SENSORREQUEST_STR_C          tokens_string_da->xpl_sensorrequest_str_c

#else

#define DEVICE_PARAMETERS_STR_C          get_token_string_by_id(DEVICE_PARAMETERS_ID)
#define DEVICE_TYPE_ID_STR_C             get_token_string_by_id(DEVICE_TYPE_ID_ID)
#define FALSE_STR_C                      get_token_string_by_id(FALSE_ID)
#define HIGH_STR_C                       get_token_string_by_id(HIGH_ID)
#define ID_ENOCEAN_STR_C                 get_token_string_by_id(ID_ENOCEAN_ID)
#define LOW_STR_C                        get_token_string_by_id(LOW_ID)
#define NAME_STR_C                       get_token_string_by_id(NAME_ID)
#define ON_STR_C                         get_token_string_by_id(ON_ID)
#define REACHABLE_STR_C                  get_token_string_by_id(REACHABLE_ID)
#define STATE_STR_C                      get_token_string_by_id(STATE_ID)
#define TRUE_STR_C                       get_token_string_by_id(TRUE_ID)
#define XPL_BASIC_STR_C                  get_token_string_by_id(XPL_BASIC_ID)
#define XPL_CURRENT_STR_C                get_token_string_by_id(XPL_CURRENT_ID)
#define XPL_CONTROL_STR_C                get_token_string_by_id(XPL_CONTROL_ID)
#define XPL_DEVICE_STR_C                 get_token_string_by_id(XPL_DEVICE_ID)
#define XPL_ENOCEAN_ADDR_STR_C           get_token_string_by_id(XPL_ENOCEAN_ADDR_ID)
#define XPL_LAST_STR_C                   get_token_string_by_id(XPL_LAST_ID)
#define XPL_OUTPUT_STR_C                 get_token_string_by_id(XPL_OUTPUT_STR_C)
#define XPL_REQUEST_STR_C                get_token_string_by_id(XPL_REQUEST_ID)
#define XPL_SENSOR_STR_C                 get_token_string_by_id(XPL_SENSOR_ID)
#define XPL_TYPE_STR_C                   get_token_string_by_id(XPL_TYPE_ID)
#define DEVICE_ID_STR_C                  get_token_string_by_id(DEVICE_ID_ID)
#define DEVICE_NAME_STR_C                get_token_string_by_id(DEVICE_NAME_ID)
#define DEVICE_TYPE_ID_STR_C             get_token_string_by_id(DEVICE_TYPE_ID_ID)
#define DEVICE_LOCATION_ID_STR_C         get_token_string_by_id(DEVICE_LOCATION_ID_ID)
#define DEVICE_INTERFACE_NAME_STR_C      get_token_string_by_id(DEVICE_INTERFACE_NAME_ID)
#define DEVICE_INTERFACE_TYPE_NAME_STR_C get_token_string_by_id(DEVICE_INTERFACE_TYPE_NAME_ID)
#define INTERFACE_PARAMETERS_STR_C       get_token_string_by_id(INTERFACE_PARAMETERS_ID)
#define DEVICE_STATE_STR_C               get_token_string_by_id(DEVICE_STATE_ID)
#define DEVICE_TYPE_PARAMETERS_STR_C     get_token_string_by_id(DEVICE_TYPE_PARAMETERS_ID)
#define DEVICE_TODBFLAG_STR_C            get_token_string_by_id(DEVICE_TODBFLAG_ID)
#define DEVICE_ADDR_H_STR_C              get_token_string_by_id(DEVICE_ADDR_H_ID)
#define DEVICE_ADDR_L_STR_C              get_token_string_by_id(DEVICE_ADDR_L_ID)
#define INTERFACE_ID_STR_C               get_token_string_by_id(INTERFACE_ID_ID)
#define INTERFACE_TYPE_ID_STR_C          get_token_string_by_id(INTERFACE_TYPE_ID_ID)
#define INTERFACE_NAME_STR_C             get_token_string_by_id(INTERFACE_NAME_ID)
#define INTERFACE_STATE_STR_C            get_token_string_by_id(INTERFACE_STATE_ID)
#define LOCAL_XBEE_ADDR_H_STR_C          get_token_string_by_id(LOCAL_XBEE_ADDR_H_ID)
#define LOCAL_XBEE_ADDR_L_STR_C          get_token_string_by_id(LOCAL_XBEE_ADDR_L_ID)
#define INTERNAL_STR_C                   get_token_string_by_id(INTERNAL_ID)
#define XPLMSG_STR_C                     get_token_string_by_id(XPLMSG_ID)
#define XPLSOURCE_STR_C                  get_token_string_by_id(XPLSOURCE_ID)
#define XPLTARGET_STR_C                  get_token_string_by_id(XPLTARGET_ID)
#define XPLMSGTYPE_STR_C                 get_token_string_by_id(XPLMSGTYPE_ID)
#define XPLSCHEMA_STR_C                  get_token_string_by_id(XPLSCHEMA_ID)
#define XPL_TRIG_STR_C                   get_token_string_by_id(XPL_TRIG_ID)
#define XPL_STAT_STR_C                   get_token_string_by_id(XPL_STAT_ID)
#define XPL_CMND_STR_C                   get_token_string_by_id(XPL_CMND_ID)
#define XPL_PULSE_STR_C                  get_token_string_by_id(XPL_PULSE_ID)
#define XPL_CONTROLBASIC_STR_C           get_token_string_by_id(XPL_CONTROLBASIC_ID)
#define XPL_SENSORREQUEST_STR_C          get_token_string_by_id(XPL_SENSORREQUEST_ID)

#endif

int16_t init_strings_da(void);
void release_strings_da(void);

#endif

