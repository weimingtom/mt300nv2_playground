//20170420
#include "python.h"
#include "structmember.h"

typedef struct {
	PyObject_HEAD
	PyObject *md_dict;
} PyModuleObject;

static PyMemberDef module_members[] = {
	{"__dict__", T_OBJECT, offsetof(PyModuleObject, md_dict), READONLY},
	{0}
};

PyObject *PyModule_New(char *name)
{
	PyModuleObject *m;
	PyObject *nameobj;
	m = PyObject_GC_New(PyModuleObject, &PyModule_Type);
	if (m == NULL)
	{
		return NULL;
	}
	nameobj = PyString_FromString(name);
	m->md_dict = PyDict_New();
	if (m->md_dict == NULL || nameobj == NULL)
	{
		goto fail;
	}
	if (PyDict_SetItemString(m->md_dict, "__name__", nameobj) != 0)
	{
		goto fail;
	}
	if (PyDict_SetItemString(m->md_dict, "__doc__", Py_None) != 0)
	{
		goto fail;
	}
	Py_DECREF(nameobj);
	PyObject_GC_Track(m);
	return (PyObject *)m;

fail:
	Py_XDECREF(nameobj);
	Py_DECREF(m);
	return NULL;
}

PyObject *PyModule_GetDict(PyObject *m)
{
	PyObject *d;
	if (!PyModule_Check(m)) 
	{
		PyErr_BadInternalCall();
		return NULL;
	}
	d = ((PyModuleObject *)m) -> md_dict;
	if (d == NULL)
	{
		((PyModuleObject *)m) -> md_dict = d = PyDict_New();
	}
	return d;
}

char *PyModule_GetName(PyObject *m)
{
	PyObject *d;
	PyObject *nameobj;
	if (!PyModule_Check(m)) 
	{
		PyErr_BadArgument();
		return NULL;
	}
	d = ((PyModuleObject *)m)->md_dict;
	if (d == NULL ||
	    (nameobj = PyDict_GetItemString(d, "__name__")) == NULL ||
	    !PyString_Check(nameobj))
	{
		PyErr_SetString(PyExc_SystemError, "nameless module");
		return NULL;
	}
	return PyString_AsString(nameobj);
}

char *PyModule_GetFilename(PyObject *m)
{
	PyObject *d;
	PyObject *fileobj;
	if (!PyModule_Check(m)) 
	{
		PyErr_BadArgument();
		return NULL;
	}
	d = ((PyModuleObject *)m)->md_dict;
	if (d == NULL ||
	    (fileobj = PyDict_GetItemString(d, "__file__")) == NULL ||
	    !PyString_Check(fileobj))
	{
		PyErr_SetString(PyExc_SystemError, "module filename missing");
		return NULL;
	}
	return PyString_AsString(fileobj);
}

void _PyModule_Clear(PyObject *m)
{
	int pos;
	PyObject *key, *value;
	PyObject *d;

	d = ((PyModuleObject *)m)->md_dict;
	if (d == NULL)
	{
		return;
	}

	pos = 0;
	while (PyDict_Next(d, &pos, &key, &value)) 
	{
		if (value != Py_None && PyString_Check(key)) 
		{
			char *s = PyString_AsString(key);
			if (s[0] == '_' && s[1] != '_') 
			{
				if (Py_VerboseFlag > 1)
				{
					PySys_WriteStderr("#   clear[1] %s\n", s);
				}
				PyDict_SetItem(d, key, Py_None);
			}
		}
	}

	pos = 0;
	while (PyDict_Next(d, &pos, &key, &value)) 
	{
		if (value != Py_None && PyString_Check(key)) 
		{
			char *s = PyString_AsString(key);
			if (s[0] != '_' || strcmp(s, "__builtins__") != 0) 
			{
				if (Py_VerboseFlag > 1)
				{
					PySys_WriteStderr("#   clear[2] %s\n", s);
				}
				PyDict_SetItem(d, key, Py_None);
			}
		}
	}
}

static int module_init(PyModuleObject *m, PyObject *args, PyObject *kw)
{
	m->md_dict = PyDict_New();
	if (m->md_dict == NULL)
	{
		return -1;
	}
	return 0;
}

static void module_dealloc(PyModuleObject *m)
{
	PyObject_GC_UnTrack(m);
	if (m->md_dict != NULL) 
	{
		_PyModule_Clear((PyObject *)m);
		Py_DECREF(m->md_dict);
	}
	m->ob_type->tp_free((PyObject *)m);
}

static PyObject *module_repr(PyModuleObject *m)
{
	char *name;
	char *filename;

	name = PyModule_GetName((PyObject *)m);
	if (name == NULL) 
	{
		PyErr_Clear();
		name = "?";
	}
	filename = PyModule_GetFilename((PyObject *)m);
	if (filename == NULL) 
	{
		PyErr_Clear();
		return PyString_FromFormat("<module '%s' (built-in)>", name);
	}
	return PyString_FromFormat("<module '%s' from '%s'>", name, filename);
}

static int module_traverse(PyModuleObject *m, visitproc visit, void *arg)
{
	if (m->md_dict != NULL)
	{
		return visit(m->md_dict, arg);
	}
	return 0;
}

PyTypeObject PyModule_Type = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,				
	"module",			
	sizeof(PyModuleObject),	
	0,					
	(destructor)module_dealloc,	
	0,					
	0,					
	0,					
	0,					
	(reprfunc)module_repr,			
	0,					
	0,					
	0,					
	0,					
	0,					
	0,					
	PyObject_GenericGetAttr,		
	PyObject_GenericSetAttr,		
	0,					
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC |
		Py_TPFLAGS_BASETYPE,		
	0,					
	(traverseproc)module_traverse,	
	0,					
	0,					
	0,					
	0,					
	0,					
	0,					
	module_members,			
	0,				
	0,					
	0,					
	0,					
	0,					
	offsetof(PyModuleObject, md_dict),
	(initproc)module_init,	
	PyType_GenericAlloc,	
	PyType_GenericNew,		
	_PyObject_GC_Del,		
};
