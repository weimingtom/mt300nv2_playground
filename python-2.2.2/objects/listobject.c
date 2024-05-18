//20170418
#include "python.h"

#include <stddef.h>

static int roundupsize(int n)
{
	unsigned int nbits = 0;
	unsigned int n2 = (unsigned int)n >> 5;

	do 
	{
		n2 >>= 3;
		nbits += 3;
	} while (n2);
	return ((n >> nbits) + 1) << nbits;
}

#define NRESIZE(var, type, nitems)				\
do {								\
	size_t _new_size = roundupsize(nitems);			\
	if (_new_size <= ((~(size_t)0) / sizeof(type)))		\
		PyMem_RESIZE(var, type, _new_size);		\
	else							\
		var = NULL;					\
} while (0)

PyObject *PyList_New(int size)
{
	int i;
	PyListObject *op;
	size_t nbytes;
	if (size < 0) 
	{
		PyErr_BadInternalCall();
		return NULL;
	}
	nbytes = size * sizeof(PyObject *);
	if (nbytes / sizeof(PyObject *) != (size_t)size) 
	{
		return PyErr_NoMemory();
	}
	op = PyObject_GC_New(PyListObject, &PyList_Type);
	if (op == NULL) 
	{
		return NULL;
	}
	if (size <= 0) 
	{
		op->ob_item = NULL;
	}
	else 
	{
		op->ob_item = (PyObject **) PyMem_MALLOC(nbytes);
		if (op->ob_item == NULL) 
		{
			return PyErr_NoMemory();
		}
	}
	op->ob_size = size;
	for (i = 0; i < size; i++)
	{
		op->ob_item[i] = NULL;
	}
	_PyObject_GC_TRACK(op);
	return (PyObject *) op;
}

int PyList_Size(PyObject *op)
{
	if (!PyList_Check(op)) 
	{
		PyErr_BadInternalCall();
		return -1;
	}
	else
	{
		return ((PyListObject *)op) -> ob_size;
	}
}

static PyObject *indexerr;

PyObject *PyList_GetItem(PyObject *op, int i)
{
	if (!PyList_Check(op)) 
	{
		PyErr_BadInternalCall();
		return NULL;
	}
	if (i < 0 || i >= ((PyListObject *)op) -> ob_size) 
	{
		if (indexerr == NULL)
		{
			indexerr = PyString_FromString(
				"list index out of range");
		}
		PyErr_SetObject(PyExc_IndexError, indexerr);
		return NULL;
	}
	return ((PyListObject *)op) -> ob_item[i];
}

int PyList_SetItem(PyObject *op, int i,
               PyObject *newitem)
{
	PyObject *olditem;
	PyObject **p;
	if (!PyList_Check(op)) 
	{
		Py_XDECREF(newitem);
		PyErr_BadInternalCall();
		return -1;
	}
	if (i < 0 || i >= ((PyListObject *)op) -> ob_size) 
	{
		Py_XDECREF(newitem);
		PyErr_SetString(PyExc_IndexError,
				"list assignment index out of range");
		return -1;
	}
	p = ((PyListObject *)op) -> ob_item + i;
	olditem = *p;
	*p = newitem;
	Py_XDECREF(olditem);
	return 0;
}

static int ins1(PyListObject *self, int where, PyObject *v)
{
	int i;
	PyObject **items;
	if (v == NULL) 
	{
		PyErr_BadInternalCall();
		return -1;
	}
	if (self->ob_size == INT_MAX) 
	{
		PyErr_SetString(PyExc_OverflowError,
			"cannot add more objects to list");
		return -1;
	}
	items = self->ob_item;
	NRESIZE(items, PyObject *, self->ob_size+1);
	if (items == NULL) 
	{
		PyErr_NoMemory();
		return -1;
	}
	if (where < 0)
	{
		where = 0;
	}
	if (where > self->ob_size)
	{
		where = self->ob_size;
	}
	for (i = self->ob_size; --i >= where; )
	{
		items[i + 1] = items[i];
	}
	Py_INCREF(v);
	items[where] = v;
	self->ob_item = items;
	self->ob_size++;
	return 0;
}

int PyList_Insert(PyObject *op, int where, PyObject *newitem)
{
	if (!PyList_Check(op)) 
	{
		PyErr_BadInternalCall();
		return -1;
	}
	return ins1((PyListObject *)op, where, newitem);
}

int PyList_Append(PyObject *op, PyObject *newitem)
{
	if (!PyList_Check(op)) 
	{
		PyErr_BadInternalCall();
		return -1;
	}
	return ins1((PyListObject *)op,
		(int) ((PyListObject *)op)->ob_size, newitem);
}

static void list_dealloc(PyListObject *op)
{
	int i;
	PyObject_GC_UnTrack(op);
	Py_TRASHCAN_SAFE_BEGIN(op)
	if (op->ob_item != NULL) 
	{
		i = op->ob_size;
		while (--i >= 0) 
		{
			Py_XDECREF(op->ob_item[i]);
		}
		PyMem_FREE(op->ob_item);
	}
	op->ob_type->tp_free((PyObject *)op);
	Py_TRASHCAN_SAFE_END(op)
}

static int list_print(PyListObject *op, FILE *fp, int flags)
{
	int i;

	i = Py_ReprEnter((PyObject*)op);
	if (i != 0) 
	{
		if (i < 0)
		{
			return i;
		}
		fprintf(fp, "[...]");
		return 0;
	}
	fprintf(fp, "[");
	for (i = 0; i < op->ob_size; i++) 
	{
		if (i > 0)
		{
			fprintf(fp, ", ");
		}
		if (PyObject_Print(op->ob_item[i], fp, 0) != 0) 
		{
			Py_ReprLeave((PyObject *)op);
			return -1;
		}
	}
	fprintf(fp, "]");
	Py_ReprLeave((PyObject *)op);
	return 0;
}

static PyObject *list_repr(PyListObject *v)
{
	int i;
	PyObject *s, *temp;
	PyObject *pieces = NULL, *result = NULL;

	i = Py_ReprEnter((PyObject*)v);
	if (i != 0) 
	{
		return i > 0 ? PyString_FromString("[...]") : NULL;
	}

	if (v->ob_size == 0) 
	{
		result = PyString_FromString("[]");
		goto Done;
	}

	pieces = PyList_New(0);
	if (pieces == NULL)
	{
		goto Done;
	}

	for (i = 0; i < v->ob_size; ++i) 
	{
		int status;
		s = PyObject_Repr(v->ob_item[i]);
		if (s == NULL)
		{
			goto Done;
		}
		status = PyList_Append(pieces, s);
		Py_DECREF(s);  
		if (status < 0)
		{
			goto Done;
		}
	}

	assert(PyList_GET_SIZE(pieces) > 0);
	s = PyString_FromString("[");
	if (s == NULL)
	{
		goto Done;
	}
	temp = PyList_GET_ITEM(pieces, 0);
	PyString_ConcatAndDel(&s, temp);
	PyList_SET_ITEM(pieces, 0, s);
	if (s == NULL)
	{
		goto Done;
	}

	s = PyString_FromString("]");
	if (s == NULL)
	{
		goto Done;
	}
	temp = PyList_GET_ITEM(pieces, PyList_GET_SIZE(pieces) - 1);
	PyString_ConcatAndDel(&temp, s);
	PyList_SET_ITEM(pieces, PyList_GET_SIZE(pieces) - 1, temp);
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
	Py_XDECREF(pieces);
	Py_ReprLeave((PyObject *)v);
	return result;
}

static int list_length(PyListObject *a)
{
	return a->ob_size;
}

static int list_contains(PyListObject *a, PyObject *el)
{
	int i;

	for (i = 0; i < a->ob_size; ++i) 
	{
		int cmp = PyObject_RichCompareBool(el, PyList_GET_ITEM(a, i),
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


static PyObject *list_item(PyListObject *a, int i)
{
	if (i < 0 || i >= a->ob_size) 
	{
		if (indexerr == NULL)
		{
			indexerr = PyString_FromString(
				"list index out of range");
		}
		PyErr_SetObject(PyExc_IndexError, indexerr);
		return NULL;
	}
	Py_INCREF(a->ob_item[i]);
	return a->ob_item[i];
}

static PyObject *list_slice(PyListObject *a, int ilow, int ihigh)
{
	PyListObject *np;
	int i;
	if (ilow < 0)
	{
		ilow = 0;
	}
	else if (ilow > a->ob_size)
	{
		ilow = a->ob_size;
	}
	if (ihigh < ilow)
	{
		ihigh = ilow;
	}
	else if (ihigh > a->ob_size)
	{
		ihigh = a->ob_size;
	}
	np = (PyListObject *) PyList_New(ihigh - ilow);
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

PyObject *PyList_GetSlice(PyObject *a, int ilow, int ihigh)
{
	if (!PyList_Check(a)) 
	{
		PyErr_BadInternalCall();
		return NULL;
	}
	return list_slice((PyListObject *)a, ilow, ihigh);
}

static PyObject *list_concat(PyListObject *a, PyObject *bb)
{
	int size;
	int i;
	PyListObject *np;
	if (!PyList_Check(bb)) 
	{
		PyErr_Format(PyExc_TypeError,
			  "can only concatenate list (not \"%.200s\") to list",
			  bb->ob_type->tp_name);
		return NULL;
	}
#define b ((PyListObject *)bb)
	size = a->ob_size + b->ob_size;
	if (size < 0)
	{
		return PyErr_NoMemory();
	}
	np = (PyListObject *) PyList_New(size);
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

static PyObject *list_repeat(PyListObject *a, int n)
{
	int i, j;
	int size;
	PyListObject *np;
	PyObject **p;
	if (n < 0)
	{
		n = 0;
	}
	size = a->ob_size * n;
	if (n && size/n != a->ob_size)
	{
		return PyErr_NoMemory();
	}
	np = (PyListObject *) PyList_New(size);
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

static int list_ass_slice(PyListObject *a, int ilow, int ihigh, PyObject *v)
{
	PyObject **recycle, **p;
	PyObject **item;
	int n; 
	int d; 
	int k; 
#define b ((PyListObject *)v)
	if (v == NULL)
	{
		n = 0;
	}
	else if (PyList_Check(v)) 
	{
		n = b->ob_size;
		if (a == b) 
		{
			int ret;
			v = list_slice(b, 0, n);
			ret = list_ass_slice(a, ilow, ihigh, v);
			Py_DECREF(v);
			return ret;
		}
	}
	else 
	{
		PyErr_Format(PyExc_TypeError,
			     "must assign list (not \"%.200s\") to slice",
			     v->ob_type->tp_name);
		return -1;
	}
	if (ilow < 0)
	{
		ilow = 0;
	}
	else if (ilow > a->ob_size)
	{
		ilow = a->ob_size;
	}
	if (ihigh < ilow)
	{
		ihigh = ilow;
	}
	else if (ihigh > a->ob_size)
	{
		ihigh = a->ob_size;
	}
	item = a->ob_item;
	d = n - (ihigh-ilow);
	if (ihigh > ilow)
	{
		p = recycle = PyMem_NEW(PyObject *, (ihigh-ilow));
	}
	else
	{
		p = recycle = NULL;
	}
	if (d <= 0) 
	{ 
		for (k = ilow; k < ihigh; k++)
		{
			*p++ = item[k];
		}
		if (d < 0) 
		{
			for (; k < a->ob_size; k++)
			{
				item[k+d] = item[k];
			}
			a->ob_size += d;
			NRESIZE(item, PyObject *, a->ob_size);
			a->ob_item = item;
		}
	}
	else 
	{ 
		NRESIZE(item, PyObject *, a->ob_size + d);
		if (item == NULL) 
		{
			if (recycle != NULL)
			{
				PyMem_DEL(recycle);
			}
			PyErr_NoMemory();
			return -1;
		}
		for (k = a->ob_size; --k >= ihigh; )
		{
			item[k+d] = item[k];
		}
		for (; k >= ilow; --k)
		{
			*p++ = item[k];
		}
		a->ob_item = item;
		a->ob_size += d;
	}
	for (k = 0; k < n; k++, ilow++) 
	{
		PyObject *w = b->ob_item[k];
		Py_XINCREF(w);
		item[ilow] = w;
	}
	if (recycle) 
	{
		while (--p >= recycle)
		{
			Py_XDECREF(*p);
		}
		PyMem_DEL(recycle);
	}
	if (a->ob_size == 0 && a->ob_item != NULL) 
	{
		PyMem_FREE(a->ob_item);
		a->ob_item = NULL;
	}
	return 0;
#undef b
}

int PyList_SetSlice(PyObject *a, int ilow, int ihigh, PyObject *v)
{
	if (!PyList_Check(a)) 
	{
		PyErr_BadInternalCall();
		return -1;
	}
	return list_ass_slice((PyListObject *)a, ilow, ihigh, v);
}

static PyObject *list_inplace_repeat(PyListObject *self, int n)
{
	PyObject **items;
	int size, i, j;

	size = PyList_GET_SIZE(self);
	if (size == 0) 
	{
		Py_INCREF(self);
		return (PyObject *)self;
	}

	items = self->ob_item;

	if (n < 1) 
	{
		self->ob_item = NULL;
		self->ob_size = 0;
		for (i = 0; i < size; i++)
			Py_XDECREF(items[i]);
		PyMem_DEL(items);
		Py_INCREF(self);
		return (PyObject *)self;
	}

	NRESIZE(items, PyObject*, size*n);
	if (items == NULL) 
	{
		PyErr_NoMemory();
		goto finally;
	}
	self->ob_item = items;
	for (i = 1; i < n; i++) 
	{ 
		for (j = 0; j < size; j++) 
		{
			PyObject *o = PyList_GET_ITEM(self, j);
			Py_INCREF(o);
			PyList_SET_ITEM(self, self->ob_size++, o);
		}
	}
	Py_INCREF(self);
	return (PyObject *)self;
finally:
  	return NULL;
}

static int list_ass_item(PyListObject *a, int i, PyObject *v)
{
	PyObject *old_value;
	if (i < 0 || i >= a->ob_size) 
	{
		PyErr_SetString(PyExc_IndexError,
				"list assignment index out of range");
		return -1;
	}
	if (v == NULL)
	{
		return list_ass_slice(a, i, i+1, v);
	}
	Py_INCREF(v);
	old_value = a->ob_item[i];
	a->ob_item[i] = v;
	Py_DECREF(old_value); 
	return 0;
}

static PyObject *ins(PyListObject *self, int where, PyObject *v)
{
	if (ins1(self, where, v) != 0)
	{
		return NULL;
	}
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *listinsert(PyListObject *self, PyObject *args)
{
	int i;
	PyObject *v;
	if (!PyArg_ParseTuple(args, "iO:insert", &i, &v))
	{
		return NULL;
	}
	return ins(self, i, v);
}

static PyObject *listappend(PyListObject *self, PyObject *v)
{
	return ins(self, (int) self->ob_size, v);
}

static int listextend_internal(PyListObject *self, PyObject *b)
{
	PyObject **items;
	int selflen = PyList_GET_SIZE(self);
	int blen;
	int i;

	if (PyObject_Size(b) == 0) 
	{
		Py_DECREF(b);
		return 0;
	}

	if (self == (PyListObject*)b) 
	{
		Py_DECREF(b);
		b = PyList_New(selflen);
		if (!b)
		{
			return -1;
		}
		for (i = 0; i < selflen; i++) 
		{
			PyObject *o = PyList_GET_ITEM(self, i);
			Py_INCREF(o);
			PyList_SET_ITEM(b, i, o);
		}
	}

	blen = PyObject_Size(b);

	items = self->ob_item;
	NRESIZE(items, PyObject*, selflen + blen);
	if (items == NULL) 
	{
		PyErr_NoMemory();
		Py_DECREF(b);
		return -1;
	}

	self->ob_item = items;

	for (i = 0; i < blen; i++) 
	{
		PyObject *o = PySequence_Fast_GET_ITEM(b, i);
		Py_INCREF(o);
		PyList_SET_ITEM(self, self->ob_size++, o);
	}
	Py_DECREF(b);
	return 0;
}


static PyObject *list_inplace_concat(PyListObject *self, PyObject *other)
{
	other = PySequence_Fast(other, "argument to += must be iterable");
	if (!other)
	{
		return NULL;
	}

	if (listextend_internal(self, other) < 0)
	{
		return NULL;
	}

	Py_INCREF(self);
	return (PyObject *)self;
}

static PyObject *listextend(PyListObject *self, PyObject *b)
{

	b = PySequence_Fast(b, "list.extend() argument must be iterable");
	if (!b)
	{
		return NULL;
	}

	if (listextend_internal(self, b) < 0)
	{
		return NULL;
	}

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *listpop(PyListObject *self, PyObject *args)
{
	int i = -1;
	PyObject *v;
	if (!PyArg_ParseTuple(args, "|i:pop", &i))
	{
		return NULL;
	}
	if (self->ob_size == 0) 
	{
		PyErr_SetString(PyExc_IndexError, "pop from empty list");
		return NULL;
	}
	if (i < 0)
	{
		i += self->ob_size;
	}
	if (i < 0 || i >= self->ob_size) 
	{
		PyErr_SetString(PyExc_IndexError, "pop index out of range");
		return NULL;
	}
	v = self->ob_item[i];
	Py_INCREF(v);
	if (list_ass_slice(self, i, i+1, (PyObject *)NULL) != 0) 
	{
		Py_DECREF(v);
		return NULL;
	}
	return v;
}

#define CMPERROR ( (int) ((unsigned int)1 << (8*sizeof(int) - 1)) )

static int docompare(PyObject *x, PyObject *y, PyObject *compare)
{
	PyObject *args, *res;
	int i;

	if (compare == NULL) 
	{
		i = PyObject_RichCompareBool(x, y, Py_LT);
		if (i < 0)
		{
			return CMPERROR;
		}
		else
		{
			return -i;
		}
	}

	args = Py_BuildValue("(OO)", x, y);
	if (args == NULL)
	{
		return CMPERROR;
	}
	res = PyEval_CallObject(compare, args);
	Py_DECREF(args);
	if (res == NULL)
	{
		return CMPERROR;
	}
	if (!PyInt_Check(res)) 
	{
		Py_DECREF(res);
		PyErr_SetString(PyExc_TypeError,
				"comparison function must return int");
		return CMPERROR;
	}
	i = PyInt_AsLong(res);
	Py_DECREF(res);
	if (i < 0)
	{
		return -1;
	}
	if (i > 0)
	{
		return 1;
	}
	return 0;
}

#define MINSIZE 100

#define MINPARTITIONSIZE 40

#define MAXMERGE 15

#define STACKSIZE 60

#define SETK(X,Y) if ((k = docompare(X,Y,compare))==CMPERROR) goto fail

static int binarysort(PyObject **lo, PyObject **hi, PyObject **start, PyObject *compare)
{
	int k;
	PyObject **l, **p, **r;
	PyObject *pivot;

	if (lo == start)
	{
		++start;
	}
	for (; start < hi; ++start) 
	{
		l = lo;
		r = start;
		pivot = *r;
		do 
		{
			p = l + ((r - l) >> 1);
			SETK(pivot, *p);
			if (k < 0)
			{
				r = p;
			}
			else
			{
				l = p + 1;
			}
		} while (l < r);
		for (p = start; p > l; --p)
		{
			*p = *(p-1);
		}
		*l = pivot;
	}
	return 0;
fail:
	return -1;
}

struct SamplesortStackNode {
	PyObject **lo;
	PyObject **hi;
	int extra;
};

#define CUTOFFBASE 4
static long cutoff[] = {
	43,        
	106,       
	250,
	576,
	1298,
	2885,
	6339,
	13805,
	29843,
	64116,
	137030,
	291554,
	617916,
	1305130,
	2748295,
	5771662,
	12091672,
	25276798,
	52734615,
	109820537,
	228324027,
	473977813,
	982548444,   
	2034159050   
};

static int samplesortslice(PyObject **lo, PyObject **hi, PyObject *compare)
{
	PyObject **l, **r;
	PyObject *tmp, *pivot;
	int k;
	int n, extra, top, extraOnRight;
	struct SamplesortStackNode stack[STACKSIZE];

	n = hi - lo;

	if (n < 2)
	{
		return 0;
	}

	for (r = lo+1; r < hi; ++r) 
	{
		SETK(*r, *(r-1));
		if (k < 0)
		{
			break;
		}
	}
	if (hi - r <= MAXMERGE || n < MINSIZE)
	{
		return binarysort(lo, hi, r, compare);
	}

	for (r = lo+1; r < hi; ++r) 
	{
		SETK(*(r-1), *r);
		if (k < 0)
		{
			break;
		}
	}
	if (hi - r <= MAXMERGE) 
	{
		PyObject **originalr = r;
		l = lo;
		do 
		{
			--r;
			tmp = *l; *l = *r; *r = tmp;
			++l;
		} while (l < r);
		return binarysort(lo, hi, originalr, compare);
	}

	for (extra = 0;
	     extra < sizeof(cutoff) / sizeof(cutoff[0]);
	     ++extra) 
	{
		if (n < cutoff[extra])
		{
			break;
		}
	}
	extra = (1 << (extra - 1 + CUTOFFBASE)) - 1;
	{
		unsigned int seed = n / extra; 
		unsigned int i;
		for (i = 0; i < (unsigned)extra; ++i) 
		{
			unsigned int j;
			seed = seed * 69069 + 7;
			j = i + seed % (n - i);
			tmp = lo[i]; lo[i] = lo[j]; lo[j] = tmp;
		}
	}

	if (samplesortslice(lo, lo + extra, compare) < 0)
	{
		goto fail;
	}

	top = 0;          
	lo += extra;      
	extraOnRight = 0; 

	for (;;) 
	{
		n = hi - lo;

		if (n < MINPARTITIONSIZE || extra == 0) 
		{
			if (n >= MINSIZE) 
			{
				if (samplesortslice(lo, hi, compare) < 0)
				{
					goto fail;
				}
			}
			else 
			{
				if (extraOnRight && extra) 
				{
					k = extra;
					do 
					{
						tmp = *lo;
						*lo = *hi;
						*hi = tmp;
						++lo; ++hi;
					} while (--k);
				}
				if (binarysort(lo - extra, hi, lo,
					       compare) < 0)
				{
					goto fail;
				}
			}

			if (--top < 0)
			{
				break;   
			}
			lo = stack[top].lo;
			hi = stack[top].hi;
			extra = stack[top].extra;
			extraOnRight = 0;
			if (extra < 0) 
			{
				extraOnRight = 1;
				extra = -extra;
			}
			continue;
		}

		k = extra >>= 1; 
		if (extraOnRight) 
		{
			do 
			{
				tmp = *lo; 
				*lo = *hi; 
				*hi = tmp;
				++lo; 
				++hi;
			} while (k--);
		}
		else 
		{
			while (k--) 
			{
				--lo; 
				--hi;
				tmp = *lo; 
				*lo = *hi; 
				*hi = tmp;
			}
		}
		--lo;  
		pivot = *lo;

		l = lo + 1;
		r = hi - 1;

		do 
		{
			do 
			{
				SETK(*l, pivot);
				if (k < 0)
				{
					++l;
				}
				else
				{
					break;
				}
			} while (l < r);

			while (l < r) 
			{
				PyObject *rval = *r--;
				SETK(rval, pivot);
				if (k < 0) 
				{
					r[1] = *l;
					*l++ = rval;
					break;
				}
			}

		} while (l < r);

		if (l == r) 
		{
			SETK(*r, pivot);
			if (k < 0)
			{
				++l;
			}
			else
			{
				--r;
			}
		}
		*lo = *r;
		*r = pivot;

		while (l < hi) 
		{
			SETK(pivot, *l);
			if (k < 0)
			{
				break;
			}
			else
			{
				++l;
			}
		}
		
		if (r - lo <= hi - l) 
		{
			stack[top].lo = l;
			stack[top].hi = hi;
			stack[top].extra = -extra;
			hi = r;
			extraOnRight = 0;
		}
		else 
		{
			stack[top].lo = lo;
			stack[top].hi = r;
			stack[top].extra = extra;
			lo = l;
			extraOnRight = 1;
		}
		++top;

	}   
	
	return 0;

fail:
	return -1;
}

#undef SETK

staticforward PyTypeObject immutable_list_type;

static PyObject *listsort(PyListObject *self, PyObject *args)
{
	int err;
	PyObject *compare = NULL;
	PyTypeObject *savetype;

	if (args != NULL) 
	{
		if (!PyArg_ParseTuple(args, "|O:sort", &compare))
		{
			return NULL;
		}
	}
	savetype = self->ob_type;
	self->ob_type = &immutable_list_type;
	err = samplesortslice(self->ob_item,
			      self->ob_item + self->ob_size,
			      compare);
	self->ob_type = savetype;
	if (err < 0)
	{
		return NULL;
	}
	Py_INCREF(Py_None);
	return Py_None;
}

int PyList_Sort(PyObject *v)
{
	if (v == NULL || !PyList_Check(v)) 
	{
		PyErr_BadInternalCall();
		return -1;
	}
	v = listsort((PyListObject *)v, (PyObject *)NULL);
	if (v == NULL)
	{
		return -1;
	}
	Py_DECREF(v);
	return 0;
}

static void _listreverse(PyListObject *self)
{
	PyObject **p, **q;
	PyObject *tmp;
	
	if (self->ob_size > 1) 
	{
		for (p = self->ob_item, q = self->ob_item + self->ob_size - 1;
		     p < q;
		     p++, q--)
		{
			tmp = *p;
			*p = *q;
			*q = tmp;
		}
	}
}

static PyObject *listreverse(PyListObject *self)
{
	_listreverse(self);
	Py_INCREF(Py_None);
	return Py_None;
}

int PyList_Reverse(PyObject *v)
{
	if (v == NULL || !PyList_Check(v)) 
	{
		PyErr_BadInternalCall();
		return -1;
	}
	_listreverse((PyListObject *)v);
	return 0;
}

PyObject *PyList_AsTuple(PyObject *v)
{
	PyObject *w;
	PyObject **p;
	int n;
	if (v == NULL || !PyList_Check(v)) 
	{
		PyErr_BadInternalCall();
		return NULL;
	}
	n = ((PyListObject *)v)->ob_size;
	w = PyTuple_New(n);
	if (w == NULL)
	{
		return NULL;
	}
	p = ((PyTupleObject *)w)->ob_item;
	memcpy((void *)p,
	       (void *)((PyListObject *)v)->ob_item,
	       n*sizeof(PyObject *));
	while (--n >= 0) 
	{
		Py_INCREF(*p);
		p++;
	}
	return w;
}

static PyObject *listindex(PyListObject *self, PyObject *v)
{
	int i;

	for (i = 0; i < self->ob_size; i++) 
	{
		int cmp = PyObject_RichCompareBool(self->ob_item[i], v, Py_EQ);
		if (cmp > 0)
		{
			return PyInt_FromLong((long)i);
		}
		else if (cmp < 0)
		{
			return NULL;
		}
	}
	PyErr_SetString(PyExc_ValueError, "list.index(x): x not in list");
	return NULL;
}

static PyObject *listcount(PyListObject *self, PyObject *v)
{
	int count = 0;
	int i;

	for (i = 0; i < self->ob_size; i++) 
	{
		int cmp = PyObject_RichCompareBool(self->ob_item[i], v, Py_EQ);
		if (cmp > 0)
		{
			count++;
		}
		else if (cmp < 0)
		{
			return NULL;
		}
	}
	return PyInt_FromLong((long)count);
}

static PyObject *listremove(PyListObject *self, PyObject *v)
{
	int i;

	for (i = 0; i < self->ob_size; i++) 
	{
		int cmp = PyObject_RichCompareBool(self->ob_item[i], v, Py_EQ);
		if (cmp > 0) 
		{
			if (list_ass_slice(self, i, i+1,
					   (PyObject *)NULL) != 0)
			{
				return NULL;
			}
			Py_INCREF(Py_None);
			return Py_None;
		}
		else if (cmp < 0)
		{
			return NULL;
		}
	}
	PyErr_SetString(PyExc_ValueError, "list.remove(x): x not in list");
	return NULL;
}

static int list_traverse(PyListObject *o, visitproc visit, void *arg)
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

static int list_clear(PyListObject *lp)
{
	PyList_SetSlice((PyObject *)lp, 0, lp->ob_size, 0);
	return 0;
}

static PyObject *list_richcompare(PyObject *v, PyObject *w, int op)
{
	PyListObject *vl, *wl;
	int i;

	if (!PyList_Check(v) || !PyList_Check(w)) 
	{
		Py_INCREF(Py_NotImplemented);
		return Py_NotImplemented;
	}

	vl = (PyListObject *)v;
	wl = (PyListObject *)w;

	if (vl->ob_size != wl->ob_size && (op == Py_EQ || op == Py_NE)) 
	{
		PyObject *res;
		if (op == Py_EQ)
		{
			res = Py_False;
		}
		else
		{
			res = Py_True;
		}
		Py_INCREF(res);
		return res;
	}

	for (i = 0; i < vl->ob_size && i < wl->ob_size; i++) 
	{
		int k = PyObject_RichCompareBool(vl->ob_item[i],
						 wl->ob_item[i], Py_EQ);
		if (k < 0)
		{
			return NULL;
		}
		if (!k)
		{
			break;
		}
	}

	if (i >= vl->ob_size || i >= wl->ob_size) 
	{
		int vs = vl->ob_size;
		int ws = wl->ob_size;
		int cmp;
		PyObject *res;
		switch (op) 
		{
		case Py_LT: 
			cmp = vs <  ws; 
			break;
		
		case Py_LE: 
			cmp = vs <= ws; 
			break;
		
		case Py_EQ: 
			cmp = vs == ws; 
			break;
		
		case Py_NE: 
			cmp = vs != ws; 
			break;
		
		case Py_GT: 
			cmp = vs >  ws; 
			break;
		
		case Py_GE: 
			cmp = vs >= ws; 
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

	return PyObject_RichCompare(vl->ob_item[i], wl->ob_item[i], op);
}

static int list_fill(PyListObject *result, PyObject *v)
{
	PyObject *it;      
	int n;		   
	int i;

	n = result->ob_size;


	if (PyList_Check(v)) 
	{
		if (v == (PyObject *)result)
		{
			return 0; 
		}
		return list_ass_slice(result, 0, n, v);
	}

	if (n != 0) 
	{
		if (list_ass_slice(result, 0, n, (PyObject *)NULL) != 0)
		{
			return -1;
		}
	}

	it = PyObject_GetIter(v);
	if (it == NULL)
	{
		return -1;
	}

	n = -1;	
	if (PySequence_Check(v) &&
	    v->ob_type->tp_as_sequence->sq_length) 
	{
		n = PySequence_Size(v);
		if (n < 0)
		{
			PyErr_Clear();
		}
	}
	if (n < 0)
	{
		n = 8;
	}
	NRESIZE(result->ob_item, PyObject*, n);
	if (result->ob_item == NULL) 
	{
		PyErr_NoMemory();
		goto error;
	}
	for (i = 0; i < n; i++)
	{
		result->ob_item[i] = NULL;
	}
	result->ob_size = n;

	for (i = 0; ; i++) 
	{
		PyObject *item = PyIter_Next(it);
		if (item == NULL) 
		{
			if (PyErr_Occurred())
			{
				goto error;
			}
			break;
		}
		if (i < n)
		{
			PyList_SET_ITEM(result, i, item);
		}
		else 
		{
			int status = ins1(result, result->ob_size, item);
			Py_DECREF(item); 
			if (status < 0)
			{
				goto error;
			}
		}
	}

	if (i < n && result != NULL) 
	{
		if (list_ass_slice(result, i, n, (PyObject *)NULL) != 0)
		{
			goto error;
		}
	}
	Py_DECREF(it);
	return 0;

error:
	Py_DECREF(it);
	return -1;
}

static int list_init(PyListObject *self, PyObject *args, PyObject *kw)
{
	PyObject *arg = NULL;
	static char *kwlist[] = {"sequence", 0};

	if (!PyArg_ParseTupleAndKeywords(args, kw, "|O:list", kwlist, &arg))
	{
		return -1;
	}
	if (arg != NULL)
	{
		return list_fill(self, arg);
	}
	if (self->ob_size > 0)
	{
		return list_ass_slice(self, 0, self->ob_size, (PyObject*)NULL);
	}
	return 0;
}

static long list_nohash(PyObject *self)
{
	PyErr_SetString(PyExc_TypeError, "list objects are unhashable");
	return -1;
}

static char append_doc[] =
	"L.append(object) -- append object to end";

static char extend_doc[] =
	"L.extend(list) -- extend list by appending list elements";

static char insert_doc[] =
	"L.insert(index, object) -- insert object before index";

static char pop_doc[] =
	"L.pop([index]) -> item -- remove and return item at index (default last)";

static char remove_doc[] =
	"L.remove(value) -- remove first occurrence of value";

static char index_doc[] =
	"L.index(value) -> integer -- return index of first occurrence of value";

static char count_doc[] =
	"L.count(value) -> integer -- return number of occurrences of value";

static char reverse_doc[] =
	"L.reverse() -- reverse *IN PLACE*";

static char sort_doc[] =
	"L.sort([cmpfunc]) -- sort *IN PLACE*; if given, cmpfunc(x, y) -> -1, 0, 1";

static PyMethodDef list_methods[] = {
	{"append",	(PyCFunction)listappend,  METH_O, append_doc},
	{"insert",	(PyCFunction)listinsert,  METH_VARARGS, insert_doc},
	{"extend",      (PyCFunction)listextend,  METH_O, extend_doc},
	{"pop",		(PyCFunction)listpop, 	  METH_VARARGS, pop_doc},
	{"remove",	(PyCFunction)listremove,  METH_O, remove_doc},
	{"index",	(PyCFunction)listindex,   METH_O, index_doc},
	{"count",	(PyCFunction)listcount,   METH_O, count_doc},
	{"reverse",	(PyCFunction)listreverse, METH_NOARGS, reverse_doc},
	{"sort",	(PyCFunction)listsort, 	  METH_VARARGS, sort_doc},
	{NULL,		NULL}
};

static PySequenceMethods list_as_sequence = {
	(inquiry)list_length,			
	(binaryfunc)list_concat,		
	(intargfunc)list_repeat,		
	(intargfunc)list_item,			
	(intintargfunc)list_slice,		
	(intobjargproc)list_ass_item,	
	(intintobjargproc)list_ass_slice,
	(objobjproc)list_contains,	
	(binaryfunc)list_inplace_concat,
	(intargfunc)list_inplace_repeat,
};

static char list_doc[] =
	"list() -> new list\n"
	"list(sequence) -> new list initialized from sequence's items";

PyTypeObject PyList_Type = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,
	"list",
	sizeof(PyListObject),
	0,
	(destructor)list_dealloc,	
	(printfunc)list_print,		
	0,					
	0,					
	0,					
	(reprfunc)list_repr,
	0,					
	&list_as_sequence,	
	0,					
	list_nohash,		
	0,					
	0,					
	PyObject_GenericGetAttr,
	0,					
	0,					
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC |
		Py_TPFLAGS_BASETYPE,
 	list_doc,				
 	(traverseproc)list_traverse,
 	(inquiry)list_clear,	
	list_richcompare,		
	0,				
	0,				
	0,				
	list_methods,	
	0,				
	0,				
	0,				
	0,				
	0,				
	0,				
	0,				
	(initproc)list_init,		
	PyType_GenericAlloc,		
	PyType_GenericNew,			
	_PyObject_GC_Del,        	
};

static PyObject *immutable_list_op()
{
	PyErr_SetString(PyExc_TypeError,
			"a list cannot be modified while it is being sorted");
	return NULL;
}

static PyMethodDef immutable_list_methods[] = {
	{"append",	(PyCFunction)immutable_list_op, METH_VARARGS},
	{"insert",	(PyCFunction)immutable_list_op, METH_VARARGS},
	{"extend",      (PyCFunction)immutable_list_op,  METH_O},
	{"pop",		(PyCFunction)immutable_list_op, METH_VARARGS},
	{"remove",	(PyCFunction)immutable_list_op, METH_VARARGS},
	{"index",	(PyCFunction)listindex,         METH_O},
	{"count",	(PyCFunction)listcount,         METH_O},
	{"reverse",	(PyCFunction)immutable_list_op, METH_VARARGS},
	{"sort",	(PyCFunction)immutable_list_op, METH_VARARGS},
	{NULL,		NULL}
};

static int immutable_list_ass()
{
	immutable_list_op();
	return -1;
}

static PySequenceMethods immutable_list_as_sequence = {
	(inquiry)list_length,		
	(binaryfunc)list_concat,	
	(intargfunc)list_repeat,	
	(intargfunc)list_item,		
	(intintargfunc)list_slice,	
	(intobjargproc)immutable_list_ass,
	(intintobjargproc)immutable_list_ass,
	(objobjproc)list_contains,		
};

static PyTypeObject immutable_list_type = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,
	"list (immutable, during sort)",
	sizeof(PyListObject),
	0,
	0, 
	(printfunc)list_print,	
	0,					
	0,					
	0, 
	(reprfunc)list_repr,		
	0,					
	&immutable_list_as_sequence,
	0,					
	list_nohash,		
	0,					
	0,					
	PyObject_GenericGetAttr,
	0,				
	0,				
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,
 	list_doc,				
 	(traverseproc)list_traverse,		
	0,					
	list_richcompare,	
	0,					
	0,					
	0,					
	immutable_list_methods,		
	0,					
	0,					
	0,					
	0,					
	0,					
	0,					
	0,					
};
