//20180324
#pragma once

extern int Py_DebugFlag;
extern int Py_VerboseFlag;
extern int Py_InteractiveFlag;
extern int Py_OptimizeFlag;
extern int Py_NoSiteFlag;
extern int Py_UseClassExceptionsFlag;
extern int Py_FrozenFlag;
extern int Py_TabcheckFlag;
extern int Py_UnicodeFlag;
extern int Py_IgnoreEnvironmentFlag;
extern int Py_DivisionWarningFlag;
extern int _Py_QnewFlag;

#define Py_GETENV(s) (Py_IgnoreEnvironmentFlag ? NULL : getenv(s))
void Py_FatalError(char *message);
