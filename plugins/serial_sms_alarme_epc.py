# coding: utf8

import re
import string
import sys
import unicodedata

try:
    import mea
except:
    import mea_simulation as mea

import mea_utils
from mea_utils import verbose

from sets import Set
import string

debug=0


def mea_xplCmndMsg(data):
   fn_name=sys._getframe().f_code.co_name

   # récupération des données xpl
   try:
      x=data["xplmsg"]
      body=x["body"]
      schema=x["schema"]
   except:
      return False

   try:
      id_sensor=data["device_id"]
   except:
      verbose(2, "ERROR - (mea_xplCmndMsg) : no device id")
      return False
   mem=mea.getMemory(id_sensor)

   if schema=="sensor.request":
      target="*"
      if "source" in x:
         target=x["source"]

      try:
         value=mem[body['request']]
      except:
         verbose(2, "ERROR - (mea_xplCmndMsg) : ", body['request'], " data not found")
         return False

#      xplMsg=mea_utils.xplMsgNew('me', target, "xpl-stat", "sensor", "basic")
#      mea_utils.xplMsgAddValue(xplMsg, "device", data["device_name"])
#      mea_utils.xplMsgAddValue(xplMsg, body['request'], value)
#      mea.xplSendMsg(xplMsg)
      return True
   return False


def _wordsafter(s, phrase):
   try:
      return s[s.index(phrase)+len(phrase):].split()
   except:
      return False


# +CMT: "+33661665082","","15/11/19,22:57:29+04",145,32,0,0,"+33660003151",145,140
def mea_getSMS(s):
   error=0
   try:
      start_cmt=s[s.index('+CMT: ')+6:]

      cmt_header=start_cmt[:start_cmt.index('\r\n')]
      cmt_header_values=cmt_header.split(",")

      numtel=cmt_header_values[0][1:-1]

      msg_len=int(cmt_header_values[10])
      i=start_cmt.index('\r\n')+2;
      if(len(start_cmt[i:]) < msg_len):
         return (False, 1) # données reçues incompletes
      cmt_data=start_cmt[i:i+msg_len]

      return(numtel, cmt_data, i+msg_len+1)

   except:
      return (False, 0) # probablement pas un sms


# format du SMS que nous devons traiter :
# Info télésurveillance 13/11/2015 08:40:27 : mise à l'arret via code NATHALIE du clavier ENTREE (zone 1) au 16 RUE JULES GUESDE - ROSNY SOUS
def _analyseData(s):
   alarm=-1
   if s.find(u"l'arret") <> -1:
      alarm=1
   elif s.find(u"en marche") <> -1:
      alarm=2
   else:
      return(False, 3)

   date_time = _wordsafter(s, u"surveillance ")
   if date_time == False:
      return(False, 4)

   if s.find(u"via code") <> -1:
      code = _wordsafter(s, u"via code ")[0]
      return (alarm, code, date_time[0], date_time[1])
   else:
      return (alarm, False, date_time[0], date_time[1])


def mea_serialData(data):
   fn_name=sys._getframe().f_code.co_name

   parameters=False
   try:
       id_sensor=data["device_id"]
       device=data["device_name"]
       serial_data=data["data"]
       l_serial_data=data["l_data"]
#       parameters=data["device_parameters"]
   except:
      verbose(2, "ERROR (", fn_name, ") - invalid data")
      return False

   mem=mea.getMemory(id_sensor)

   # récupération des paramétres
   if parameters <> False:
      params=mea_utils.parseKeyValueDatasToDictionary(parameters, ",", ":")

   # faire ce qu'il faut ici avec les parametres ici s'il y en a

   # conversion des données si nécessaire
   # récupération des données dans une chaine de caractères unicode
   s=serial_data.decode('latin-1')

   # traitement des données
   # récupère (ou pas) un SMS dans les données
   sms=mea_getSMS(s)
   if sms[0] == False:
      if sms[1] == 1:
         verbose(9, "ERROR (", fn_name, ") - incomplete SMS data")
      else:
         verbose(9, "ERROR (", fn_name, ") - not a SMS")
      return False

   # analyse des données
   alarm=_analyseData(sms[1])
   if alarm[0] == False:
      verbose(2, "ERROR (", fn_name, ") - not an EPG SMS : ", alarm[1])
      return False

   # stockage des données
   try:
      mem['last']=mem['current']
   except:
      mem['last']=-1
   mem['current']=alarm[0]
   mem['who']=str(alarm[1])
   mem['date']=str(alarm[2])
   mem['time']=str(alarm[3])

   # emission XPL
   xplMsg=mea_utils.xplMsgNew('me', "*", "xpl-trig", "sensor", "basic")
   mea_utils.xplMsgAddValue(xplMsg, "device", data["device_name"])
   mea_utils.xplMsgAddValue(xplMsg, 'current', mem['current'])
   mea.xplSendMsg(xplMsg)

   return True
