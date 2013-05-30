from sets import Set
import string

allowed_chars  = Set(string.ascii_lowercase + string.digits)
allowed_chars_other = Set(string.ascii_lowercase + string.digits + "-")

# retourne True si tous les caracteres sont autorises en tenant compte de
# dash_allowe et si la longueur est inferieure a max_str_len
def isXplValidCharacters(s, max_str_len, dash_allowed=True):
   if len(s) < 1 or len(s) > max_str_len:
      return False
   
   if dash_allowed == False:
      return Set(s).issubset(allowed_chars)
   else:
      return Set(s).issubset(allowed_chars) or Set(s).issubset(allowed_chars_other)


def isXplMessageType(s):
   if s=="xpl-cmnd" or s=="xpl-stat" or s=="xpl-trig":
      return True
   else:
      return False


def xplMsgNew(id_vendor, id_device, id_instance, message_xpl_type, xpl_class, xpl_type, device):
   # car "xpl" autorises : a-z, 0-9 and "-"
   # controle des parameters :
   # id_vendor 8 car "xpl" sans "-"
   # id_device 8 car "xpl" sans "-"
   # id_instance 16 car "xpl"
   # device 16 car "xpl"
   # message_xpl_type = [ "xpl-cmnd" | "xpl-stat" | "xpl-trig" ]
   # xpl_class 8 car "xpl"
   # xpl_type 8 car "xpl"
   
   if not isXplValidCharacters(id_vendor.lower(),8,False):
      return None
   elif not isXplValidCharacters(id_device.lower(),8, False):
      return None
   elif not isXplValidCharacters(id_instance.lower(),16, True):
      return None
   elif not isXplValidCharacters(device.lower(),16, True):
      return None
   elif not isXplMessageType(message_xpl_type.lower()):
      return None
   elif not isXplValidCharacters(xpl_class.lower(),8, True):
      return None
   elif not isXplValidCharacters(xpl_type.lower(),8, True):
      return None

# le format des donnees est valide, on peut creer le message
   xplMsg={} # creation du dictionnaire
   xplMsg["xplmsg"]=True # pour des besoins internes (controle)
   xplMsg["message_xpl_type"]=message_xpl_type.lower()
   xplMsg["hop"]="1"
   xplMsg["source"]=id_vendor.lower()+ "-" + id_device.lower() + "." + id_instance.lower()
   xplMsg["target"]= "*"
   xplMsg["schema"]=xpl_class.lower() + "." + xpl_type.lower()
   body={}
   body["device"]=device
   xplMsg["xpl-body"]=body

   return xplMsg


def xplMsgAddValue(xplMsg, key, value):
   if not isXplValidCharacters(key.lower(),16,True):
      return False
   
   if "xplmsg" in xplMsg:
      if xplMsg["xplmsg"] != True:
         return False
   
   xpl_body = xplMsg.get("xpl-body")
   if xpl_body == None:
      xpl_body={}
   
   xpl_body[key.lower()]=str(value)
   
   xplMsg["xpl-body"]=xpl_body
   
   return True


def xplMsgAddValues(xplMsg, aDict):
   if "xplmsg" in xplMsg:
      if xplMsg["xplmsg"] != True:
         return False
   
   for i in aDict:
      if not isXplValidCharacters(i.lower(),16,True):
         return False
   
   xpl_body= xplMsg.get("xpl-body")
   if xpl_body == None:
      xpl_body={}
   
   for i in aDict:
      xpl_body[i.lower()]=str(aDict[i])
   
   xplMsg["xpl-body"]=xpl_body
   
   return True


def xplMsgSetValues(xplMsg, aDict):
   for i in aDict:
      if not isXplValidCharacters(i,16,True):
         return False
   
   xplMsg["xpl-body"]=aDict.copy();
   
   return True


def xplMsgPrint(xplMsg):
   try:
      print xplMsg["message_xpl_type"]
      print "{"
      print "hop=" + xplMsg["hop"]
      print "source=" + xplMsg["source"]
      print "target=" + xplMsg["target"]
      print "}"
      print xplMsg["schema"]
      print "{"
      try:
         body=xplMsg["xpl-body"]
         for i in body:
            print i+"="+body[i]
      except:
         print "<probably no body now !!!>"
      print "}"
   except:
      print "not a xpl message"


def xplMsgToString(xplMsg):
   try:
      s=xplMsg["message_xpl_type"] + "\n{\n" + "hop=" + xplMsg["hop"] + "\nsource=" + xplMsg["source"] + "\ntarget=" + xplMsg["target"] + "\n}\n" + xplMsg["schema"] + "\n{\n"
      body=xplMsg["xpl-body"]
      for i in body:
         s = s + i + "=" + body[i] + "\n"
      s = s + "}\n"
      return s
   except:
      return None
 
