// A renommer en python_utils.c

//
//  python_utils.c
//  mea-eDomus
//
//  Created by Patrice Dietsch on 04/06/13.
//
//
#include <Python.h>
#include <stdio.h>
#include <sqlite3.h>

#include "debug.h"
#include "error.h"
#include "tokens.h"


PyObject *mea_getMemory(PyObject *self, PyObject *args, PyObject *mea_memory)
{
   PyObject *key;
   PyObject *mem;
   
   if(PyTuple_Size(args)!=1)
   {
      DEBUG_SECTION fprintf(stderr, "%s (%s) :  arguments error.\n", DEBUG_STR ,__func__);
      PyErr_BadArgument(); // à replacer
      return NULL;
   }
   
   key=PyTuple_GetItem(args, 0);
   if(!key)
   {
      DEBUG_SECTION fprintf(stderr, "%s (%s) :  bad mem id.\n", DEBUG_STR ,__func__);
      PyErr_BadArgument(); // à remplacer
      return NULL;
   }
         
   mem=PyDict_GetItem(mea_memory, key);
   if(!mem)
   {
      mem = PyDict_New();
      if(PyDict_SetItem(mea_memory, key, mem)==-1)
      {
         Py_DECREF(mem);
         
         return NULL;
      }
   }
   
   Py_INCREF(mem);
   
   return mem;
}


void mea_addLong_to_pydict(PyObject *data_dict, char *key, long value)
{
   PyObject * val = PyLong_FromLong((long)value);
   PyDict_SetItemString(data_dict, key, val);
   Py_DECREF(val);
}


void mea_addDouble_to_pydict(PyObject *data_dict, char *key, double value)
{
   PyObject * val = PyFloat_FromDouble((double)value);
   PyDict_SetItemString(data_dict, key, val);
   Py_DECREF(val);
}


void mea_addString_to_pydict(PyObject *data_dict, char *key, char *value)
{
   PyObject *val = PyString_FromString(value);
   PyDict_SetItemString(data_dict, key, val);
   Py_DECREF(val);
}


void mea_addpydict_to_pydict(PyObject *data_dict, char *key, PyObject *adict)
{
   PyDict_SetItemString(data_dict, key, adict);
   Py_DECREF(adict);
}


PyObject *mea_stmt_to_pydict_device(sqlite3_stmt * stmt)
{
   PyObject *data_dict=NULL;
   
   data_dict=PyDict_New();
   if(!data_dict)
      return NULL;
   
   mea_addLong_to_pydict(data_dict, get_token_string_by_id(DEVICE_ID_ID), sqlite3_column_int(stmt, 0));
   mea_addString_to_pydict(data_dict, get_token_string_by_id(DEVICE_NAME_ID), (char *)sqlite3_column_text(stmt, 6));
   mea_addLong_to_pydict(data_dict, get_token_string_by_id(DEVICE_TYPE_ID_ID), sqlite3_column_int(stmt, 5));
   mea_addLong_to_pydict(data_dict, get_token_string_by_id(DEVICE_LOCATION_ID_ID), sqlite3_column_int(stmt, 1));
   mea_addString_to_pydict(data_dict, get_token_string_by_id(DEVICE_INTERFACE_NAME_ID), (char *)sqlite3_column_text(stmt, 7));
   mea_addString_to_pydict(data_dict, get_token_string_by_id(DEVICE_INTERFACE_TYPE_NAME_ID), (char *)sqlite3_column_text(stmt, 9));
   mea_addString_to_pydict(data_dict, get_token_string_by_id(DEVICE_STATE_ID), (char *)sqlite3_column_text(stmt, 2));
   mea_addString_to_pydict(data_dict, get_token_string_by_id(DEVICE_TYPE_PARAMETERS_ID), (char *)sqlite3_column_text(stmt, 4));
   mea_addLong_to_pydict(data_dict, get_token_string_by_id(TODBFLAG_ID), sqlite3_column_int(stmt, 11));

   return data_dict;
}


PyObject *mea_stmt_to_pydict_interface(sqlite3_stmt * stmt)
{
   PyObject *data_dict;
   
   data_dict=PyDict_New();
   if(!data_dict)
      return NULL;
   
   mea_addLong_to_pydict(data_dict, get_token_string_by_id(INTERFACE_ID_ID), (long)sqlite3_column_int(stmt, 1));
   mea_addLong_to_pydict(data_dict, get_token_string_by_id(INTERFACE_TYPE_ID_ID), (long)sqlite3_column_int(stmt, 2));
   mea_addString_to_pydict(data_dict, get_token_string_by_id(INTERFACE_NAME_ID), (char *)sqlite3_column_text(stmt, 3));
   mea_addLong_to_pydict(data_dict, get_token_string_by_id(INTERFACE_STATE_ID), (long)sqlite3_column_int(stmt, 7));
   
   return data_dict;
}


PyObject *mea_stmt_to_pydict(sqlite3_stmt * stmt)
{
   PyObject *data_dict;
   
   data_dict=PyDict_New();
   if(!data_dict)
      return NULL;
      
   int nbCol = sqlite3_column_count(stmt);
   for(int i=0;i<nbCol;i++)
   {
      char name=(char *)sqlite3_column_name(stmt, i);
      int type=(int)sqlite3_column_type(stmt, i);
      switch(type)
      {
         case  SQLITE_INTEGER:
            mea_addLong_to_pydict(data_dict, name, (long)sqlite3_column_int(stmt, i));
            break;
         case SQLITE_FLOAT:
            mea_addDouble_to_pydict(data_dict, name, (double)sqlite3_column_double(stmt, i));
            break;
         case SQLITE_TEXT:
            mea_addString_to_pydict(data_dict, name, (char *)sqlite3_column_text(stmt, i));
            break;
         default: // other do nothing ...
            break;
      }
   }
   return data_dict;
}

