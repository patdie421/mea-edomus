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

#include "cJSON.h"
#include "xPL.h"

#include "mea_verbose.h"
#include "tokens.h"
#include "tokens_da.h"


PyObject *mea_getMemory(PyObject *self, PyObject *args, PyObject *mea_memory)
{
   PyObject *key;
   PyObject *mem;
   
   if(PyTuple_Size(args)!=1)
   {
      DEBUG_SECTION mea_log_printf("%s (%s) :  arguments error.\n", DEBUG_STR ,__func__);
      PyErr_BadArgument(); // à replacer
      return NULL;
   }
   
   key=PyTuple_GetItem(args, 0);
   if(!key)
   {
      DEBUG_SECTION mea_log_printf("%s (%s) :  bad mem id.\n", DEBUG_STR ,__func__);
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
   mea_addLong_to_pydict(data_dict, get_token_string_by_id(TYPEOFTYPE_ID), sqlite3_column_int(stmt, 12));
   mea_addLong_to_pydict(data_dict, get_token_string_by_id(INTERFACE_ID_ID), sqlite3_column_int(stmt, 13));

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
      char *name=(char *)sqlite3_column_name(stmt, i);
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


int mea_call_python_function2(PyObject *pFunc, PyObject *plugin_params_dict)
{
   PyObject *pArgs, *pValue=NULL;
   int retour=-1;

   if (pFunc && PyCallable_Check(pFunc))
   {
      pArgs = PyTuple_New(1);
      Py_INCREF(plugin_params_dict); // PyTuple_SetItem va voler la référence, on en rajoute une pour pouvoir ensuite faire un Py_DECREF
      PyTuple_SetItem(pArgs, 0, plugin_params_dict);

      pValue = PyObject_CallObject(pFunc, pArgs); // appel du plugin
      if (pValue != NULL)
      {
         retour=(int)PyInt_AsLong(pValue);
         Py_DECREF(pValue);
         DEBUG_SECTION mea_log_printf("%s (%s) : Result of call : %d\n", DEBUG_STR, __func__, retour);
      }
      else
      {
         if (PyErr_Occurred())
         {
            VERBOSE(5) {
               mea_log_printf("%s (%s) : python error - ", ERROR_STR, __func__ );
               PyErr_Print();
               fprintf(MEA_STDERR, "\n");
            }
         PyErr_Clear();
         }
      }
      Py_DECREF(pArgs);

      return retour;
   }
   else
   {
      return -1;
   }
}


int mea_call_python_function(char *plugin_name, char *plugin_func, PyObject *plugin_params_dict)
{
   PyObject *pName, *pModule, *pFunc;
   int retour=-1;

   PyErr_Clear();
   pName = PyString_FromString(plugin_name);
   pModule = PyImport_Import(pName);
   if(!pModule)
   {
      VERBOSE(5) mea_log_printf("%s (%s) : %s not found\n", ERROR_STR, __func__, plugin_name);
   }
   else
   {
      pFunc = PyObject_GetAttrString(pModule, plugin_func);
      if(pFunc)
      {
         retour=mea_call_python_function2(pFunc, plugin_params_dict);
      }
      else
      {
         VERBOSE(5) mea_log_printf("%s (%s) : %s not fount in %s module\n", ERROR_STR, __func__, plugin_func, plugin_name);
      }
      Py_XDECREF(pFunc);
   }
   Py_XDECREF(pModule);
   Py_DECREF(pName);
   PyErr_Clear();

   return retour;
}


PyObject *mea_xplMsgToPyDict2(cJSON *xplMsgJson)
{
}


PyObject *mea_xplMsgToPyDict(xPL_MessagePtr xplMsg)
{
   PyObject *pyXplMsg;
   PyObject *s;
   PyObject *l;
   char tmpStr[35]; // chaine temporaire. Taille max pour vendorID(8) + "-"(1) + deviceID(8) + "."(1) + instanceID(16)
   
   
   pyXplMsg = PyDict_New();
   if(!pyXplMsg)
   {
      PyErr_SetString(PyExc_RuntimeError, "ERROR (mea_xplMSgToPyDict) : PyDict_New error");
      return NULL;
   }
   
   // xplmsg
   l = PyLong_FromLong(1L);
   PyDict_SetItemString(pyXplMsg, "xplmsg", l);
   Py_DECREF(l);
   
   // message-type
   switch(xPL_getMessageType(xplMsg))
   {
      case xPL_MESSAGE_COMMAND:
         s=PyString_FromString("xpl-cmnd");
         break;
      case xPL_MESSAGE_STATUS:
         s= PyString_FromString("xpl-stat");
         break;
      case xPL_MESSAGE_TRIGGER:
         s= PyString_FromString("xpl-trig");
         break;
      default:
         PyErr_SetString(PyExc_RuntimeError, "ERROR (mea_xplMsgSend) : ...");
         return NULL;
   }
   PyDict_SetItemString(pyXplMsg, "message_xpl_type", s);
   Py_DECREF(s);
   
   // hop
   sprintf(tmpStr,"%d",xPL_getHopCount(xplMsg));
   s=PyString_FromString(tmpStr);
   PyDict_SetItemString(pyXplMsg, "hop", s);
   Py_DECREF(s);

   // source
   sprintf(tmpStr,"%s-%s.%s", xPL_getSourceVendor(xplMsg), xPL_getSourceDeviceID(xplMsg), xPL_getSourceInstanceID(xplMsg));
   s=PyString_FromString(tmpStr);
   PyDict_SetItemString(pyXplMsg, "source", s);
   Py_DECREF(s);

   if (xPL_isBroadcastMessage(xplMsg))
   {
      strcpy(tmpStr,"*");
   }
   else
   {
      sprintf(tmpStr,"%s-%s.%s", xPL_getTargetVendor(xplMsg), xPL_getTargetDeviceID(xplMsg), xPL_getTargetInstanceID(xplMsg));
   }
   s=PyString_FromString(tmpStr);
   PyDict_SetItemString(pyXplMsg, "target", s);
   Py_DECREF(s);
   
   // schema
   sprintf(tmpStr,"%s.%s", xPL_getSchemaClass(xplMsg), xPL_getSchemaType(xplMsg));
   s=PyString_FromString(tmpStr);
   PyDict_SetItemString(pyXplMsg, "schema", s);
   Py_DECREF(s);
   
   // body
   PyObject *pyBody=PyDict_New();
   xPL_NameValueListPtr body = xPL_getMessageBody(xplMsg);
   int n = xPL_getNamedValueCount(body);
   for (int16_t i = 0; i < n; i++)
   {
      xPL_NameValuePairPtr keyValuePtr = xPL_getNamedValuePairAt(body, i);
      if (keyValuePtr->itemValue != NULL)
      {
         s=PyString_FromString(keyValuePtr->itemValue);
      }
      else
      {
         s=Py_None;
         Py_INCREF(s);
      }
      PyDict_SetItemString(pyBody, keyValuePtr->itemName, s);
      Py_DECREF(s);
   }
   
   PyDict_SetItemString(pyXplMsg, "body", pyBody);
   Py_DECREF(pyBody);
   
   return pyXplMsg;
}

