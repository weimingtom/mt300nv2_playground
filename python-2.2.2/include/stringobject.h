//20180324
#pragma once

#include <stdarg.h>

#define CACHE_HASH
#define INTERN_STRINGS

typedef struct {
    PyObject_VAR_HEAD
    long ob_shash;
    PyObject *ob_sinterned;
    char ob_sval[1];
} PyStringObject;

extern PyTypeObject PyString_Type;
#define PyString_Check(op) PyObject_TypeCheck(op, &PyString_Type)
#define PyString_CheckExact(op) ((op)->ob_type == &PyString_Type)
extern PyObject * PyString_FromStringAndSize(const char *, int);
extern PyObject * PyString_FromString(const char *);
extern PyObject * PyString_FromFormatV(const char*, va_list);
extern PyObject * PyString_FromFormat(const char*, ...);
extern int PyString_Size(PyObject *);
extern char * PyString_AsString(PyObject *);
extern void PyString_Concat(PyObject **, PyObject *);
extern void PyString_ConcatAndDel(PyObject **, PyObject *);
extern int _PyString_Resize(PyObject **, int);
extern int _PyString_Eq(PyObject *, PyObject*);
extern PyObject * PyString_Format(PyObject *, PyObject *);
extern PyObject * _PyString_FormatLong(PyObject*, int, int, int, char**, int*);
extern void PyString_InternInPlace(PyObject **);
extern PyObject * PyString_InternFromString(const char *);
extern void _Py_ReleaseInternedStrings();
#define PyString_AS_STRING(op) (((PyStringObject *)(op))->ob_sval)
#define PyString_GET_SIZE(op)  (((PyStringObject *)(op))->ob_size)
extern PyObject * _PyString_Join(PyObject *sep, PyObject *x);
extern PyObject* PyString_Decode(const char *s, int size, const char *encoding, const char *errors);
extern PyObject* PyString_Encode(const char *s, int size, const char *encoding, const char *errors);
extern PyObject* PyString_AsEncodedObject(PyObject *str, const char *encoding, const char *errors);
extern PyObject* PyString_AsEncodedString(PyObject *str, const char *encoding, const char *errors);
extern PyObject* PyString_AsDecodedObject(PyObject *str, const char *encoding, const char *errors);
extern PyObject* PyString_AsDecodedString(PyObject *str, const char *encoding, const char *errors);
extern int PyString_AsStringAndSize(PyObject *obj, char **s, int *len);
    
