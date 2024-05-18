//20180406
#include "python.h"
#include "structmember.h"

static char xxsubtype__doc__[] =
	"xxsubtype is an example module showing how to subtype builtin types from C.\n"
	"test_descr.py in the standard test suite requires it in order to complete.\n"
	"If you don't care about the examples, and don't intend to run the Python\n"
	"test suite, you can recompile Python without Modules/xxsubtype.c.";

#define DEFERRED_ADDRESS(ADDR) 0

typedef struct {
	PyListObject list;
	int state;
} spamlistobject;

static PyObject *spamlist_getstate(spamlistobject *self, PyObject *args)
{
	if (!PyArg_ParseTuple(args, ":getstate"))
	{
		return NULL;
	}
	return PyInt_FromLong(self->state);
}

static PyObject *spamlist_setstate(spamlistobject *self, PyObject *args)
{
	int state;

	if (!PyArg_ParseTuple(args, "i:setstate", &state))
	{
		return NULL;
	}
	self->state = state;
	Py_INCREF(Py_None);
	return Py_None;
}

static PyMethodDef spamlist_methods[] = {
	{"getstate", (PyCFunction)spamlist_getstate, METH_VARARGS,
	 	"getstate() -> state"},
	{"setstate", (PyCFunction)spamlist_setstate, METH_VARARGS,
	 	"setstate(state)"},
	{NULL,	NULL},
};

staticforward PyTypeObject spamlist_type;

static int spamlist_init(spamlistobject *self, PyObject *args, PyObject *kwds)
{
	if (PyList_Type.tp_init((PyObject *)self, args, kwds) < 0)
	{
		return -1;
	}
	self->state = 0;
	return 0;
}

static PyObject *spamlist_state_get(spamlistobject *self)
{
	return PyInt_FromLong(self->state);
}

static PyGetSetDef spamlist_getsets[] = {
	{"state", (getter)spamlist_state_get, NULL,
	 "an int variable for demonstration purposes"},
	{0}
};

static PyTypeObject spamlist_type = {
	PyObject_HEAD_INIT(DEFERRED_ADDRESS(&PyType_Type))
	0,
	"xxsubtype.spamlist",
	sizeof(spamlistobject),
	0,
	0,				
	0,				
	0,				
	0,				
	0,				
	0,				
	0,			
	0,				
	0,				
	0,				
	0,				
	0,				
	0,				
	0,				
	0,				
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, 
	0,				
	0,				
	0,				
	0,				
	0,				
	0,				
	0,				
	spamlist_methods,	
	0,				
	spamlist_getsets,		
	DEFERRED_ADDRESS(&PyList_Type),	
	0,				
	0,				
	0,				
	0,				
	(initproc)spamlist_init,
	0,				
	0,				
};

typedef struct {
	PyDictObject dict;
	int state;
} spamdictobject;

static PyObject *spamdict_getstate(spamdictobject *self, PyObject *args)
{
	if (!PyArg_ParseTuple(args, ":getstate"))
	{
		return NULL;
	}
	return PyInt_FromLong(self->state);
}

static PyObject *spamdict_setstate(spamdictobject *self, PyObject *args)
{
	int state;

	if (!PyArg_ParseTuple(args, "i:setstate", &state))
	{
		return NULL;
	}
	self->state = state;
	Py_INCREF(Py_None);
	return Py_None;
}

static PyMethodDef spamdict_methods[] = {
	{"getstate", (PyCFunction)spamdict_getstate, METH_VARARGS,
	 	"getstate() -> state"},
	{"setstate", (PyCFunction)spamdict_setstate, METH_VARARGS,
	 	"setstate(state)"},
	{NULL,	NULL},
};

staticforward PyTypeObject spamdict_type;

static int spamdict_init(spamdictobject *self, PyObject *args, PyObject *kwds)
{
	if (PyDict_Type.tp_init((PyObject *)self, args, kwds) < 0)
	{
		return -1;
	}
	self->state = 0;
	return 0;
}

static PyMemberDef spamdict_members[] = {
	{"state", T_INT, offsetof(spamdictobject, state), READONLY,
	 "an int variable for demonstration purposes"},
	{0}
};

static PyTypeObject spamdict_type = {
	PyObject_HEAD_INIT(DEFERRED_ADDRESS(&PyType_Type))
	0,
	"xxsubtype.spamdict",
	sizeof(spamdictobject),
	0,
	0,				
	0,				
	0,				
	0,				
	0,				
	0,				
	0,				
	0,					
	0,				
	0,				
	0,				
	0,				
	0,				
	0,				
	0,				
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, 
	0,				
	0,				
	0,				
	0,				
	0,				
	0,				
	0,				
	spamdict_methods,
	spamdict_members,
	0,				
	DEFERRED_ADDRESS(&PyDict_Type),	
	0,				
	0,				
	0,				
	0,				
	(initproc)spamdict_init,	
	0,				
	0,				
};

static PyObject *spam_bench(PyObject *self, PyObject *args)
{
	PyObject *obj, *name, *res;
	int n = 1000;
	time_t t0, t1;

	if (!PyArg_ParseTuple(args, "OS|i", &obj, &name, &n))
	{
		return NULL;
	}
	t0 = clock();
	while (--n >= 0) 
	{
		res = PyObject_GetAttr(obj, name);
		if (res == NULL)
		{
			return NULL;
		}
		Py_DECREF(res);
	}
	t1 = clock();
	return PyFloat_FromDouble((double)(t1-t0) / CLOCKS_PER_SEC);
}

static PyMethodDef xxsubtype_functions[] = {
	{"bench",	spam_bench, 	METH_VARARGS},
	{NULL,		NULL}
};

void initxxsubtype()
{
	PyObject *m, *d;

	spamdict_type.tp_base = &PyDict_Type;
	if (PyType_Ready(&spamdict_type) < 0)
	{
		return;
	}

	spamlist_type.tp_base = &PyList_Type;
	if (PyType_Ready(&spamlist_type) < 0)
	{
		return;
	}

	m = Py_InitModule3("xxsubtype",
			   xxsubtype_functions,
			   xxsubtype__doc__);
	if (m == NULL)
	{
		return;
	}

	if (PyType_Ready(&spamlist_type) < 0)
	{
		return;
	}
	if (PyType_Ready(&spamdict_type) < 0)
	{
		return;
	}

	d = PyModule_GetDict(m);
	if (d == NULL)
	{
		return;
	}

	Py_INCREF(&spamlist_type);
	if (PyDict_SetItemString(d, "spamlist",
				 (PyObject *) &spamlist_type) < 0)
	{
		return;
	}

	Py_INCREF(&spamdict_type);
	if (PyDict_SetItemString(d, "spamdict",
				 (PyObject *) &spamdict_type) < 0)
	{
		return;
	}
}
