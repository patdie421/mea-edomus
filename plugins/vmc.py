import re
from sets import Set
import string

import mea_utils
try:
    import mea
except:
    import mea_simulation as mea


def mea_dataFromSensor(data):
   return False


def mea_commissionningRequest(data):
   return True


def mea_xplCmndMsg(data):
   try:
      id_sensor=data["device_id"]
   except:
      print "device_id not found"
      return 0

   mem=mea.getMemory(id_sensor)
   try:
      xplmsg=data["xplmsg"]
      body=xplmsg["body"]
      vs=body["data1"]
      v = long(vs)
   except:
	   return 0

   if v==4 or v==5:
      mea.sendAtCmd(data["ID_XBEE"], data["ADDR_H"], data["ADDR_L"], "D1", v);
    
   return True
