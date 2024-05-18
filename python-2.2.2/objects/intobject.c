//20170418
#include "python.h"
#include <ctype.h>

long PyInt_GetMax()
{
	return LONG_MAX;
}

PyIntObject _Py_ZeroStruct = {
	PyObject_HEAD_INIT(&PyInt_Type)
	0
};

PyIntObject _Py_TrueStruct = {
	PyObject_HEAD_INIT(&PyInt_Type)
	1
};

static int err_ovf(char *msg)
{
	if (PyErr_Warn(PyExc_OverflowWarning, msg) < 0) 
	{
		if (PyErr_ExceptionMatches(PyExc_OverflowWarning))
		{
			PyErr_SetString(PyExc_OverflowError, msg);
		}
		return 1;
	}
	else
	{
		return 0;
	}
}

#define BLOCK_SIZE	1000
#define BHEAD_SIZE	8
#define N_INTOBJECTS	((BLOCK_SIZE - BHEAD_SIZE) / sizeof(PyIntObject))

struct _intblock {
	struct _intblock *next;
	PyIntObject objects[N_INTOBJECTS];
};

typedef struct _intblock PyIntBlock;

static PyIntBlock *block_list = NULL;
static PyIntObject *free_list = NULL;

static PyIntObject *fill_free_list()
{
	PyIntObject *p, *q;
	p = (PyIntObject *) PyMem_MALLOC(sizeof(PyIntBlock));
	if (p == NULL)
	{
		return (PyIntObject *) PyErr_NoMemory();
	}
	((PyIntBlock *)p)->next = block_list;
	block_list = (PyIntBlock *)p;
	p = &((PyIntBlock *)p)->objects[0];
	q = p + N_INTOBJECTS;
	while (--q > p)
	{
		q->ob_type = (struct _typeobject *)(q-1);
	}
	q->ob_type = NULL;
	return p + N_INTOBJECTS - 1;
}

#define NSMALLPOSINTS		100

#define NSMALLNEGINTS		1


static PyIntObject *small_ints[NSMALLNEGINTS + NSMALLPOSINTS];

PyObject *PyInt_FromLong(long ival)
{
	PyIntObject *v;
	if (-NSMALLNEGINTS <= ival && ival < NSMALLPOSINTS &&
	    (v = small_ints[ival + NSMALLNEGINTS]) != NULL) 
	{
		Py_INCREF(v);
		return (PyObject *) v;
	}
	if (free_list == NULL) 
	{
		if ((free_list = fill_free_list()) == NULL)
		{
			return NULL;
		}
	}
	v = free_list;
	free_list = (PyIntObject *)v->ob_type;
	PyObject_INIT(v, &PyInt_Type);
	v->ob_ival = ival;
	if (-NSMALLNEGINTS <= ival && ival < NSMALLPOSINTS) 
	{
		Py_INCREF(v);
		small_ints[ival + NSMALLNEGINTS] = v;
	}
	return (PyObject *) v;
}

static void int_dealloc(PyIntObject *v)
{
	if (PyInt_CheckExact(v)) 
	{
		v->ob_type = (struct _typeobject *)free_list;
		free_list = v;
	}
	else
	{
		v->ob_type->tp_free((PyObject *)v);
	}
}

static void int_free(PyIntObject *v)
{
	v->ob_type = (struct _typeobject *)free_list;
	free_list = v;
}

long PyInt_AsLong(PyObject *op)
{
	PyNumberMethods *nb;
	PyIntObject *io;
	long val;

	if (op && PyInt_Check(op))
	{
		return PyInt_AS_LONG((PyIntObject*) op);
	}

	if (op == NULL || (nb = op->ob_type->tp_as_number) == NULL ||
	    nb->nb_int == NULL) 
	{
		PyErr_SetString(PyExc_TypeError, "an integer is required");
		return -1;
	}

	io = (PyIntObject*) (*nb->nb_int) (op);
	if (io == NULL)
	{
		return -1;
	}
	if (!PyInt_Check(io)) 
	{
		PyErr_SetString(PyExc_TypeError,
				"nb_int should return int object");
		return -1;
	}

	val = PyInt_AS_LONG(io);
	Py_DECREF(io);

	return val;
}

PyObject *PyInt_FromString(char *s, char **pend, int base)
{
	char *end;
	long x;
	char buffer[256]; 

	if ((base != 0 && base < 2) || base > 36) 
	{
		PyErr_SetString(PyExc_ValueError, "int() base must be >= 2 and <= 36");
		return NULL;
	}

	while (*s && isspace(Py_CHARMASK(*s)))
	{
		s++;
	}
	errno = 0;
	if (base == 0 && s[0] == '0')
	{
		x = (long) PyOS_strtoul(s, &end, base);
	}
	else
	{
		x = PyOS_strtol(s, &end, base);
	}
	if (end == s || !isalnum(Py_CHARMASK(end[-1])))
	{
		goto bad;
	}
	while (*end && isspace(Py_CHARMASK(*end)))
	{
		end++;
	}
	if (*end != '\0') 
	{
bad:
		PyOS_snprintf(buffer, sizeof(buffer),
			      "invalid literal for int(): %.200s", s);
		PyErr_SetString(PyExc_ValueError, buffer);
		return NULL;
	}
	else if (errno != 0) 
	{
		PyOS_snprintf(buffer, sizeof(buffer),
			      "int() literal too large: %.200s", s);
		PyErr_SetString(PyExc_ValueError, buffer);
		return NULL;
	}
	if (pend)
	{
		*pend = end;
	}
	return PyInt_FromLong(x);
}

PyObject *PyInt_FromUnicode(Py_UNICODE *s, int length, int base)
{
	char buffer[256];

	if (length >= sizeof(buffer)) 
	{
		PyErr_SetString(PyExc_ValueError,
				"int() literal too large to convert");
		return NULL;
	}
	if (PyUnicode_EncodeDecimal(s, length, buffer, NULL))
	{
		return NULL;
	}
	return PyInt_FromString(buffer, NULL, base);
}

#define CONVERT_TO_LONG(obj, lng)		\
	if (PyInt_Check(obj)) {			\
		lng = PyInt_AS_LONG(obj);	\
	}					\
	else {					\
		Py_INCREF(Py_NotImplemented);	\
		return Py_NotImplemented;	\
	}

static int int_print(PyIntObject *v, FILE *fp, int flags)
{
	fprintf(fp, "%ld", v->ob_ival);
	return 0;
}

static PyObject *int_repr(PyIntObject *v)
{
	char buf[64];
	PyOS_snprintf(buf, sizeof(buf), "%ld", v->ob_ival);
	return PyString_FromString(buf);
}

static int int_compare(PyIntObject *v, PyIntObject *w)
{
	long i = v->ob_ival;
	long j = w->ob_ival;
	return (i < j) ? -1 : (i > j) ? 1 : 0;
}

static long int_hash(PyIntObject *v)
{
	long x = v -> ob_ival;
	if (x == -1)
	{
		x = -2;
	}
	return x;
}

static PyObject *int_add(PyIntObject *v, PyIntObject *w)
{
	long a, b, x;
	CONVERT_TO_LONG(v, a);
	CONVERT_TO_LONG(w, b);
	x = a + b;
	if ((x^a) >= 0 || (x^b) >= 0)
	{
		return PyInt_FromLong(x);
	}
	if (err_ovf("integer addition"))
	{
		return NULL;
	}
	return PyLong_Type.tp_as_number->nb_add((PyObject *)v, (PyObject *)w);
}

static PyObject *int_sub(PyIntObject *v, PyIntObject *w)
{
	long a, b, x;
	CONVERT_TO_LONG(v, a);
	CONVERT_TO_LONG(w, b);
	x = a - b;
	if ((x^a) >= 0 || (x^~b) >= 0)
	{
		return PyInt_FromLong(x);
	}
	if (err_ovf("integer subtraction"))
	{
		return NULL;
	}
	return PyLong_Type.tp_as_number->nb_subtract((PyObject *)v,
						     (PyObject *)w);
}

#define USE_SQ_REPEAT(o) (!PyInt_Check(o) && \
			  o->ob_type->tp_as_sequence && \
			  o->ob_type->tp_as_sequence->sq_repeat && \
			  !(o->ob_type->tp_as_number && \
                            o->ob_type->tp_flags & Py_TPFLAGS_CHECKTYPES && \
			    o->ob_type->tp_as_number->nb_multiply))

static PyObject *int_mul(PyObject *v, PyObject *w)
{
	long a, b;
	long longprod;			
	double doubled_longprod;	
	double doubleprod;

	if (USE_SQ_REPEAT(v)) 
	{
repeat:
		a = PyInt_AsLong(w);
		return (*v->ob_type->tp_as_sequence->sq_repeat)(v, a);
	}
	if (USE_SQ_REPEAT(w)) 
	{
		PyObject *tmp = v;
		v = w;
		w = tmp;
		goto repeat;
	}

	CONVERT_TO_LONG(v, a);
	CONVERT_TO_LONG(w, b);
	longprod = a * b;
	doubleprod = (double)a * (double)b;
	doubled_longprod = (double)longprod;

	if (doubled_longprod == doubleprod)
	{
		return PyInt_FromLong(longprod);
	}

	{
		const double diff = doubled_longprod - doubleprod;
		const double absdiff = diff >= 0.0 ? diff : -diff;
		const double absprod = doubleprod >= 0.0 ? doubleprod :
							  -doubleprod;
		if (32.0 * absdiff <= absprod)
		{
			return PyInt_FromLong(longprod);
		}
		else if (err_ovf("integer multiplication"))
		{
			return NULL;
		}
		else
		{
			return PyLong_Type.tp_as_number->nb_multiply(v, w);
		}
	}
}

enum divmod_result {
	DIVMOD_OK,		
	DIVMOD_OVERFLOW,	
	DIVMOD_ERROR
};

static enum divmod_result i_divmod(long x, long y,
         long *p_xdivy, long *p_xmody)
{
	long xdivy, xmody;

	if (y == 0) 
	{
		PyErr_SetString(PyExc_ZeroDivisionError,
				"integer division or modulo by zero");
		return DIVMOD_ERROR;
	}
	if (y == -1 && x < 0 && x == -x) 
	{
		if (err_ovf("integer division"))
		{
			return DIVMOD_ERROR;
		}
		return DIVMOD_OVERFLOW;
	}
	xdivy = x / y;
	xmody = x - xdivy * y;
	if (xmody && ((y ^ xmody) < 0)) 
	{
		xmody += y;
		--xdivy;
		assert(xmody && ((y ^ xmody) >= 0));
	}
	*p_xdivy = xdivy;
	*p_xmody = xmody;
	return DIVMOD_OK;
}

static PyObject *int_div(PyIntObject *x, PyIntObject *y)
{
	long xi, yi;
	long d, m;
	CONVERT_TO_LONG(x, xi);
	CONVERT_TO_LONG(y, yi);
	switch (i_divmod(xi, yi, &d, &m)) 
	{
	case DIVMOD_OK:
		return PyInt_FromLong(d);
	
	case DIVMOD_OVERFLOW:
		return PyLong_Type.tp_as_number->nb_divide((PyObject *)x,
							   (PyObject *)y);
	
	default:
		return NULL;
	}
}

static PyObject *int_classic_div(PyIntObject *x, PyIntObject *y)
{
	long xi, yi;
	long d, m;
	CONVERT_TO_LONG(x, xi);
	CONVERT_TO_LONG(y, yi);
	if (Py_DivisionWarningFlag &&
	    PyErr_Warn(PyExc_DeprecationWarning, "classic int division") < 0)
	{
		return NULL;
	}
	switch (i_divmod(xi, yi, &d, &m)) 
	{
	case DIVMOD_OK:
		return PyInt_FromLong(d);
	
	case DIVMOD_OVERFLOW:
		return PyLong_Type.tp_as_number->nb_divide((PyObject *)x,
							   (PyObject *)y);
	
	default:
		return NULL;
	}
}

static PyObject *int_true_divide(PyObject *v, PyObject *w)
{
	if (PyInt_Check(v) && PyInt_Check(w))
	{
		return PyFloat_Type.tp_as_number->nb_true_divide(v, w);
	}
	Py_INCREF(Py_NotImplemented);
	return Py_NotImplemented;
}

static PyObject *int_mod(PyIntObject *x, PyIntObject *y)
{
	long xi, yi;
	long d, m;
	CONVERT_TO_LONG(x, xi);
	CONVERT_TO_LONG(y, yi);
	switch (i_divmod(xi, yi, &d, &m)) 
	{
	case DIVMOD_OK:
		return PyInt_FromLong(m);
	
	case DIVMOD_OVERFLOW:
		return PyLong_Type.tp_as_number->nb_remainder((PyObject *)x,
							      (PyObject *)y);
	
	default:
		return NULL;
	}
}

static PyObject *int_divmod(PyIntObject *x, PyIntObject *y)
{
	long xi, yi;
	long d, m;
	CONVERT_TO_LONG(x, xi);
	CONVERT_TO_LONG(y, yi);
	switch (i_divmod(xi, yi, &d, &m)) 
	{
	case DIVMOD_OK:
		return Py_BuildValue("(ll)", d, m);
	
	case DIVMOD_OVERFLOW:
		return PyLong_Type.tp_as_number->nb_divmod((PyObject *)x,
							   (PyObject *)y);
	
	default:
		return NULL;
	}
}

static PyObject *int_pow(PyIntObject *v, PyIntObject *w, PyIntObject *z)
{
	long iv, iw, iz=0, ix, temp, prev;
	CONVERT_TO_LONG(v, iv);
	CONVERT_TO_LONG(w, iw);
	if (iw < 0) 
	{
		if ((PyObject *)z != Py_None) 
		{
			PyErr_SetString(PyExc_TypeError, "pow() 2nd argument "
			     "cannot be negative when 3rd argument specified");
			return NULL;
		}
		return PyFloat_Type.tp_as_number->nb_power(
			(PyObject *)v, (PyObject *)w, (PyObject *)z);
	}
 	if ((PyObject *)z != Py_None) 
	{
		CONVERT_TO_LONG(z, iz);
		if (iz == 0) 
		{
			PyErr_SetString(PyExc_ValueError,
					"pow() 3rd argument cannot be 0");
			return NULL;
		}
	}
	temp = iv;
	ix = 1;
	while (iw > 0) 
	{
	 	prev = ix;
	 	if (iw & 1) 
		{
		 	ix = ix * temp;
			if (temp == 0)
			{
				break;
			}
			if (ix / temp != prev) 
			{
				if (err_ovf("integer exponentiation"))
				{
					return NULL;
				}
				return PyLong_Type.tp_as_number->nb_power(
					(PyObject *)v,
					(PyObject *)w,
					(PyObject *)z);
			}
		}
	 	iw >>= 1;
	    if (iw == 0) 
		{
			break;
		}
		prev = temp;
	 	temp *= temp;
		if (prev!=0 && temp/prev!=prev) 
		{
			if (err_ovf("integer exponentiation"))
			{
				return NULL;
			}
			return PyLong_Type.tp_as_number->nb_power(
				(PyObject *)v, (PyObject *)w, (PyObject *)z);
		}
	 	if (iz) 
		{
		 	ix = ix % iz;
		 	temp = temp % iz;
		}
	}
	if (iz) 
	{
	 	long div, mod;
		switch (i_divmod(ix, iz, &div, &mod)) 
		{
		case DIVMOD_OK:
			ix = mod;
			break;
		
		case DIVMOD_OVERFLOW:
			return PyLong_Type.tp_as_number->nb_power(
				(PyObject *)v, (PyObject *)w, (PyObject *)z);
		
		default:
			return NULL;
		}
	}
	return PyInt_FromLong(ix);
}

static PyObject *int_neg(PyIntObject *v)
{
	long a, x;
	a = v->ob_ival;
	x = -a;
	if (a < 0 && x < 0) 
	{
		if (err_ovf("integer negation"))
		{
			return NULL;
		}
		return PyNumber_Negative(PyLong_FromLong(a));
	}
	return PyInt_FromLong(x);
}

static PyObject *int_pos(PyIntObject *v)
{
	if (PyInt_CheckExact(v)) 
	{
		Py_INCREF(v);
		return (PyObject *)v;
	}
	else
	{
		return PyInt_FromLong(v->ob_ival);
	}
}

static PyObject *int_abs(PyIntObject *v)
{
	if (v->ob_ival >= 0)
	{
		return int_pos(v);
	}
	else
	{
		return int_neg(v);
	}
}

static int int_nonzero(PyIntObject *v)
{
	return v->ob_ival != 0;
}

static PyObject *int_invert(PyIntObject *v)
{
	return PyInt_FromLong(~v->ob_ival);
}

static PyObject *int_lshift(PyIntObject *v, PyIntObject *w)
{
	long a, b;
	CONVERT_TO_LONG(v, a);
	CONVERT_TO_LONG(w, b);
	if (b < 0) 
	{
		PyErr_SetString(PyExc_ValueError, "negative shift count");
		return NULL;
	}
	if (a == 0 || b == 0)
	{
		return int_pos(v);
	}
	if (b >= LONG_BIT) 
	{
		return PyInt_FromLong(0L);
	}
	a = (long)((unsigned long)a << b);
	return PyInt_FromLong(a);
}

static PyObject *int_rshift(PyIntObject *v, PyIntObject *w)
{
	long a, b;
	CONVERT_TO_LONG(v, a);
	CONVERT_TO_LONG(w, b);
	if (b < 0) 
	{
		PyErr_SetString(PyExc_ValueError, "negative shift count");
		return NULL;
	}
	if (a == 0 || b == 0)
	{
		return int_pos(v);
	}
	if (b >= LONG_BIT) 
	{
		if (a < 0)
		{
			a = -1;
		}
		else
		{
			a = 0;
		}
	}
	else 
	{
		a = Py_ARITHMETIC_RIGHT_SHIFT(long, a, b);
	}
	return PyInt_FromLong(a);
}

static PyObject *int_and(PyIntObject *v, PyIntObject *w)
{
	long a, b;
	CONVERT_TO_LONG(v, a);
	CONVERT_TO_LONG(w, b);
	return PyInt_FromLong(a & b);
}

static PyObject *int_xor(PyIntObject *v, PyIntObject *w)
{
	long a, b;
	CONVERT_TO_LONG(v, a);
	CONVERT_TO_LONG(w, b);
	return PyInt_FromLong(a ^ b);
}

static PyObject *int_or(PyIntObject *v, PyIntObject *w)
{
	long a, b;
	CONVERT_TO_LONG(v, a);
	CONVERT_TO_LONG(w, b);
	return PyInt_FromLong(a | b);
}

static int int_coerce(PyObject **pv, PyObject **pw)
{
	if (PyInt_Check(*pw)) 
	{
		Py_INCREF(*pv);
		Py_INCREF(*pw);
		return 0;
	}
	return 1;
}

static PyObject *int_int(PyIntObject *v)
{
	Py_INCREF(v);
	return (PyObject *)v;
}

static PyObject *int_long(PyIntObject *v)
{
	return PyLong_FromLong((v -> ob_ival));
}

static PyObject *int_float(PyIntObject *v)
{
	return PyFloat_FromDouble((double)(v -> ob_ival));
}

static PyObject *int_oct(PyIntObject *v)
{
	char buf[100];
	long x = v -> ob_ival;
	if (x == 0)
	{
		strcpy(buf, "0");
	}
	else
	{
		PyOS_snprintf(buf, sizeof(buf), "0%lo", x);
	}
	return PyString_FromString(buf);
}

static PyObject *int_hex(PyIntObject *v)
{
	char buf[100];
	long x = v -> ob_ival;
	PyOS_snprintf(buf, sizeof(buf), "0x%lx", x);
	return PyString_FromString(buf);
}

staticforward PyObject *int_subtype_new(PyTypeObject *type, PyObject *args, PyObject *kwds);

static PyObject *int_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	PyObject *x = NULL;
	int base = -909;
	static char *kwlist[] = {"x", "base", 0};

	if (type != &PyInt_Type)
	{
		return int_subtype_new(type, args, kwds);
	}
	if (!PyArg_ParseTupleAndKeywords(args, kwds, "|Oi:int", kwlist,
					 &x, &base))
	{
		return NULL;
	}
	if (x == NULL)
	{
		return PyInt_FromLong(0L);
	}
	if (base == -909)
	{
		return PyNumber_Int(x);
	}
	if (PyString_Check(x))
	{
		return PyInt_FromString(PyString_AS_STRING(x), NULL, base);
	}
	if (PyUnicode_Check(x))
	{
		return PyInt_FromUnicode(PyUnicode_AS_UNICODE(x),
					 PyUnicode_GET_SIZE(x),
					 base);
	}
	PyErr_SetString(PyExc_TypeError,
			"int() can't convert non-string with explicit base");
	return NULL;
}

static PyObject *int_subtype_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	PyObject *tmp, *new;

	assert(PyType_IsSubtype(type, &PyInt_Type));
	tmp = int_new(&PyInt_Type, args, kwds);
	if (tmp == NULL)
	{
		return NULL;
	}
	assert(PyInt_Check(tmp));
	new = type->tp_alloc(type, 0);
	if (new == NULL)
	{
		return NULL;
	}
	((PyIntObject *)new)->ob_ival = ((PyIntObject *)tmp)->ob_ival;
	Py_DECREF(tmp);
	return new;
}

static char int_doc[] =
	"int(x[, base]) -> integer\n"
	"\n"
	"Convert a string or number to an integer, if possible.  A floating point\n"
	"argument will be truncated towards zero (this does not include a string\n"
	"representation of a floating point number!)  When converting a string, use\n"
	"the optional base.  It is an error to supply a base when converting a\n"
	"non-string.";

static PyNumberMethods int_as_number = {
	(binaryfunc)int_add,	
	(binaryfunc)int_sub,	
	(binaryfunc)int_mul,	
	(binaryfunc)int_classic_div,
	(binaryfunc)int_mod,	
	(binaryfunc)int_divmod,	
	(ternaryfunc)int_pow,	
	(unaryfunc)int_neg,	
	(unaryfunc)int_pos,	
	(unaryfunc)int_abs,	
	(inquiry)int_nonzero,
	(unaryfunc)int_invert,
	(binaryfunc)int_lshift,
	(binaryfunc)int_rshift,
	(binaryfunc)int_and,
	(binaryfunc)int_xor,
	(binaryfunc)int_or,
	int_coerce,	
	(unaryfunc)int_int,
	(unaryfunc)int_long,
	(unaryfunc)int_float,
	(unaryfunc)int_oct,
	(unaryfunc)int_hex,
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
	(binaryfunc)int_div,	
	int_true_divide,
	0,			
	0,			
};

PyTypeObject PyInt_Type = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,
	"int",
	sizeof(PyIntObject),
	0,
	(destructor)int_dealloc,	
	(printfunc)int_print,
	0,					
	0,					
	(cmpfunc)int_compare,
	(reprfunc)int_repr,	
	&int_as_number,	
	0,				
	0,				
	(hashfunc)int_hash,
    0,
    (reprfunc)int_repr,			
	PyObject_GenericGetAttr,	
	0,					
	0,					
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_CHECKTYPES |
		Py_TPFLAGS_BASETYPE,
	int_doc,			
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
	int_new,				
	(destructor)int_free,   
};

void PyInt_Fini()
{
	PyIntObject *p;
	PyIntBlock *list, *next;
	int i;
	int bc, bf;	
	int irem, isum;

    PyIntObject **q;

    i = NSMALLNEGINTS + NSMALLPOSINTS;
    q = small_ints;
    while (--i >= 0) 
	{
        Py_XDECREF(*q);
        *q++ = NULL;
    }
	bc = 0;
	bf = 0;
	isum = 0;
	list = block_list;
	block_list = NULL;
	free_list = NULL;
	while (list != NULL) 
	{
		bc++;
		irem = 0;
		for (i = 0, p = &list->objects[0];
		     i < N_INTOBJECTS;
		     i++, p++) 
		{
			if (PyInt_CheckExact(p) && p->ob_refcnt != 0)
			{
				irem++;
			}
		}
		next = list->next;
		if (irem) 
		{
			list->next = block_list;
			block_list = list;
			for (i = 0, p = &list->objects[0];
			     i < N_INTOBJECTS;
			     i++, p++) 
			{
				if (!PyInt_CheckExact(p) ||
				    p->ob_refcnt == 0) 
				{
					p->ob_type = (struct _typeobject *)
						free_list;
					free_list = p;
				}
				else if (-NSMALLNEGINTS <= p->ob_ival &&
					 p->ob_ival < NSMALLPOSINTS &&
					 small_ints[p->ob_ival +
						    NSMALLNEGINTS] == NULL) 
				{
					Py_INCREF(p);
					small_ints[p->ob_ival +
						   NSMALLNEGINTS] = p;
				}
			}
		}
		else 
		{
			PyMem_FREE(list); 
			bf++;
		}
		isum += irem;
		list = next;
	}
	if (!Py_VerboseFlag)
	{
		return;
	}
	fprintf(stderr, "# cleanup ints");
	if (!isum) 
	{
		fprintf(stderr, "\n");
	}
	else 
	{
		fprintf(stderr,
			": %d unfreed int%s in %d out of %d block%s\n",
			isum, isum == 1 ? "" : "s",
			bc - bf, bc, bc == 1 ? "" : "s");
	}
	if (Py_VerboseFlag > 1) 
	{
		list = block_list;
		while (list != NULL) 
		{
			for (i = 0, p = &list->objects[0];
			     i < N_INTOBJECTS;
			     i++, p++) 
			{
				if (PyInt_CheckExact(p) && p->ob_refcnt != 0)
				{
					fprintf(stderr,
						"#   <int at %p, refcnt=%d, val=%ld>\n",
						p, p->ob_refcnt, p->ob_ival);
				}
			}
			list = list->next;
		}
	}
}
