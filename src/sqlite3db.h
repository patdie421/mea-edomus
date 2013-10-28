//
//  sqldb.h
//  mea-edomus
//
//  Created by Patrice Dietsch on 22/10/13.
//
//

#ifndef mea_edomus_sqldb_h
#define mea_edomus_sqldb_h

#include <inttypes.h>
#include <sqlite3.h>

int16_t sqlite3_dropTable(sqlite3 *sqlite3_param_db, char *table);
int16_t sqlite3_tableExist(sqlite3 *sqlite3_param_db, char *table);
int16_t sqlite3_dropDatabase(char *db_path);
int16_t sqlite3_doSqlQueries(sqlite3 *sqlite3_db, char *queries[]);

#endif
