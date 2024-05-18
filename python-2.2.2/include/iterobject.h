//20180318
#pragma once

extern PyTypeObject PySeqIter_Type;
#define PySeqIter_Check(op) ((op)->ob_type == &PySeqIter_Type)
extern PyObject * PySeqIter_New(PyObject *);

extern PyTypeObject PyCallIter_Type;
#define PyCallIter_Check(op) ((op)->ob_type == &PyCallIter_Type)
extern PyObject * PyCallIter_New(PyObject *, PyObject *);
