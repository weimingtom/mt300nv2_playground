//20180109
#include "python.h"

#include <ctype.h>

void PyErr_Restore(PyObject *type, PyObject *value, PyObject *traceback)
{
	PyThreadState *tstate = PyThreadState_GET();
	PyObject *oldtype, *oldvalue, *oldtraceback;

	if (traceback != NULL && !PyTraceBack_Check(traceback)) 
	{
		Py_DECREF(traceback);
		traceback = NULL;
	}

	oldtype = tstate->curexc_type;
	oldvalue = tstate->curexc_value;
	oldtraceback = tstate->curexc_traceback;

	tstate->curexc_type = type;
	tstate->curexc_value = value;
	tstate->curexc_traceback = traceback;

	Py_XDECREF(oldtype);
	Py_XDECREF(oldvalue);
	Py_XDECREF(oldtraceback);
}

void PyErr_SetObject(PyObject *exception, PyObject *value)
{
	Py_XINCREF(exception);
	Py_XINCREF(value);
	PyErr_Restore(exception, value, (PyObject *)NULL);
}

void PyErr_SetNone(PyObject *exception)
{
	PyErr_SetObject(exception, (PyObject *)NULL);
}

void PyErr_SetString(PyObject *exception, const char *string)
{
	PyObject *value = PyString_FromString(string);
	PyErr_SetObject(exception, value);
	Py_XDECREF(value);
}

PyObject *PyErr_Occurred()
{
	PyThreadState *tstate = PyThreadState_GET();

	return tstate->curexc_type;
}

int PyErr_GivenExceptionMatches(PyObject *err, PyObject *exc)
{
	if (err == NULL || exc == NULL) 
	{
		return 0;
	}
	if (PyTuple_Check(exc)) 
	{
		int i, n;
		n = PyTuple_Size(exc);
		for (i = 0; i < n; i++) 
		{
			if (PyErr_GivenExceptionMatches(
			     err, PyTuple_GET_ITEM(exc, i)))
		    {
			     return 1;
		    }
		}
		return 0;
	}
	if (PyInstance_Check(err))
	{
		err = (PyObject*)((PyInstanceObject*)err)->in_class;
	}
	if (PyClass_Check(err) && PyClass_Check(exc))
	{
		return PyClass_IsSubclass(err, exc);
	}
	return err == exc;
}

int PyErr_ExceptionMatches(PyObject *exc)
{
	return PyErr_GivenExceptionMatches(PyErr_Occurred(), exc);
}

void PyErr_NormalizeException(PyObject **exc, PyObject **val, PyObject **tb)
{
	PyObject *type = *exc;
	PyObject *value = *val;
	PyObject *inclass = NULL;
	PyObject *initial_tb = NULL;

	if (type == NULL) 
	{
		PyErr_SetString(PyExc_SystemError,
			"PyErr_NormalizeException() called without exception");
	}

	if (!value) 
	{
		value = Py_None;
		Py_INCREF(value);
	}

	if (PyInstance_Check(value))
	{
		inclass = (PyObject*)((PyInstanceObject*)value)->in_class;
	}

	if (PyClass_Check(type)) 
	{
		if (!inclass || !PyClass_IsSubclass(inclass, type)) 
		{
			PyObject *args, *res;

			if (value == Py_None)
			{
				args = Py_BuildValue("()");
			}
			else if (PyTuple_Check(value)) 
			{
				Py_INCREF(value);
				args = value;
			}
			else
			{
				args = Py_BuildValue("(O)", value);
			}

			if (args == NULL)
			{
				goto finally;
			}
			res = PyEval_CallObject(type, args);
			Py_DECREF(args);
			if (res == NULL)
			{
				goto finally;
			}
			Py_DECREF(value);
			value = res;
		}
		else if (inclass != type) 
		{
 			Py_DECREF(type);
			type = inclass;
			Py_INCREF(type);
		}
	}
	*exc = type;
	*val = value;
	return;

finally:
	Py_DECREF(type);
	Py_DECREF(value);
	initial_tb = *tb;
	PyErr_Fetch(exc, val, tb);
	if (initial_tb != NULL) 
	{
		if (*tb == NULL)
		{
			*tb = initial_tb;
		}
		else
		{
			Py_DECREF(initial_tb);
		}
	}
	PyErr_NormalizeException(exc, val, tb);
}

void PyErr_Fetch(PyObject **p_type, PyObject **p_value, PyObject **p_traceback)
{
	PyThreadState *tstate = PyThreadState_Get();

	*p_type = tstate->curexc_type;
	*p_value = tstate->curexc_value;
	*p_traceback = tstate->curexc_traceback;

	tstate->curexc_type = NULL;
	tstate->curexc_value = NULL;
	tstate->curexc_traceback = NULL;
}

void PyErr_Clear()
{
	PyErr_Restore(NULL, NULL, NULL);
}

int PyErr_BadArgument()
{
	PyErr_SetString(PyExc_TypeError,
			"bad argument type for built-in operation");
	return 0;
}

PyObject *PyErr_NoMemory()
{
	if (PyErr_ExceptionMatches(PyExc_MemoryError))
	{
		return NULL;
	}

	if (PyExc_MemoryErrorInst)
	{
		PyErr_SetObject(PyExc_MemoryError, PyExc_MemoryErrorInst);
	}
	else
	{
		PyErr_SetNone(PyExc_MemoryError);
	}

	return NULL;
}

PyObject *PyErr_SetFromErrnoWithFilename(PyObject *exc, char *filename)
{
	PyObject *v;
	const char *s;
	int i = errno;
	char *s_buf = NULL;
	if (i == EINTR && PyErr_CheckSignals())
	{
		return NULL;
	}
	if (i == 0)
	{
		s = "Error";
	}
	else
	{
		s = strerror(i);
	}
	if (filename != NULL)
	{
		v = Py_BuildValue("(iss)", i, s, filename);
	}
	else
	{
		v = Py_BuildValue("(is)", i, s);
	}
	if (v != NULL) 
	{
		PyErr_SetObject(exc, v);
		Py_DECREF(v);
	}
	return NULL;
}

PyObject *PyErr_SetFromErrno(PyObject *exc)
{
	return PyErr_SetFromErrnoWithFilename(exc, NULL);
}

void _PyErr_BadInternalCall(char *filename, int lineno)
{
	PyErr_Format(PyExc_SystemError,
		     "%s:%d: bad argument to internal function",
		     filename, lineno);
}

#undef PyErr_BadInternalCall
void PyErr_BadInternalCall()
{
	PyErr_Format(PyExc_SystemError,
		     "bad argument to internal function");
}
#define PyErr_BadInternalCall() _PyErr_BadInternalCall(__FILE__, __LINE__)

PyObject *PyErr_Format(PyObject *exception, const char *format, ...)
{
	va_list vargs;
	PyObject* string;

	va_start(vargs, format);

	string = PyString_FromFormatV(format, vargs);
	PyErr_SetObject(exception, string);
	Py_XDECREF(string);
	va_end(vargs);
	return NULL;
}

PyObject *PyErr_NewException(char *name, PyObject *base, PyObject *dict)
{
	char *dot;
	PyObject *modulename = NULL;
	PyObject *classname = NULL;
	PyObject *mydict = NULL;
	PyObject *bases = NULL;
	PyObject *result = NULL;
	dot = strrchr(name, '.');
	if (dot == NULL) 
	{
		PyErr_SetString(PyExc_SystemError,
			"PyErr_NewException: name must be module.class");
		return NULL;
	}
	if (base == NULL)
	{
		base = PyExc_Exception;
	}
	if (!PyClass_Check(base)) 
	{
		return PyString_FromString(name);
	}
	if (dict == NULL) 
	{
		dict = mydict = PyDict_New();
		if (dict == NULL)
		{
			goto failure;
		}
	}
	if (PyDict_GetItemString(dict, "__module__") == NULL) 
	{
		modulename = PyString_FromStringAndSize(name, (int)(dot-name));
		if (modulename == NULL)
		{
			goto failure;
		}
		if (PyDict_SetItemString(dict, "__module__", modulename) != 0)
		{
			goto failure;
		}
	}
	classname = PyString_FromString(dot+1);
	if (classname == NULL)
	{
		goto failure;
	}
	bases = Py_BuildValue("(O)", base);
	if (bases == NULL)
	{
		goto failure;
	}
	result = PyClass_New(bases, dict, classname);
  
failure:
	Py_XDECREF(bases);
	Py_XDECREF(mydict);
	Py_XDECREF(classname);
	Py_XDECREF(modulename);
	return result;
}

void PyErr_WriteUnraisable(PyObject *obj)
{
	PyObject *f, *t, *v, *tb;
	PyErr_Fetch(&t, &v, &tb);
	f = PySys_GetObject("stderr");
	if (f != NULL) 
	{
		PyFile_WriteString("Exception ", f);
		if (t) 
		{
			PyFile_WriteObject(t, f, Py_PRINT_RAW);
			if (v && v != Py_None) 
			{
				PyFile_WriteString(": ", f);
				PyFile_WriteObject(v, f, 0);
			}
		}
		PyFile_WriteString(" in ", f);
		PyFile_WriteObject(obj, f, 0);
		PyFile_WriteString(" ignored\n", f);
		PyErr_Clear();
	}
	Py_XDECREF(t);
	Py_XDECREF(v);
	Py_XDECREF(tb);
}


int PyErr_Warn(PyObject *category, char *message)
{
	PyObject *mod, *dict, *func = NULL;

	mod = PyImport_ImportModule("warnings");
	if (mod != NULL) 
	{
		dict = PyModule_GetDict(mod);
		func = PyDict_GetItemString(dict, "warn");
		Py_DECREF(mod);
	}
	if (func == NULL) 
	{
		PySys_WriteStderr("warning: %s\n", message);
		return 0;
	}
	else 
	{
		PyObject *args, *res;

		if (category == NULL)
		{
			category = PyExc_RuntimeWarning;
		}
		args = Py_BuildValue("(sO)", message, category);
		if (args == NULL)
		{
			return -1;
		}
		res = PyEval_CallObject(func, args);
		Py_DECREF(args);
		if (res == NULL)
		{
			return -1;
		}
		Py_DECREF(res);
		return 0;
	}
}

int PyErr_WarnExplicit(PyObject *category, char *message,
		   char *filename, int lineno,
		   char *module, PyObject *registry)
{
	PyObject *mod, *dict, *func = NULL;

	mod = PyImport_ImportModule("warnings");
	if (mod != NULL) 
	{
		dict = PyModule_GetDict(mod);
		func = PyDict_GetItemString(dict, "warn_explicit");
		Py_DECREF(mod);
	}
	if (func == NULL) 
	{
		PySys_WriteStderr("warning: %s\n", message);
		return 0;
	}
	else 
	{
		PyObject *args, *res;

		if (category == NULL)
		{
			category = PyExc_RuntimeWarning;
		}
		if (registry == NULL)
		{
			registry = Py_None;
		}
		args = Py_BuildValue("(sOsizO)", message, category,
				     filename, lineno, module, registry);
		if (args == NULL)
		{
			return -1;
		}
		res = PyEval_CallObject(func, args);
		Py_DECREF(args);
		if (res == NULL)
		{
			return -1;
		}
		Py_DECREF(res);
		return 0;
	}
}

void PyErr_SyntaxLocation(char *filename, int lineno)
{
	PyObject *exc, *v, *tb, *tmp;

	PyErr_Fetch(&exc, &v, &tb);
	PyErr_NormalizeException(&exc, &v, &tb);
	tmp = PyInt_FromLong(lineno);
	if (tmp == NULL)
	{
		PyErr_Clear();
	}
	else 
	{
		if (PyObject_SetAttrString(v, "lineno", tmp))
		{
			PyErr_Clear();
		}
		Py_DECREF(tmp);
	}
	if (filename != NULL) 
	{
		tmp = PyString_FromString(filename);
		if (tmp == NULL)
		{
			PyErr_Clear();
		}
		else 
		{
			if (PyObject_SetAttrString(v, "filename", tmp))
			{
				PyErr_Clear();
			}
			Py_DECREF(tmp);
		}
		tmp = PyErr_ProgramText(filename, lineno);
		if (tmp) 
		{
			PyObject_SetAttrString(v, "text", tmp);
			Py_DECREF(tmp);
		}
	}
	if (PyObject_SetAttrString(v, "offset", Py_None)) 
	{
		PyErr_Clear();
	}
	if (exc != PyExc_SyntaxError) 
	{
		if (!PyObject_HasAttrString(v, "msg")) 
		{
			tmp = PyObject_Str(v);
			if (tmp) 
			{
				if (PyObject_SetAttrString(v, "msg", tmp))
				{
					PyErr_Clear();
				}
				Py_DECREF(tmp);
			} 
			else 
			{
				PyErr_Clear();
			}
		}
		if (!PyObject_HasAttrString(v, "print_file_and_line")) 
		{
			if (PyObject_SetAttrString(v, "print_file_and_line",
						   Py_None))
			{
				PyErr_Clear();
			}
		}
	}
	PyErr_Restore(exc, v, tb);
}

PyObject *PyErr_ProgramText(char *filename, int lineno)
{
	FILE *fp;
	int i;
	char linebuf[1000];

	if (filename == NULL || lineno <= 0)
	{
		return NULL;
	}
	fp = fopen(filename, "r");
	if (fp == NULL)
	{
		return NULL;
	}
	for (i = 0; i < lineno; i++) 
	{
		char *pLastChar = &linebuf[sizeof(linebuf) - 2];
		do 
		{
			*pLastChar = '\0';
			if (fgets(linebuf, sizeof linebuf, fp) == NULL)
			{
				break;
			}
		} while (*pLastChar != '\0' && *pLastChar != '\n');
	}
	fclose(fp);
	if (i == lineno) 
	{
		char *p = linebuf;
		while (*p == ' ' || *p == '\t' || *p == '\014')
		{
			p++;
		}
		return PyString_FromString(p);
	}
	return NULL;
}
