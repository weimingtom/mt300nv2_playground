//20180324
#pragma once

PyObject * PySys_GetObject(char *);
int PySys_SetObject(char *, PyObject *);
FILE * PySys_GetFile(char *, FILE *);
void PySys_SetArgv(int, char **);
void PySys_SetPath(char *);
void PySys_WriteStdout(const char *format, ...);
void PySys_WriteStderr(const char *format, ...);
extern PyObject * _PySys_TraceFunc, *_PySys_ProfileFunc;
extern int _PySys_CheckInterval;
void PySys_ResetWarnOptions();
void PySys_AddWarnOption(char *);
