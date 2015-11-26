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
      parameter=data["device_parameters"]
      parameter=parameter.strip().upper()
   except:
      verbose(2, "ERROR (", fn_name, ") - device_id not found")
      return 0

# validation du parametre
   if len(parameter) != 3:
      verbose(2, "ERROR (", fn_name, ") - parameter error") 
      return True
 
   if parameter[0]!='P':
      verbose(2, "ERROR (", fn_name, ") - parameter error") 
      return True
 
   pin=parameter[1]
   if pin<'0' or pin>'9':
      verbose(2, "ERROR (", fn_name, ") - pin id error") 
      return True
   else:
      pin=int(pin)

   action=parameter[2]
   if action!='L' and action!='A' and action!='P':
      verbose(2, "ERROR (", fn_name, ") - cmnd error") 
      return True

   mem=mea.getMemory(id_actuator)

   try: 
      x=data["xplmsg"]
      body=x["body"]
   except:
      return False

   target="*"
   if "source" in x:
      target=x["source"]

   if x["schema"]!="control.basic":
      return True

   value=body['current']
   if value == 'high':
      value='H'
   elif value == 'low':
      value='L'
   elif value.isdigit() == False:
      verbose(2, "ERROR (", fn_name, ") - xpl: current value error") 
      return True

   if value.isdigit()==False and (action=='P' or action=='A') :
      verbose(2, "ERROR (", fn_name, ") - xpl: current must be numeric") 
      return True
 
   cmnd="~~~~CMD:##"+str(pin)+","+action+","+str(value)+"##\r\nEND\r\n"

   mea.sendSerialData(data['fd'],bytearray(cmnd))

   return True;


def mea_pre(data):
   fn_name=sys._getframe().f_code.co_name

   verbose(9,"INFO  (",fn_name, ") - lancement mea_pre v2")

   verbose(9, data)
   try:
       serial_data=data["data"]
   except:
      verbose(2, "ERROR (", fn_name, ") - invalid data")
      return False

   try:
      s=serial_data.decode('latin-1')
      sigq=s[s.index('$$SIG=')+len('$$SIG='):]
      sigq=sigq[:sigq.index('$$')]
      verbose(9,"signal=",str(sigq))
      return False
   except:
      return True

def mea_init(data):
   fn_name=sys._getframe().f_code.co_name

   verbose(9,"INFO  (", fn_name, ") - lancement mea_init v2")

   return 1
