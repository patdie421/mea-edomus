import re
import string

import mea_utils
try:
   import mea
except:
   import mea_simulation as mea

from sets import Set


def getInternalVarValue(data, var):
   if var=="$local_xbee":
      local_xbee="%0*x" % (8,data["ADDR_H"]) + "-" + "%0*x" % (8, data["ADDR_L"])
      return (-1,local_xbee)
   elif var=="$my_name":
      return (-1, data["interface_name"])
   else:
      return (-1, "")


def config_IO(data,pin,val,mem):
   if len(pin)!=3:
      print "WARNING - invalid pin :", pin[1:], "not not allowed, skipped"
      return -1
   try:
      pinNum=int(pin[2:3])
      if(pinNum>8):
         print "WARNING - invalid pin :", pin[1:3], "not not allowed, skipped"
         return -1
   except:
      print "WARNING - invalid pin :", pin[1:3], "not not allowed, skipped"
      return -1
   
   if val=="digital_in":
      atVal=3
   elif val=="digital_out_l":
      atVal=4
   elif val=="digital_out_h":
      atVal=5
   elif val=="default":
      atVal=1
   elif val=="analog_in":
      if pinNum==0 or pinNum==1 or pinNum==2 or pinNum==3:
         atVal=2
      else:
         print "WARNING - invalid pin :", pin[1:3], "not not allowed for analog input, skipped"
         return -1
   elif val=="none":
      atVal=0
   else:
      print "WARNING - syntaxe error :", i[0], "not an at cmnd, skipped"
      return -1
   
   ret=mea.atCmdSend(data["ID_XBEE"], data["ADDR_H"], data["ADDR_L"], pin[1:3].upper(), atVal);
   
   if ret == True:
      mem[pin[1:3]]=atVal
   
   return ret


def config_DHDL(data,alphaVal):
   vals=alphaVal.split("-")
   if len(vals) != 2:
      print "WARNING - syntaxe error :", alphaVal, "not an xbee address, skipped"
      return -1
   try:
      h=int(vals[0],16)
      l=int(vals[1],16)
   except:
      print "WARNING - syntaxe error :", alphaVal, "not an xbee address, skipped"
      return -1
   
   ret=mea.atCmdSend(data["ID_XBEE"], data["ADDR_H"], data["ADDR_L"], "DH", h)
   ret=mea.atCmdSend(data["ID_XBEE"], data["ADDR_H"], data["ADDR_L"], "DL", l)
   
   return ret


def config_sleep(data, val):
   if val>=0 and val <= 2800:
      # mettre aussi a jour les autres registres
      ret=mea.atCmdSend(data["ID_XBEE"], data["ADDR_H"], data["ADDR_L"], "ST", val)
      return ret
   else:
      return -1


def config_name(data, alphaVal):
   if len(alphaVal)>1:
      ret=mea.atCmdSend(data["ID_XBEE"], data["ADDR_H"], data["ADDR_L"], "NI", alphaVal)
      return ret
   else:
      return -1


def config_sample(data,val):
   if val>=0:
      ret=mea.atCmdSend(data["ID_XBEE"], data["ADDR_H"], data["ADDR_L"], "IR", val)
      return ret
   else:
      return -1


def enable_change_detection(data,mem):
   # a ecrire
   # Bit (IO pin):
   # 0 (DIO0) 4 (DIO4)  8  (DIO8)
   # 1 (DIO1) 5 (DIO5)  9  (DIO9)
   # 2 (DIO2) 6 (DIO6) 10 (DIO10)
   # 3 (DIO3) 7 (DIO7) 11 (DIO11)
   #
   # activer IO Digital Change Detection (ATIC)
   
   mask=0
   for i in mem:
      if i[0:1]=="d":
         if mem[i]==4 or mem[i]==5:
            v=int(i[1:2])
            if v<=8:
               bit=1<<v
               mask=mask+bit

   ret=0
   if mask != 0:
      ret=mea.atCmdSend(data["ID_XBEE"], data["ADDR_H"], data["ADDR_L"], "IC", mask)
   return ret


def enable_sample(data,mem):
   analog_in_enabled=False
   for i in mem:
      print "i[0:1]=",i[0:2]
      if i[0:1]=="d":
         if mem[i]==2:
            analog_in_enabled=True
            break
   if analog_in_enabled == True:
      ret=mea.atCmdSend(data["ID_XBEE"], data["ADDR_H"], data["ADDR_L"], "IR", 5*1000)
   else:
      return -1
   return ret


def mea_commissionningRequest(data):
   
   #   print data
   if "interface_parameters" in data:
      mem={}
      try:
         mem=mea.get_memory(data["interface_id"])
      except:
         print "ERROR - can't acces share memory"
         return False
      
      sample_set=False
      print data["interface_parameters"]
      paramsList=mea_utils.parseKeyValueDatasToList(data["interface_parameters"], ",", ":")
      print "Paramslist"
      print paramsList
      if paramsList != None:
         for i in paramsList:
            numVal=-1;
            alphaVal="";
            
            if i[0][0] != "#" and i[0][0] !="@":
               print "syntaxe error :", i[0], "not a variable, skipped"
               continue
            
            if i[1]!=None:
               if i[1][0:2]=="0x":
                  try:
                     numVal=int(i[1],16)
                  except:
                     alphaVal=i[1]
               elif i[1][0]=="$":
                  (numVal,alphaVal)=getInternalVarValue(data, i[1])
               else:
                  try:
                     numVal=int(i[1],10)
                  except:
                     alphaVal=i[1]
               # commande AT ?
               if i[0][0] == "@":
                  if len(i[0])==3:
                     if numVal>=0:
                        ret=mea.atCmdSend(data["ID_XBEE"], data["ADDR_H"], data["ADDR_L"], i[0][1:3].upper(), numVal);
                        if ret == 0:
                           print "WARNING - Transmission error for", i[0], "= ", numVal
                     else:
                        ret=mea.atCmdSend(data["ID_XBEE"], data["ADDR_H"], data["ADDR_L"], i[0][1:3].upper(), alphaVal);
                        if ret == 0:
                           print "WARNING - Transmission error for", i[0], "= ", alphaVal
                           continue
                  else:
                     print "WARNING - syntaxe error :", i[0], "not an at cmnd, skipped"
                     continue
               
               # commande special ?
               if i[0][0:2]=="#d":
                  ret=config_IO(data, i[0], alphaVal, mem)
                  continue
               
               if i[0] == "#set_dhdl":
                  ret=config_DHDL(data,alphaVal)
                  if ret==-1:
                     print "WARNING - value error :", i[1], "not an xbee adress, skipped"
                  continue
               
               if i[0] == "#set_sleep":
                  ret=config_sleep(data,numVal)
                  continue
               
               if i[0] == "#set_name":
                  ret=config_name(data, alphaVal)
                  if ret==-1:
                     print "WARNING - value error :", i[1], "not an xbee adress, skipped"
                  continue
               
               if i[0] == "#set_sample":
                  sample_set=True
                  config_sample(data,numVal)
                  continue
            
            else:
               if len(i[0])==3:
                  ret=mea.atCmdSend(data["ID_XBEE"], data["ADDR_H"], data["ADDR_L"], i[0][1:3].upper(), "");
                  if ret == 0:
                     print "Transmission error for", i[0], "= ", numVal
         
         enable_change_detection(data,mem)
         if sample_set==False:
            enable_sample(data,mem)
      
      else:
         return False
   else:
      return False