//
//  init.h
//  mea-edomus
//
//  Created by Patrice Dietsch on 18/08/13.
//
//

#ifndef mea_edomus_init_h
#define mea_edomus_init_h

#include <inttypes.h>

int16_t checkInstallationPaths(char *base_path, int16_t try_to_create_flag);
int16_t checkParamsDb(char *sqlite3_db_param_path, int16_t *cause);
int16_t create_configs_php(char *gui_home, char *params_db_fullname, char *php_log_fullname);
int16_t initMeaEdomus(int16_t mode, char **params_list, char **keys);
int16_t updateMeaEdomus(char **params_list, char **keys);

#endif
