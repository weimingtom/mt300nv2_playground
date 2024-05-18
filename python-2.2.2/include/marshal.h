//20180318
#pragma once

void PyMarshal_WriteLongToFile(long, FILE *);
void PyMarshal_WriteShortToFile(int, FILE *);
void PyMarshal_WriteObjectToFile(PyObject *, FILE *);
PyObject * PyMarshal_WriteObjectToString(PyObject *);
long PyMarshal_ReadLongFromFile(FILE *);
int PyMarshal_ReadShortFromFile(FILE *);
PyObject * PyMarshal_ReadObjectFromFile(FILE *);
PyObject * PyMarshal_ReadLastObjectFromFile(FILE *);
PyObject * PyMarshal_ReadObjectFromString(char *, int);
