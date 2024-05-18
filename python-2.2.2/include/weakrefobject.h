//20180324
#pragma once

typedef struct _PyWeakReference PyWeakReference;

struct _PyWeakReference {
    PyObject_HEAD
    PyObject *wr_object;
    PyObject *wr_callback;
    long hash;
    PyWeakReference *wr_prev;
    PyWeakReference *wr_next;
};

extern PyTypeObject _PyWeakref_RefType;
extern PyTypeObject _PyWeakref_ProxyType;
extern PyTypeObject _PyWeakref_CallableProxyType;
#define PyWeakref_CheckRef(op) ((op)->ob_type == &_PyWeakref_RefType)
#define PyWeakref_CheckProxy(op) (((op)->ob_type == &_PyWeakref_ProxyType) || ((op)->ob_type == &_PyWeakref_CallableProxyType))
#define PyWeakref_Check(op) (PyWeakref_CheckRef(op) || PyWeakref_CheckProxy(op))
extern PyObject * PyWeakref_NewRef(PyObject *ob, PyObject *callback);
extern PyObject * PyWeakref_NewProxy(PyObject *ob, PyObject *callback);
extern PyObject * PyWeakref_GetObject(PyObject *ref);
extern long _PyWeakref_GetWeakrefCount(PyWeakReference *head);
#define PyWeakref_GET_OBJECT(ref) (((PyWeakReference *)(ref))->wr_object)
