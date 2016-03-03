#ifndef __automator_h
#define __automator_h

#include "cJSON.h"
#include "xPL.h"

#define VALUE_MAX_STR_SIZE 41

extern char *_automatorServer_fn;
extern char *_automatorEvalStrArg;
extern char *_automatorEvalStrCaller;
extern char _automatorEvalStrOperation;

extern char *inputs_rules;
extern char *outputs_rules;

extern cJSON *_inputs_rules;
extern cJSON *_outputs_rules;

//int automator_sendxpl(cJSON *parameters);
int automator_sendxpl2(cJSON *parameters);

int automator_init(char *rulesfile);
cJSON *automator_load_rules(char *rules);
//int automator_match_inputs_rules(cJSON *rules, xPL_MessagePtr message);
int automator_match_inputs_rules(cJSON *rules, cJSON *message_json);
int automator_play_output_rules(cJSON *rules);
int automator_reset_inputs_change_flags();
int automator_send_all_inputs();
char *automator_inputs_table_to_json_string_alloc();

#endif
