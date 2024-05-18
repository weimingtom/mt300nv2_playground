//20180324
#pragma once

extern PyObject _Py_EllipsisObject; 
#define Py_Ellipsis (&_Py_EllipsisObject)

typedef struct {
    PyObject_HEAD
    PyObject *start, *stop, *step;
} PySliceObject;

extern PyTypeObject PySlice_Type;
#define PySlice_Check(op) ((op)->ob_type == &PySlice_Type)
PyObject * PySlice_New(PyObject* start, PyObject* stop, PyObject* step);
int PySlice_GetIndices(PySliceObject *r, int length, int *start, int *stop, int *step);

