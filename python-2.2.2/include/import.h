//20180318
#pragma once

long PyImport_GetMagicNumber();
PyObject * PyImport_ExecCodeModule(char *name, PyObject *co);
PyObject * PyImport_ExecCodeModuleEx(char *name, PyObject *co, char *pathname);
PyObject * PyImport_GetModuleDict();
PyObject * PyImport_AddModule(char *name);
PyObject * PyImport_ImportModule(char *name);
PyObject * PyImport_ImportModuleEx(char *name, PyObject *globals, PyObject *locals, PyObject *fromlist);
PyObject * PyImport_Import(PyObject *name);
PyObject * PyImport_ReloadModule(PyObject *m);
void PyImport_Cleanup();
int PyImport_ImportFrozenModule(char *);
extern PyObject *_PyImport_FindExtension(char *, char *);
extern PyObject *_PyImport_FixupExtension(char *, char *);

struct _inittab {
    char *name;
    void (*initfunc)();
};

extern struct _inittab * PyImport_Inittab;
extern int PyImport_AppendInittab(char *name, void (*initfunc)());
extern int PyImport_ExtendInittab(struct _inittab *newtab);

struct _frozen {
    char *name;
    unsigned char *code;
    int size;
};

extern struct _frozen * PyImport_FrozenModules;
