#ifndef __automatorServer_h
#define __automatorServer_h

#include "cJSON.h"
#include "xPL.h"

extern char *inputs_rules;
extern char *outputs_rules;

extern cJSON *_inputs_rules;
extern cJSON *_outputs_rules;

cJSON *automator_load_rules(char *rules);
int automator_match_inputs_rules(cJSON *rules, xPL_MessagePtr message);
int automator_play_output_rules(cJSON *rules);
int automator_reset_inputs_change_flags();

#endif
