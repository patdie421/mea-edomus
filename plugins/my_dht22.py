import re
import mea_utils
try:
    import mea
except:
    import mea_simulation as mea

from sets import Set
import string

debug=0

def mea_xplCmndMsg(data):
    try:
        id_sensor=data["device_id"]
    except:
        print "device_id not found"
        return 0

    mem=mea.getMemory(id_sensor)
    xplMsg=mea_utils.xplMsgNew("mea", "edomus", "cheznousdev", "xpl-stat", "sensor", "basic", data["device_name"])
    mea_utils.xplMsgAddValue(xplMsg,"current",mem["current_h"])
    mea_utils.xplMsgAddValue(xplMsg,"type","humidity")
    mea_utils.xplMsgAddValue(xplMsg,"last",mem["last_h"])
    mea.xplSendMsg(xplMsg)

    return True


def mea_dataFromSensor(data):
    xplMsg = {}
    try:
        id_type=data["device_type_id"]
        cmd=data["cmd"]
        id_sensor=data["device_id"]
    except:
        print "CMD not found"
        return 0

    mem=mea.getMemory(id_sensor)

    if debug == 1:
        print mem

    x=re.match("<(\S+)>",cmd[12:-2])
    t=x.group()[1:-1]
    list=t.split(';') 

    for i in list:
        l2=i.split('=')

        if id_type == 2000 and l2[0] == 'H':
            humidite = float(l2[1])
            last_h=0
            try:
                last_h=mem["current_h"]
                mem["last_h"]=mem["current_h"]
            except:
                pass
            mem["current_h"]=humidite
            print "INFO  (mea_dataFromSensor) : humidite =", humidite, "%"
            if humidite < 45:
                mem["speed"]="LOW"
            if humidite > 60:
                mem["speed"]="HIGH"
            if last_h != humidite:
                xplMsg=mea_utils.xplMsgNew("mea", "edomus", "cheznousdev", "xpl-trig", "sensor", "basic", data["device_name"])
                mea_utils.xplMsgAddValue(xplMsg,"current",humidite)
                mea_utils.xplMsgAddValue(xplMsg,"type","humidity")
                mea_utils.xplMsgAddValue(xplMsg,"last",last_h)
                # print mea_utils.xplMsgToString(xplMsg)
                mea.xplSendMsg(xplMsg)
            return True
                
        if id_type == 2001 and l2[0] == 'T':
            temperature = float(l2[1])
            print "INFO  (mea_dataFromSensor) : Temperature =", temperature, "C"
            mem["last_t"]=temperature
            return True

        if id_type == 2002 and l2[0] == 'P':
            pile =float(l2[1])
            print "INFO  (mea_dataFromSensor) : Pile =", pile, "V"
            mem["last_p"]=pile
            return True
    return False
