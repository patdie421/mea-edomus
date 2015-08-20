//
//  python_utils.h
//  mea-eDomus
//
//  Created by Patrice Dietsch on 04/06/13.
//
//

#ifndef mea_eDomus_python_utils_h
#define mea_eDomus_python_utils_h

PyObject *mea_getMemory(PyObject *self, PyObject *args, PyObject *mea_memory);

void mea_addLong_to_pydict(PyObject *data_dict, char *key, long value);
void mea_addDouble_to_pydict(PyObject *data_dict, char *key, double value);
void mea_addString_to_pydict(PyObject *data_dict, char *key, char *value);
void mea_addpydict_to_pydict(PyObject *data_dict, char *key, PyObject *adict);

PyObject *mea_stmt_to_pydict(sqlite3_stmt * stmt);
PyObject *mea_stmt_to_pydict_device(sqlite3_stmt * stmt);
PyObject *mea_stmt_to_pydict_interface(sqlite3_stmt * stmt);

#endif
