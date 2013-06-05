//
//  python_api_utils.c
//  mea-eDomus
//
//  Created by Patrice Dietsch on 04/06/13.
//
//
#include <python.h>

#include <stdio.h>

void addLong_to_pydict(PyObject *data_dict, char *key, long value)
{
   PyObject * val = PyLong_FromLong((long)value);
   PyDict_SetItemString(data_dict, key, val);
   Py_DECREF(val);
}

void addString_to_pydict(PyObject *data_dict, char *key, char *value)
{
   PyObject *val = PyString_FromString(value);
   PyDict_SetItemString(data_dict, key, val);
   Py_DECREF(val);
}


