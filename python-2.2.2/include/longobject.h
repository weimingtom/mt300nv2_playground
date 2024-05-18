//20180318
#pragma once

typedef struct _longobject PyLongObject;
extern PyTypeObject PyLong_Type;
#define PyLong_Check(op) PyObject_TypeCheck(op, &PyLong_Type)
#define PyLong_CheckExact(op) ((op)->ob_type == &PyLong_Type)
extern PyObject * PyLong_FromLong(long);
extern PyObject * PyLong_FromUnsignedLong(unsigned long);
extern PyObject * PyLong_FromDouble(double);
extern long PyLong_AsLong(PyObject *);
extern unsigned long PyLong_AsUnsignedLong(PyObject *);
extern double _PyLong_AsScaledDouble(PyObject *vv, int *e);
extern double PyLong_AsDouble(PyObject *);
extern PyObject * PyLong_FromVoidPtr(void *);
extern void * PyLong_AsVoidPtr(PyObject *);
#define LONGLONG_MAX 9223372036854775807LL
#define ULONGLONG_MAX 0xffffffffffffffffULL
extern PyObject * PyLong_FromLongLong(LONG_LONG);
extern PyObject * PyLong_FromUnsignedLongLong(unsigned LONG_LONG);
extern LONG_LONG PyLong_AsLongLong(PyObject *);
extern unsigned LONG_LONG PyLong_AsUnsignedLongLong(PyObject *);
PyObject * PyLong_FromString(char *, char **, int);
PyObject * PyLong_FromUnicode(Py_UNICODE*, int, int);
extern PyObject * _PyLong_FromByteArray(const unsigned char* bytes, size_t n, int little_endian, int is_signed);
extern int _PyLong_AsByteArray(PyLongObject* v, unsigned char* bytes, size_t n, int little_endian, int is_signed);

