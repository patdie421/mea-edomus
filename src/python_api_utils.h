//
//  python_api_utils.h
//  mea-eDomus
//
//  Created by Patrice Dietsch on 04/06/13.
//
//

#ifndef mea_eDomus_python_api_utils_h
#define mea_eDomus_python_api_utils_h

void addLong_to_pydict(PyObject *data_dict, char *key, long value);
void addString_to_pydict(PyObject *data_dict, char *key, char *value);
void addpydict_to_pydict(PyObject *data_dict, char *key, PyObject *adict);

#endif
