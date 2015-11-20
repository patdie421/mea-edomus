//
//  mea_api.c
//
//  Created by Patrice Dietsch on 09/05/13.
//
//
#include <Python.h>
#include <stdio.h>

#include "globals.h"
#include "xPL.h"
#include "xPLServer.h"
#include "tokens.h"
#include "dbServer.h"
#include "xbee.h"
#include "enocean.h"
#include "mea_verbose.h"

#include "python_utils.h"

#include "mea_api.h"


PyObject *mea_memory=NULL;
PyObject *mea_module=NULL;


static PyMethodDef MeaMethods[] = {
   {"getMemory",                    mea_api_getMemory,                METH_VARARGS, "Return a dictionary"},
   {"sendXbeeCmdAndWaitResp",       mea_sendAtCmdAndWaitResp,         METH_VARARGS, "Envoie d'une commande AT et recupere la reponse"},
   {"sendXbeeCmd",                  mea_sendAtCmd,                    METH_VARARGS, "Envoie d'une commande AT sans attendre de reponse"},
   {"sendEnoceanPacketAndWaitResp", mea_sendEnoceanPacketAndWaitResp, METH_VARARGS, "Envoie un packet et recupere la reponse"},
   {"enoceanCRC",                   mea_enoceanCRC,                   METH_VARARGS, "Envoie un packet et recupere la reponse"},
   {"xplGetVendorID",               mea_xplGetVendorID,               METH_VARARGS, "VendorID"},
   {"xplGetDeviceID",               mea_xplGetDeviceID,               METH_VARARGS, "DeviceID"},
   {"xplGetInstanceID",             mea_xplGetInstanceID,             METH_VARARGS, "InstanceID"},
   {"xplSendMsg",                   mea_xplSendMsg,                   METH_VARARGS, "Envoie un message XPL"},
   {"addDataToSensorsValuesTable",  mea_addDataToSensorsValuesTable,  METH_VARARGS, "Envoi des donnees dans la table sensors_values"},
   {"sendSerialData",               mea_write,                        METH_VARARGS, "Envoi des donnees vers une ligne serie"},
   {"receiveSerialData",            mea_read,                         METH_VARARGS, "recupere des donnees depuis une ligne serie"},
   {NULL, NULL, 0, NULL}
};


static PyObject *mea_api_getMemory(PyObject *self, PyObject *args)
{
   return mea_getMemory(self, args, mea_memory);
}


int16_t _check_todbflag(sqlite3 *db, uint16_t sensor_id)
{
   char sql[255];
   sqlite3_stmt * stmt;
   int ret;
   
   DEBUG_SECTION mea_log_printf("%s (%s) : check loggin for sensor n#%d.\n", DEBUG_STR ,__func__,sensor_id);
   snprintf(sql,sizeof(sql),"SELECT id,todbflag,id_sensor_actuator FROM sensors_actuators WHERE id_sensor_actuator = %d",sensor_id);
   ret = sqlite3_prepare_v2(db, sql, strlen(sql)+1, &stmt, NULL);
   if(ret)
   {
      VERBOSE(2) mea_log_printf("%s (%s) : sqlite3_prepare_v2 - %s\n", ERROR_STR, __func__, sqlite3_errmsg (db));
      return -1;
   }
   DEBUG_SECTION mea_log_printf("%s (%s) : query = %s\n", DEBUG_STR ,__func__, sql);
   
   while(1)
   {
      int s = sqlite3_step(stmt);
      if (s == SQLITE_ROW)
      {
         int val=sqlite3_column_int(stmt, 1);
         DEBUG_SECTION mea_log_printf("%s (%s) : %d == %d\n", DEBUG_STR ,__func__, sqlite3_column_int(stmt,2),sensor_id);
         
         if(sqlite3_column_int(stmt,2)==sensor_id)
         {
            sqlite3_finalize(stmt);
            DEBUG_SECTION mea_log_printf("%s (%s) : sensor found, flag = %d\n", DEBUG_STR ,__func__, val);
            return val;
         }
      }
      else
      {
         sqlite3_finalize(stmt);
         DEBUG_SECTION mea_log_printf("%s (%s) : sensor not found\n", DEBUG_STR ,__func__);
         return -1;
      }
   }
   return -1;
}


uint32_t _indianConvertion(uint32_t val_x86)
{
   uint32_t val_xbee;
   char *val_x86_ptr;
   char *val_xbee_ptr;
   
   val_x86_ptr = (char *)&val_x86;
   val_xbee_ptr = (char *)&val_xbee;
   
   // conversion little vers big indian
   for(int16_t i=0,j=3;i<sizeof(uint32_t);i++)
      val_xbee_ptr[i]=val_x86_ptr[j-i];

   return val_xbee;
}


PyObject *mea_enoceanCRC(PyObject *self, PyObject *args)
{
   PyObject *arg;

   unsigned long *prev;
   unsigned long *c;
   
   if(PyTuple_Size(args)!=2)
      goto mea_enoceanCRC_arg_err;

   arg=PyTuple_GetItem(args, 0);
   if(PyLong_Check(arg))
      prev=(unsigned long *)PyLong_AsVoidPtr(arg);
   else
      goto mea_enoceanCRC_arg_err;
      
   arg=PyTuple_GetItem(args, 1);
   if(PyLong_Check(arg))
      c=(unsigned long *)PyLong_AsVoidPtr(arg);
   else
      goto mea_enoceanCRC_arg_err;
      
   uint8_t _prev=(uint8_t)(*prev & 0xFF);
   uint8_t _c=(uint8_t)(*c & 0xFF);
   uint8_t _crc = proc_crc8(_prev,_c);
   
   return PyLong_FromLong((unsigned long)_crc);
   
mea_enoceanCRC_arg_err:
   PyErr_BadArgument();
   return NULL;
}


PyObject *mea_sendEnoceanPacketAndWaitResp(PyObject *self, PyObject *args)
{
   PyObject *arg;
   enocean_ed_t *ed;
   int16_t nerr;
   int16_t ret;
   
   // récupération des paramètres et contrôle des types
   if(PyTuple_Size(args)!=3)
      goto mea_sendEnoceanPacketAndWaitResp_arg_err;

   // enocean_ed
   arg=PyTuple_GetItem(args, 0);
   if(PyLong_Check(arg))
      ed=(enocean_ed_t *)PyLong_AsLong(arg);
   else
      goto mea_sendEnoceanPacketAndWaitResp_arg_err;

   // packet
   arg=PyTuple_GetItem(args, 1);
   Py_buffer py_packet;
   if(PyObject_CheckBuffer(arg))
   {
      ret=PyObject_GetBuffer(arg, &py_packet, PyBUF_SIMPLE);
      if(ret<0)
         goto mea_sendEnoceanPacketAndWaitResp_arg_err;
   }
   else
      goto mea_sendEnoceanPacketAndWaitResp_arg_err;

   // response
   arg=PyTuple_GetItem(args, 2);
   Py_buffer py_response;
   if(PyObject_CheckBuffer(arg))
   {
      ret=PyObject_GetBuffer(arg, &py_response, PyBUF_SIMPLE | PyBUF_WRITABLE);
      if(ret<0)
         goto mea_sendEnoceanPacketAndWaitResp_arg_err;
   }
   else
      goto mea_sendEnoceanPacketAndWaitResp_arg_err;

   uint16_t l_response=py_response.len;
   ret=enocean_send_packet(ed,
                           py_packet.buf,
                           py_packet.len,
                           py_response.buf,
                           &l_response,
                           &nerr);

   // réponse
   PyObject *t=PyTuple_New(3);
   PyTuple_SetItem(t, 0, PyLong_FromLong(ret));
   PyTuple_SetItem(t, 1, PyLong_FromLong(nerr));
   PyTuple_SetItem(t, 2, PyLong_FromLong(l_response));
   return t;
   
mea_sendEnoceanPacketAndWaitResp_arg_err:
   PyErr_BadArgument();
   return NULL;
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


void mea_api_init()
{
   mea_memory=PyDict_New(); // initialisation de la mémoire
   
   mea_module=Py_InitModule("mea", MeaMethods);  
}


void mea_api_release()
{
// /!\ a ecrire pour librérer tous le contenu de la memoire partagé ...
   if(mea_memory)
      Py_DECREF(mea_memory);
   if(mea_module)
      Py_DECREF(mea_module);
}


static PyObject *mea_xplGetVendorID()
{
   return PyString_FromString(mea_getXPLVendorID());
}


static PyObject *mea_xplGetDeviceID()
{
   return  PyString_FromString(mea_getXPLDeviceID());
}


static PyObject *mea_xplGetInstanceID()
{
   return PyString_FromString(mea_getXPLInstanceID());
}


static PyObject *mea_xplSendMsg(PyObject *self, PyObject *args)
{
   PyObject *PyXplMsg;
   PyObject *item;
   PyObject *body;

            
   char vendor_id[40]="";
   char device_id[40]="";
   char instance_id[40]="";
   
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
         item = PyDict_GetItemString(PyXplMsg, "source");
         if(!item)
         {
            PyErr_SetString(PyExc_RuntimeError, "ERROR (mea_xplMsgSend) : xpl message no source");
            return NULL;
         }
         p=PyString_AsString(item);
         
         xPL_ServicePtr servicePtr = mea_getXPLServicePtr();
         if(servicePtr)
         {
            int16_t n=sscanf(p,"%[^-]-%[^.].%s", vendor_id, device_id, instance_id);
            if(n==3)
            {
               if(strcmp(device_id,"internal")==0)
               {
                  xplMsg = mea_createSendableMessage(message_type, vendor_id, device_id, instance_id);
                  xPL_setBroadcastMessage(xplMsg, TRUE);
               }
               else
               {
                  xplMsg = xPL_createBroadcastMessage(servicePtr, message_type);
               }
            }
            else
               xplMsg = xPL_createBroadcastMessage(servicePtr, message_type);

         }

         if(!xplMsg)
         {
            PyErr_SetString(PyExc_RuntimeError, "ERROR (mea_xplMsgSend) : internal error");
            return NULL;
         }
      }
   }
   else
   {
      DEBUG_SECTION mea_log_printf("ERROR (mea_xplMsgSend) : message-type not found\n");
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
         int16_t n=sscanf(p,"%[^.].%s", xpl_class, xpl_type);
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
   
   item = PyDict_GetItemString(PyXplMsg, "target");
   if(item)
   {
      char *p=PyString_AsString(item);
      if(strcmp(p,"*")!=0)
      {
         // spliter correctement d'adresse et faire le xPL_setMessageTarget qui va bien
         int16_t n=sscanf(p,"%[^-]-%[^.].%s", vendor_id, device_id, instance_id);
         if(n==3)
         {
            // valider le format ici et si OK

            xPL_setTarget(xplMsg, vendor_id, device_id, instance_id);

            // si target exist et est conforme => ce n'est plus un message broadcast
            xPL_setBroadcastMessage(xplMsg, FALSE);
         }

      }
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
            PyErr_SetString(PyExc_RuntimeError, "ERROR (mea_xplMsgSend) : incorrect data in body.");
            goto mea_xplSendMsg_exit;
         }
         xPL_setMessageNamedValue(xplMsg, skey, svalue);
      }
   }
   else
   {
      PyErr_SetString(PyExc_RuntimeError, "ERROR (mea_xplMsgSend) : xpl body data not found.");
      goto mea_xplSendMsg_exit;
   }
   
   //xPL_sendMessage(xplMsg);
   mea_sendXPLMessage(xplMsg);

   xPL_releaseMessage(xplMsg);
   
   return PyLong_FromLong(1L); // return True
   
mea_xplSendMsg_exit:
   if(xplMsg)
      xPL_releaseMessage(xplMsg);
   
   return NULL; // retour un exception python
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
      uint32_t val_xbee=_indianConvertion(val);
      char *val_xbee_ptr=(char *)&val_xbee;
      
      for(int16_t i=0;i<sizeof(uint32_t);i++)
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
      DEBUG_SECTION mea_log_printf("%s (%s) : host not found.\n", DEBUG_STR ,__func__);
      if(host)
      {
         free(host);
         host=NULL;
      }
      PyErr_BadArgument(); // à remplacer
      return NULL;
   }
   
   int16_t nerr;
   ret=xbee_atCmdSendAndWaitResp(xd, host, at_cmd, l_at_cmd, resp, &l_resp, &nerr);
   if(ret==-1)
   {
      DEBUG_SECTION mea_log_printf("%s (%s) : error %d.\n", DEBUG_STR, __func__, nerr);
      if(host)
      {
         free(host);
         host=NULL;
      }
      PyErr_BadArgument(); // à remplacer
      return NULL;
   }
   
   struct xbee_remote_cmd_response_s *mapped_resp=(struct xbee_remote_cmd_response_s *)resp;
   
   void *data;
   long l_data;

   PyObject *t=PyTuple_New(3);
   PyObject *py_cmd=PyBuffer_New(l_resp);
   if (!PyObject_AsWriteBuffer(py_cmd, &data, (Py_ssize_t *)&l_data))
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
   host=NULL;

   return t; // return True
   
mea_AtCmdToXbee_arg_err:
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
      uint32_t val_xbee=_indianConvertion(val);
      char *val_xbee_ptr=(char *)&val_xbee;
      
      for(int16_t i=0;i<sizeof(uint32_t);i++)
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
   
   if(addr_l != 0xFFFFFFFF && addr_h != 0xFFFFFFFF)
   {
      host=(xbee_host_t *)malloc(sizeof(xbee_host_t)); // description de l'xbee directement connecté
      xbee_get_host_by_addr_64(xd, host, addr_h, addr_l, &err);
      if(err!=XBEE_ERR_NOERR)
      {
         DEBUG_SECTION mea_log_printf("%s (%s) : host not found.\n", DEBUG_STR,__func__);
         goto mea_atCmdSend_arg_err;
      }
   }
   else
      host=NULL;
   
   int16_t nerr;
   // ret=
   xbee_atCmdSend(xd, host, at_cmd, l_at_cmd, &nerr);
   
   if(host)
   {
      free(host);
      host=NULL;
   }
   return PyLong_FromLong(1L); // return True

mea_atCmdSend_arg_err:
   DEBUG_SECTION mea_log_printf("%s (%s) : arguments error\n", DEBUG_STR,__func__);
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
   double value1, value2;
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
      value1=PyFloat_AsDouble(arg);
   else
      goto mea_addDataToSensorsValuesTable_arg_err;

   arg=PyTuple_GetItem(args, 2);
   if(PyNumber_Check(arg))
      unit=(uint16_t)PyLong_AsLong(arg);
   else
      goto mea_addDataToSensorsValuesTable_arg_err;
   
   arg=PyTuple_GetItem(args, 3);
   if(PyNumber_Check(arg))
      value2=PyFloat_AsDouble(arg);
   else
      goto mea_addDataToSensorsValuesTable_arg_err;
   
   arg=PyTuple_GetItem(args, 4);
   if(PyString_Check(arg))
      complement=(char *)PyString_AsString(arg);
   else
      goto mea_addDataToSensorsValuesTable_arg_err;

   sqlite3 *db=get_sqlite3_param_db();
   if(db)
   {
      if(_check_todbflag(db, sensor_id)==1)
      {
         dbServer_add_data_to_sensors_values(sensor_id, value1, unit, value2, complement);
      }
      else
      {
      }
   }
   
   return PyLong_FromLong(1L); // True
   
mea_addDataToSensorsValuesTable_arg_err:
   DEBUG_SECTION mea_log_printf("%s (%s) : arguments error\n", DEBUG_STR,__func__);
   PyErr_BadArgument();
   return NULL;
}


static PyObject *mea_read(PyObject *self, PyObject *args)
{
   PyObject *arg;

   int fd=-1;
   char *data=NULL;
   int l_data=0;

   // récupération des paramètres et contrôle des types
   if(PyTuple_Size(args)!=2)
      goto mea_write_arg_err;

   arg=PyTuple_GetItem(args, 0);
   if(PyNumber_Check(arg))
      fd=(int)PyLong_AsLong(arg);
   else
      goto mea_write_arg_err;

   arg=PyTuple_GetItem(args, 1);
   if(PyNumber_Check(arg))
      l_data=PyLong_AsLong(arg);
   else
      goto mea_write_arg_err;

   data=malloc(l_data);
   if(data==NULL)
   {
      PyErr_SetString(PyExc_RuntimeError, "malloc error");
      return NULL;
   }
 
   int ret=read(fd, data, l_data);
   if(ret<0)
   {
      PyErr_SetString(PyExc_RuntimeError, strerror(errno));
      return NULL;
   }
   else
   {
      PyObject *_ret;
       _ret=PyByteArray_FromStringAndSize(data, ret);
      return _ret;
   }

mea_write_arg_err:
   PyErr_BadArgument();
   return NULL;
}


static PyObject *mea_write(PyObject *self, PyObject *args)
{
   PyObject *arg;

   int fd=-1;
   char *data=NULL;
   int l_data=0;

   // récupération des paramètres et contrôle des types
   if(PyTuple_Size(args)!=2)
      goto mea_write_arg_err;

   arg=PyTuple_GetItem(args, 0);
   if(PyNumber_Check(arg))
      fd=(int)PyLong_AsLong(arg);
   else
      goto mea_write_arg_err;

   arg=PyTuple_GetItem(args, 1);
   if(PyByteArray_Check(arg))
   {
      data=PyByteArray_AsString(arg);
      l_data=PyByteArray_Size(arg);
   }
   else
      goto mea_write_arg_err;

   int ret=write(fd,data,l_data);
   if(ret<0)
   {
      PyErr_SetString(PyExc_RuntimeError, strerror(errno));
      return NULL; // False
   }
   else
      return PyLong_FromLong(1L); // True

mea_write_arg_err:
   PyErr_BadArgument();
   return NULL;
}

