//
//  mea_api.h
//
//  Created by Patrice Dietsch on 09/05/13.
//
//

#ifndef mea_api_h
#define mea_api_h

#include <Python.h>
#include "xPL.h"

void mea_api_init();

static PyObject *mea_get_memory(PyObject *self, PyObject *args);
static PyObject *mea_xplMsgSend(PyObject *self, PyObject *args);
static PyObject *mea_atCmdToXbee(PyObject *self, PyObject *args);

PyObject *xplMsgToPyDict(xPL_MessagePtr xplMsg);

#endif
