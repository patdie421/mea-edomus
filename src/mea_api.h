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

static PyObject *mea_getMemory(PyObject *self, PyObject *args);

static PyObject *mea_sendAtCmdAndWaitResp(PyObject *self, PyObject *args);
static PyObject *mea_sendAtCmd(PyObject *self, PyObject *args);

static PyObject *mea_xplSendMsg(PyObject *self, PyObject *args);
static PyObject *mea_xplGetVendorID();
static PyObject *mea_xplGetDeviceID();
static PyObject *mea_xplGetInstanceID();

static PyObject *mea_addDataToSensorsValuesTable(PyObject *self, PyObject *args);

PyObject *xplMsgToPyDict(xPL_MessagePtr xplMsg);

#endif
