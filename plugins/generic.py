import re
import string

import mea_utils
try:
    import mea
except:
    import mea_simulation as mea

from sets import Set


def mea_commissionningRequest(data):
   print data
   if "interface_parameters" in data:
      paramsList=mea_utils.parseKeyValueDatasToList(data["interface_parameters"], ",", ":")
      if paramsList != None:
         print paramsList
         for i in paramsList:
            val=-1;
            if i[1]!=None:
               if i[1][0:2]=="0x":
	          try:
                     val=int(i[1],16)
                  except:
                     pass
               else:
                  try:
                     val=int(i[1],10)
                  except:
                     pass
               if val>=0:
                  print "Envoyer AT cmnd =", i[0], ", val(n) =", val
               else:
                  print "Envoyer AT cmnd =", i[0], ", val(s) =", i[0]

            else:
               if i[0][0]=="#":
                  print "special cmnd = ", i[0]
               else:
                  print "Envoyer AT cmnd =", i[0]
         return True
      else:
         print "No parameters !"
   return False