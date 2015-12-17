# coding: utf8

import re
#import string
import sys
import json
from optparse import OptionParser

def isname(str):
   if not str[0].isalpha():
      return False
   if not str[1:].isalnum():
      return False
   return True


def rulesToJSON(file, verboseFlag, debugFlag, numFile):
   inputs_rules=[]
   outputs_rules=[]
   files=[]
   try: 
      f = open(file, 'r')
   except IOError as e:
      print "can't open file '"+file+"': ", e.strerror
      return False
   except:
      print "unexpected error: ", sys.exc_info()[0]
      return False

   numLine=0
   for line in f:
      rule={}
      conditions={}
      numLine=numLine+1

      # nettoyage de la ligne
      line=line.strip()        # suppression des blancs devant et derrière
      line=line.split("**")[0] # suppression des commentaires
      if len(line)==0:         # plus rien dans la ligne ?
         continue              # passage à la ligne suivante

      # découpage de la ligne et contrôle du nombre d'élément
      s=re.split('(is:|if:|onmatch:|do:|with:|when:)',line) # découpage suivant les 6 mots clés
      nb_elems=len(s)
      if not nb_elems in [3, 5, 7]:
         print "line "+str(numLine)+" : malformed rule, check syntax"
         return False
      # "nettoyage" (suppression des blancs en debut et fin de chaine)  des éléments résultats du découpage
      for i in range(0, len(s)):
         s[i]=s[i].strip()

      toInput=False
      toOutput=False
      # récupération nom et valeur
      try:
         if s[1]=='is:':
            toInput=True
         elif s[1]=='do:':
            toOutput=True
         else:
            print "line "+str(numLine)+": 'is:' or 'do:' is mandatory"
            return False

         if isname(s[0])<>True:
            print "line "+str(numLine)+": name format error"
            return False

         if toInput==True:
            rule={ "name" : s[0] , "value"  : s[2] }
         elif toOutput==True:
            rule={ "name" : s[0] , "action" : s[2] }

         if debugFlag==True:
            rule["file"]=numFile
            rule["line"]=numLine
      except:
         print "line "+str(numLine)+": unexpected error - ", sys.exc_info()[0]
         return False

      if toInput==True:
         # récupération des conditions si elle existe
         n=3
         if nb_elems>n:
            if s[n]=='if:':
               rule['conditions']=[]
               _conditions = s[n+1]
               if _conditions[:1]=='(' and _conditions[-1:]==')':
                  conditionsList=_conditions[1:-1].split(",")
                  for i in conditionsList:
                     i=i.strip()
                     c=re.split('(==|!=|<=|>=|<|>)',i)
                     rule['conditions'].append({ 'value1' : c[0].strip(), 'op' : c[1].strip(), 'value2' : c[2].strip() })
               else:
                  print "line "+str(numLine)+": conditions error"
                  return False
               n=n+2
         # onmatch
         if nb_elems>n:
            if s[n]=='onmatch:':
               rule['onmatch']=s[n+1]
               n=n+2

      elif toOutput==True:
         if nb_elems <> 7:
            print "line "+str(numLine)+" : malformed rule, check syntax"
            return False
         else:
            n=7
         if s[3]!='with:' or s[5]!='when:':
            print "line "+str(numLine)+" : malformed rule, check syntax"
            return False

         rule['parameters']={}
         _parameters=s[4]
         if _parameters[:1]=='(' and _parameters[-1:]==')':
            _parametersList=_parameters[1:-1].split(",")
            for i in _parametersList:
               i=i.strip()
               c=re.split('(=)',i)
               rule['parameters'][c[0].strip()]=c[2].strip()

         condition=re.split('(rise|fall|change|new)',s[6])
         if len(condition) != 3:
            print "line "+str(numLine)+" : malformed rule, check syntax"
            return False
         for i in range(0, len(condition)-1):
            condition[i]=condition[i].strip()
         rule['condition']={ condition[0] : condition[1] }

      if n==nb_elems:
         if toInput == True:
            inputs_rules.append(rule)
         if toOutput == True:
            outputs_rules.append(rule)
      else:
         print "line "+str(numLine)+": not a rule, check syntax (probably 'if:' or 'onmatch:' dupplication, or if:/onmatch: inversion)'"
         return False

   f.close()

   rules={}
   rules['inputs']=inputs_rules;
   rules['outputs']=outputs_rules;

   return rules;


def main():
   usage = "usage: %prog [options] rules_files"

   parser = OptionParser(usage)

   parser.add_option("-o", "--output", dest="outputfile", help="write result to FILE", metavar="FILE", default=False)
   parser.add_option("-i", "--indent", action="store_true", dest="indent", help="JSON indented output")
   parser.add_option("-v", "--verbose", action="store_true", dest="verbose", help="verbose")
   parser.add_option("-d", "--debug", action="store_true", dest="debug", help="debug")

   (options, args) = parser.parse_args()
   if len(args) < 1:
      parser.error("incorrect number of arguments")

   files=[]
   fileNum=0;
   rules={}
   rules["inputs"]=[]
   rules["outputs"]=[]
   _rules=False
 
   for i in args:
      if options.verbose == True:
         print "Processing file : "+i
      ret=rulesToJSON(i, options.verbose, options.debug, fileNum)
      if ret!=False:
         try:
            rules["inputs"]=rules["inputs"]+ret["inputs"]
            rules["outputs"]=rules["outputs"]+ret["outputs"]
         except:
            print "unexpected error: ", sys.exc_info()[0]
            exit(1)
      else:
         exit(1)
      files.append(i)
      fileNum=fileNum+1

   if options.debug == True:
      rules["files"]=files;

   if options.indent==True:
      _rules=json.dumps(rules, sort_keys=False, indent=4, separators=(',', ': '))
   else:
      _rules=json.dumps(rules)

   if options.outputfile!=False:
      try:
         f = open(options.outputfile, 'w')
         f.write(_rules)
         f.close()
      except IOError as e:
         print "can't open file '"+file+"': ", e.strerror
         exit(1)
      except:
         print "unexpected error: ", sys.exc_info()[0]
         exit(1)
   else:
      print _rules 


if __name__ == "__main__":
   main()
