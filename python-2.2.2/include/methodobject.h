//20180318
#pragma once

extern PyTypeObject PyCFunction_Type;
#define PyCFunction_Check(op) ((op)->ob_type == &PyCFunction_Type)
typedef PyObject *(*PyCFunction)(PyObject *, PyObject *);
typedef PyObject *(*PyCFunctionWithKeywords)(PyObject *, PyObject *, PyObject *);
typedef PyObject *(*PyNoArgsFunction)(PyObject *);
extern PyCFunction PyCFunction_GetFunction(PyObject *);
extern PyObject * PyCFunction_GetSelf(PyObject *);
extern int PyCFunction_GetFlags(PyObject *);
#define PyCFunction_GET_FUNCTION(func) (((PyCFunctionObject *)func) -> m_ml -> ml_meth)
#define PyCFunction_GET_SELF(func) (((PyCFunctionObject *)func) -> m_self)
#define PyCFunction_GET_FLAGS(func) (((PyCFunctionObject *)func) -> m_ml -> ml_flags)
extern PyObject * PyCFunction_Call(PyObject *, PyObject *, PyObject *);

struct PyMethodDef {
    char	*ml_name;
    PyCFunction  ml_meth;
    int		 ml_flags;
    char	*ml_doc;
};
typedef struct PyMethodDef PyMethodDef;

extern PyObject * Py_FindMethod(PyMethodDef[], PyObject *, char *);
extern PyObject * PyCFunction_New(PyMethodDef *, PyObject *);

#define METH_OLDARGS  0x0000
#define METH_VARARGS  0x0001
#define METH_KEYWORDS 0x0002
#define METH_NOARGS   0x0004
#define METH_O        0x0008

typedef struct PyMethodChain {
    PyMethodDef *methods;		
    struct PyMethodChain *link;	
} PyMethodChain;

extern PyObject * Py_FindMethodInChain(PyMethodChain *, PyObject *, char *);

typedef struct {
    PyObject_HEAD
    PyMethodDef *m_ml;
    PyObject    *m_self;
} PyCFunctionObject;
