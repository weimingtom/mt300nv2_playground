//20180318
#pragma once

extern PyTypeObject PyFile_Type;
#define PyFile_Check(op) PyObject_TypeCheck(op, &PyFile_Type)
#define PyFile_CheckExact(op) ((op)->ob_type == &PyFile_Type)
extern PyObject * PyFile_FromString(char *, char *);
extern void PyFile_SetBufSize(PyObject *, int);
extern PyObject * PyFile_FromFile(FILE *, char *, char *, int (*)(FILE *));
extern FILE * PyFile_AsFile(PyObject *);
extern PyObject * PyFile_Name(PyObject *);
extern PyObject * PyFile_GetLine(PyObject *, int);
extern int PyFile_WriteObject(PyObject *, PyObject *, int);
extern int PyFile_SoftSpace(PyObject *, int);
extern int PyFile_WriteString(const char *, PyObject *);
extern int PyObject_AsFileDescriptor(PyObject *);
extern const char * Py_FileSystemDefaultEncoding;
