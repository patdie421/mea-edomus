import re
import string
import sys

import mea_automator
import mea_utils
from mea_utils import verbose

# les règles sont évaluées dans l'ordre
# par defaut toutes les règles sont évaluées
# le comportement par défaut est modifiée par "onmatch" : break : l'évaluation s'arrête, continue : l'évaluation se poursuit
# mon langage de regles
# V1 is: #1
# V2 is: #2.1     if: (source == mea-edomus.home, schema == sensor.basic, device == "BUTTON3", current == #2) onmatch: break
# V2 is: #10      if: (source == mea-edomus.home, schema == sensor.basic, device == "BUTTON3", current > #3, current < #5) onmatch: continue
# E1 is: current  if: (source == mea-edomus.home, schema == sensor.basic, device == "BUTTON1", type == "input") onmatch: break
# E2 is: last     if: (source == mea-edomus.home, schema == sensor.basic, device == "BUTTON2") onmatch: continue
# T2 is: $now()   if: (source == mea-edomus.home, schema == sensor.basic, device == "BUTTON2") onmatch: continue
# P1 is: current  if: (source == mea-edomus.home, schema == sensor.basic, device == "CONSO", type == "power") onmatch: continue
# P2 is: current  if: (source == mea-edomus.home, schema == sensor.basic, device == "CONSO", type == "power", current != #0) onmatch: continue
# P3 is: current  if: (source == mea-edomus.home, schema == sensor.basic, device == "PROD", type == "power", current != {V1}) onmatch: continue
# C1 is: current  if: (source == mea-edomus.home, schema == sensor.basic, device == "PROD", type == "power", {T2}>0 ) onmatch: continue
#
# if faut ecrire une fonction "loadrules" qui traduit ce langage en dictionaire

# un exemple de règle "compliqué"
# T1_last is: {T1} if: (source == mea-edomus.home, schema == sensor.basic, device == "BUTTON2", current == "high")
# T1 is: $now() if: (source == mea-edomus.home, schema == sensor.basic, device == "BUTTON2", current == "high")
# DIFF is: $eval({T2} - {T2_last}) if: (source == mea-edomus.home, schema == sensor.basic, device == "BUTTON2", current == "high")
# P1 is: "high" if: (source == mea-edomus.home, schema == sensor.basic, device == "BUTTON2", current == "high", DIFF > #1000)
# P1 is: "low"  if: (source == mea-edomus.home, schema == sensor.basic, device == "BUTTON2", current == "high", DIFF <= #1000)
#
acquiringRules=[
                   {
                      "name" : "V1",
                      "value" : 1.0,
                   },
                   {
                      "name" : "E1",
                      "value" : "current",
                      "conditions" : {
                                        "source" : {"op" : "==", "value" : "mea-edomus.home" },
                                        "schema" : {"op" : "==", "value" : "sensor.basic"},
                                        "device" : {"op" : "==", "value" : "BUTTON1"},
                                        "type"   : {"op" : "==", "value" : "input"}
                                     },
                      "break" : "false"
                   },
                   {
                      "name" : "E2",
                      "value" : "current",
                      "conditions" : {
                                        "source" : {"op" : "==", "value" : "mea-edomus.home" },
                                        "schema" : {"op" : "==", "value" : "sensor.basic"},
                                        "device" : {"op" : "==", "value" : "BUTTON2"}
                                     }
                      "break" : "true"
                   },
                   {
                      "name" : "T1",
                      "value" : "$now()",
                      "conditions" : {
                                        "source" : {"op" : "==", "value" : "mea-edomus.home" },
                                        "schema" : {"op" : "==", "value" : "sensor.basic"},
                                        "device" : {"op" : "==", "value" : "BUTTON2"}
                                     }
                      "break" : "true"
                   }
                   ]
]

inputs={}

def acquire(data):
   fn_name=sys._getframe().f_code.co_name

   if data["source"]== "mea-edomus.home":
      if data["schema"] == "sensor.basic":
         if data["body"]["device"] == "BUTTON1":
            inputs["B1"] = data["body"]["current"]
            verbose(5, "B1 = ", inputs["B1"])
            return 0
         if data["body"]["device"] == "BUTTON2":
            inputs["B2"] = data["body"]["current"]
            verbose(5, "B2 = ", inputs["B2"])
            return 0
         if data["body"]["device"] == "PROD" and data["body"]["type"]== "power":
            inputs["C1"] == data["body"]["current"]
            verbose(5, "C1 = ", inputs["C1"])
            return 0
         return -1
      return -1


def automator(data):
   fn_name=sys._getframe().f_code.co_name
   print inputs
   return 1
