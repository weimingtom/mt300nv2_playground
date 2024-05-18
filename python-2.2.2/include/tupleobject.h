//20180324
#pragma once

typedef struct {
    PyObject_VAR_HEAD
    PyObject *ob_item[1];
} PyTupleObject;

extern PyTypeObject PyTuple_Type;
#define PyTuple_Check(op) PyObject_TypeCheck(op, &PyTuple_Type)
#define PyTuple_CheckExact(op) ((op)->ob_type == &PyTuple_Type)
extern PyObject * PyTuple_New(int size);
extern int PyTuple_Size(PyObject *);
extern PyObject * PyTuple_GetItem(PyObject *, int);
extern int PyTuple_SetItem(PyObject *, int, PyObject *);
extern PyObject * PyTuple_GetSlice(PyObject *, int, int);
extern int _PyTuple_Resize(PyObject **, int);
#define PyTuple_GET_ITEM(op, i) (((PyTupleObject *)(op))->ob_item[i])
#define PyTuple_GET_SIZE(op) (((PyTupleObject *)(op))->ob_size)
#define PyTuple_SET_ITEM(op, i, v) (((PyTupleObject *)(op))->ob_item[i] = v)
