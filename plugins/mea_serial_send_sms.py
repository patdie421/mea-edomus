# coding: utf8

import re
import string
import sys

try:
    import mea
except:
    import mea_simulation as mea

import mea_utils
from mea_utils import verbose
from time import sleep
import string

debug=0


def mea_xplCmndMsg(data):
   fn_name=str(sys._getframe().f_code.co_name) + "/" + __name__

   try:
      id_actuator=data["device_id"]
   except:
      verbose(2, "ERROR (", fn_name, ") - device_id not found")
      return False

   mem_actuator=mea.getMemory(id_actuator)

   # récupération des données xpl
   try:
      x=data["xplmsg"]
      body=x["body"]
      schema=x["schema"]
   except:
      return False

   if schema=="sendmsg.basic":
      try:
         msg=str(body["body"])
         to=str(body["to"])
      except:
         verbose(2, "ERROR - (", fn_name, ") : bad data")
         return False

      mea.sendSerialData(data['fd'], bytearray("AT+CMGS=\""+to+"\"\r\n"))
      mem_actuator['msg']= msg
      return True
   else:
      return False
   return False


def mea_serialData(data):
   fn_name= __name__ + "/" + str(sys._getframe().f_code.co_name)

   try:
      id_actuator=data["device_id"]
   except:
      verbose(2, "ERROR (", fn_name, ") - device_id not found")
      return False

   try:
      serial_data=data["data"]
   except:
      return False
   try:
      s=serial_data.decode('latin-1')
   except:
      return False

   mem_actuator=mea.getMemory(id_actuator)
   if mem_actuator['msg']!=False:
      if s.find(u"\r\n> ") >=0:
         mea.sendSerialData(data['fd'], bytearray(mem_actuator['msg']+"\r\n"+chr(26)))
      mem_actuator['msg']==False
