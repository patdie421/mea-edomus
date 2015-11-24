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


def mea_pre(data):
   fn_name=sys._getframe().f_code.co_name

   verbose(9,"INFO  (",fn_name,") - lancement mea_pre")

   return True

def mea_init(data):
   fn_name=sys._getframe().f_code.co_name

   verbose(9,"INFO  (",fn_name,") - lancement mea_init")

   return 1
