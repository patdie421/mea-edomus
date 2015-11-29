# coding: utf8

import re
import string
import sys

try:
   import mea
except:
   import mea_simulation as mea

from sets import Set

import mea_utils
from mea_utils import verbose


def mea_xplCmndMsg(data):

   fn_name=str(sys._getframe().f_code.co_name) + "/" + __name__

   try:
      id_actuator=data["device_id"]
   except:
      verbose(2, "ERROR (", fn_name, ") - device_id not found")
      return 0

   try:
      interface_id=data["interface_id"]
      parameters=data["device_parameters"]
      parameters=parameters.strip().lower()
      params=mea_utils.parseKeyValueDatasToDictionary(parameters, ",", ":")
      typeoftype=data["typeoftype"]

   except:
      verbose(2, "ERROR (", fn_name, ") - parameters error")
      return False

   mem_actuator=mea.getMemory(id_actuator)
   mem_interface=mea.getMemory("interface"+str(interface_id))
   verbose(9, "DEBUG (", fn_name, ") mem_interface=", mem_interface)

   # validation du parametre
   pin=0
   try:
      pin=params["pin"]
   except:
      verbose(2, "ERROR (", fn_name, ") - parameters error : no pin")
      
   if len(pin) != 2:
      verbose(2, "ERROR (", fn_name, ") - PIN parameter error") 
      return True

   if pin[0]!='l' and pin[0]!='a':
      verbose(2, "ERROR (", fn_name, ") - PIN type error") 
      return True
   else:
      pinType=pin[0]

   if pin[1]<'0' or pin[1]>'9':
      verbose(2, "ERROR (", fn_name, ") - PIN id error") 
      return True
   else:
      pinNum=int(pin[1])

   if pinNum==0 or pinNum==3:
      verbose(2, "ERROR (", fn_name, ") - PIN 0 and 3 are input only") 
      return True
      
   # validation du message xpl
   try: 
      x=data["xplmsg"]

      schema=x["schema"]
      body=x["body"]

      current=body["current"]
      device=body["device"]
   except:
      verbose(2, "ERROR (", fn_name, ") - no or incomplet xpl message error") 
      return True

   # préparation de la commande
   _cmnd=str(pinNum)+","

   if schema=="control.basic" and typeoftype==1:
      type=body["type"]
      if type=="output" and pinType=='l':
         if current=="pulse":
            if "data1" in body:
               data1=body["data1"]
               if not data1.isdigit():
                  verbose(2, "ERROR (", fn_name, ") - data1 not numeric value")
                  return False
               data1=int(data1)
            else:
               data1=200

            _cmnd=_cmnd+"P,"+str(data1)
         elif current=="high" or (current.isdigit() and int(current)==1):
            _cmnd=_cmnd+"L,H"
         elif current=="low" or (current.isdigit() and int(current)==0):
            _cmnd=_cmnd+"L,L"

      elif type=="variable" and pinType=='a':
         if pinNum != 1 and pinNum != 2:
            verbose(2, "ERROR (", fn_name, ") - Analog output(PWM) only available on pin 1 and 2")
            return False
         else:
            value=0
            if 'current' in mem_interface[pinNum]:
               value=mem_interface[pinNum]['current']

            if current=="dec":
               value=value-1
            elif current=="inc":
               value=value+1
            elif current[0]=='-' or current[0]=='+':
               if current[1:].isdigit():
                  value=int(current)
               else:
                  verbose(2, "ERROR (", fn_name, ") - analog value error (",current,")")
                  return False
            elif current.isdigit():
               value=int(current)
            else:
               verbose(2, "ERROR (", fn_name, ") - analog value error (",current,")")
               return False

            if value<0:
               value=0;
            if value>255:
               value=255;
            _cmnd=_cmnd+"A,"+str(value)
      else:
         verbose(2, "ERROR (", fn_name, ") - xpl command error")
         return False

   elif schema!="sensor.basic":
      if current!="request":
         verbose(2, "ERROR (", fn_name, ") - xpl error current!=request")
         return False
      if typeoftype==1:
         _cmnd=_cmnd+pinType.upper()+",G"
      else:
         _cmnd=False
         # récupérer les info aux niveaux de l'interface et les transmettre

   else:
      verbose(2, "ERROR (", fn_name, ") - xpl schema incompatible with sensor/actuator (", schema, ")")
      return False

   if _cmnd != False:
      cmnd="~~~~CMD:##"+_cmnd+"##\r\nEND\r\n"
      verbose(9, cmnd)
      mea.sendSerialData(data['fd'],bytearray(cmnd))

   return True



def mea_serialData(data):
   fn_name=str(sys._getframe().f_code.co_name) + "/" + __name__

   verbose(9, "DEBUG (", fn_name, ") data = ", data)

   try:
      device_id=data["device_id"]
   except:
      verbose(2, "ERROR (", fn_name, ") - device_id not found")
      return False

   try:
      interface_id=data["interface_id"]
      parameters=data["device_parameters"]
      parameters=parameters.strip().lower()
      params=mea_utils.parseKeyValueDatasToDictionary(parameters, ",", ":")
      typeoftype=data["typeoftype"]
   except:
      verbose(2, "ERROR (", fn_name, ") - invalid data")
      return False

   mem_device=mea.getMemory(device_id)
   mem_interface=mea.getMemory("interface"+str(interface_id))
   verbose(9, "DEBUG (", fn_name, ") mem_interface = ", mem_interface)

   # validation du parametre
   pin=0
   try:
      pin=params["pin"]
   except:
      verbose(2, "ERROR (", fn_name, ") - parameters error : no PIN defined")
      return False
      
   if len(pin) != 2:
      verbose(2, "ERROR (", fn_name, ") - PIN format error") 
      return False

   if pin[0]!='l' and pin[0]!='a':
      verbose(2, "ERROR (", fn_name, ") - PIN type error") 
      return Flase
   else:
      pinType=pin[0]

   if pin[1]<'0' or pin[1]>'9':
      verbose(2, "ERROR (", fn_name, ") - PIN number error") 
      return False
   else:
      pinNum=int(pin[1])

   verbose(9, "pin=",pinNum," pinType=", pinType, " sendFlag=", mem_interface[pinNum]["sendFlag"], " current=", mem_interface[pinNum]["current"])

   if mem_interface[pinNum]["sendFlag"] == True and typeoftype == 0:

      mea.addDataToSensorsValuesTable(device_id, mem_interface[pinNum]["current"],0,0,"")

      xplMsg=mea_utils.xplMsgNew("me", "*", "xpl-trig", "sensor", "basic")
      mea_utils.xplMsgAddValue(xplMsg, "device", data["device_name"])
      mea_utils.xplMsgAddValue(xplMsg, "current", mem_interface[pinNum]["current"])
      mea.xplSendMsg(xplMsg)

   return True



def mea_pre(data):
   fn_name=str(sys._getframe().f_code.co_name) + "/" + __name__
#   verbose(9, "DEBUG (", fn_name, ") data = ", data)
   try:
       serial_data=data["data"]
       interface_id=data["interface_id"]
   except:
      verbose(2, "ERROR (", fn_name, ") - invalid data")
      return False

   mem_interface=mea.getMemory("interface"+str(interface_id))
#   verbose(9, "DEBUG (", fn_name, ") mem_interface = ", mem_interface)

   try:
      s=serial_data.decode('latin-1')
   except:
      return False

   for i in range(0, 9):
      mem_interface[i]["sendFlag"]=False
   verbose(9, "DEBUG (", fn_name, ") mem_interface = ", mem_interface)

   retour=False
   if s.find(u"CMT: ") > 0: # un sms dans le buffer ?
      retour=True # oui, on passera le buffer

   while len(s)>0:
      ptrStart=s.find(u"$$")
      if ptrStart<0:
         break
      ptrStart=ptrStart+2
      ptrEnd=s[ptrStart:].find(u"$$")
      if ptrEnd<0:
         break
      info=s[ptrStart:ptrStart+ptrEnd]
     
      # niveau du signal radio
      if info.find(u"SIG=") == 0:
         sig = info[4:]
         try:
            sig=float(sig)
            verbose(9, "INFO  SIM900 SIGNAL LEVEL=", sig)
            mem_interface['siglvl']=sig
         except:
            pass
      else:
         # retour d'I/O ?
         a=info.split(";")
         for e in a:
            e=e.lower()
            if e[0]!='l' and e[0]!='p' and e[0] != 'a':
               continue 
            type=e[0]
            if e[1]!="/":
               continue 
            if e[2]<'0' or e[2]>'9':
               continue
            pinNum=int(e[2])
            if e[3]!='=':
               continue
            value=e[4:]

            if value=='h' or value=='l' or value.isdigit():
               if value=='h':
                  value=1
                  _value='high'
               elif value=='l':
                  value=0
                  _value='low'
               else:
                  _value=value
                  value=int(value)

               mem_interface[pinNum]["current"]=int(value)
               mem_interface[pinNum]["sendFlag"]=True
               retour=True;

            else:
               continue

      # on recommence avec la suite de la chaine
      s=s[ptrStart+ptrEnd+2:]  

   return retour



def mea_init(data):
   fn_name=sys._getframe().f_code.co_name

   verbose(9,"INFO  (", fn_name, ") - lancement mea_init v2")

   try:
      interface_id=data["interface_id"]
   except:
      verbose(2, "ERROR (", fn_name, ") - invalid data")
      return False

   mem_interface=mea.getMemory("interface"+str(interface_id))
   for i in range(0, 9):
      mem_interface[i]={}

   return True
