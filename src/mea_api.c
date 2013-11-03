//
//  mea_api.c
//
//  Created by Patrice Dietsch on 09/05/13.
//
//
#include <Python.h>
#include <stdio.h>

#include "error.h"
#include "debug.h"
#include "queue.h"
#include "xPL.h"
#include "xPLServer.h"
#include "token_strings.h"
#include "dbServer.h"
#include "xbee.h"

#include "mea_api.h"

PyObject *mea_memory;

static PyMethodDef MeaMethods[] = {
   {"getMemory",            mea_getMemory,            METH_VARARGS, "Return a dictionary"},
   {"sendAtCmdAndWaitResp", mea_sendAtCmdAndWaitResp, METH_VARARGS, "Envoie d'une commande AT et recupere la reponse"},
   {"sendAtCmd",            mea_sendAtCmd,            METH_VARARGS, "Envoie d'une commande AT sans attendre de reponse"},
   {"xplGetVendorID",       mea_xplGetVendorID,       METH_VARARGS, "VendorID"},
   {"xplGetDeviceID",       mea_xplGetDeviceID,       METH_VARARGS, "DeviceID"},
   {"xplGetInstanceID",     mea_xplGetInstanceID,     METH_VARARGS, "InstanceID"},
   {"xplSendMsg",           mea_xplSendMsg,           METH_VARARGS, "Envoie un message XPL"},
   {"addDataToSensorsValuesTable",mea_addDataToSensorsValuesTable, METH_VARARGS, "Envoi des donnees dans la table sensors_values"},
   {NULL, NULL, 0, NULL}
};


void mea_api_init()
{
   mea_memory=PyDict_New(); // initialisation de la mémoire
   
   Py_InitModule("mea", MeaMethods);  
}


static PyObject *mea_xplGetVendorID()
{
   return PyString_FromString("mea");
}


static PyObject *mea_xplGetDeviceID()
{
   return  PyString_FromString("edomus");
}


static PyObject *mea_xplGetInstanceID()
{
   return PyString_FromString("cheznousdev");
}


PyObject *xplMsgToPyDict(xPL_MessagePtr xplMsg)
{
   PyObject *pyXplMsg;
   PyObject *s;
   PyObject *l;
   char tmpStr[35]; // chaine temporaire. Taille max pour vendorID(8) + "-"(1) + deviceID(8) + "."(1) + instanceID(16)
   
   
   pyXplMsg = PyDict_New();
   if(!pyXplMsg)
   {
      PyErr_SetString(PyExc_RuntimeError, "ERROR (xplMSgToPyDict) : PyDict_New error");
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
   for (int i = 0; i < n; i++)
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


static PyObject *mea_xplSendMsg(PyObject *self, PyObject *args)
{
   PyObject *PyXplMsg;
   PyObject *item;
   PyObject *body;
   
   if(PyTuple_Size(args)!=1)
   {
      PyErr_SetString(PyExc_RuntimeError, "ERROR (mea_xplMsgSend) : need one argument");
      
      return NULL;
   }
   
   // recuperation du parametre
   PyXplMsg=PyTuple_GetItem(args, 0);
   
   // le parametre doit etre de type “dictionnaire”
   if(!PyDict_Check(PyXplMsg))
   {
      PyErr_SetString(PyExc_RuntimeError, "ERROR (mea_xplMsgSend) : argument not a dictionary");
      return NULL;
   }
   
   // on peut commencer a recuperer les elements necessaires
   // il nous faut :
   // - le type de message xpl (xpl-cmnd, xpl-trig ou xpl-stat)
   // - le schema
   // - les donnees
   
   xPL_MessagePtr xplMsg=NULL;
   
   // recuperation du type de message
   item = PyDict_GetItemString(PyXplMsg, "message_xpl_type");
   if(item)
   {
      char *p=PyString_AsString(item);
      int message_type=-1;
      if(p)
      {
         if(strcmp(p,"xpl-trig")==0)
            message_type=xPL_MESSAGE_TRIGGER;
         else if(strcmp(p,"xpl-cmnd")==0)
            message_type=xPL_MESSAGE_COMMAND;
         else if(strcmp(p,"xpl-stat")==0)
            message_type=xPL_MESSAGE_STATUS;
         else
         {
            PyErr_SetString(PyExc_RuntimeError, "ERROR (mea_xplMsgSend) : xpl message type error");
            return NULL;
         }
         
//         xplMsg=NULL;
         xPL_ServicePtr servicePtr = get_xPL_ServicePtr();
         if(servicePtr)
            xplMsg = xPL_createBroadcastMessage(servicePtr, message_type);
         if(!xplMsg)
         {
            PyErr_SetString(PyExc_RuntimeError, "ERROR (mea_xplMsgSend) : internal error");
            return NULL;
         }
      }
   }
   else
   {
      VERBOSE(9) fprintf(stderr, "ERROR (mea_xplMsgSend) : message-type not found\n");
      PyErr_SetString(PyExc_RuntimeError, "ERROR (mea_xplMsgSend) : xpl message type not found");
      return NULL;
   }
   
   
   // recuperation de la class et du type du message XPL
   item = PyDict_GetItemString(PyXplMsg, "schema");
   if(item)
   {
      char *p=PyString_AsString(item);
      if(p)
      {
         char xpl_class[9], xpl_type[9];
         int n=sscanf(p,"%[^.].%s", xpl_class, xpl_type);
         if(n==2)
            xPL_setSchema(xplMsg, xpl_class, xpl_type);
         else
         {
            PyErr_SetString(PyExc_RuntimeError, "ERROR (mea_xplMsgSend) : bad schema");
            goto mea_xplSendMsg_exit;
         }
      }
      else
      {
         PyErr_SetString(PyExc_RuntimeError, "ERROR (mea_xplMsgSend) : schema not a string");
         goto mea_xplSendMsg_exit;
      }
   }
   else
   {
      PyErr_SetString(PyExc_RuntimeError, "ERROR (mea_xplMsgSend) : xpl schema not found");
      goto mea_xplSendMsg_exit;
   }
   
   
   body = PyDict_GetItemString(PyXplMsg, "xpl-body");
   if(body)
   {
      if(!PyDict_Check(body))
      {
         PyErr_SetString(PyExc_RuntimeError, "ERROR (mea_xplMsgSend) : body not a dictionary.\n");
         goto mea_xplSendMsg_exit;
      }
      
      // parcours de la liste
      PyObject *key, *value;
      Py_ssize_t pos = 0;
      while (PyDict_Next(body, &pos, &key, &value))
      {
         char *skey=PyString_AS_STRING(key);
         char *svalue=PyString_AS_STRING(value);
         
         if(!key || !value)
         {
            PyErr_SetString(PyExc_RuntimeError, "ERROR (mea_xplMsgSend) : incorrect data in body");
            goto mea_xplSendMsg_exit;
         }
         xPL_setMessageNamedValue(xplMsg, skey, svalue);
      }
   }
   else
   {
      PyErr_SetString(PyExc_RuntimeError, "ERROR (mea_xplMsgSend) : xpl body data not found");
      goto mea_xplSendMsg_exit;
   }
   
   xPL_sendMessage(xplMsg);
   xPL_releaseMessage(xplMsg);
   
   return PyLong_FromLong(1L); // return True
   
mea_xplSendMsg_exit:
   if(xplMsg)
      xPL_releaseMessage(xplMsg);
   
   return NULL; // retour un exception python
}


static PyObject *mea_getMemory(PyObject *self, PyObject *args)
{
   PyObject *key;
   PyObject *mem;
   
   if(PyTuple_Size(args)!=1)
   {
      VERBOSE(9) fprintf(stderr, "ERROR (mea_get_memory) : arguments error\n");
      PyErr_BadArgument(); // à replacer
      return NULL;
   }
   
   key=PyTuple_GetItem(args, 0);
   if(!key)
   {
      VERBOSE(9) fprintf(stderr, "ERROR (mea_get_memory) : bad mem id\n");
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


uint32_t indianConvertion(uint32_t val_x86)
{
   uint32_t val_xbee;
   char *val_x86_ptr;
   char *val_xbee_ptr;
   
   val_x86_ptr = (char *)&val_x86;
   val_xbee_ptr = (char *)&val_xbee;
   
   // conversion little vers big indian
   for(int i=0,j=3;i<sizeof(uint32_t);i++)
      val_xbee_ptr[i]=val_x86_ptr[j-i];

   return val_xbee;
}


static PyObject *mea_sendAtCmdAndWaitResp(PyObject *self, PyObject *args)
{
   unsigned char at_cmd[81];
   uint16_t l_at_cmd;
   
   unsigned char resp[81];
   uint16_t l_resp;
   
   int16_t ret;
   PyObject *arg;

   xbee_host_t *host=NULL;

   // récupération des paramètres et contrôle des types
   if(PyTuple_Size(args)!=5)
      goto mea_AtCmdToXbee_arg_err;

   xbee_xd_t *xd;
   arg=PyTuple_GetItem(args, 0);
   if(PyLong_Check(arg))
      xd=(xbee_xd_t *)PyLong_AsLong(arg);
   else
      goto mea_AtCmdToXbee_arg_err;
   
   uint32_t addr_h;
   arg=PyTuple_GetItem(args, 1);
   if(PyNumber_Check(arg))
      addr_h=(uint32_t)PyLong_AsLong(arg);
   else
      goto mea_AtCmdToXbee_arg_err;
   
   uint32_t addr_l;
   arg=PyTuple_GetItem(args, 2);
   if(PyNumber_Check(arg))
      addr_l=(uint32_t)PyLong_AsLong(arg);
   else
      goto mea_AtCmdToXbee_arg_err;
   
   char *at;
   arg=PyTuple_GetItem(args, 3);
   if(PyString_Check(arg))
      at=(char *)PyString_AsString(arg);
   else
      goto mea_AtCmdToXbee_arg_err;

   at_cmd[0]=at[0];
   at_cmd[1]=at[1];
   
   arg=PyTuple_GetItem(args, 4);
   if(PyNumber_Check(arg))
   {
      uint32_t val=(uint32_t)PyLong_AsLong(arg);
      uint32_t val_xbee=indianConvertion(val);
      char *val_xbee_ptr=(char *)&val_xbee;
      
      for(int i=0;i<sizeof(uint32_t);i++)
         at_cmd[2+i]=val_xbee_ptr[i];
      l_at_cmd=6;
   }
   else if (PyString_Check(arg))
   {
      char *at_arg=(char *)PyString_AsString(arg);
      uint16_t i;
      for(i=0;i<strlen(at_arg);i++)
         at_cmd[2+i]=at_arg[i];
      if(i>0)
         l_at_cmd=2+i;
      else
         l_at_cmd=2;
   }
   else
      goto mea_AtCmdToXbee_arg_err;

   // recuperer le host dans la table (necessite d'avoir accès à xd
   int16_t err;
   
   host=NULL;
   host=(xbee_host_t *)malloc(sizeof(xbee_host_t)); // description de l'xbee directement connecté
   xbee_get_host_by_addr_64(xd, host, addr_h, addr_l, &err);
   if(err==XBEE_ERR_NOERR)
   {
   }
   else
   {
      VERBOSE(3) fprintf(stderr, "%s (%s) : host not found\n",ERROR_STR,__func__);
      if(host)
         free(host);
      PyErr_BadArgument(); // à remplacer
      return NULL;
   }
   
   int16_t nerr;
   ret=xbee_atCmdSendAndWaitResp(xd, host, at_cmd, l_at_cmd, resp, &l_resp, &nerr);
   if(ret==-1)
   {
      VERBOSE(9) fprintf(stderr, "%s (%s) : error %d\n",ERROR_STR,__func__,nerr);
      if(host)
         free(host);
      PyErr_BadArgument(); // à remplacer
      return NULL;
   }
   
   struct xbee_remote_cmd_response_s *mapped_resp=(struct xbee_remote_cmd_response_s *)resp;
   
   void *data;
   long l_data;

   PyObject *t=PyTuple_New(3);
   PyObject *py_cmd=PyBuffer_New(l_resp);
   if (!PyObject_AsWriteBuffer(py_cmd, &data, &l_data))
   {
      memcpy(data,resp,l_data);
      PyTuple_SetItem(t, 0, PyLong_FromLong(mapped_resp->cmd_status));
      PyTuple_SetItem(t, 1, py_cmd);
      PyTuple_SetItem(t, 2, PyLong_FromLong(l_resp));
   }
   else
   {
      Py_DECREF(py_cmd);
      Py_DECREF(t);
   }
   
   free(host);

   return t; // return True
   
mea_AtCmdToXbee_arg_err:
   VERBOSE(9) fprintf(stderr, "%s (%s) : arguments error\n",ERROR_STR,__func__);
   if(host)
   {
      free(host);
      host=NULL;
   }
   
   PyErr_BadArgument();
   return NULL;
}


static PyObject *mea_sendAtCmd(PyObject *self, PyObject *args)
{
   unsigned char at_cmd[81];
   uint16_t l_at_cmd;
   
//   int16_t ret;
   PyObject *arg;
   
   xbee_host_t *host=NULL;
   
   // récupération des paramètres et contrôle des types
   if(PyTuple_Size(args)!=5)
      goto mea_atCmdSend_arg_err;
   
   xbee_xd_t *xd;
   arg=PyTuple_GetItem(args, 0);
   if(PyLong_Check(arg))
      xd=(xbee_xd_t *)PyLong_AsLong(arg);
   else
      goto mea_atCmdSend_arg_err;
   
   uint32_t addr_h;
   arg=PyTuple_GetItem(args, 1);
   if(PyNumber_Check(arg))
      addr_h=(uint32_t)PyLong_AsLong(arg);
   else
      goto mea_atCmdSend_arg_err;
   
   uint32_t addr_l;
   arg=PyTuple_GetItem(args, 2);
   if(PyNumber_Check(arg))
      addr_l=(uint32_t)PyLong_AsLong(arg);
   else
      goto mea_atCmdSend_arg_err;
   
   char *at;
   arg=PyTuple_GetItem(args, 3);
   if(PyString_Check(arg))
      at=(char *)PyString_AsString(arg);
   else
      goto mea_atCmdSend_arg_err;
   
   at_cmd[0]=at[0];
   at_cmd[1]=at[1];
   
   arg=PyTuple_GetItem(args, 4);
   if(PyNumber_Check(arg))
   {
      uint32_t val=(uint32_t)PyLong_AsLong(arg);
      uint32_t val_xbee=indianConvertion(val);
      char *val_xbee_ptr=(char *)&val_xbee;
      
      for(int i=0;i<sizeof(uint32_t);i++)
         at_cmd[2+i]=val_xbee_ptr[i];
      l_at_cmd=6;
   }
   else if (PyString_Check(arg))
   {
      char *at_arg=(char *)PyString_AsString(arg);
      uint16_t i;
      for(i=0;i<strlen(at_arg);i++)
         at_cmd[2+i]=at_arg[i];
      if(i>0)
         l_at_cmd=2+i;
      else
         l_at_cmd=2;
   }
   else
      goto mea_atCmdSend_arg_err;
   
   int16_t err;
   
   host=(xbee_host_t *)malloc(sizeof(xbee_host_t)); // description de l'xbee directement connecté
   xbee_get_host_by_addr_64(xd, host, addr_h, addr_l, &err);
   if(err==XBEE_ERR_NOERR)
   {
   }
   else
   {
      VERBOSE(9) fprintf(stderr, "%s (%s) : host not found\n", ERROR_STR,__func__);
      goto mea_atCmdSend_arg_err;
   }
   
   int16_t nerr;
   // ret=
   xbee_atCmdSend(xd, host, at_cmd, l_at_cmd, &nerr);
   
   free(host);
   
   return PyLong_FromLong(1L); // return True

mea_atCmdSend_arg_err:
   VERBOSE(9) fprintf(stderr, "%s (%s) : arguments error\n", ERROR_STR,__func__);
   PyErr_BadArgument();
   if(host)
   {
      free(host);
      host=NULL;
   }
   return NULL;
}


//static PyObject *mea_addDataToSensorsValuesTable(tomysqldb_md_t *md, uint16_t sensor_id, float value1, uint16_t unit, float value2, char *complement)
static PyObject *mea_addDataToSensorsValuesTable(PyObject *self, PyObject *args)
{
   uint16_t sensor_id;
   float value1, value2;
   uint16_t unit;
   char *complement;

   PyObject *arg;
   
   
   // récupération des paramètres et contrôle des types
   if(PyTuple_Size(args)!=5)
      goto mea_addDataToSensorsValuesTable_arg_err;
   
   arg=PyTuple_GetItem(args, 0);
   if(PyNumber_Check(arg))
      sensor_id=(uint16_t)PyLong_AsLong(arg);
   else
      goto mea_addDataToSensorsValuesTable_arg_err;
   
   arg=PyTuple_GetItem(args, 1);
   if(PyNumber_Check(arg))
      value1=(float)PyFloat_AsDouble(arg);
   else
      goto mea_addDataToSensorsValuesTable_arg_err;

   arg=PyTuple_GetItem(args, 2);
   if(PyNumber_Check(arg))
      unit=(uint16_t)PyLong_AsLong(arg);
   else
      goto mea_addDataToSensorsValuesTable_arg_err;
   
   arg=PyTuple_GetItem(args, 3);
   if(PyNumber_Check(arg))
      value2=(float)PyFloat_AsDouble(arg);
   else
      goto mea_addDataToSensorsValuesTable_arg_err;
   
   arg=PyTuple_GetItem(args, 4);
   if(PyString_Check(arg))
      complement=(char *)PyString_AsString(arg);
   else
      goto mea_addDataToSensorsValuesTable_arg_err;

   printf("Params : %d %f %d %f %s\n",sensor_id,value1,unit,value2,complement);
   
   return PyLong_FromLong(1L); // True
   
mea_addDataToSensorsValuesTable_arg_err:
   VERBOSE(9) fprintf(stderr, "%s (%s) : arguments error\n", ERROR_STR,__func__);
   PyErr_BadArgument();
   return NULL;
}
