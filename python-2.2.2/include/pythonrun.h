//20180324
#pragma once

#define PyCF_MASK (CO_GENERATOR_ALLOWED | CO_FUTURE_DIVISION)
#define PyCF_MASK_OBSOLETE (CO_NESTED)

typedef struct {
	int cf_flags;
} PyCompilerFlags;

void Py_SetProgramName(char *);
char * Py_GetProgramName();
void Py_SetPythonHome(char *);
char * Py_GetPythonHome();
void Py_Initialize();
void Py_Finalize();
int Py_IsInitialized();
PyThreadState * Py_NewInterpreter();
void Py_EndInterpreter(PyThreadState *);

int PyRun_AnyFile(FILE *, char *);
int PyRun_AnyFileEx(FILE *, char *, int);
int PyRun_AnyFileFlags(FILE *, char *, PyCompilerFlags *);
int PyRun_AnyFileExFlags(FILE *, char *, int, PyCompilerFlags *);
int PyRun_SimpleString(char *);
int PyRun_SimpleStringFlags(char *, PyCompilerFlags *);
int PyRun_SimpleFile(FILE *, char *);
int PyRun_SimpleFileEx(FILE *, char *, int);
int PyRun_SimpleFileExFlags(FILE *, char *, int, PyCompilerFlags *);
int PyRun_InteractiveOne(FILE *, char *);
int PyRun_InteractiveOneFlags(FILE *, char *, PyCompilerFlags *);
int PyRun_InteractiveLoop(FILE *, char *);
int PyRun_InteractiveLoopFlags(FILE *, char *, PyCompilerFlags *);

struct _node * PyParser_SimpleParseString(char *, int);
struct _node * PyParser_SimpleParseFile(FILE *, char *, int);
struct _node * PyParser_SimpleParseStringFlags(char *, int, int);
struct _node * PyParser_SimpleParseFileFlags(FILE *, char *, int, int);

PyObject * PyRun_String(char *, int, PyObject *, PyObject *);
PyObject * PyRun_File(FILE *, char *, int, PyObject *, PyObject *);
PyObject * PyRun_FileEx(FILE *, char *, int, PyObject *, PyObject *, int);
PyObject * PyRun_StringFlags(char *, int, PyObject *, PyObject *, PyCompilerFlags *);
PyObject * PyRun_FileFlags(FILE *, char *, int, PyObject *, PyObject *, PyCompilerFlags *);
PyObject * PyRun_FileExFlags(FILE *, char *, int, PyObject *, PyObject *, int, PyCompilerFlags *);

PyObject * Py_CompileString(char *, char *, int);
PyObject * Py_CompileStringFlags(char *, char *, int, PyCompilerFlags *);
struct symtable * Py_SymtableString(char *, char *, int);
void PyErr_Print();
void PyErr_PrintEx(int);
void PyErr_Display(PyObject *, PyObject *, PyObject *);
int Py_AtExit(void (*func)());
void Py_Exit(int);
int Py_FdIsInteractive(FILE *, char *);
char * Py_GetProgramFullPath();
char * Py_GetPrefix();
char * Py_GetExecPrefix();
char * Py_GetPath();
const char * Py_GetVersion();
const char * Py_GetPlatform();
const char * Py_GetCopyright();
const char * Py_GetCompiler();
const char * Py_GetBuildInfo();

PyObject * _PyBuiltin_Init();
PyObject * _PySys_Init();
void _PyImport_Init();
void _PyExc_Init();
void _PyExc_Fini();
void _PyImport_Fini();
void PyMethod_Fini();
void PyFrame_Fini();
void PyCFunction_Fini();
void PyTuple_Fini();
void PyString_Fini();
void PyInt_Fini();
void PyFloat_Fini();
void PyOS_FiniInterrupts();
char * PyOS_Readline(char *);
extern int (*PyOS_InputHook)();
char *(*PyOS_ReadlineFunctionPointer)(char *);
#define PYOS_STACK_MARGIN 2048
#define USE_STACKCHECK
int PyOS_CheckStack();
typedef void (*PyOS_sighandler_t)(int);
PyOS_sighandler_t PyOS_getsig(int);
PyOS_sighandler_t PyOS_setsig(int, PyOS_sighandler_t);
