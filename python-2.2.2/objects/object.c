//20170420
#include "python.h"

#include "compile.h"
#include "frameobject.h"
#include "traceback.h"

#ifdef _DEBUG
long _Py_RefTotal;
#endif

int Py_DivisionWarningFlag;

PyObject *PyObject_Init(PyObject *op, PyTypeObject *tp)
{
	if (op == NULL)
	{
		return PyErr_NoMemory();
	}
	op->ob_type = tp;
	_Py_NewReference(op);
	return op;
}

PyVarObject *PyObject_InitVar(PyVarObject *op, PyTypeObject *tp, int size)
{
	if (op == NULL)
	{
		return (PyVarObject *) PyErr_NoMemory();
	}
	op->ob_size = size;
	op->ob_type = tp;
	_Py_NewReference((PyObject *)op);
	return op;
}

PyObject *_PyObject_New(PyTypeObject *tp)
{
	PyObject *op;
	op = (PyObject *) PyObject_MALLOC(_PyObject_SIZE(tp));
	if (op == NULL)
	{
		return PyErr_NoMemory();
	}
	return PyObject_INIT(op, tp);
}

PyVarObject *_PyObject_NewVar(PyTypeObject *tp, int nitems)
{
	PyVarObject *op;
	const size_t size = _PyObject_VAR_SIZE(tp, nitems);
	op = (PyVarObject *) PyObject_MALLOC(size);
	if (op == NULL)
	{
		return (PyVarObject *)PyErr_NoMemory();
	}
	return PyObject_INIT_VAR(op, tp, nitems);
}

void _PyObject_Del(PyObject *op)
{
	PyObject_FREE(op);
}

int PyObject_Print(PyObject *op, FILE *fp, int flags)
{
	int ret = 0;
	if (PyErr_CheckSignals())
	{
		return -1;
	}
	if (PyOS_CheckStack()) 
	{
		PyErr_SetString(PyExc_MemoryError, "stack overflow");
		return -1;
	}
	clearerr(fp); 
	if (op == NULL) 
	{
		fprintf(fp, "<nil>");
	}
	else 
	{
		if (op->ob_refcnt <= 0)
		{
			fprintf(fp, "<refcnt %u at %p>",
				op->ob_refcnt, op);
		}
		else if (op->ob_type->tp_print == NULL) 
		{
			PyObject *s;
			if (flags & Py_PRINT_RAW)
			{
				s = PyObject_Str(op);
			}
			else
			{
				s = PyObject_Repr(op);
			}
			if (s == NULL)
			{
				ret = -1;
			}
			else 
			{
				ret = PyObject_Print(s, fp, Py_PRINT_RAW);
			}
			Py_XDECREF(s);
		}
		else
		{
			ret = (*op->ob_type->tp_print)(op, fp, flags);
		}
	}
	if (ret == 0) 
	{
		if (ferror(fp)) 
		{
			PyErr_SetFromErrno(PyExc_IOError);
			clearerr(fp);
			ret = -1;
		}
	}
	return ret;
}

void _PyObject_Dump(PyObject* op) 
{
	if (op == NULL)
	{
		fprintf(stderr, "NULL\n");
	}
	else 
	{
		fprintf(stderr, "object  : ");
		PyObject_Print(op, stderr, 0);
		fprintf(stderr, "\n"
			"type    : %s\n"
			"refcount: %d\n"
			"address : %p\n",
			op->ob_type==NULL ? "NULL" : op->ob_type->tp_name,
			op->ob_refcnt,
			op);
	}
}

PyObject *PyObject_Repr(PyObject *v)
{
	if (PyErr_CheckSignals())
	{
		return NULL;
	}
	if (PyOS_CheckStack()) 
	{
		PyErr_SetString(PyExc_MemoryError, "stack overflow");
		return NULL;
	}
	if (v == NULL)
	{
		return PyString_FromString("<NULL>");
	}
	else if (v->ob_type->tp_repr == NULL)
	{
		return PyString_FromFormat("<%s object at %p>",
					   v->ob_type->tp_name, v);
	}
	else 
	{
		PyObject *res;
		res = (*v->ob_type->tp_repr)(v);
		if (res == NULL)
		{
			return NULL;
		}
		if (PyUnicode_Check(res)) 
		{
			PyObject* str;
			str = PyUnicode_AsUnicodeEscapeString(res);
			Py_DECREF(res);
			if (str)
			{
				res = str;
			}
			else
			{
				return NULL;
			}
		}
		if (!PyString_Check(res)) 
		{
			PyErr_Format(PyExc_TypeError,
				     "__repr__ returned non-string (type %.200s)",
				     res->ob_type->tp_name);
			Py_DECREF(res);
			return NULL;
		}
		return res;
	}
}

PyObject *PyObject_Str(PyObject *v)
{
	PyObject *res;
	
	if (v == NULL)
	{
		return PyString_FromString("<NULL>");
	}
	if (PyString_CheckExact(v)) 
	{
		Py_INCREF(v);
		return v;
	}
	if (v->ob_type->tp_str == NULL)
	{
		return PyObject_Repr(v);
	}

	res = (*v->ob_type->tp_str)(v);
	if (res == NULL)
	{
		return NULL;
	}
	if (PyUnicode_Check(res)) 
	{
		PyObject* str;
		str = PyUnicode_AsEncodedString(res, NULL, NULL);
		Py_DECREF(res);
		if (str)
		{
			res = str;
		}
		else
		{
			return NULL;
		}
	}
	if (!PyString_Check(res)) 
	{
		PyErr_Format(PyExc_TypeError,
			     "__str__ returned non-string (type %.200s)",
			     res->ob_type->tp_name);
		Py_DECREF(res);
		return NULL;
	}
	return res;
}

PyObject *PyObject_Unicode(PyObject *v)
{
	PyObject *res;
	
	if (v == NULL)
	{
		res = PyString_FromString("<NULL>");
	}
	if (PyUnicode_CheckExact(v)) 
	{
		Py_INCREF(v);
		return v;
	}
	if (PyUnicode_Check(v)) 
	{
		return PyUnicode_FromUnicode(PyUnicode_AS_UNICODE(v),
					     PyUnicode_GET_SIZE(v));
	}
	if (PyString_Check(v)) 
	{
		Py_INCREF(v);
		res = v;
    }
	else 
	{
		PyObject *func;
		static PyObject *unicodestr;
		if (unicodestr == NULL) 
		{
			unicodestr= PyString_InternFromString(
				"__unicode__");
			if (unicodestr == NULL)
			{
				return NULL;
			}
		}
		func = PyObject_GetAttr(v, unicodestr);
		if (func != NULL) 
		{
		    res = PyEval_CallObject(func, (PyObject *)NULL);
			Py_DECREF(func);
		}
		else 
		{
			PyErr_Clear();
			if (v->ob_type->tp_str != NULL)
			{
				res = (*v->ob_type->tp_str)(v);
			}
			else
			{
				res = PyObject_Repr(v);
			}
		}
	}
	if (res == NULL)
	{
		return NULL;
	}
	if (!PyUnicode_Check(res)) 
	{
		PyObject *str;
		str = PyUnicode_FromEncodedObject(res, NULL, "strict");
		Py_DECREF(res);
		if (str)
		{
			res = str;
		}
		else
		{
			return NULL;
		}
	}
	return res;
}


#define RICHCOMPARE(t) (PyType_HasFeature((t), Py_TPFLAGS_HAVE_RICHCOMPARE) \
                         ? (t)->tp_richcompare : NULL)

static int swapped_op[] = {Py_GT, Py_GE, Py_EQ, Py_NE, Py_LT, Py_LE};

static PyObject *try_rich_compare(PyObject *v, PyObject *w, int op)
{
	richcmpfunc f;
	PyObject *res;

	if (v->ob_type != w->ob_type &&
	    PyType_IsSubtype(w->ob_type, v->ob_type) &&
	    (f = RICHCOMPARE(w->ob_type)) != NULL) 
	{
		res = (*f)(w, v, swapped_op[op]);
		if (res != Py_NotImplemented)
		{
			return res;
		}
		Py_DECREF(res);
	}
	if ((f = RICHCOMPARE(v->ob_type)) != NULL) 
	{
		res = (*f)(v, w, op);
		if (res != Py_NotImplemented)
		{
			return res;
		}
		Py_DECREF(res);
	}
	if ((f = RICHCOMPARE(w->ob_type)) != NULL) 
	{
		return (*f)(w, v, swapped_op[op]);
	}
	res = Py_NotImplemented;
	Py_INCREF(res);
	return res;
}

static int try_rich_compare_bool(PyObject *v, PyObject *w, int op)
{
	PyObject *res;
	int ok;

	if (RICHCOMPARE(v->ob_type) == NULL && RICHCOMPARE(w->ob_type) == NULL)
	{
		return 2;
	}
	res = try_rich_compare(v, w, op);
	if (res == NULL)
	{
		return -1;
	}
	if (res == Py_NotImplemented) 
	{
		Py_DECREF(res);
		return 2;
	}
	ok = PyObject_IsTrue(res);
	Py_DECREF(res);
	return ok;
}

static int try_rich_to_3way_compare(PyObject *v, PyObject *w)
{
	static struct { int op; int outcome; } tries[3] = {
		{Py_EQ, 0},
		{Py_LT, -1},
		{Py_GT, 1},
	};
	int i;

	if (RICHCOMPARE(v->ob_type) == NULL && RICHCOMPARE(w->ob_type) == NULL)
	{
		return 2;
	}

	for (i = 0; i < 3; i++) 
	{
		switch (try_rich_compare_bool(v, w, tries[i].op)) 
		{
		case -1:
			return -2;
		
		case 1:
			return tries[i].outcome;
		}
	}

	return 2;
}

static int try_3way_compare(PyObject *v, PyObject *w)
{
	int c;
	cmpfunc f;

	f = v->ob_type->tp_compare;
	if (PyInstance_Check(v))
	{
		return (*f)(v, w);
	}
	if (PyInstance_Check(w))
	{
		return (*w->ob_type->tp_compare)(v, w);
	}

	if (f != NULL && f == w->ob_type->tp_compare) 
	{
		c = (*f)(v, w);
		if (c < 0 && PyErr_Occurred())
		{
			return -1;
		}
		return c < 0 ? -1 : c > 0 ? 1 : 0;
	}

	if (f == _PyObject_SlotCompare ||
	    w->ob_type->tp_compare == _PyObject_SlotCompare)
	{
		return _PyObject_SlotCompare(v, w);
	}

	c = PyNumber_CoerceEx(&v, &w);
	if (c < 0)
	{
		return -2;
	}
	if (c > 0)
	{
		return 2;
	}

	if ((f = v->ob_type->tp_compare) != NULL) 
	{
		c = (*f)(v, w);
		Py_DECREF(v);
		Py_DECREF(w);
		if (c < 0 && PyErr_Occurred())
		{
			return -2;
		}
		return c < 0 ? -1 : c > 0 ? 1 : 0;
	}

	if ((f = w->ob_type->tp_compare) != NULL) 
	{
		c = (*f)(w, v);
		Py_DECREF(v);
		Py_DECREF(w);
		if (c < 0 && PyErr_Occurred())
		{
			return -2;
		}
		return c < 0 ? 1 : c > 0 ? -1 : 0;
	}

	Py_DECREF(v);
	Py_DECREF(w);
	return 2;
}

static int default_3way_compare(PyObject *v, PyObject *w)
{
	int c;
	char *vname, *wname;

	if (v->ob_type == w->ob_type) 
	{
		Py_uintptr_t vv = (Py_uintptr_t)v;
		Py_uintptr_t ww = (Py_uintptr_t)w;
		return (vv < ww) ? -1 : (vv > ww) ? 1 : 0;
	}

	if (PyUnicode_Check(v) || PyUnicode_Check(w)) 
	{
		c = PyUnicode_Compare(v, w);
		if (!PyErr_Occurred())
		{
			return c;
		}
		if (!PyErr_ExceptionMatches(PyExc_TypeError))
		{
			return -2;
		}
		PyErr_Clear();
	}

	if (v == Py_None)
	{
		return -1;
	}
	if (w == Py_None)
	{
		return 1;
	}

	if (v->ob_type->tp_as_number)
	{
		vname = "";
	}
	else
	{
		vname = v->ob_type->tp_name;
	}
	if (w->ob_type->tp_as_number)
	{
		wname = "";
	}
	else
	{
		wname = w->ob_type->tp_name;
	}
	c = strcmp(vname, wname);
	if (c < 0)
	{
		return -1;
	}
	if (c > 0)
	{
		return 1;
	}
	return ((Py_uintptr_t)(v->ob_type) < (
		Py_uintptr_t)(w->ob_type)) ? -1 : 1;
}

#define CHECK_TYPES(o) PyType_HasFeature((o)->ob_type, Py_TPFLAGS_CHECKTYPES)

static int do_cmp(PyObject *v, PyObject *w)
{
	int c;
	cmpfunc f;

	if (v->ob_type == w->ob_type
	    && (f = v->ob_type->tp_compare) != NULL) 
	{
		c = (*f)(v, w);
		if (c != 2 || !PyInstance_Check(v))
		{
			return c;
		}
	}
	c = try_rich_to_3way_compare(v, w);
	if (c < 2)
	{
		return c;
	}
	c = try_3way_compare(v, w);
	if (c < 2)
	{
		return c;
	}
	return default_3way_compare(v, w);
}

#define NESTING_LIMIT 20

static int compare_nesting = 0;

static PyObject *get_inprogress_dict()
{
	static PyObject *key;
	PyObject *tstate_dict, *inprogress;

	if (key == NULL) 
	{
		key = PyString_InternFromString("cmp_state");
		if (key == NULL)
		{
			return NULL;
		}
	}

	tstate_dict = PyThreadState_GetDict();
	if (tstate_dict == NULL) 
	{
		PyErr_BadInternalCall();
		return NULL;
	} 

	inprogress = PyDict_GetItem(tstate_dict, key); 
	if (inprogress == NULL) 
	{
		inprogress = PyDict_New();
		if (inprogress == NULL)
		{
			return NULL;
		}
		if (PyDict_SetItem(tstate_dict, key, inprogress) == -1) 
		{
		    Py_DECREF(inprogress);
		    return NULL;
		}
		Py_DECREF(inprogress);
	}

	return inprogress;
}

static PyObject *check_recursion(PyObject *v, PyObject *w, int op)
{
	PyObject *inprogress;
	PyObject *token;
	Py_uintptr_t iv = (Py_uintptr_t)v;
	Py_uintptr_t iw = (Py_uintptr_t)w;
	PyObject *x, *y, *z;

	inprogress = get_inprogress_dict();
	if (inprogress == NULL)
	{
		return NULL;
	}

	token = PyTuple_New(3);
	if (token == NULL)
	{
		return NULL;
	}

	if (iv <= iw) 
	{
		PyTuple_SET_ITEM(token, 0, x = PyLong_FromVoidPtr((void *)v));
		PyTuple_SET_ITEM(token, 1, y = PyLong_FromVoidPtr((void *)w));
		if (op >= 0)
		{
			op = swapped_op[op];
		}
	} 
	else 
	{
		PyTuple_SET_ITEM(token, 0, x = PyLong_FromVoidPtr((void *)w));
		PyTuple_SET_ITEM(token, 1, y = PyLong_FromVoidPtr((void *)v));
	}
	PyTuple_SET_ITEM(token, 2, z = PyInt_FromLong((long)op));
	if (x == NULL || y == NULL || z == NULL) 
	{
		Py_DECREF(token);
		return NULL;
	}

	if (PyDict_GetItem(inprogress, token) != NULL) 
	{
		Py_DECREF(token);
		return Py_None;
	}

	if (PyDict_SetItem(inprogress, token, token) < 0) 
	{
		Py_DECREF(token);
		return NULL;
	}

	return token;
}

static void delete_token(PyObject *token)
{
	PyObject *inprogress;

	if (token == NULL || token == Py_None)
	{
		return;
	}
	inprogress = get_inprogress_dict();
	if (inprogress == NULL)
	{
		PyErr_Clear();
	}
	else
	{
		PyDict_DelItem(inprogress, token);
	}
	Py_DECREF(token);
}

int PyObject_Compare(PyObject *v, PyObject *w)
{
	PyTypeObject *vtp;
	int result;

	if (PyOS_CheckStack()) 
	{
		PyErr_SetString(PyExc_MemoryError, "Stack overflow");
		return -1;
	}
	if (v == NULL || w == NULL) 
	{
		PyErr_BadInternalCall();
		return -1;
	}
	if (v == w)
	{
		return 0;
	}
	vtp = v->ob_type;
	compare_nesting++;
	if (compare_nesting > NESTING_LIMIT &&
		(vtp->tp_as_mapping
		 || (vtp->tp_as_sequence
		     && !PyString_Check(v)
		     && !PyTuple_Check(v)))) 
	{
		PyObject *token = check_recursion(v, w, -1);

		if (token == NULL) 
		{
			result = -1;
		}
		else if (token == Py_None) 
		{
			result = 0;
		}
		else 
		{
			result = do_cmp(v, w);
			delete_token(token);
		}
	}
	else 
	{
		result = do_cmp(v, w);
	}
	compare_nesting--;
	return result < 0 ? -1 : result;
}

static PyObject *convert_3way_to_object(int op, int c)
{
	PyObject *result;
	switch (op) 
	{
	case Py_LT: 
		c = c <  0; 
		break;
	
	case Py_LE: 
		c = c <= 0; 
		break;
	
	case Py_EQ: 
		c = c == 0; 
		break;
	
	case Py_NE: 
		c = c != 0; 
		break;
	
	case Py_GT: 
		c = c >  0; 
		break;
	
	case Py_GE: 
		c = c >= 0; 
		break;
	}
	result = c ? Py_True : Py_False;
	Py_INCREF(result);
	return result;
}

static PyObject *try_3way_to_rich_compare(PyObject *v, PyObject *w, int op)
{
	int c;

	c = try_3way_compare(v, w);
	if (c >= 2)
	{
		c = default_3way_compare(v, w);
	}
	if (c <= -2)
	{
		return NULL;
	}
	return convert_3way_to_object(op, c);
}

static PyObject *do_richcmp(PyObject *v, PyObject *w, int op)
{
	PyObject *res;

	res = try_rich_compare(v, w, op);
	if (res != Py_NotImplemented)
	{
		return res;
	}
	Py_DECREF(res);

	return try_3way_to_rich_compare(v, w, op);
}

PyObject *PyObject_RichCompare(PyObject *v, PyObject *w, int op)
{
	PyObject *res;

	assert(Py_LT <= op && op <= Py_GE);

	compare_nesting++;
	if (compare_nesting > NESTING_LIMIT &&
		(v->ob_type->tp_as_mapping
		 || (v->ob_type->tp_as_sequence
		     && !PyString_Check(v)
		     && !PyTuple_Check(v)))) 
	{

		PyObject *token = check_recursion(v, w, op);
		if (token == NULL) 
		{
			res = NULL;
			goto Done;
		}
		else if (token == Py_None) 
		{
			if (op == Py_EQ)
			{
				res = Py_True;
			}
			else if (op == Py_NE)
			{
				res = Py_False;
			}
			else 
			{
				PyErr_SetString(PyExc_ValueError,
					"can't order recursive values");
				res = NULL;
			}
			Py_XINCREF(res);
		}
		else 
		{
			res = do_richcmp(v, w, op);
			delete_token(token);
		}
		goto Done;
	}

	if (v->ob_type == w->ob_type && !PyInstance_Check(v)) 
	{
		cmpfunc fcmp;
		richcmpfunc frich = RICHCOMPARE(v->ob_type);
		if (frich != NULL) 
		{
			res = (*frich)(v, w, op);
			if (res != Py_NotImplemented)
			{
				goto Done;
			}
			Py_DECREF(res);
		}
		fcmp = v->ob_type->tp_compare;
		if (fcmp != NULL) 
		{
			int c = (*fcmp)(v, w);
			if (c < 0 && PyErr_Occurred()) 
			{
				res = NULL;
				goto Done;
			}
			res = convert_3way_to_object(op, c);
			goto Done;
		}
	}

	res = do_richcmp(v, w, op);
Done:
	compare_nesting--;
	return res;
}

int PyObject_RichCompareBool(PyObject *v, PyObject *w, int op)
{
	PyObject *res = PyObject_RichCompare(v, w, op);
	int ok;

	if (res == NULL)
	{
		return -1;
	}
	ok = PyObject_IsTrue(res);
	Py_DECREF(res);
	return ok;
}

long _Py_HashDouble(double v)
{
	double intpart, fractpart;
	int expo;
	long hipart;
	long x;	

	fractpart = modf(v, &intpart);
	if (fractpart == 0.0) 
	{
		if (intpart > LONG_MAX || -intpart > LONG_MAX) 
		{
			PyObject *plong;
			if (Py_IS_INFINITY(intpart))
			{
				v = v < 0 ? -271828.0 : 314159.0;
			}
			plong = PyLong_FromDouble(v);
			if (plong == NULL)
			{
				return -1;
			}
			x = PyObject_Hash(plong);
			Py_DECREF(plong);
			return x;
		}
		x = (long)intpart;
		if (x == -1)
		{
			x = -2;
		}
		return x;
	}
	v = frexp(v, &expo);
	v *= 2147483648.0;	
	hipart = (long)v;	
	v = (v - (double)hipart) * 2147483648.0;
	x = hipart + (long)v + (expo << 15);
	if (x == -1)
	{
		x = -2;
	}
	return x;
}

long _Py_HashPointer(void *p)
{
	return (long)p;
}

long PyObject_Hash(PyObject *v)
{
	PyTypeObject *tp = v->ob_type;
	if (tp->tp_hash != NULL)
	{
		return (*tp->tp_hash)(v);
	}
	if (tp->tp_compare == NULL && RICHCOMPARE(tp) == NULL) 
	{
		return _Py_HashPointer(v);
	}
	PyErr_SetString(PyExc_TypeError, "unhashable type");
	return -1;
}

PyObject *PyObject_GetAttrString(PyObject *v, char *name)
{
	PyObject *w, *res;

	if (v->ob_type->tp_getattr != NULL)
	{
		return (*v->ob_type->tp_getattr)(v, name);
	}
	w = PyString_InternFromString(name);
	if (w == NULL)
	{
		return NULL;
	}
	res = PyObject_GetAttr(v, w);
	Py_XDECREF(w);
	return res;
}

int PyObject_HasAttrString(PyObject *v, char *name)
{
	PyObject *res = PyObject_GetAttrString(v, name);
	if (res != NULL) 
	{
		Py_DECREF(res);
		return 1;
	}
	PyErr_Clear();
	return 0;
}

int PyObject_SetAttrString(PyObject *v, char *name, PyObject *w)
{
	PyObject *s;
	int res;

	if (v->ob_type->tp_setattr != NULL)
	{
		return (*v->ob_type->tp_setattr)(v, name, w);
	}
	s = PyString_InternFromString(name);
	if (s == NULL)
	{
		return -1;
	}
	res = PyObject_SetAttr(v, s, w);
	Py_XDECREF(s);
	return res;
}

PyObject *PyObject_GetAttr(PyObject *v, PyObject *name)
{
	PyTypeObject *tp = v->ob_type;

	if (PyUnicode_Check(name)) 
	{
		name = _PyUnicode_AsDefaultEncodedString(name, NULL);
		if (name == NULL)
		{
			return NULL;
		}
	}
	else
	if (!PyString_Check(name)) 
	{
		PyErr_SetString(PyExc_TypeError,
				"attribute name must be string");
		return NULL;
	}
	if (tp->tp_getattro != NULL)
	{
		return (*tp->tp_getattro)(v, name);
	}
	if (tp->tp_getattr != NULL)
	{
		return (*tp->tp_getattr)(v, PyString_AS_STRING(name));
	}
	PyErr_Format(PyExc_AttributeError,
		     "'%.50s' object has no attribute '%.400s'",
		     tp->tp_name, PyString_AS_STRING(name));
	return NULL;
}

int PyObject_HasAttr(PyObject *v, PyObject *name)
{
	PyObject *res = PyObject_GetAttr(v, name);
	if (res != NULL) 
	{
		Py_DECREF(res);
		return 1;
	}
	PyErr_Clear();
	return 0;
}

int PyObject_SetAttr(PyObject *v, PyObject *name, PyObject *value)
{
	PyTypeObject *tp = v->ob_type;
	int err;

	if (PyUnicode_Check(name)) 
	{
		name = PyUnicode_AsEncodedString(name, NULL, NULL);
		if (name == NULL)
		{
			return -1;
		}
	}
	else 
	if (!PyString_Check(name))
	{
		PyErr_SetString(PyExc_TypeError,
				"attribute name must be string");
		return -1;
	}
	else
	{
		Py_INCREF(name);
	}

	PyString_InternInPlace(&name);
	if (tp->tp_setattro != NULL) 
	{
		err = (*tp->tp_setattro)(v, name, value);
		Py_DECREF(name);
		return err;
	}
	if (tp->tp_setattr != NULL) 
	{
		err = (*tp->tp_setattr)(v, PyString_AS_STRING(name), value);
		Py_DECREF(name);
		return err;
	}
	Py_DECREF(name);
	if (tp->tp_getattr == NULL && tp->tp_getattro == NULL)
	{
		PyErr_Format(PyExc_TypeError,
			     "'%.100s' object has no attributes "
			     "(%s .%.100s)",
			     tp->tp_name,
			     value==NULL ? "del" : "assign to",
			     PyString_AS_STRING(name));
	}
	else
	{
		PyErr_Format(PyExc_TypeError,
			     "'%.100s' object has only read-only attributes "
			     "(%s .%.100s)",
			     tp->tp_name,
			     value==NULL ? "del" : "assign to",
			     PyString_AS_STRING(name));
	}
	return -1;
}

PyObject **_PyObject_GetDictPtr(PyObject *obj)
{
	long dictoffset;
	PyTypeObject *tp = obj->ob_type;

	if (!(tp->tp_flags & Py_TPFLAGS_HAVE_CLASS))
	{
		return NULL;
	}
	dictoffset = tp->tp_dictoffset;
	if (dictoffset == 0)
	{
		return NULL;
	}
	if (dictoffset < 0) 
	{
		int tsize;
		size_t size;

		tsize = ((PyVarObject *)obj)->ob_size;
		if (tsize < 0)
		{
			tsize = -tsize;
		}
		size = _PyObject_VAR_SIZE(tp, tsize);

		dictoffset += (long)size;
		assert(dictoffset > 0);
		assert(dictoffset % SIZEOF_VOID_P == 0);
	}
	return (PyObject **) ((char *)obj + dictoffset);
}

PyObject *PyObject_GenericGetAttr(PyObject *obj, PyObject *name)
{
	PyTypeObject *tp = obj->ob_type;
	PyObject *descr;
	PyObject *res = NULL;
	descrgetfunc f;
	PyObject **dictptr;

	if (PyUnicode_Check(name)) 
	{
		name = PyUnicode_AsEncodedString(name, NULL, NULL);
		if (name == NULL)
		{
			return NULL;
		}
	}
	else 
	if (!PyString_Check(name))
	{
		PyErr_SetString(PyExc_TypeError,
				"attribute name must be string");
		return NULL;
	}
	else
	{
		Py_INCREF(name);
	}

	if (tp->tp_dict == NULL) 
	{
		if (PyType_Ready(tp) < 0)
		{
			goto done;
		}
	}

	descr = _PyType_Lookup(tp, name);
	f = NULL;
	if (descr != NULL) 
	{
		f = descr->ob_type->tp_descr_get;
		if (f != NULL && PyDescr_IsData(descr)) 
		{
			res = f(descr, obj, (PyObject *)obj->ob_type);
			goto done;
		}
	}

	dictptr = _PyObject_GetDictPtr(obj);
	if (dictptr != NULL) 
	{
		PyObject *dict = *dictptr;
		if (dict != NULL) 
		{
			res = PyDict_GetItem(dict, name);
			if (res != NULL) 
			{
				Py_INCREF(res);
				goto done;
			}
		}
	}

	if (f != NULL) 
	{
		res = f(descr, obj, (PyObject *)obj->ob_type);
		goto done;
	}

	if (descr != NULL) 
	{
		Py_INCREF(descr);
		res = descr;
		goto done;
	}

	PyErr_Format(PyExc_AttributeError,
		     "'%.50s' object has no attribute '%.400s'",
		     tp->tp_name, PyString_AS_STRING(name));
done:
	Py_DECREF(name);
	return res;
}

int PyObject_GenericSetAttr(PyObject *obj, PyObject *name, PyObject *value)
{
	PyTypeObject *tp = obj->ob_type;
	PyObject *descr;
	descrsetfunc f;
	PyObject **dictptr;
	int res = -1;

	if (PyUnicode_Check(name)) 
	{
		name = PyUnicode_AsEncodedString(name, NULL, NULL);
		if (name == NULL)
		{
			return -1;
		}
	}
	else 
	if (!PyString_Check(name))
	{
		PyErr_SetString(PyExc_TypeError,
				"attribute name must be string");
		return -1;
	}
	else
	{
		Py_INCREF(name);
	}

	if (tp->tp_dict == NULL) 
	{
		if (PyType_Ready(tp) < 0)
		{
			goto done;
		}
	}

	descr = _PyType_Lookup(tp, name);
	f = NULL;
	if (descr != NULL) 
	{
		f = descr->ob_type->tp_descr_set;
		if (f != NULL && PyDescr_IsData(descr)) 
		{
			res = f(descr, obj, value);
			goto done;
		}
	}

	dictptr = _PyObject_GetDictPtr(obj);
	if (dictptr != NULL) 
	{
		PyObject *dict = *dictptr;
		if (dict == NULL && value != NULL) 
		{
			dict = PyDict_New();
			if (dict == NULL)
			{
				goto done;
			}
			*dictptr = dict;
		}
		if (dict != NULL) 
		{
			if (value == NULL)
			{
				res = PyDict_DelItem(dict, name);
			}
			else
			{
				res = PyDict_SetItem(dict, name, value);
			}
			if (res < 0 && PyErr_ExceptionMatches(PyExc_KeyError))
			{
				PyErr_SetObject(PyExc_AttributeError, name);
			}
			goto done;
		}
	}

	if (f != NULL) 
	{
		res = f(descr, obj, value);
		goto done;
	}

	if (descr == NULL) 
	{
		PyErr_Format(PyExc_AttributeError,
			     "'%.50s' object has no attribute '%.400s'",
			     tp->tp_name, PyString_AS_STRING(name));
		goto done;
	}

	PyErr_Format(PyExc_AttributeError,
		     "'%.50s' object attribute '%.400s' is read-only",
		     tp->tp_name, PyString_AS_STRING(name));
done:
	Py_DECREF(name);
	return res;
}

int PyObject_IsTrue(PyObject *v)
{
	int res;
	if (v == Py_None)
	{
		res = 0;
	}
	else if (v->ob_type->tp_as_number != NULL &&
		 v->ob_type->tp_as_number->nb_nonzero != NULL)
	{
		res = (*v->ob_type->tp_as_number->nb_nonzero)(v);
	}
	else if (v->ob_type->tp_as_mapping != NULL &&
		 v->ob_type->tp_as_mapping->mp_length != NULL)
	{
		res = (*v->ob_type->tp_as_mapping->mp_length)(v);
	}
	else if (v->ob_type->tp_as_sequence != NULL &&
		 v->ob_type->tp_as_sequence->sq_length != NULL)
	{
		res = (*v->ob_type->tp_as_sequence->sq_length)(v);
	}
	else
	{
		res = 1;
	}
	if (res > 0)
	{
		res = 1;
	}
	return res;
}

int PyObject_Not(PyObject *v)
{
	int res;
	res = PyObject_IsTrue(v);
	if (res < 0)
	{
		return res;
	}
	return res == 0;
}

int PyNumber_CoerceEx(PyObject **pv, PyObject **pw)
{
	PyObject *v = *pv;
	PyObject *w = *pw;
	int res;

	if (v->ob_type == w->ob_type && !PyInstance_Check(v)) 
	{
		Py_INCREF(v);
		Py_INCREF(w);
		return 0;
	}
	if (v->ob_type->tp_as_number && v->ob_type->tp_as_number->nb_coerce) 
	{
		res = (*v->ob_type->tp_as_number->nb_coerce)(pv, pw);
		if (res <= 0)
		{
			return res;
		}
	}
	if (w->ob_type->tp_as_number && w->ob_type->tp_as_number->nb_coerce) 
	{
		res = (*w->ob_type->tp_as_number->nb_coerce)(pw, pv);
		if (res <= 0)
		{
			return res;
		}
	}
	return 1;
}

int PyNumber_Coerce(PyObject **pv, PyObject **pw)
{
	int err = PyNumber_CoerceEx(pv, pw);
	if (err <= 0)
	{
		return err;
	}
	PyErr_SetString(PyExc_TypeError, "number coercion failed");
	return -1;
}

int PyCallable_Check(PyObject *x)
{
	if (x == NULL)
	{
		return 0;
	}
	if (PyInstance_Check(x)) 
	{
		PyObject *call = PyObject_GetAttrString(x, "__call__");
		if (call == NULL) 
		{
			PyErr_Clear();
			return 0;
		}
		Py_DECREF(call);
		return 1;
	}
	else 
	{
		return x->ob_type->tp_call != NULL;
	}
}

static int merge_class_dict(PyObject* dict, PyObject* aclass)
{
	PyObject *classdict;
	PyObject *bases;

	assert(PyDict_Check(dict));
	assert(aclass);

	classdict = PyObject_GetAttrString(aclass, "__dict__");
	if (classdict == NULL)
	{
		PyErr_Clear();
	}
	else 
	{
		int status = PyDict_Update(dict, classdict);
		Py_DECREF(classdict);
		if (status < 0)
		{
			return -1;
		}
	}

	bases = PyObject_GetAttrString(aclass, "__bases__");
	if (bases == NULL)
	{
		PyErr_Clear();
	}
	else 
	{
		int i, n;
		n = PySequence_Size(bases);
		if (n < 0)
		{
			PyErr_Clear();
		}
		else 
		{
			for (i = 0; i < n; i++) 
			{
				PyObject *base = PySequence_GetItem(bases, i);
				if (base == NULL) 
				{
					Py_DECREF(bases);
					return -1;
				}
				if (merge_class_dict(dict, base) < 0) 
				{
					Py_DECREF(bases);
					return -1;
				}
			}
		}
		Py_DECREF(bases);
	}
	return 0;
}

static int merge_list_attr(PyObject* dict, PyObject* obj, char *attrname)
{
	PyObject *list;
	int result = 0;

	assert(PyDict_Check(dict));
	assert(obj);
	assert(attrname);

	list = PyObject_GetAttrString(obj, attrname);
	if (list == NULL)
	{
		PyErr_Clear();
	}
	else if (PyList_Check(list)) 
	{
		int i;
		for (i = 0; i < PyList_GET_SIZE(list); ++i) 
		{
			PyObject *item = PyList_GET_ITEM(list, i);
			if (PyString_Check(item)) 
			{
				result = PyDict_SetItem(dict, item, Py_None);
				if (result < 0)
				{
					break;
				}
			}
		}
	}

	Py_XDECREF(list);
	return result;
}

PyObject *PyObject_Dir(PyObject *arg)
{
	PyObject *result = NULL;	
	PyObject *masterdict = NULL;

	if (arg == NULL) 
	{
		PyObject *locals = PyEval_GetLocals();
		if (locals == NULL)
		{
			goto error;
		}
		result = PyDict_Keys(locals);
		if (result == NULL)
		{
			goto error;
		}
	}
	else if (PyModule_Check(arg)) 
	{
		masterdict = PyObject_GetAttrString(arg, "__dict__");
		if (masterdict == NULL)
		{
			goto error;
		}
		if (!PyDict_Check(masterdict)) 
		{
			PyErr_SetString(PyExc_TypeError,
					"module.__dict__ is not a dictionary");
			goto error;
		}
	}
	else if (PyType_Check(arg) || PyClass_Check(arg)) 
	{
		masterdict = PyDict_New();
		if (masterdict == NULL)
		{
			goto error;
		}
		if (merge_class_dict(masterdict, arg) < 0)
		{
			goto error;
		}
	}
	else 
	{
		PyObject *itsclass;
		masterdict = PyObject_GetAttrString(arg, "__dict__");
		if (masterdict == NULL) 
		{
			PyErr_Clear();
			masterdict = PyDict_New();
		}
		else if (!PyDict_Check(masterdict)) 
		{
			Py_DECREF(masterdict);
			masterdict = PyDict_New();
		}
		else 
		{
			PyObject *temp = PyDict_Copy(masterdict);
			Py_DECREF(masterdict);
			masterdict = temp;
		}
		if (masterdict == NULL)
		{
			goto error;
		}


		if (merge_list_attr(masterdict, arg, "__members__") < 0)
		{
			goto error;
		}
		if (merge_list_attr(masterdict, arg, "__methods__") < 0)
		{
			goto error;
		}

		itsclass = PyObject_GetAttrString(arg, "__class__");
		if (itsclass == NULL)
		{
			PyErr_Clear();
		}
		else 
		{
			int status = merge_class_dict(masterdict, itsclass);
			Py_DECREF(itsclass);
			if (status < 0)
			{
				goto error;
			}
		}
	}

	assert((result == NULL) ^ (masterdict == NULL));
	if (masterdict != NULL) 
	{
		assert(result == NULL);
		result = PyDict_Keys(masterdict);
		if (result == NULL)
		{
			goto error;
		}
	}

	assert(result);
	if (PyList_Sort(result) != 0)
	{
		goto error;
	}
	else
	{
		goto normal_return;
	}

error:
	Py_XDECREF(result);
	result = NULL;
normal_return:
  	Py_XDECREF(masterdict);
	return result;
}


static PyObject *none_repr(PyObject *op)
{
	return PyString_FromString("None");
}

static void none_dealloc(PyObject* ignore) 
{
	abort();
}


static PyTypeObject PyNone_Type = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,
	"NoneType",
	0,
	0,
	(destructor)none_dealloc,	
	0,		
	0,		
	0,		
	0,		
	(reprfunc)none_repr, 
	0,		
	0,		
	0,		
	0,		
};

PyObject _Py_NoneStruct = {
	PyObject_HEAD_INIT(&PyNone_Type)
};

static PyObject *NotImplemented_repr(PyObject *op)
{
	return PyString_FromString("NotImplemented");
}

static PyTypeObject PyNotImplemented_Type = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,
	"NotImplementedType",
	0,
	0,
	(destructor)none_dealloc,	    
	0,	
	0,		
	0,		
	0,		
	(reprfunc)NotImplemented_repr, 
	0,		
	0,		
	0,		
	0,		
};

PyObject _Py_NotImplementedStruct = {
	PyObject_HEAD_INIT(&PyNotImplemented_Type)
};

void _Py_ReadyTypes()
{
	if (PyType_Ready(&PyType_Type) < 0)
	{
		Py_FatalError("Can't initialize 'type'");
	}

	if (PyType_Ready(&PyList_Type) < 0)
	{
		Py_FatalError("Can't initialize 'list'");
	}

	if (PyType_Ready(&PyNone_Type) < 0)
	{
		Py_FatalError("Can't initialize type(None)");
	}

	if (PyType_Ready(&PyNotImplemented_Type) < 0)
	{
		Py_FatalError("Can't initialize type(NotImplemented)");
	}
}


#ifdef _DEBUG

static PyObject refchain = {&refchain, &refchain};

void _Py_ResetReferences()
{
	refchain._ob_prev = refchain._ob_next = &refchain;
	_Py_RefTotal = 0;
}

void _Py_NewReference(PyObject *op)
{
	_Py_RefTotal++;
	op->ob_refcnt = 1;
	op->_ob_next = refchain._ob_next;
	op->_ob_prev = &refchain;
	refchain._ob_next->_ob_prev = op;
	refchain._ob_next = op;
}

void _Py_ForgetReference(PyObject *op)
{
	if (op->ob_refcnt < 0)
	{
		Py_FatalError("UNREF negative refcnt");
	}
	if (op == &refchain ||
	    op->_ob_prev->_ob_next != op || op->_ob_next->_ob_prev != op)
	{
		Py_FatalError("UNREF invalid object");
	}
	op->_ob_next->_ob_prev = op->_ob_prev;
	op->_ob_prev->_ob_next = op->_ob_next;
	op->_ob_next = op->_ob_prev = NULL;
}

void _Py_Dealloc(PyObject *op)
{
	destructor dealloc = op->ob_type->tp_dealloc;
	_Py_ForgetReference(op);
	(*dealloc)(op);
}

void _Py_PrintReferences(FILE *fp)
{
	PyObject *op;
	fprintf(fp, "Remaining objects:\n");
	for (op = refchain._ob_next; op != &refchain; op = op->_ob_next) 
	{
		fprintf(fp, "[%d] ", op->ob_refcnt);
		if (PyObject_Print(op, fp, 0) != 0)
		{
			PyErr_Clear();
		}
		putc('\n', fp);
	}
}

PyObject *_Py_GetObjects(PyObject *self, PyObject *args)
{
	int i, n;
	PyObject *t = NULL;
	PyObject *res, *op;

	if (!PyArg_ParseTuple(args, "i|O", &n, &t))
	{
		return NULL;
	}
	op = refchain._ob_next;
	res = PyList_New(0);
	if (res == NULL)
	{
		return NULL;
	}
	for (i = 0; (n == 0 || i < n) && op != &refchain; i++) 
	{
		while (op == self || op == args || op == res || op == t ||
		       (t != NULL && op->ob_type != (PyTypeObject *) t)) 
		{
			op = op->_ob_next;
			if (op == &refchain)
			{
				return res;
			}
		}
		if (PyList_Append(res, op) < 0) 
		{
			Py_DECREF(res);
			return NULL;
		}
		op = op->_ob_next;
	}
	return res;
}

#endif


PyTypeObject *_Py_cobject_hack = &PyCObject_Type;


int (*_Py_abstract_hack)(PyObject *) = PyObject_Size;


void *PyMem_Malloc(size_t nbytes)
{
	return PyMem_MALLOC(nbytes);
}

void *PyMem_Realloc(void *p, size_t nbytes)
{
	return PyMem_REALLOC(p, nbytes ? nbytes : 1);
}

void PyMem_Free(void *p)
{
	PyMem_FREE(p);
}

void *PyObject_Malloc(size_t nbytes)
{
	return PyObject_MALLOC(nbytes);
}

void *PyObject_Realloc(void *p, size_t nbytes)
{
	return PyObject_REALLOC(p, nbytes);
}

void PyObject_Free(void *p)
{
	PyObject_FREE(p);
}

#define KEY "Py_Repr"

int Py_ReprEnter(PyObject *obj)
{
	PyObject *dict;
	PyObject *list;
	int i;

	dict = PyThreadState_GetDict();
	if (dict == NULL)
	{
		return -1;
	}
	list = PyDict_GetItemString(dict, KEY);
	if (list == NULL) 
	{
		list = PyList_New(0);
		if (list == NULL)
		{
			return -1;
		}
		if (PyDict_SetItemString(dict, KEY, list) < 0)
		{
			return -1;
		}
		Py_DECREF(list);
	}
	i = PyList_GET_SIZE(list);
	while (--i >= 0) 
	{
		if (PyList_GET_ITEM(list, i) == obj)
		{
			return 1;
		}
	}
	PyList_Append(list, obj);
	return 0;
}

void Py_ReprLeave(PyObject *obj)
{
	PyObject *dict;
	PyObject *list;
	int i;

	dict = PyThreadState_GetDict();
	if (dict == NULL)
	{
		return;
	}
	list = PyDict_GetItemString(dict, KEY);
	if (list == NULL || !PyList_Check(list))
	{
		return;
	}
	i = PyList_GET_SIZE(list);
	while (--i >= 0) 
	{
		if (PyList_GET_ITEM(list, i) == obj) 
		{
			PyList_SetSlice(list, i, i + 1, NULL);
			break;
		}
	}
}

#define Py_TRASHCAN_TUPLE       1
#define Py_TRASHCAN_LIST        2
#define Py_TRASHCAN_DICT        3
#define Py_TRASHCAN_FRAME       4
#define Py_TRASHCAN_TRACEBACK   5

int _PyTrash_delete_nesting = 0;

PyObject * _PyTrash_delete_later = NULL;

void _PyTrash_deposit_object(PyObject *op)
{
	int typecode;

	if (PyTuple_Check(op))
	{
		typecode = Py_TRASHCAN_TUPLE;
	}
	else if (PyList_Check(op))
	{
		typecode = Py_TRASHCAN_LIST;
	}
	else if (PyDict_Check(op))
	{
		typecode = Py_TRASHCAN_DICT;
	}
	else if (PyFrame_Check(op))
	{
		typecode = Py_TRASHCAN_FRAME;
	}
	else if (PyTraceBack_Check(op))
	{
		typecode = Py_TRASHCAN_TRACEBACK;
	}
	else 
	{
		Py_FatalError("Type not supported in GC -- internal bug");
		return;
	}
	op->ob_refcnt = typecode;

	op->ob_type = (PyTypeObject*)_PyTrash_delete_later;
	_PyTrash_delete_later = op;
}

void _PyTrash_destroy_chain()
{
	while (_PyTrash_delete_later) 
	{
		PyObject *shredder = _PyTrash_delete_later;
		_PyTrash_delete_later = (PyObject*) shredder->ob_type;

		switch (shredder->ob_refcnt) 
		{
		case Py_TRASHCAN_TUPLE:
			shredder->ob_type = &PyTuple_Type;
			break;

		case Py_TRASHCAN_LIST:
			shredder->ob_type = &PyList_Type;
			break;

		case Py_TRASHCAN_DICT:
			shredder->ob_type = &PyDict_Type;
			break;

		case Py_TRASHCAN_FRAME:
			shredder->ob_type = &PyFrame_Type;
			break;

		case Py_TRASHCAN_TRACEBACK:
			shredder->ob_type = &PyTraceBack_Type;
			break;
		}
		_Py_NewReference(shredder);

		++_PyTrash_delete_nesting;
		Py_DECREF(shredder);
		--_PyTrash_delete_nesting;
	}
}

