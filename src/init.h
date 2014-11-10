//
//  init.h
//  mea-edomus
//
//  Created by Patrice Dietsch on 18/08/13.
//
//

#ifndef __init_h
#define __init_h

#include <inttypes.h>

struct upgrade_params_s
{
   *params_list[];
}

int16_t create_queries_db(char *queries_db_path);
int16_t checkInstallationPaths(char *base_path, int16_t try_to_create_flag);
int16_t checkParamsDb(char *sqlite3_db_param_path, int16_t *cause);
int16_t create_configs_php(char *gui_home, char *params_db_fullname, char *php_log_fullname, char *php_sessions_fullname, int iosocket_port);
int16_t initMeaEdomus(int16_t mode, char **params_list, char **keys);
int16_t updateMeaEdomus(char **params_list, char **keys);
int16_t upgrade_params_db(uint16_t fromVersion, uint16_t toVersion, struct upgrade_params_s *upgrade_params);

#endif
