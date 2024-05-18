//20170427
#include "python.h"

#define MAXSAVESIZE	20 

#define MAXSAVEDTUPLES  2000 

static PyTupleObject *free_tuples[MAXSAVESIZE];
static int num_free_tuples[MAXSAVESIZE];

PyObject *PyTuple_New(int size)
{
	int i;
	PyTupleObject *op;
	if (size < 0) 
	{
		PyErr_BadInternalCall();
		return NULL;
	}
	if (size == 0 && free_tuples[0]) 
	{
		op = free_tuples[0];
		Py_INCREF(op);
		return (PyObject *) op;
	}
	
	if (0 < size && size < MAXSAVESIZE &&
	    (op = free_tuples[size]) != NULL)
	{
		free_tuples[size] = (PyTupleObject *) op->ob_item[0];
		num_free_tuples[size]--;
#ifdef _DEBUG
		op->ob_size = size;
		op->ob_type = &PyTuple_Type;
#endif
		_Py_NewReference((PyObject *)op);
	}
	else
	{
		int nbytes = size * sizeof(PyObject *);
		if (nbytes / sizeof(PyObject *) != (size_t)size ||
		    (nbytes += sizeof(PyTupleObject) - sizeof(PyObject *))
		    <= 0)
		{
			return PyErr_NoMemory();
		}
		op = PyObject_GC_NewVar(PyTupleObject, &PyTuple_Type, size);
		if (op == NULL)
		{
			return NULL;
		}
	}
	for (i = 0; i < size; i++)
	{
		op->ob_item[i] = NULL;
	}
	if (size == 0) 
	{
		free_tuples[0] = op;
		++num_free_tuples[0];
		Py_INCREF(op);
	}
	_PyObject_GC_TRACK(op);
	return (PyObject *) op;
}

int PyTuple_Size(PyObject *op)
{
	if (!PyTuple_Check(op)) 
	{
		PyErr_BadInternalCall();
		return -1;
	}
	else
	{
		return ((PyTupleObject *)op)->ob_size;
	}
}

PyObject *PyTuple_GetItem(PyObject *op, int i)
{
	if (!PyTuple_Check(op)) 
	{
		PyErr_BadInternalCall();
		return NULL;
	}
	if (i < 0 || i >= ((PyTupleObject *)op) -> ob_size) 
	{
		PyErr_SetString(PyExc_IndexError, "tuple index out of range");
		return NULL;
	}
	return ((PyTupleObject *)op) -> ob_item[i];
}

int PyTuple_SetItem(PyObject *op, int i, PyObject *newitem)
{
	PyObject *olditem;
	PyObject **p;
	if (!PyTuple_Check(op) || op->ob_refcnt != 1) 
	{
		Py_XDECREF(newitem);
		PyErr_BadInternalCall();
		return -1;
	}
	if (i < 0 || i >= ((PyTupleObject *)op) -> ob_size) 
	{
		Py_XDECREF(newitem);
		PyErr_SetString(PyExc_IndexError,
				"tuple assignment index out of range");
		return -1;
	}
	p = ((PyTupleObject *)op) -> ob_item + i;
	olditem = *p;
	*p = newitem;
	Py_XDECREF(olditem);
	return 0;
}

static void tupledealloc(PyTupleObject *op)
{
	int i;
	int len =  op->ob_size;
	PyObject_GC_UnTrack(op);
	Py_TRASHCAN_SAFE_BEGIN(op)
	if (len > 0) 
	{
		i = len;
		while (--i >= 0)
		{
			Py_XDECREF(op->ob_item[i]);
		}
		if (len < MAXSAVESIZE &&
		    num_free_tuples[len] < MAXSAVEDTUPLES &&
		    op->ob_type == &PyTuple_Type)
		{
			op->ob_item[0] = (PyObject *) free_tuples[len];
			num_free_tuples[len]++;
			free_tuples[len] = op;
			goto done;
		}
	}
	op->ob_type->tp_free((PyObject *)op);
done:
	Py_TRASHCAN_SAFE_END(op)
}

static int tupleprint(PyTupleObject *op, FILE *fp, int flags)
{
	int i;
	fprintf(fp, "(");
	for (i = 0; i < op->ob_size; i++) 
	{
		if (i > 0)
		{
			fprintf(fp, ", ");
		}
		if (PyObject_Print(op->ob_item[i], fp, 0) != 0)
		{
			return -1;
		}
	}
	if (op->ob_size == 1)
	{
		fprintf(fp, ",");
	}
	fprintf(fp, ")");
	return 0;
}

static PyObject *tuplerepr(PyTupleObject *v)
{
	int i, n;
	PyObject *s, *temp;
	PyObject *pieces, *result = NULL;

	n = v->ob_size;
	if (n == 0)
	{
		return PyString_FromString("()");
	}

	pieces = PyTuple_New(n);
	if (pieces == NULL)
	{
		return NULL;
	}

	for (i = 0; i < n; ++i) 
	{
		s = PyObject_Repr(v->ob_item[i]);
		if (s == NULL)
		{
			goto Done;
		}
		PyTuple_SET_ITEM(pieces, i, s);
	}

	assert(n > 0);
	s = PyString_FromString("(");
	if (s == NULL)
	{
		goto Done;
	}
	temp = PyTuple_GET_ITEM(pieces, 0);
	PyString_ConcatAndDel(&s, temp);
	PyTuple_SET_ITEM(pieces, 0, s);
	if (s == NULL)
	{
		goto Done;
	}

	s = PyString_FromString(n == 1 ? ",)" : ")");
	if (s == NULL)
	{
		goto Done;
	}
	temp = PyTuple_GET_ITEM(pieces, n-1);
	PyString_ConcatAndDel(&temp, s);
	PyTuple_SET_ITEM(pieces, n-1, temp);
	if (temp == NULL)
	{
		goto Done;
	}

	s = PyString_FromString(", ");
	if (s == NULL)
	{
		goto Done;
	}
	result = _PyString_Join(s, pieces);
	Py_DECREF(s);	

Done:
	Py_DECREF(pieces);
	return result;
}

static long tuplehash(PyTupleObject *v)
{
	long x, y;
	int len = v->ob_size;
	PyObject **p;
	x = 0x345678L;
	p = v->ob_item;
	while (--len >= 0) 
	{
		y = PyObject_Hash(*p++);
		if (y == -1)
		{
			return -1;
		}
		x = (1000003*x) ^ y;
	}
	x ^= v->ob_size;
	if (x == -1)
	{
		x = -2;
	}
	return x;
}

static int tuplelength(PyTupleObject *a)
{
	return a->ob_size;
}

static int tuplecontains(PyTupleObject *a, PyObject *el)
{
	int i, cmp;

	for (i = 0; i < a->ob_size; ++i) 
	{
		cmp = PyObject_RichCompareBool(el, PyTuple_GET_ITEM(a, i),
					       Py_EQ);
		if (cmp > 0)
		{
			return 1;
		}
		else if (cmp < 0)
		{
			return -1;
		}
	}
	return 0;
}

static PyObject *tupleitem(PyTupleObject *a, int i)
{
	if (i < 0 || i >= a->ob_size) 
	{
		PyErr_SetString(PyExc_IndexError, "tuple index out of range");
		return NULL;
	}
	Py_INCREF(a->ob_item[i]);
	return a->ob_item[i];
}

static PyObject *tupleslice(PyTupleObject *a, int ilow, int ihigh)
{
	PyTupleObject *np;
	int i;
	if (ilow < 0)
	{
		ilow = 0;
	}
	if (ihigh > a->ob_size)
	{
		ihigh = a->ob_size;
	}
	if (ihigh < ilow)
	{
		ihigh = ilow;
	}
	if (ilow == 0 && ihigh == a->ob_size && PyTuple_CheckExact(a)) 
	{
		Py_INCREF(a);
		return (PyObject *)a;
	}
	np = (PyTupleObject *)PyTuple_New(ihigh - ilow);
	if (np == NULL)
	{
		return NULL;
	}
	for (i = ilow; i < ihigh; i++) 
	{
		PyObject *v = a->ob_item[i];
		Py_INCREF(v);
		np->ob_item[i - ilow] = v;
	}
	return (PyObject *)np;
}

PyObject *PyTuple_GetSlice(PyObject *op, int i, int j)
{
	if (op == NULL || !PyTuple_Check(op)) 
	{
		PyErr_BadInternalCall();
		return NULL;
	}
	return tupleslice((PyTupleObject *)op, i, j);
}

static PyObject *tupleconcat(PyTupleObject *a, PyObject *bb)
{
	int size;
	int i;
	PyTupleObject *np;
	if (!PyTuple_Check(bb)) 
	{
		PyErr_Format(PyExc_TypeError,
       		     "can only concatenate tuple (not \"%.200s\") to tuple",
			     bb->ob_type->tp_name);
		return NULL;
	}
#define b ((PyTupleObject *)bb)
	size = a->ob_size + b->ob_size;
	if (size < 0)
	{
		return PyErr_NoMemory();
	}
	np = (PyTupleObject *) PyTuple_New(size);
	if (np == NULL) 
	{
		return NULL;
	}
	for (i = 0; i < a->ob_size; i++) 
	{
		PyObject *v = a->ob_item[i];
		Py_INCREF(v);
		np->ob_item[i] = v;
	}
	for (i = 0; i < b->ob_size; i++) 
	{
		PyObject *v = b->ob_item[i];
		Py_INCREF(v);
		np->ob_item[i + a->ob_size] = v;
	}
	return (PyObject *)np;
#undef b
}

static PyObject *tuplerepeat(PyTupleObject *a, int n)
{
	int i, j;
	int size;
	PyTupleObject *np;
	PyObject **p;
	if (n < 0)
	{
		n = 0;
	}
	if (a->ob_size == 0 || n == 1) 
	{
		if (PyTuple_CheckExact(a)) 
		{
			Py_INCREF(a);
			return (PyObject *)a;
		}
		if (a->ob_size == 0)
		{
			return PyTuple_New(0);
		}
	}
	size = a->ob_size * n;
	if (size/a->ob_size != n)
	{
		return PyErr_NoMemory();
	}
	np = (PyTupleObject *) PyTuple_New(size);
	if (np == NULL)
	{
		return NULL;
	}
	p = np->ob_item;
	for (i = 0; i < n; i++) 
	{
		for (j = 0; j < a->ob_size; j++) 
		{
			*p = a->ob_item[j];
			Py_INCREF(*p);
			p++;
		}
	}
	return (PyObject *) np;
}

static int tupletraverse(PyTupleObject *o, visitproc visit, void *arg)
{
	int i, err;
	PyObject *x;

	for (i = o->ob_size; --i >= 0; ) 
	{
		x = o->ob_item[i];
		if (x != NULL) 
		{
			err = visit(x, arg);
			if (err)
			{
				return err;
			}
		}
	}
	return 0;
}

static PyObject *tuplerichcompare(PyObject *v, PyObject *w, int op)
{
	PyTupleObject *vt, *wt;
	int i;
	int vlen, wlen;

	if (!PyTuple_Check(v) || !PyTuple_Check(w)) 
	{
		Py_INCREF(Py_NotImplemented);
		return Py_NotImplemented;
	}

	vt = (PyTupleObject *)v;
	wt = (PyTupleObject *)w;

	vlen = vt->ob_size;
	wlen = wt->ob_size;

	for (i = 0; i < vlen && i < wlen; i++) 
	{
		int k = PyObject_RichCompareBool(vt->ob_item[i],
						 wt->ob_item[i], Py_EQ);
		if (k < 0)
		{
			return NULL;
		}
		if (!k)
		{
			break;
		}
	}

	if (i >= vlen || i >= wlen) 
	{
		int cmp;
		PyObject *res;
		switch (op) 
		{
		case Py_LT: 
			cmp = vlen <  wlen; 
			break;
		
		case Py_LE: 
			cmp = vlen <= wlen; 
			break;
		
		case Py_EQ: 
			cmp = vlen == wlen; 
			break;
		
		case Py_NE: 
			cmp = vlen != wlen; 
			break;
		
		case Py_GT: 
			cmp = vlen >  wlen; 
			break;
		
		case Py_GE: 
			cmp = vlen >= wlen; 
			break;
		
		default: 
			return NULL; 
		}
		if (cmp)
		{
			res = Py_True;
		}
		else
		{
			res = Py_False;
		}
		Py_INCREF(res);
		return res;
	}

	if (op == Py_EQ) 
	{
		Py_INCREF(Py_False);
		return Py_False;
	}
	if (op == Py_NE) 
	{
		Py_INCREF(Py_True);
		return Py_True;
	}

	return PyObject_RichCompare(vt->ob_item[i], wt->ob_item[i], op);
}

staticforward PyObject *tuple_subtype_new(PyTypeObject *type, PyObject *args, PyObject *kwds);

static PyObject *tuple_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	PyObject *arg = NULL;
	static char *kwlist[] = {"sequence", 0};

	if (type != &PyTuple_Type)
	{
		return tuple_subtype_new(type, args, kwds);
	}
	if (!PyArg_ParseTupleAndKeywords(args, kwds, "|O:tuple", kwlist, &arg))
	{
		return NULL;
	}

	if (arg == NULL)
	{
		return PyTuple_New(0);
	}
	else
	{
		return PySequence_Tuple(arg);
	}
}

static PyObject *tuple_subtype_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	PyObject *tmp, *new, *item;
	int i, n;

	assert(PyType_IsSubtype(type, &PyTuple_Type));
	tmp = tuple_new(&PyTuple_Type, args, kwds);
	if (tmp == NULL)
	{
		return NULL;
	}
	assert(PyTuple_Check(tmp));
	new = type->tp_alloc(type, n = PyTuple_GET_SIZE(tmp));
	if (new == NULL)
	{
		return NULL;
	}
	for (i = 0; i < n; i++) 
	{
		item = PyTuple_GET_ITEM(tmp, i);
		Py_INCREF(item);
		PyTuple_SET_ITEM(new, i, item);
	}
	Py_DECREF(tmp);
	return new;
}

static char tuple_doc[] =
	"tuple() -> an empty tuple\n"
	"tuple(sequence) -> tuple initialized from sequence's items\n"
	"\n"
	"If the argument is a tuple, the return value is the same object.";

static PySequenceMethods tuple_as_sequence = {
	(inquiry)tuplelength,	
	(binaryfunc)tupleconcat,
	(intargfunc)tuplerepeat,
	(intargfunc)tupleitem,	
	(intintargfunc)tupleslice,
	0,					
	0,					
	(objobjproc)tuplecontains,
};

PyTypeObject PyTuple_Type = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,
	"tuple",
	sizeof(PyTupleObject) - sizeof(PyObject *),
	sizeof(PyObject *),
	(destructor)tupledealloc,
	(printfunc)tupleprint,	
	0,					
	0,					
	0,					
	(reprfunc)tuplerepr,	
	0,					
	&tuple_as_sequence,		
	0,					
	(hashfunc)tuplehash,	
	0,					
	0,					
	PyObject_GenericGetAttr,
	0,					
	0,					
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC |
		Py_TPFLAGS_BASETYPE,
	tuple_doc,			
 	(traverseproc)tupletraverse,
	0,					
	tuplerichcompare,	
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
	tuple_new,			
	_PyObject_GC_Del,		
};

int _PyTuple_Resize(PyObject **pv, int newsize)
{
	PyTupleObject *v;
	PyTupleObject *sv;
	int i;
	int oldsize;

	v = (PyTupleObject *) *pv;
	if (v == NULL || v->ob_type != &PyTuple_Type ||
	    (v->ob_size != 0 && v->ob_refcnt != 1)) 
	{
		*pv = 0;
		Py_XDECREF(v);
		PyErr_BadInternalCall();
		return -1;
	}
	oldsize = v->ob_size;
	if (oldsize == newsize)
	{
		return 0;
	}

	if (oldsize == 0) 
	{
		Py_DECREF(v);
		*pv = PyTuple_New(newsize);
		return *pv == NULL ? -1 : 0;
	}

#ifdef _DEBUG
	--_Py_RefTotal;
#endif
	_PyObject_GC_UNTRACK(v);
	_Py_ForgetReference((PyObject *) v);
	for (i = newsize; i < oldsize; i++) 
	{
		Py_XDECREF(v->ob_item[i]);
		v->ob_item[i] = NULL;
	}
	sv = PyObject_GC_Resize(PyTupleObject, v, newsize);
	if (sv == NULL) 
	{
		*pv = NULL;
		PyObject_GC_Del(v);
		return -1;
	}
	_Py_NewReference((PyObject *) sv);
	for (i = oldsize; i < newsize; i++)
	{
		sv->ob_item[i] = NULL;
	}
	*pv = (PyObject *) sv;
	_PyObject_GC_TRACK(sv);
	return 0;
}

void PyTuple_Fini()
{
	int i;

	Py_XDECREF(free_tuples[0]);
	free_tuples[0] = NULL;

	for (i = 1; i < MAXSAVESIZE; i++) 
	{
		PyTupleObject *p, *q;
		p = free_tuples[i];
		free_tuples[i] = NULL;
		while (p) 
		{
			q = p;
			p = (PyTupleObject *)(p->ob_item[0]);
			PyObject_GC_Del(q);
		}
	}
}
