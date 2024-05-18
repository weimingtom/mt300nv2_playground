//20170415
#include "python.h"
#include <ctype.h>

extern double fmod(double, double);
extern double pow(double, double);

#define BLOCK_SIZE	1000
#define BHEAD_SIZE	8
#define N_FLOATOBJECTS	((BLOCK_SIZE - BHEAD_SIZE) / sizeof(PyFloatObject))

struct _floatblock {
	struct _floatblock *next;
	PyFloatObject objects[N_FLOATOBJECTS];
};

typedef struct _floatblock PyFloatBlock;

static PyFloatBlock *block_list = NULL;
static PyFloatObject *free_list = NULL;

static PyFloatObject *fill_free_list()
{
	PyFloatObject *p, *q;
	p = (PyFloatObject *)PyMem_MALLOC(sizeof(PyFloatBlock));
	if (p == NULL)
	{
		return (PyFloatObject *)PyErr_NoMemory();
	}
	((PyFloatBlock *)p)->next = block_list;
	block_list = (PyFloatBlock *)p;
	p = &((PyFloatBlock *)p)->objects[0];
	q = p + N_FLOATOBJECTS;
	while (--q > p)
	{
		q->ob_type = (struct _typeobject *)(q - 1);
	}
	q->ob_type = NULL;
	return p + N_FLOATOBJECTS - 1;
}

PyObject *PyFloat_FromDouble(double fval)
{
	PyFloatObject *op;
	if (free_list == NULL) 
	{
		if ((free_list = fill_free_list()) == NULL)
		{
			return NULL;
		}
	}
	op = free_list;
	free_list = (PyFloatObject *)op->ob_type;
	PyObject_INIT(op, &PyFloat_Type);
	op->ob_fval = fval;
	return (PyObject *) op;
}

PyObject *PyFloat_FromString(PyObject *v, char **pend)
{
	const char *s, *last, *end;
	double x;
	char buffer[256]; 
	char s_buffer[256]; 
	int len;

	if (pend)
	{
		*pend = NULL;
	}
	if (PyString_Check(v)) 
	{
		s = PyString_AS_STRING(v);
		len = PyString_GET_SIZE(v);
	}
	else if (PyUnicode_Check(v)) 
	{
		if (PyUnicode_GET_SIZE(v) >= sizeof(s_buffer)) 
		{
			PyErr_SetString(PyExc_ValueError,
				"Unicode float() literal too long to convert");
			return NULL;
		}
		if (PyUnicode_EncodeDecimal(PyUnicode_AS_UNICODE(v),
					    PyUnicode_GET_SIZE(v),
					    s_buffer,
					    NULL))
		{
			return NULL;
		}
		s = s_buffer;
		len = (int)strlen(s);
	}
	else if (PyObject_AsCharBuffer(v, &s, &len)) 
	{
		PyErr_SetString(PyExc_TypeError,
				"float() argument must be a string or a number");
		return NULL;
	}

	last = s + len;
	while (*s && isspace(Py_CHARMASK(*s)))
	{
		s++;
	}
	if (*s == '\0') 
	{
		PyErr_SetString(PyExc_ValueError, "empty string for float()");
		return NULL;
	}
	PyFPE_START_PROTECT("strtod", return NULL)
	x = strtod(s, (char **)&end);
	PyFPE_END_PROTECT(x)
	errno = 0;
	if (end > last)
	{
		end = last;
	}
	if (end == s) 
	{
		PyOS_snprintf(buffer, sizeof(buffer),
			      "invalid literal for float(): %.200s", s);
		PyErr_SetString(PyExc_ValueError, buffer);
		return NULL;
	}
	while (*end && isspace(Py_CHARMASK(*end)))
	{
		end++;
	}
	if (*end != '\0') 
	{
		PyOS_snprintf(buffer, sizeof(buffer),
			      "invalid literal for float(): %.200s", s);
		PyErr_SetString(PyExc_ValueError, buffer);
		return NULL;
	}
	else if (end != last) 
	{
		PyErr_SetString(PyExc_ValueError,
				"null byte in argument for float()");
		return NULL;
	}
	if (x == 0.0) 
	{
		PyFPE_START_PROTECT("atof", return NULL)
		x = atof(s);
		PyFPE_END_PROTECT(x)
		errno = 0;
	}
	return PyFloat_FromDouble(x);
}

static void float_dealloc(PyFloatObject *op)
{
	if (PyFloat_CheckExact(op)) 
	{
		op->ob_type = (struct _typeobject *)free_list;
		free_list = op;
	}
	else
	{
		op->ob_type->tp_free((PyObject *)op);
	}
}

double PyFloat_AsDouble(PyObject *op)
{
	PyNumberMethods *nb;
	PyFloatObject *fo;
	double val;

	if (op && PyFloat_Check(op))
	{
		return PyFloat_AS_DOUBLE((PyFloatObject*) op);
	}

	if (op == NULL || (nb = op->ob_type->tp_as_number) == NULL ||
	    nb->nb_float == NULL) 
	{
		PyErr_BadArgument();
		return -1;
	}

	fo = (PyFloatObject*) (*nb->nb_float) (op);
	if (fo == NULL)
	{
		return -1;
	}
	if (!PyFloat_Check(fo)) 
	{
		PyErr_SetString(PyExc_TypeError,
				"nb_float should return float object");
		return -1;
	}

	val = PyFloat_AS_DOUBLE(fo);
	Py_DECREF(fo);

	return val;
}

static void format_float(char *buf, size_t buflen, PyFloatObject *v, int precision)
{
	char *cp;

	assert(PyFloat_Check(v));
	PyOS_snprintf(buf, buflen, "%.*g", precision, v->ob_fval);
	cp = buf;
	if (*cp == '-')
	{
		cp++;
	}
	for (; *cp != '\0'; cp++) 
	{
		if (!isdigit(Py_CHARMASK(*cp)))
		{
			break;
		}
	}
	if (*cp == '\0') 
	{
		*cp++ = '.';
		*cp++ = '0';
		*cp++ = '\0';
	}
}

void PyFloat_AsStringEx(char *buf, PyFloatObject *v, int precision)
{
	format_float(buf, 100, v, precision);
}

#define CONVERT_TO_DOUBLE(obj, dbl)			\
	if (PyFloat_Check(obj))				\
		dbl = PyFloat_AS_DOUBLE(obj);		\
	else if (convert_to_double(&(obj), &(dbl)) < 0)	\
		return obj;

static int convert_to_double(PyObject **v, double *dbl)
{
	PyObject *obj = *v;

	if (PyInt_Check(obj)) 
	{
		*dbl = (double)PyInt_AS_LONG(obj);
	}
	else if (PyLong_Check(obj)) 
	{
		*dbl = PyLong_AsDouble(obj);
		if (*dbl == -1.0 && PyErr_Occurred()) 
		{
			*v = NULL;
			return -1;
		}
	}
	else 
	{
		Py_INCREF(Py_NotImplemented);
		*v = Py_NotImplemented;
		return -1;
	}
	return 0;
}

#define PREC_REPR	17
#define PREC_STR	12

void PyFloat_AsString(char *buf, PyFloatObject *v)
{
	format_float(buf, 100, v, PREC_STR);
}

void PyFloat_AsReprString(char *buf, PyFloatObject *v)
{
	format_float(buf, 100, v, PREC_REPR);
}

static int float_print(PyFloatObject *v, FILE *fp, int flags)
{
	char buf[100];
	format_float(buf, sizeof(buf), v,
		     (flags & Py_PRINT_RAW) ? PREC_STR : PREC_REPR);
	fputs(buf, fp);
	return 0;
}

static PyObject *float_repr(PyFloatObject *v)
{
	char buf[100];
	format_float(buf, sizeof(buf), v, PREC_REPR);
	return PyString_FromString(buf);
}

static PyObject *float_str(PyFloatObject *v)
{
	char buf[100];
	format_float(buf, sizeof(buf), v, PREC_STR);
	return PyString_FromString(buf);
}

static int float_compare(PyFloatObject *v, PyFloatObject *w)
{
	double i = v->ob_fval;
	double j = w->ob_fval;
	return (i < j) ? -1 : (i > j) ? 1 : 0;
}

static long float_hash(PyFloatObject *v)
{
	return _Py_HashDouble(v->ob_fval);
}

static PyObject *float_add(PyObject *v, PyObject *w)
{
	double a,b;
	CONVERT_TO_DOUBLE(v, a);
	CONVERT_TO_DOUBLE(w, b);
	PyFPE_START_PROTECT("add", return 0)
	a = a + b;
	PyFPE_END_PROTECT(a)
	return PyFloat_FromDouble(a);
}

static PyObject *float_sub(PyObject *v, PyObject *w)
{
	double a,b;
	CONVERT_TO_DOUBLE(v, a);
	CONVERT_TO_DOUBLE(w, b);
	PyFPE_START_PROTECT("subtract", return 0)
	a = a - b;
	PyFPE_END_PROTECT(a)
	return PyFloat_FromDouble(a);
}

static PyObject *float_mul(PyObject *v, PyObject *w)
{
	double a,b;
	CONVERT_TO_DOUBLE(v, a);
	CONVERT_TO_DOUBLE(w, b);
	PyFPE_START_PROTECT("multiply", return 0)
	a = a * b;
	PyFPE_END_PROTECT(a)
	return PyFloat_FromDouble(a);
}

static PyObject *float_div(PyObject *v, PyObject *w)
{
	double a,b;
	CONVERT_TO_DOUBLE(v, a);
	CONVERT_TO_DOUBLE(w, b);
	if (b == 0.0) 
	{
		PyErr_SetString(PyExc_ZeroDivisionError, "float division");
		return NULL;
	}
	PyFPE_START_PROTECT("divide", return 0)
	a = a / b;
	PyFPE_END_PROTECT(a)
	return PyFloat_FromDouble(a);
}

static PyObject *float_classic_div(PyObject *v, PyObject *w)
{
	double a,b;
	CONVERT_TO_DOUBLE(v, a);
	CONVERT_TO_DOUBLE(w, b);
	if (Py_DivisionWarningFlag >= 2 &&
	    PyErr_Warn(PyExc_DeprecationWarning, "classic float division") < 0)
	{
		return NULL;
	}
	if (b == 0.0) 
	{
		PyErr_SetString(PyExc_ZeroDivisionError, "float division");
		return NULL;
	}
	PyFPE_START_PROTECT("divide", return 0)
	a = a / b;
	PyFPE_END_PROTECT(a)
	return PyFloat_FromDouble(a);
}

static PyObject *float_rem(PyObject *v, PyObject *w)
{
	double vx, wx;
	double mod;
 	CONVERT_TO_DOUBLE(v, vx);
 	CONVERT_TO_DOUBLE(w, wx);
	if (wx == 0.0) 
	{
		PyErr_SetString(PyExc_ZeroDivisionError, "float modulo");
		return NULL;
	}
	PyFPE_START_PROTECT("modulo", return 0)
	mod = fmod(vx, wx);
	if (mod && ((wx < 0) != (mod < 0))) 
	{
		mod += wx;
	}
	PyFPE_END_PROTECT(mod)
	return PyFloat_FromDouble(mod);
}

static PyObject *float_divmod(PyObject *v, PyObject *w)
{
	double vx, wx;
	double div, mod, floordiv;
 	CONVERT_TO_DOUBLE(v, vx);
 	CONVERT_TO_DOUBLE(w, wx);
	if (wx == 0.0) 
	{
		PyErr_SetString(PyExc_ZeroDivisionError, "float divmod()");
		return NULL;
	}
	PyFPE_START_PROTECT("divmod", return 0)
	mod = fmod(vx, wx);
	div = (vx - mod) / wx;
	if (mod) 
	{
		if ((wx < 0) != (mod < 0)) 
		{
			mod += wx;
			div -= 1.0;
		}
	}
	else 
	{
		mod *= mod; 
		if (wx < 0.0)
		{
			mod = -mod;
		}
	}
	if (div) 
	{
		floordiv = floor(div);
		if (div - floordiv > 0.5)
		{
			floordiv += 1.0;
		}
	}
	else 
	{
		div *= div;	
		floordiv = div * vx / wx; 
	}
	PyFPE_END_PROTECT(floordiv)
	return Py_BuildValue("(dd)", floordiv, mod);
}

static PyObject *float_floor_div(PyObject *v, PyObject *w)
{
	PyObject *t, *r;

	t = float_divmod(v, w);
	if (t == NULL || t == Py_NotImplemented)
	{
		return t;
	}
	assert(PyTuple_CheckExact(t));
	r = PyTuple_GET_ITEM(t, 0);
	Py_INCREF(r);
	Py_DECREF(t);
	return r;
}

static PyObject *float_pow(PyObject *v, PyObject *w, PyObject *z)
{
	double iv, iw, ix;

	if ((PyObject *)z != Py_None) 
	{
		PyErr_SetString(PyExc_TypeError, "pow() 3rd argument not "
			"allowed unless all arguments are integers");
		return NULL;
	}

	CONVERT_TO_DOUBLE(v, iv);
	CONVERT_TO_DOUBLE(w, iw);

	if (iw == 0) 
	{ 		
		PyFPE_START_PROTECT("pow", return NULL)
		if ((PyObject *)z != Py_None) 
		{
			double iz;
			CONVERT_TO_DOUBLE(z, iz);
			ix = fmod(1.0, iz);
			if (ix != 0 && iz < 0)
			{
				ix += iz;
			}
		}
		else
		{
			ix = 1.0;
		}
		PyFPE_END_PROTECT(ix)
		return PyFloat_FromDouble(ix);
	}
	if (iv == 0.0) 
	{  
		if (iw < 0.0) 
		{
			PyErr_SetString(PyExc_ZeroDivisionError,
					"0.0 cannot be raised to a negative power");
			return NULL;
		}
		return PyFloat_FromDouble(0.0);
	}
	if (iv < 0.0 && iw != floor(iw)) 
	{
		PyErr_SetString(PyExc_ValueError,
				"negative number cannot be raised to a fractional power");
		return NULL;
	}
	errno = 0;
	PyFPE_START_PROTECT("pow", return NULL)
	ix = pow(iv, iw);
	PyFPE_END_PROTECT(ix)
	Py_ADJUST_ERANGE1(ix);
	if (errno != 0) 
	{
		assert(errno == ERANGE);
		PyErr_SetFromErrno(PyExc_OverflowError);
		return NULL;
	}
	return PyFloat_FromDouble(ix);
}

static PyObject *float_neg(PyFloatObject *v)
{
	return PyFloat_FromDouble(-v->ob_fval);
}

static PyObject *float_pos(PyFloatObject *v)
{
	if (PyFloat_CheckExact(v)) 
	{
		Py_INCREF(v);
		return (PyObject *)v;
	}
	else
	{
		return PyFloat_FromDouble(v->ob_fval);
	}
}

static PyObject *float_abs(PyFloatObject *v)
{
	return PyFloat_FromDouble(fabs(v->ob_fval));
}

static int float_nonzero(PyFloatObject *v)
{
	return v->ob_fval != 0.0;
}

static int float_coerce(PyObject **pv, PyObject **pw)
{
	if (PyInt_Check(*pw)) 
	{
		long x = PyInt_AsLong(*pw);
		*pw = PyFloat_FromDouble((double)x);
		Py_INCREF(*pv);
		return 0;
	}
	else if (PyLong_Check(*pw)) 
	{
		*pw = PyFloat_FromDouble(PyLong_AsDouble(*pw));
		Py_INCREF(*pv);
		return 0;
	}
	else if (PyFloat_Check(*pw)) 
	{
		Py_INCREF(*pv);
		Py_INCREF(*pw);
		return 0;
	}
	return 1;
}

static PyObject *float_int(PyObject *v)
{
	double x = PyFloat_AsDouble(v);
	double wholepart;
	long aslong;

	modf(x, &wholepart);
	aslong = (long)wholepart;
	if ((double)aslong == wholepart)
	{
		return PyInt_FromLong(aslong);
	}
	PyErr_SetString(PyExc_OverflowError, "float too large to convert");
	return NULL;
}

static PyObject *float_long(PyObject *v)
{
	double x = PyFloat_AsDouble(v);
	return PyLong_FromDouble(x);
}

static PyObject *float_float(PyObject *v)
{
	Py_INCREF(v);
	return v;
}

staticforward PyObject *float_subtype_new(PyTypeObject *type, PyObject *args, PyObject *kwds);

static PyObject *float_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	PyObject *x = Py_False;
	static char *kwlist[] = {"x", 0};

	if (type != &PyFloat_Type)
	{
		return float_subtype_new(type, args, kwds);
	}
	if (!PyArg_ParseTupleAndKeywords(args, kwds, "|O:float", kwlist, &x))
	{
		return NULL;
	}
	if (PyString_Check(x))
	{
		return PyFloat_FromString(x, NULL);
	}
	return PyNumber_Float(x);
}

static PyObject *float_subtype_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	PyObject *tmp, *new;

	assert(PyType_IsSubtype(type, &PyFloat_Type));
	tmp = float_new(&PyFloat_Type, args, kwds);
	if (tmp == NULL)
	{
		return NULL;
	}
	assert(PyFloat_CheckExact(tmp));
	new = type->tp_alloc(type, 0);
	if (new == NULL)
	{
		return NULL;
	}
	((PyFloatObject *)new)->ob_fval = ((PyFloatObject *)tmp)->ob_fval;
	Py_DECREF(tmp);
	return new;
}

static char float_doc[] =
	"float(x) -> floating point number\n"
	"\n"
	"Convert a string or number to a floating point number, if possible.";


static PyNumberMethods float_as_number = {
	(binaryfunc)float_add, 
	(binaryfunc)float_sub, 
	(binaryfunc)float_mul, 
	(binaryfunc)float_classic_div, 
	(binaryfunc)float_rem, 
	(binaryfunc)float_divmod, 
	(ternaryfunc)float_pow, 
	(unaryfunc)float_neg, 
	(unaryfunc)float_pos, 
	(unaryfunc)float_abs, 
	(inquiry)float_nonzero, 
	0,		
	0,		
	0,		
	0,		
	0,		
	0,		
	(coercion)float_coerce,
	(unaryfunc)float_int, 
	(unaryfunc)float_long, 
	(unaryfunc)float_float,
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
	float_floor_div, 
	float_div,	
	0,		
	0,		
};

PyTypeObject PyFloat_Type = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,
	"float",
	sizeof(PyFloatObject),
	0,
	(destructor)float_dealloc,		
	(printfunc)float_print, 		
	0,					
	0,					
	(cmpfunc)float_compare, 
	(reprfunc)float_repr,	
	&float_as_number,		
	0,					
	0,					
	(hashfunc)float_hash,
	0,					
	(reprfunc)float_str,	
	PyObject_GenericGetAttr,	
	0,					
	0,					
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_CHECKTYPES |
		Py_TPFLAGS_BASETYPE, 
	float_doc,				
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
	float_new,			
};

void PyFloat_Fini()
{
	PyFloatObject *p;
	PyFloatBlock *list, *next;
	int i;
	int bc, bf;	
	int frem, fsum;	

	bc = 0;
	bf = 0;
	fsum = 0;
	list = block_list;
	block_list = NULL;
	free_list = NULL;
	while (list != NULL) 
	{
		bc++;
		frem = 0;
		for (i = 0, p = &list->objects[0];
		     i < N_FLOATOBJECTS;
		     i++, p++) 
		{
			if (PyFloat_CheckExact(p) && p->ob_refcnt != 0)
			{
				frem++;
			}
		}
		next = list->next;
		if (frem) 
		{
			list->next = block_list;
			block_list = list;
			for (i = 0, p = &list->objects[0];
			     i < N_FLOATOBJECTS;
			     i++, p++) 
			{
				if (!PyFloat_CheckExact(p) ||
				    p->ob_refcnt == 0) 
				{
					p->ob_type = (struct _typeobject *)
						free_list;
					free_list = p;
				}
			}
		}
		else 
		{
			PyMem_FREE(list); 
			bf++;
		}
		fsum += frem;
		list = next;
	}
	if (!Py_VerboseFlag)
	{
		return;
	}
	fprintf(stderr, "# cleanup floats");
	if (!fsum) 
	{
		fprintf(stderr, "\n");
	}
	else 
	{
		fprintf(stderr,
			": %d unfreed float%s in %d out of %d block%s\n",
			fsum, fsum == 1 ? "" : "s",
			bc - bf, bc, bc == 1 ? "" : "s");
	}
	if (Py_VerboseFlag > 1) 
	{
		list = block_list;
		while (list != NULL) 
		{
			for (i = 0, p = &list->objects[0];
			     i < N_FLOATOBJECTS;
			     i++, p++) 
			{
				if (PyFloat_CheckExact(p) &&
				    p->ob_refcnt != 0) 
				{
					char buf[100];
					PyFloat_AsString(buf, p);
					fprintf(stderr,
						"#   <float at %p, refcnt=%d, val=%s>\n",
						p, p->ob_refcnt, buf);
				}
			}
			list = list->next;
		}
	}
}
