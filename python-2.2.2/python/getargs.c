//20180111
//20180123
#include "python.h"
#include <ctype.h>

int PyArg_Parse(PyObject *, char *, ...);
int PyArg_ParseTuple(PyObject *, char *, ...);
int PyArg_VaParse(PyObject *, char *, va_list);

int PyArg_ParseTupleAndKeywords(PyObject *, PyObject *,
				char *, char **, ...);

static int vgetargs1(PyObject *, char *, va_list *, int);
static void seterror(int, char *, int *, char *, char *);
static char *convertitem(PyObject *, char **, va_list *, int *, char *, 
			 size_t);
static char *converttuple(PyObject *, char **, va_list *,
			  int *, char *, size_t, int);
static char *convertsimple(PyObject *, char **, va_list *, char *, size_t);
static int convertbuffer(PyObject *, void **p, char **);

static int vgetargskeywords(PyObject *, PyObject *,
			    char *, char **, va_list *);
static char *skipitem(char **, va_list *);

int PyArg_Parse(PyObject *args, char *format, ...)
{
	int retval;
	va_list va;
	
	va_start(va, format);
	retval = vgetargs1(args, format, &va, 1);
	va_end(va);
	return retval;
}

int PyArg_ParseTuple(PyObject *args, char *format, ...)
{
	int retval;
	va_list va;
	
	va_start(va, format);
	retval = vgetargs1(args, format, &va, 0);
	va_end(va);
	return retval;
}

int PyArg_VaParse(PyObject *args, char *format, va_list va)
{
	va_list lva;
	lva = va;
	return vgetargs1(args, format, &lva, 0);
}

static int vgetargs1(PyObject *args, char *format, va_list *p_va, int compat)
{
	char msgbuf[256];
	int levels[32];
	char *fname = NULL;
	char *message = NULL;
	int min = -1;
	int max = 0;
	int level = 0;
	int endfmt = 0;
	char *formatsave = format;
	int i, len;
	char *msg;
	
	assert(compat || (args != (PyObject*)NULL));

	while (endfmt == 0) 
	{
		int c = *format++;
		switch (c) 
		{
		case '(':
			if (level == 0)
			{
				max++;
			}
			level++;
			break;
		
		case ')':
			if (level == 0)
			{
				Py_FatalError("excess ')' in getargs format");
			}
			else
			{
				level--;
			}
			break;
		
		case '\0':
			endfmt = 1;
			break;

		case ':':
			fname = format;
			endfmt = 1;
			break;

		case ';':
			message = format;
			endfmt = 1;
			break;

		default:
			if (level == 0) 
			{
				if (c == 'O')
				{
					max++;
				}
				else if (isalpha(c)) 
				{
					if (c != 'e')
					{
						max++;
					}
				} 
				else if (c == '|')
				{
					min = max;
				}
			}
			break;
		}
	}
	
	if (level != 0)
	{
		Py_FatalError("missing ')' in getargs format");
	}

	if (min < 0)
	{
		min = max;
	}

	format = formatsave;
	
	if (compat) 
	{
		if (max == 0) 
		{
			if (args == NULL)
			{
				return 1;
			}
			PyOS_snprintf(msgbuf, sizeof(msgbuf),
				      "%.200s%s takes no arguments",
				      fname==NULL ? "function" : fname,
				      fname==NULL ? "" : "()");
			PyErr_SetString(PyExc_TypeError, msgbuf);
			return 0;
		}
		else if (min == 1 && max == 1) 
		{
			if (args == NULL) 
			{
				PyOS_snprintf(msgbuf, sizeof(msgbuf),
				      "%.200s%s takes at least one argument",
					      fname==NULL ? "function" : fname,
					      fname==NULL ? "" : "()");
				PyErr_SetString(PyExc_TypeError, msgbuf);
				return 0;
			}
			msg = convertitem(args, &format, p_va, levels, msgbuf,
					  sizeof(msgbuf));
			if (msg == NULL)
			{
				return 1;
			}
			seterror(levels[0], msg, levels + 1, fname, message);
			return 0;
		}
		else 
		{
			PyErr_SetString(PyExc_SystemError,
			    "old style getargs format uses new features");
			return 0;
		}
	}
	
	if (!PyTuple_Check(args)) 
	{
		PyErr_SetString(PyExc_SystemError,
		    "new style getargs format but argument is not a tuple");
		return 0;
	}
	
	len = PyTuple_GET_SIZE(args);
	
	if (len < min || max < len) 
	{
		if (message == NULL) 
		{
			PyOS_snprintf(msgbuf, sizeof(msgbuf),
				      "%.150s%s takes %s %d argument%s "
				      "(%d given)",
				      fname==NULL ? "function" : fname,
				      fname==NULL ? "" : "()",
				      min==max ? "exactly"
				      : len < min ? "at least" : "at most",
				      len < min ? min : max,
				      (len < min ? min : max) == 1 ? "" : "s",
				      len);
			message = msgbuf;
		}
		PyErr_SetString(PyExc_TypeError, message);
		return 0;
	}
	
	for (i = 0; i < len; i++) 
	{
		if (*format == '|')
		{
			format++;
		}
		msg = convertitem(PyTuple_GET_ITEM(args, i), &format, p_va,
				  levels, msgbuf, sizeof(msgbuf));
		if (msg) 
		{
			seterror(i + 1, msg, levels, fname, message);
			return 0;
		}
	}

	if (*format != '\0' && !isalpha((int)(*format)) &&
	    *format != '(' &&
	    *format != '|' && *format != ':' && *format != ';') 
	{
		PyErr_Format(PyExc_SystemError,
			     "bad format string: %.200s", formatsave);
		return 0;
	}
	
	return 1;
}

static void seterror(int iarg, char *msg, int *levels, char *fname, char *message)
{
	char buf[512];
	int i;
	char *p = buf;

	if (PyErr_Occurred())
	{
		return;
	}
	else if (message == NULL) 
	{
		if (fname != NULL) 
		{
			PyOS_snprintf(p, sizeof(buf), "%.200s() ", fname);
			p += strlen(p);
		}
		if (iarg != 0) 
		{
			PyOS_snprintf(p, sizeof(buf) - (p - buf),
				      "argument %d", iarg);
			i = 0;
			p += strlen(p);
			while (levels[i] > 0 && (int)(p - buf) < 220) 
			{
				PyOS_snprintf(p, sizeof(buf) - (buf - p),
					      ", item %d", levels[i]-1);
				p += strlen(p);
				i++;
			}
		}
		else 
		{
			PyOS_snprintf(p, sizeof(buf) - (p - buf), "argument");
			p += strlen(p);
		}
		PyOS_snprintf(p, sizeof(buf) - (p - buf), " %.256s", msg);
		message = buf;
	}
	PyErr_SetString(PyExc_TypeError, message);
}

static char *converttuple(PyObject *arg, char **p_format, va_list *p_va, int *levels,
	     char *msgbuf, size_t bufsize, int toplevel)
{
	int level = 0;
	int n = 0;
	char *format = *p_format;
	int i;
	
	for (;;) 
	{
		int c = *format++;
		if (c == '(') 
		{
			if (level == 0)
			{
				n++;
			}
			level++;
		}
		else if (c == ')') 
		{
			if (level == 0)
			{
				break;
			}
			level--;
		}
		else if (c == ':' || c == ';' || c == '\0')
		{
			break;
		}
		else if (level == 0 && isalpha(c))
		{
			n++;
		}
	}
	
	if (!PySequence_Check(arg) || PyString_Check(arg)) 
	{
		levels[0] = 0;
		PyOS_snprintf(msgbuf, bufsize,
			      toplevel ? "expected %d arguments, not %.50s" :
			              "must be %d-item sequence, not %.50s",
			      n, 
			      arg == Py_None ? "None" : arg->ob_type->tp_name);
		return msgbuf;
	}
	
	if ((i = PySequence_Size(arg)) != n) 
	{
		levels[0] = 0;
		PyOS_snprintf(msgbuf, bufsize,
			      toplevel ? "expected %d arguments, not %d" :
			             "must be sequence of length %d, not %d",
			      n, i);
		return msgbuf;
	}

	format = *p_format;
	for (i = 0; i < n; i++) 
	{
		char *msg;
		PyObject *item;
		item = PySequence_GetItem(arg, i);
		msg = convertitem(item, &format, p_va, levels+1, msgbuf,
				  bufsize);
		Py_XDECREF(item);
		if (msg != NULL) 
		{
			levels[0] = i+1;
			return msg;
		}
	}

	*p_format = format;
	return NULL;
}

static char *convertitem(PyObject *arg, char **p_format, va_list *p_va, int *levels,
	    char *msgbuf, size_t bufsize)
{
	char *msg;
	char *format = *p_format;
	
	if (*format == '(') 
	{
		format++;
		msg = converttuple(arg, &format, p_va, levels, msgbuf, 
				   bufsize, 0);
		if (msg == NULL)
		{
			format++;
		}
	}
	else 
	{
		msg = convertsimple(arg, &format, p_va, msgbuf, bufsize);
		if (msg != NULL)
		{
			levels[0] = 0;
		}
	}
	if (msg == NULL)
	{
		*p_format = format;
	}
	return msg;
}



#define UNICODE_DEFAULT_ENCODING(arg) \
        _PyUnicode_AsDefaultEncodedString(arg, NULL)

static char *converterr(char *expected, PyObject *arg, char *msgbuf, size_t bufsize)
{
	assert(expected != NULL);
	assert(arg != NULL); 
	PyOS_snprintf(msgbuf, bufsize,
		      "must be %.50s, not %.50s", expected,
		      arg == Py_None ? "None" : arg->ob_type->tp_name);
	return msgbuf;
}

#define CONV_UNICODE "(unicode conversion error)"

static char *convertsimple(PyObject *arg, char **p_format, va_list *p_va, char *msgbuf,
	      size_t bufsize)
{
	char *format = *p_format;
	char c = *format++;
	PyObject *uarg;
	
	switch (c) 
	{
	case 'b': 
		{
			char *p = va_arg(*p_va, char *);
			long ival = PyInt_AsLong(arg);
			if (ival == -1 && PyErr_Occurred())
			{
				return converterr("integer<b>", arg, msgbuf, bufsize);
			}
			else if (ival < 0) 
			{
				PyErr_SetString(PyExc_OverflowError,
					"unsigned byte integer is less than minimum");
				return converterr("integer<b>", arg, msgbuf, bufsize);
			}
			else if (ival > UCHAR_MAX) 
			{
				PyErr_SetString(PyExc_OverflowError,
					"unsigned byte integer is greater than maximum");
				return converterr("integer<b>", arg, msgbuf, bufsize);
			}
			else
			{
				*p = (unsigned char) ival;
			}
			break;
		}
	
	case 'B': 
		{  
			char *p = va_arg(*p_va, char *);
			long ival = PyInt_AsLong(arg);
			if (ival == -1 && PyErr_Occurred())
			{
				return converterr("integer<b>", arg, msgbuf, bufsize);
			}
			else if (ival < SCHAR_MIN) 
			{
				PyErr_SetString(PyExc_OverflowError,
					"byte-sized integer bitfield is less than minimum");
				return converterr("integer<B>", arg, msgbuf, bufsize);
			}
			else if (ival > (int)UCHAR_MAX) 
			{
				PyErr_SetString(PyExc_OverflowError,
					"byte-sized integer bitfield is greater than maximum");
				return converterr("integer<B>", arg, msgbuf, bufsize);
			}
			else
			{
				*p = (unsigned char) ival;
			}
			break;
		}
	
	case 'h': 
		{
			short *p = va_arg(*p_va, short *);
			long ival = PyInt_AsLong(arg);
			if (ival == -1 && PyErr_Occurred())
			{
				return converterr("integer<h>", arg, msgbuf, bufsize);
			}
			else if (ival < SHRT_MIN) 
			{
				PyErr_SetString(PyExc_OverflowError,
					"signed short integer is less than minimum");
				return converterr("integer<h>", arg, msgbuf, bufsize);
			}
			else if (ival > SHRT_MAX) 
			{
				PyErr_SetString(PyExc_OverflowError,
					"signed short integer is greater than maximum");
				return converterr("integer<h>", arg, msgbuf, bufsize);
			}
			else
			{
				*p = (short) ival;
			}
			break;
		}
	
	case 'H': 
		{
			unsigned short *p = va_arg(*p_va, unsigned short *);
			long ival = PyInt_AsLong(arg);
			if (ival == -1 && PyErr_Occurred())
			{
				return converterr("integer<H>", arg, msgbuf, bufsize);
			}
			else if (ival < SHRT_MIN) 
			{
				PyErr_SetString(PyExc_OverflowError,
					"short integer bitfield is less than minimum");
				return converterr("integer<H>", arg, msgbuf, bufsize);
			}
			else if (ival > USHRT_MAX) 
			{
				PyErr_SetString(PyExc_OverflowError,
					"short integer bitfield is greater than maximum");
				return converterr("integer<H>", arg, msgbuf, bufsize);
			}
			else
			{
				*p = (unsigned short) ival;
			}
			break;
		}
	
	case 'i': 
		{
			int *p = va_arg(*p_va, int *);
			long ival = PyInt_AsLong(arg);
			if (ival == -1 && PyErr_Occurred())
			{
				return converterr("integer<i>", arg, msgbuf, bufsize);
			}
			else if (ival > INT_MAX) 
			{
				PyErr_SetString(PyExc_OverflowError,
					"signed integer is greater than maximum");
				return converterr("integer<i>", arg, msgbuf, bufsize);
			}
			else if (ival < INT_MIN) 
			{
				PyErr_SetString(PyExc_OverflowError,
					"signed integer is less than minimum");
				return converterr("integer<i>", arg, msgbuf, bufsize);
			}
			else
			{
				*p = ival;
			}
			break;
		}

	case 'l': 
		{
			long *p = va_arg(*p_va, long *);
			long ival = PyInt_AsLong(arg);
			if (ival == -1 && PyErr_Occurred())
			{
				return converterr("integer<l>", arg, msgbuf, bufsize);
			}
			else
			{
				*p = ival;
			}
			break;
		}
	
	case 'L': 
		{
			LONG_LONG *p = va_arg( *p_va, LONG_LONG * );
			LONG_LONG ival = PyLong_AsLongLong( arg );
			if( ival == (LONG_LONG)-1 && PyErr_Occurred() ) 
			{
				return converterr("long<L>", arg, msgbuf, bufsize);
			} 
			else 
			{
				*p = ival;
			}
			break;
		}

	case 'f': 
		{
			float *p = va_arg(*p_va, float *);
			double dval = PyFloat_AsDouble(arg);
			if (PyErr_Occurred())
			{
				return converterr("float<f>", arg, msgbuf, bufsize);
			}
			else
			{
				*p = (float) dval;
			}
			break;
		}
	
	case 'd': 
		{
			double *p = va_arg(*p_va, double *);
			double dval = PyFloat_AsDouble(arg);
			if (PyErr_Occurred())
			{
				return converterr("float<d>", arg, msgbuf, bufsize);
			}
			else
			{
				*p = dval;
			}
			break;
		}
	
	case 'D': 
		{
			Py_complex *p = va_arg(*p_va, Py_complex *);
			Py_complex cval;
			cval = PyComplex_AsCComplex(arg);
			if (PyErr_Occurred())
			{
				return converterr("complex<D>", arg, msgbuf, bufsize);
			}
			else
			{
				*p = cval;
			}
			break;
		}

	case 'c': 
		{
			char *p = va_arg(*p_va, char *);
			if (PyString_Check(arg) && PyString_Size(arg) == 1)
			{
				*p = PyString_AS_STRING(arg)[0];
			}
			else
			{
				return converterr("char", arg, msgbuf, bufsize);
			}
			break;
		}
	
	case 's': 
		{
			if (*format == '#') 
			{
				void **p = (void **)va_arg(*p_va, char **);
				int *q = va_arg(*p_va, int *);
				
				if (PyString_Check(arg)) 
				{
					*p = PyString_AS_STRING(arg);
					*q = PyString_GET_SIZE(arg);
				}
				else if (PyUnicode_Check(arg)) 
				{
					uarg = UNICODE_DEFAULT_ENCODING(arg);
					if (uarg == NULL)
					{
						return converterr(CONV_UNICODE,
								  arg, msgbuf, bufsize);
					}
					*p = PyString_AS_STRING(uarg);
					*q = PyString_GET_SIZE(uarg);
				}
				else 
				{
					char *buf;
					int count = convertbuffer(arg, p, &buf);
					if (count < 0)
					{
						return converterr(buf, arg, msgbuf, bufsize);
					}
					*q = count;
				}
				format++;
			} 
			else 
			{
				char **p = va_arg(*p_va, char **);
				
				if (PyString_Check(arg))
				{
					*p = PyString_AS_STRING(arg);
				}
				else if (PyUnicode_Check(arg)) 
				{
					uarg = UNICODE_DEFAULT_ENCODING(arg);
					if (uarg == NULL)
					{
						return converterr(CONV_UNICODE,
								  arg, msgbuf, bufsize);
					}
					*p = PyString_AS_STRING(uarg);
				}
				else
				{
					return converterr("string", arg, msgbuf, bufsize);
				}
				if ((int)strlen(*p) != PyString_Size(arg))
				{
					return converterr("string without null bytes",
							  arg, msgbuf, bufsize);
				}
			}
			break;
		}

	case 'z': 
		{
			if (*format == '#') 
			{ 
				void **p = (void **)va_arg(*p_va, char **);
				int *q = va_arg(*p_va, int *);
				
				if (arg == Py_None) 
				{
					*p = 0;
					*q = 0;
				}
				else if (PyString_Check(arg)) 
				{
					*p = PyString_AS_STRING(arg);
					*q = PyString_GET_SIZE(arg);
				}
				else if (PyUnicode_Check(arg)) 
				{
					uarg = UNICODE_DEFAULT_ENCODING(arg);
					if (uarg == NULL)
					{
						return converterr(CONV_UNICODE,
								  arg, msgbuf, bufsize);
					}
					*p = PyString_AS_STRING(uarg);
					*q = PyString_GET_SIZE(uarg);
				}
				else 
				{ 
					char *buf;
					int count = convertbuffer(arg, p, &buf);
					if (count < 0)
					{
						return converterr(buf, arg, msgbuf, bufsize);
					}
					*q = count;
				}
				format++;
			} 
			else 
			{
				char **p = va_arg(*p_va, char **);
				
				if (arg == Py_None)
				{
					*p = 0;
				}
				else if (PyString_Check(arg))
				{
					*p = PyString_AS_STRING(arg);
				}
				else if (PyUnicode_Check(arg)) 
				{
					uarg = UNICODE_DEFAULT_ENCODING(arg);
					if (uarg == NULL)
					{
						return converterr(CONV_UNICODE,
								  arg, msgbuf, bufsize);
					}
					*p = PyString_AS_STRING(uarg);
				}
				else
				{
					return converterr("string or None", 
							  arg, msgbuf, bufsize);
				}
				if (*format == '#') 
				{
					int *q = va_arg(*p_va, int *);
					if (arg == Py_None)
					{
						*q = 0;
					}
					else
					{
						*q = PyString_Size(arg);
					}
					format++;
				}
				else if (*p != NULL &&
					 (int)strlen(*p) != PyString_Size(arg))
				{
					return converterr(
							"string without null bytes or None", 
							arg, msgbuf, bufsize);
				}
			}
			break;
		}
	
	case 'e': 
		{
			char **buffer;
			const char *encoding;
			PyObject *s;
			int size, recode_strings;

			encoding = (const char *)va_arg(*p_va, const char *);
			if (encoding == NULL)
			{
				encoding = PyUnicode_GetDefaultEncoding();
			}
			if (*format == 's')
			{
				recode_strings = 1;
			}
			else if (*format == 't')
			{
				recode_strings = 0;
			}
			else
			{
				return converterr(
					"(unknown parser marker combination)",
					arg, msgbuf, bufsize);
			}
			buffer = (char **)va_arg(*p_va, char **);
			format++;
			if (buffer == NULL)
			{
				return converterr("(buffer is NULL)", 
						  arg, msgbuf, bufsize);
			}
			
			if (!recode_strings && PyString_Check(arg)) 
			{
				s = arg;
				Py_INCREF(s);
			}
			else 
			{
		    	PyObject *u;

				u = PyUnicode_FromObject(arg);
				if (u == NULL)
				{
					return converterr(
						"string or unicode or text buffer", 
						arg, msgbuf, bufsize);
				}

				s = PyUnicode_AsEncodedString(u,
								  encoding,
								  NULL);
				Py_DECREF(u);
				if (s == NULL)
				{
					return converterr("(encoding failed)",
							  arg, msgbuf, bufsize);
				}
				if (!PyString_Check(s)) 
				{
					Py_DECREF(s);
					return converterr(
						"(encoder failed to return a string)",
						arg, msgbuf, bufsize);
				}
			}
			size = PyString_GET_SIZE(s);

			if (*format == '#') 
			{
				int *buffer_len = va_arg(*p_va, int *);

				format++;
				if (buffer_len == NULL)
				{
					return converterr(
						"(buffer_len is NULL)",
						arg, msgbuf, bufsize);
				}
				if (*buffer == NULL) 
				{
					*buffer = PyMem_NEW(char, size + 1);
					if (*buffer == NULL) 
					{
						Py_DECREF(s);
						return converterr(
							"(memory error)",
							arg, msgbuf, bufsize);
					}
				} 
				else 
				{
					if (size + 1 > *buffer_len) 
					{
						Py_DECREF(s);
						return converterr(
							"(buffer overflow)", 
							arg, msgbuf, bufsize);
					}
				}
				memcpy(*buffer,
					   PyString_AS_STRING(s),
					   size + 1);
				*buffer_len = size;
			} 
			else 
			{
				if ((int)strlen(PyString_AS_STRING(s)) != size)
				{
					return converterr(
						"(encoded string without NULL bytes)",
						arg, msgbuf, bufsize);
				}
				*buffer = PyMem_NEW(char, size + 1);
				if (*buffer == NULL) 
				{
					Py_DECREF(s);
					return converterr("(memory error)",
							  arg, msgbuf, bufsize);
				}
				memcpy(*buffer,
					   PyString_AS_STRING(s),
					   size + 1);
			}
			Py_DECREF(s);
			break;
		}

	case 'u': 
		{
			if (*format == '#') 
			{
				void **p = (void **)va_arg(*p_va, char **);
				int *q = va_arg(*p_va, int *);
				char *buf;
				int count = convertbuffer(arg, p, &buf);

				if (count < 0)
				{
					return converterr(buf, arg, msgbuf, bufsize);
				}
				*q = count/(sizeof(Py_UNICODE)); 
				format++;
			} 
			else 
			{
				Py_UNICODE **p = va_arg(*p_va, Py_UNICODE **);
				
				if (PyUnicode_Check(arg))
				{
					*p = PyUnicode_AS_UNICODE(arg);
				}
				else
				{
					return converterr("unicode", arg, msgbuf, bufsize);
				}
			}
			break;
		}

	case 'S': 
		{ 
			PyObject **p = va_arg(*p_va, PyObject **);
			if (PyString_Check(arg))
			{
				*p = arg;
			}
			else
			{
				return converterr("string", arg, msgbuf, bufsize);
			}
			break;
		}
	
	case 'U': 
		{ 
			PyObject **p = va_arg(*p_va, PyObject **);
			if (PyUnicode_Check(arg))
			{
				*p = arg;
			}
			else
			{
				return converterr("unicode", arg, msgbuf, bufsize);
			}
			break;
		}
	
	case 'O': 
		{
			PyTypeObject *type;
			PyObject **p;
			if (*format == '!') 
			{
				type = va_arg(*p_va, PyTypeObject*);
				p = va_arg(*p_va, PyObject **);
				format++;
				if (PyType_IsSubtype(arg->ob_type, type))
				{
					*p = arg;
				}
				else
				{
					return converterr(type->tp_name, arg, msgbuf, bufsize);
				}
			}
			else if (*format == '?') 
			{
				inquiry pred = va_arg(*p_va, inquiry);
				p = va_arg(*p_va, PyObject **);
				format++;
				if ((*pred)(arg)) 
				{
					*p = arg;
				}
				else
				{
					return converterr("(unspecified)", 
							  arg, msgbuf, bufsize);
				}	
			}
			else if (*format == '&') 
			{
				typedef int (*converter)(PyObject *, void *);
				converter convert = va_arg(*p_va, converter);
				void *addr = va_arg(*p_va, void *);
				format++;
				if (! (*convert)(arg, addr))
				{
					return converterr("(unspecified)", 
							  arg, msgbuf, bufsize);
				}
			}
			else 
			{
				p = va_arg(*p_va, PyObject **);
				*p = arg;
			}
			break;
		}
		
	case 'w': 
		{ 
			void **p = va_arg(*p_va, void **);
			PyBufferProcs *pb = arg->ob_type->tp_as_buffer;
			int count;
				
			if (pb == NULL || 
				pb->bf_getwritebuffer == NULL ||
				pb->bf_getsegcount == NULL)
			{
				return converterr("read-write buffer", arg, msgbuf, bufsize);
			}
			if ((*pb->bf_getsegcount)(arg, NULL) != 1)
			{
				return converterr("single-segment read-write buffer", 
						  arg, msgbuf, bufsize);
			}
			if ((count = pb->bf_getwritebuffer(arg, 0, p)) < 0)
			{
				return converterr("(unspecified)", arg, msgbuf, bufsize);
			}
			if (*format == '#') 
			{
				int *q = va_arg(*p_va, int *);
				
				*q = count;
				format++;
			}
			break;
		}
		
	case 't': 
		{ 
			const char **p = va_arg(*p_va, const char **);
			PyBufferProcs *pb = arg->ob_type->tp_as_buffer;
			int count;
			
			if (*format++ != '#')
			{
				return converterr(
					"invalid use of 't' format character", 
					arg, msgbuf, bufsize);
			}
			if (!PyType_HasFeature(arg->ob_type,
						   Py_TPFLAGS_HAVE_GETCHARBUFFER) ||
				pb == NULL || pb->bf_getcharbuffer == NULL ||
				pb->bf_getsegcount == NULL)
			{
				return converterr(
					"string or read-only character buffer",
					arg, msgbuf, bufsize);
			}

			if (pb->bf_getsegcount(arg, NULL) != 1)
			{
				return converterr(
					"string or single-segment read-only buffer",
					arg, msgbuf, bufsize);
			}

			count = pb->bf_getcharbuffer(arg, 0, p);
			if (count < 0)
			{
				return converterr("(unspecified)", arg, msgbuf, bufsize);
			}
			*va_arg(*p_va, int *) = count;
			break;
		}

	default:
		return converterr("impossible<bad format char>", arg, msgbuf, bufsize);
	}
	
	*p_format = format;
	return NULL;
}

static int convertbuffer(PyObject *arg, void **p, char **errmsg)
{
	PyBufferProcs *pb = arg->ob_type->tp_as_buffer;
	int count;
	if (pb == NULL ||
	    pb->bf_getreadbuffer == NULL ||
	    pb->bf_getsegcount == NULL) 
	{
		*errmsg = "string or read-only buffer";
		return -1;
	}
	if ((*pb->bf_getsegcount)(arg, NULL) != 1) 
	{
		*errmsg = "string or single-segment read-only buffer";
		return -1;
	}
	if ((count = (*pb->bf_getreadbuffer)(arg, 0, p)) < 0) 
	{
		*errmsg = "(unspecified)";
	}
	return count;
}

int PyArg_ParseTupleAndKeywords(PyObject *args,
			    PyObject *keywords,
			    char *format, 
			    char **kwlist, ...)
{
	int retval;
	va_list va;

	if ((args == NULL || !PyTuple_Check(args)) ||
	    (keywords != NULL && !PyDict_Check(keywords)) ||
	    format == NULL ||
	    kwlist == NULL)
	{
		PyErr_BadInternalCall();
		return 0;
	}

	va_start(va, kwlist);
	retval = vgetargskeywords(args, keywords, format, kwlist, &va);	
	va_end(va);
	return retval;
}

static int vgetargskeywords(PyObject *args, PyObject *keywords, char *format,
	         char **kwlist, va_list *p_va)
{
	char msgbuf[512];
	int levels[32];
	char *fname, *message;
	int min, max;
	char *formatsave;
	int i, len, nargs, nkeywords;
	char *msg, **p;

	assert(args != NULL && PyTuple_Check(args));
	assert(keywords == NULL || PyDict_Check(keywords));
	assert(format != NULL);
	assert(kwlist != NULL);
	assert(p_va != NULL);

	fname = message = NULL;
	formatsave = format;
	p = kwlist;
	min = -1;
	max = 0;
	while ((i = *format++) != '\0') 
	{
		if (isalpha(i) && i != 'e') 
		{
			max++;
			if (*p == NULL) 
			{
				PyErr_SetString(PyExc_RuntimeError,
					"more argument specifiers than "
					"keyword list entries");
				return 0;
			}
			p++;
		}
		else if (i == '|')
		{
			min = max;
		}
		else if (i == ':') 
		{
			fname = format;
			break;
		}
		else if (i == ';') 
		{
			message = format;
			break;
		}
		else if (i == '(') 
		{
			PyErr_SetString(PyExc_RuntimeError,
				"tuple found in format when using keyword "
				"arguments");
			return 0;
		}
	}
	format = formatsave;
	if (*p != NULL) 
	{
		PyErr_SetString(PyExc_RuntimeError,
			"more keyword list entries than "
			"argument specifiers");
		return 0;
	}
	if (min < 0) 
	{
		min = max;
	}

	nargs = PyTuple_GET_SIZE(args);
	nkeywords = keywords == NULL ? 0 : PyDict_Size(keywords);

	if (nkeywords > 0) 
	{
		for (i = 0; i < nargs; i++) 
		{
			char *thiskw = kwlist[i];
			if (thiskw == NULL)
			{
				break;
			}
			if (PyDict_GetItemString(keywords, thiskw)) 
			{
				PyErr_Format(PyExc_TypeError,
					"keyword parameter '%s' was given "
					"by position and by name",
					thiskw);
				return 0;
			}
			else if (PyErr_Occurred())
			{
				return 0;
			}
		}
	}

	len = nargs;
	if (nkeywords > 0 && nargs < min) 
	{
		for (i = nargs; i < min; i++) 
		{
			if (PyDict_GetItemString(keywords, kwlist[i]))
			{
				len++;
			}
			else if (PyErr_Occurred())
			{
				return 0;
			}
		}
	}

	if (len < min || max < len) 
	{
		if (message == NULL) 
		{
			PyOS_snprintf(msgbuf, sizeof(msgbuf),
				      "%.200s%s takes %s %d argument%s "
				      "(%d given)",
				      fname==NULL ? "function" : fname,
				      fname==NULL ? "" : "()",
				      min==max ? "exactly"
			              : len < min ? "at least" : "at most",
				      len < min ? min : max,
				      (len < min ? min : max) == 1 ? "" : "s",
				      len);
			message = msgbuf;
		}
		PyErr_SetString(PyExc_TypeError, message);
		return 0;
	}

	for (i = 0; i < nargs; i++) 
	{
		if (*format == '|')
		{
			format++;
		}
		msg = convertitem(PyTuple_GET_ITEM(args, i), &format, p_va,
				 levels, msgbuf, sizeof(msgbuf));
		if (msg) 
		{
			seterror(i+1, msg, levels, fname, message);
			return 0;
		}
	}

	if (nkeywords == 0)
	{
		return 1; 
	}

	for (i = nargs; i < max; i++) 
	{
		PyObject *item;
		if (*format == '|')
		{
			format++;
		}
		item = PyDict_GetItemString(keywords, kwlist[i]);
		if (item != NULL) 
		{
			Py_INCREF(item);
			msg = convertitem(item, &format, p_va, levels, msgbuf,
					  sizeof(msgbuf));
			Py_DECREF(item);
			if (msg) 
			{
				seterror(i+1, msg, levels, fname, message);
				return 0;
			}
			--nkeywords;
			if (nkeywords == 0)
			{
				break;
			}
		}
		else if (PyErr_Occurred())
		{
			return 0;
		}
		else 
		{
			msg = skipitem(&format, p_va);
			if (msg) 
			{
				seterror(i+1, msg, levels, fname, message);
				return 0;
			}
		}
	}

	if (nkeywords > 0) 
	{
		PyObject *key, *value;
		int pos = 0;
		while (PyDict_Next(keywords, &pos, &key, &value)) 
		{
			int match = 0;
			char *ks = PyString_AsString(key);
			for (i = 0; i < max; i++) 
			{
				if (!strcmp(ks, kwlist[i])) 
				{
					match = 1;
					break;
				}
			}
			if (!match) 
			{
				PyErr_Format(PyExc_TypeError,
					     "'%s' is an invalid keyword "
					     "argument for this function",
					     ks);
				return 0;
			}
		}
	}

	return 1;
}


static char *skipitem(char **p_format, va_list *p_va)
{
	char *format = *p_format;
	char c = *format++;
	
	switch (c) 
	{
	case 'b': 
	case 'B':
		{
			va_arg(*p_va, char *);
			break;
		}
	
	case 'h':
		{
			va_arg(*p_va, short *);
			break;
		}
	
	case 'H':
		{
			va_arg(*p_va, unsigned short *);
			break;
		}
	
	case 'i':
		{
			va_arg(*p_va, int *);
			break;
		}
	
	case 'l':
		{
			va_arg(*p_va, long *);
			break;
		}
	
	case 'L':
		{
			va_arg(*p_va, LONG_LONG *);
			break;
		}
	
	case 'f':
		{
			va_arg(*p_va, float *);
			break;
		}
	
	case 'd':
		{
			va_arg(*p_va, double *);
			break;
		}
	
	case 'D':
		{
			va_arg(*p_va, Py_complex *);
			break;
		}

	case 'c':
		{
			va_arg(*p_va, char *);
			break;
		}
	
	case 's':
		{
			va_arg(*p_va, char **);
			if (*format == '#') 
			{
				va_arg(*p_va, int *);
				format++;
			}
			break;
		}
	
	case 'z':
		{
			va_arg(*p_va, char **);
			if (*format == '#') 
			{
				va_arg(*p_va, int *);
				format++;
			}
			break;
		}
	
	case 'S':
		{
			va_arg(*p_va, PyObject **);
			break;
		}
	
	case 'O':
		{
			if (*format == '!') 
			{
				format++;
				va_arg(*p_va, PyTypeObject*);
				va_arg(*p_va, PyObject **);
			}
			else if (*format == '&') 
			{
				typedef int (*converter)(PyObject *, void *);
				va_arg(*p_va, converter);
				va_arg(*p_va, void *);
				format++;
			}
			else 
			{
				va_arg(*p_va, PyObject **);
			}
			break;
		}
	
	default:
		return "impossible<bad format char>";
	
	}
	
	*p_format = format;
	return NULL;
}


int PyArg_UnpackTuple(PyObject *args, char *name, int min, int max, ...)
{
	int i, l;
	PyObject **o;
	va_list vargs;

	va_start(vargs, max);

	assert(min >= 0);
	assert(min <= max);
	if (!PyTuple_Check(args)) 
	{
		PyErr_SetString(PyExc_SystemError,
		    "PyArg_UnpackTuple() argument list is not a tuple");
		return 0;
	}	
	l = PyTuple_GET_SIZE(args);
	if (l < min) 
	{
		if (name != NULL)
		{
			PyErr_Format(
			    PyExc_TypeError,
			    "%s expected %s%d arguments, got %d", 
			    name, (min == max ? "" : "at least "), min, l);
		}
		else
		{
			PyErr_Format(
			    PyExc_TypeError,
			    "unpacked tuple should have %s%d elements,"
			    " but has %d", 
			    (min == max ? "" : "at least "), min, l);
		}
		va_end(vargs);
		return 0;
	}
	if (l > max) 
	{
		if (name != NULL)
		{
			PyErr_Format(
			    PyExc_TypeError,
			    "%s expected %s%d arguments, got %d", 
			    name, (min == max ? "" : "at most "), max, l);
		}
		else
		{
			PyErr_Format(
			    PyExc_TypeError,
			    "unpacked tuple should have %s%d elements,"
			    " but has %d", 
			    (min == max ? "" : "at most "), max, l);
		}
		va_end(vargs);
		return 0;
	}
	for (i = 0; i < l; i++) 
	{
		o = va_arg(vargs, PyObject **);
		*o = PyTuple_GET_ITEM(args, i);
	}
	va_end(vargs);
	return 1;
}
