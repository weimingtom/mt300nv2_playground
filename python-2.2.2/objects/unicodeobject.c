//20170502

#include "python.h"
#include "unicodeobject.h"
#include "ucnhash.h"

#define MAX_UNICODE_FREELIST_SIZE       1024
#define KEEPALIVE_SIZE_LIMIT       9

# define BYTEORDER_IS_LITTLE_ENDIAN

static PyUnicodeObject *unicode_freelist;
static int unicode_freelist_size;
static PyUnicodeObject *unicode_empty;
static PyUnicodeObject *unicode_latin1[256];
static char unicode_default_encoding[100];

Py_UNICODE PyUnicode_GetMax()
{
	return 0xFFFF;
}

static int unicode_resize(PyUnicodeObject *unicode,
                      int length)
{
    void *oldstr;
    
    if (unicode->length == length)
	{
		goto reset;
	}

    if (unicode == unicode_empty || 
		(unicode->length == 1 && 
		unicode->str[0] < 256 &&
		unicode_latin1[unicode->str[0]] == unicode)) 
	{
        PyErr_SetString(PyExc_SystemError,
                        "can't resize shared unicode objects");
        return -1;
    }

    oldstr = unicode->str;
    PyMem_RESIZE(unicode->str, Py_UNICODE, length + 1);
    if (!unicode->str) 
	{
		unicode->str = oldstr;
        PyErr_NoMemory();
        return -1;
    }
    unicode->str[length] = 0;
    unicode->length = length;

reset:
    if (unicode->defenc) 
	{
        Py_DECREF(unicode->defenc);
        unicode->defenc = NULL;
    }
    unicode->hash = -1;
    
    return 0;
}

static PyUnicodeObject *_PyUnicode_New(int length)
{
    PyUnicodeObject *unicode;

    if (length == 0 && unicode_empty != NULL) 
	{
        Py_INCREF(unicode_empty);
        return unicode_empty;
    }

    if (unicode_freelist) 
	{
        unicode = unicode_freelist;
        unicode_freelist = *(PyUnicodeObject **)unicode;
        unicode_freelist_size--;
		if (unicode->str) 
		{
			if ((unicode->length < length) &&
				unicode_resize(unicode, length)) 
			{
				PyMem_DEL(unicode->str);
				goto onError;
			}
		}
        else 
		{
			unicode->str = PyMem_NEW(Py_UNICODE, length + 1);
        }
        PyObject_INIT(unicode, &PyUnicode_Type);
    }
    else 
	{
        unicode = PyObject_NEW(PyUnicodeObject, &PyUnicode_Type);
        if (unicode == NULL)
        {
			return NULL;
		}
		unicode->str = PyMem_NEW(Py_UNICODE, length + 1);
    }

    if (!unicode->str) 
	{
		PyErr_NoMemory();
		goto onError;
    }
    unicode->str[length] = 0;
    unicode->length = length;
    unicode->hash = -1;
    unicode->defenc = NULL;
    return unicode;

onError:
    _Py_ForgetReference((PyObject *)unicode);
    PyObject_DEL(unicode);
    return NULL;
}

static void unicode_dealloc(PyUnicodeObject *unicode)
{
    if (PyUnicode_CheckExact(unicode) &&
		unicode_freelist_size < MAX_UNICODE_FREELIST_SIZE) 
	{
        if (unicode->length >= KEEPALIVE_SIZE_LIMIT) 
		{
			PyMem_DEL(unicode->str);
			unicode->str = NULL;
			unicode->length = 0;
		}
		if (unicode->defenc) 
		{
			Py_DECREF(unicode->defenc);
			unicode->defenc = NULL;
		}
	    *(PyUnicodeObject **)unicode = unicode_freelist;
        unicode_freelist = unicode;
        unicode_freelist_size++;
    }
    else 
	{
		PyMem_DEL(unicode->str);
		Py_XDECREF(unicode->defenc);
		unicode->ob_type->tp_free((PyObject *)unicode);
    }
}

int PyUnicode_Resize(PyObject **unicode,
		     int length)
{
    PyUnicodeObject *v;

    if (unicode == NULL) 
	{
		PyErr_BadInternalCall();
		return -1;
    }
    v = (PyUnicodeObject *)*unicode;
    if (v == NULL || !PyUnicode_Check(v) || v->ob_refcnt != 1 || length < 0) 
	{
		PyErr_BadInternalCall();
		return -1;
    }

    if (v->length != length && 
		(v == unicode_empty || v->length == 1)) 
	{
		PyUnicodeObject *w = _PyUnicode_New(length);
		if (w == NULL)
		{
			return -1;
		}
		Py_UNICODE_COPY(w->str, v->str,
			length < v->length ? length : v->length);
		*unicode = (PyObject *)w;
		return 0;
    }

    return unicode_resize(v, length);
}

#define _PyUnicode_Resize(unicodevar, length) \
        PyUnicode_Resize(((PyObject **)(unicodevar)), length)

PyObject *PyUnicode_FromUnicode(const Py_UNICODE *u,
				int size)
{
    PyUnicodeObject *unicode;

    if (u != NULL) 
	{
		if (size == 0 && unicode_empty != NULL) 
		{
			Py_INCREF(unicode_empty);
			return (PyObject *)unicode_empty;
		}

		if (size == 1 && *u < 256) 
		{
			unicode = unicode_latin1[*u];
			if (!unicode) 
			{
				unicode = _PyUnicode_New(1);
				if (!unicode)
				{
					return NULL;
				}
				unicode->str[0] = *u;
				unicode_latin1[*u] = unicode;
			}
			Py_INCREF(unicode);
			return (PyObject *)unicode;
		}
    }
    
    unicode = _PyUnicode_New(size);
    if (!unicode)
	{
        return NULL;
	}

    if (u != NULL)
	{
		Py_UNICODE_COPY(unicode->str, u, size);
	}

    return (PyObject *)unicode;
}

PyObject *PyUnicode_FromWideChar(const wchar_t *w,
				 int size)
{
    PyUnicodeObject *unicode;

    if (w == NULL) 
	{
		PyErr_BadInternalCall();
		return NULL;
    }

    unicode = _PyUnicode_New(size);
    if (!unicode)
    {
		return NULL;
	}

    memcpy(unicode->str, w, size * sizeof(wchar_t));
    return (PyObject *)unicode;
}

int PyUnicode_AsWideChar(PyUnicodeObject *unicode,
			 wchar_t *w,
			 int size)
{
    if (unicode == NULL) 
	{
		PyErr_BadInternalCall();
		return -1;
    }
    if (size > PyUnicode_GET_SIZE(unicode))
	{
		size = PyUnicode_GET_SIZE(unicode);
	}
	memcpy(w, unicode->str, size * sizeof(wchar_t));

    return size;
}

PyObject *PyUnicode_FromOrdinal(int ordinal)
{
    Py_UNICODE s[2];

    if (ordinal < 0 || ordinal > 0xffff) 
	{
		PyErr_SetString(PyExc_ValueError,
			"unichr() arg not in range(0x10000) "
			"(narrow Python build)");
		return NULL;
    }

    if (ordinal <= 0xffff) 
	{
		s[0] = (Py_UNICODE) ordinal;
		return PyUnicode_FromUnicode(s, 1);
    }
    else 
	{
		ordinal -= 0x10000L;
		s[0] = 0xD800 + (Py_UNICODE) (ordinal >> 10);
		s[1] = 0xDC00 + (Py_UNICODE) (ordinal & 0x03FF);
		return PyUnicode_FromUnicode(s, 2);
    }
}

PyObject *PyUnicode_FromObject(PyObject *obj)
{
    if (PyUnicode_CheckExact(obj)) 
	{
		Py_INCREF(obj);
		return obj;
    }
    if (PyUnicode_Check(obj)) 
	{
		return PyUnicode_FromUnicode(PyUnicode_AS_UNICODE(obj),
				     PyUnicode_GET_SIZE(obj));
    }
    return PyUnicode_FromEncodedObject(obj, NULL, "strict");
}

PyObject *PyUnicode_FromEncodedObject(PyObject *obj,
				      const char *encoding,
				      const char *errors)
{
    const char *s = NULL;
    int len;
    int owned = 0;
    PyObject *v;
    
    if (obj == NULL) 
	{
		PyErr_BadInternalCall();
		return NULL;
    }

    if (PyUnicode_Check(obj)) {
		PyErr_SetString(PyExc_TypeError,
			"decoding Unicode is not supported");
		return NULL;
	}

    if (PyString_Check(obj)) 
	{
	    s = PyString_AS_STRING(obj);
	    len = PyString_GET_SIZE(obj);
	}
    else if (PyObject_AsCharBuffer(obj, &s, &len)) 
	{
		if (PyErr_ExceptionMatches(PyExc_TypeError))
		{
			PyErr_Format(PyExc_TypeError,
				 "coercing to Unicode: need string or buffer, "
				 "%.80s found",
				 obj->ob_type->tp_name);
		}
		goto onError;
    }
    
    if (len == 0) 
	{
		Py_INCREF(unicode_empty);
		v = (PyObject *)unicode_empty;
    }
    else 
	{
		v = PyUnicode_Decode(s, len, encoding, errors);
	}

    if (owned) 
	{
		Py_DECREF(obj);
    }
    return v;

onError:
    if (owned) 
	{
		Py_DECREF(obj);
    }
    return NULL;
}

PyObject *PyUnicode_Decode(const char *s,
			   int size,
			   const char *encoding,
			   const char *errors)
{
    PyObject *buffer = NULL, *unicode;
    
    if (encoding == NULL) 
	{
		encoding = PyUnicode_GetDefaultEncoding();
	}

    if (strcmp(encoding, "utf-8") == 0)
    {
		return PyUnicode_DecodeUTF8(s, size, errors);
    }
	else if (strcmp(encoding, "latin-1") == 0)
    {
		return PyUnicode_DecodeLatin1(s, size, errors);
    }
	else if (strcmp(encoding, "ascii") == 0)
    {
		return PyUnicode_DecodeASCII(s, size, errors);
	}

    buffer = PyBuffer_FromMemory((void *)s, size);
    if (buffer == NULL)
    {
		goto onError;
    }
	unicode = PyCodec_Decode(buffer, encoding, errors);
    if (unicode == NULL)
    {
		goto onError;
    }
	if (!PyUnicode_Check(unicode)) 
	{
        PyErr_Format(PyExc_TypeError,
                     "decoder did not return an unicode object (type=%.400s)",
                     unicode->ob_type->tp_name);
        Py_DECREF(unicode);
        goto onError;
    }
    Py_DECREF(buffer);
    return unicode;
    
onError:
    Py_XDECREF(buffer);
    return NULL;
}

PyObject *PyUnicode_Encode(const Py_UNICODE *s,
			   int size,
			   const char *encoding,
			   const char *errors)
{
    PyObject *v, *unicode;
    
    unicode = PyUnicode_FromUnicode(s, size);
    if (unicode == NULL)
	{
		return NULL;
    }
	v = PyUnicode_AsEncodedString(unicode, encoding, errors);
    Py_DECREF(unicode);
    return v;
}

PyObject *PyUnicode_AsEncodedString(PyObject *unicode,
                                    const char *encoding,
                                    const char *errors)
{
    PyObject *v;
    
    if (!PyUnicode_Check(unicode)) 
	{
        PyErr_BadArgument();
        goto onError;
    }

    if (encoding == NULL) 
	{
		encoding = PyUnicode_GetDefaultEncoding();
	}

    if (errors == NULL) 
	{
		if (strcmp(encoding, "utf-8") == 0)
		{
			return PyUnicode_AsUTF8String(unicode);
		}
		else if (strcmp(encoding, "latin-1") == 0)
		{
			return PyUnicode_AsLatin1String(unicode);
		}
		else if (strcmp(encoding, "ascii") == 0)
		{
			return PyUnicode_AsASCIIString(unicode);
		}
    }

    v = PyCodec_Encode(unicode, encoding, errors);
    if (v == NULL)
    {
		goto onError;
    }
	if (!PyString_Check(v)) 
	{
        PyErr_Format(PyExc_TypeError,
                     "encoder did not return a string object (type=%.400s)",
                     v->ob_type->tp_name);
        Py_DECREF(v);
        goto onError;
    }
    return v;
    
onError:
    return NULL;
}

PyObject *_PyUnicode_AsDefaultEncodedString(PyObject *unicode,
					    const char *errors)
{
    PyObject *v = ((PyUnicodeObject *)unicode)->defenc;

    if (v)
    {
		return v;
    }
	v = PyUnicode_AsEncodedString(unicode, NULL, errors);
    if (v && errors == NULL)
    {
		((PyUnicodeObject *)unicode)->defenc = v;
    }
	return v;
}

Py_UNICODE *PyUnicode_AsUnicode(PyObject *unicode)
{
    if (!PyUnicode_Check(unicode)) 
	{
        PyErr_BadArgument();
        goto onError;
    }
    return PyUnicode_AS_UNICODE(unicode);

onError:
    return NULL;
}

int PyUnicode_GetSize(PyObject *unicode)
{
    if (!PyUnicode_Check(unicode)) 
	{
        PyErr_BadArgument();
        goto onError;
    }
    return PyUnicode_GET_SIZE(unicode);

onError:
    return -1;
}

const char *PyUnicode_GetDefaultEncoding()
{
    return unicode_default_encoding;
}

int PyUnicode_SetDefaultEncoding(const char *encoding)
{
    PyObject *v;
    
    v = _PyCodec_Lookup(encoding);
    if (v == NULL)
	{
		goto onError;
    }
	Py_DECREF(v);
    strncpy(unicode_default_encoding,
	    encoding, 
	    sizeof(unicode_default_encoding));
    return 0;

onError:
    return -1;
}

static char utf7_special[128] = {
    1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 1, 1, 2, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    2, 3, 3, 3, 3, 3, 3, 0, 0, 0, 3, 1, 0, 0, 0, 1,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 3, 3, 3, 0,
    3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 1, 3, 3, 3,
    3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 3, 3, 1, 1,

};

#define SPECIAL(c, encodeO, encodeWS) \
	(((c)>127 || utf7_special[(c)] == 1) || \
	 (encodeWS && (utf7_special[(c)] == 2)) || \
     (encodeO && (utf7_special[(c)] == 3)))

#define B64(n)  ("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"[(n) & 0x3f])
#define B64CHAR(c) (isalnum(c) || (c) == '+' || (c) == '/')
#define UB64(c)        ((c) == '+' ? 62 : (c) == '/' ? 63 : (c) >= 'a' ? \
                        (c) - 71 : (c) >= 'A' ? (c) - 65 : (c) + 4)

#define ENCODE(out, ch, bits) \
    while (bits >= 6) { \
        *out++ = B64(ch >> (bits-6)); \
        bits -= 6; \
    }

#define DECODE(out, ch, bits, surrogate) \
    while (bits >= 16) { \
        Py_UNICODE outCh = (Py_UNICODE) ((ch >> (bits-16)) & 0xffff); \
        bits -= 16; \
		if (surrogate) { \
			surrogate = 0; \
		} else if (0xDC00 <= outCh && outCh <= 0xDFFF) { \
			surrogate = 1; \
            errmsg = "code pairs are not supported"; \
	        goto utf7Error; \
		} else { \
				*out++ = outCh; \
		} \
    } \

static int utf7_decoding_error(Py_UNICODE **dest,
                        const char *errors,
                        const char *details) 
{
    if ((errors == NULL) ||
        (strcmp(errors,"strict") == 0)) 
	{
        PyErr_Format(PyExc_UnicodeError,
                     "UTF-7 decoding error: %.400s",
                     details);
        return -1;
    }
    else if (strcmp(errors,"ignore") == 0) 
	{
        return 0;
    }
    else if (strcmp(errors,"replace") == 0) 
	{
        if (dest != NULL) 
		{
            **dest = Py_UNICODE_REPLACEMENT_CHARACTER;
            (*dest)++;
        }
        return 0;
    }
    else 
	{
        PyErr_Format(PyExc_ValueError,
                     "UTF-7 decoding error; unknown error handling code: %.400s",
                     errors);
        return -1;
    }
}

PyObject *PyUnicode_DecodeUTF7(const char *s,
			       int size,
			       const char *errors)
{
    const char *e;
    PyUnicodeObject *unicode;
    Py_UNICODE *p;
    const char *errmsg = "";
    int inShift = 0;
    unsigned int bitsleft = 0;
    unsigned long charsleft = 0;
	int surrogate = 0;

    unicode = _PyUnicode_New(size);
    if (!unicode)
    {
		return NULL;
    }
	if (size == 0)
    {
		return (PyObject *)unicode;
	}

    p = unicode->str;
    e = s + size;

    while (s < e) 
	{
        Py_UNICODE ch = *s;

        if (inShift) 
		{
            if ((ch == '-') || !B64CHAR(ch)) 
			{
                inShift = 0;
                s++;
                    
                DECODE(p, charsleft, bitsleft, surrogate);
                if (bitsleft >= 6) 
				{
                    errmsg = "partial character in shift sequence";
                    goto utf7Error; 
                }
                if (bitsleft && charsleft << (sizeof(charsleft) * 8 - bitsleft)) 
				{
                    errmsg = "non-zero padding bits in shift sequence";
                    goto utf7Error; 
                }

                if (ch == '-') 
				{
                    if ((s < e) && (*(s) == '-')) 
					{
                        *p++ = '-';   
                        inShift = 1;
                    }
                } 
				else if (SPECIAL(ch, 0, 0)) 
				{
                    errmsg = "unexpected special character";
	                goto utf7Error;  
                } 
				else  
				{
                    *p++ = ch;
                }
            } 
			else 
			{
                charsleft = (charsleft << 6) | UB64(ch);
                bitsleft += 6;
                s++;
                DECODE(p, charsleft, bitsleft, surrogate);
            }
        }
        else if (ch == '+') 
		{
            s++;
            if (s < e && *s == '-') 
			{
                s++;
                *p++ = '+';
            } 
			else
            {
                inShift = 1;
                bitsleft = 0;
            }
        }
        else if (SPECIAL(ch, 0, 0)) 
		{
            errmsg = "unexpected special character";
            s++;
	        goto utf7Error;  
        }
        else 
		{
            *p++ = ch;
            s++;
        }
        continue;
utf7Error:
		if (utf7_decoding_error(&p, errors, errmsg))
        {
			goto onError;
		}
    }

    if (inShift) 
	{
        if (utf7_decoding_error(&p, errors, "unterminated shift sequence"))
		{
			goto onError;
		}
    }

    if (_PyUnicode_Resize(&unicode, p - unicode->str))
	{
		goto onError;
	}

    return (PyObject *)unicode;

onError:
    Py_DECREF(unicode);
    return NULL;
}

PyObject *PyUnicode_EncodeUTF7(const Py_UNICODE *s,
                   int size,
                   int encodeSetO,
                   int encodeWhiteSpace,
                   const char *errors)
{
    PyObject *v;
    unsigned int cbAllocated = 5 * size;
    int inShift = 0;
    int i = 0;
    unsigned int bitsleft = 0;
    unsigned long charsleft = 0;
    char * out;
    char * start;

    if (size == 0)
	{
		return PyString_FromStringAndSize(NULL, 0);
	}

    v = PyString_FromStringAndSize(NULL, cbAllocated);
    if (v == NULL)
    {
		return NULL;
	}

    start = out = PyString_AS_STRING(v);
    for (;i < size; ++i) 
	{
        Py_UNICODE ch = s[i];

        if (!inShift) 
		{
			if (ch == '+') 
			{
				*out++ = '+';
                *out++ = '-';
            } 
			else if (SPECIAL(ch, encodeSetO, encodeWhiteSpace)) 
			{
                charsleft = ch;
                bitsleft = 16;
                *out++ = '+';
				ENCODE(out, charsleft, bitsleft);
                inShift = bitsleft > 0;
			} 
			else 
			{
				*out++ = (char) ch;
			}
		} 
		else 
		{
            if (!SPECIAL(ch, encodeSetO, encodeWhiteSpace)) 
			{
                *out++ = B64(charsleft << (6-bitsleft));
                charsleft = 0;
                bitsleft = 0;
                if (B64CHAR(ch) || ch == '-') 
				{
                    *out++ = '-';
                }
                inShift = 0;
                *out++ = (char) ch;
            } 
			else 
			{
                bitsleft += 16;
                charsleft = (charsleft << 16) | ch;
                ENCODE(out, charsleft, bitsleft);

                if (bitsleft == 0) 
				{
                    if (i + 1 < size) 
					{
                        Py_UNICODE ch2 = s[i+1];

                        if (SPECIAL(ch2, encodeSetO, encodeWhiteSpace)) 
						{
                           
                        } 
						else if (B64CHAR(ch2) || ch2 == '-') 
						{
                            *out++ = '-';
                            inShift = 0;
                        } 
						else 
						{
                            inShift = 0;
                        }
                    }
                    else 
					{
                        *out++ = '-';
                        inShift = 0;
                    }
                }
            }            
        }
	}
    if (bitsleft) 
	{
        *out++= B64(charsleft << (6 - bitsleft));
        *out++ = '-';
    }

    _PyString_Resize(&v, out - start);
    return v;
}

#undef SPECIAL
#undef B64
#undef B64CHAR
#undef UB64
#undef ENCODE
#undef DECODE

static  char utf8_code_length[256] = {
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
    4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 6, 6, 0, 0
};

static int utf8_decoding_error(const char **source,
                        Py_UNICODE **dest,
                        const char *errors,
                        const char *details) 
{
    if ((errors == NULL) ||
        (strcmp(errors,"strict") == 0)) 
	{
        PyErr_Format(PyExc_UnicodeError,
                     "UTF-8 decoding error: %.400s",
                     details);
        return -1;
    }
    else if (strcmp(errors,"ignore") == 0) 
	{
        (*source)++;
        return 0;
    }
    else if (strcmp(errors,"replace") == 0) 
	{
        (*source)++;
        **dest = Py_UNICODE_REPLACEMENT_CHARACTER;
        (*dest)++;
        return 0;
    }
    else 
	{
        PyErr_Format(PyExc_ValueError,
                     "UTF-8 decoding error; unknown error handling code: %.400s",
                     errors);
        return -1;
    }
}

PyObject *PyUnicode_DecodeUTF8(const char *s,
			       int size,
			       const char *errors)
{
    int n;
    const char *e;
    PyUnicodeObject *unicode;
    Py_UNICODE *p;
    const char *errmsg = "";

    unicode = _PyUnicode_New(size);
    if (!unicode)
	{
		return NULL;
    }
	if (size == 0)
    {
		return (PyObject *)unicode;
	}

    p = unicode->str;
    e = s + size;

    while (s < e) 
	{
        Py_UCS4 ch = (unsigned char)*s;

        if (ch < 0x80) 
		{
            *p++ = (Py_UNICODE)ch;
            s++;
            continue;
        }

        n = utf8_code_length[ch];

        if (s + n > e) 
		{
			errmsg = "unexpected end of data";
			goto utf8Error;
		}

        switch (n) 
		{
		case 0:
			if (((unsigned char)s[0] >= 0xa0) && 
				((unsigned char)s[0] <= 0xaf)) 
			{
				n = 2;
				if (s + n > e) 
				{
					errmsg = "unexpected end of data";
					goto utf8Error;
				}
				if ((s[0] & 0xc0) != 0x80 || 
					(s[1] & 0xc0) != 0x80) 
				{
					errmsg = "invalid data";
					goto utf8Error;
				}
				ch = 0xd000 + ((s[0] & 0x3f) << 6) + (s[1] & 0x3f);
				if (ch < 0x0800) 
				{
					errmsg = "illegal encoding";
					goto utf8Error;
				}
				else
				{
					*p++ = (Py_UNICODE)ch;
				}
				break;
			}
			errmsg = "unexpected code byte";
			goto utf8Error;

        case 1:
            errmsg = "internal error";
			goto utf8Error;

        case 2:
            if ((s[1] & 0xc0) != 0x80) 
			{
                errmsg = "invalid data";
				goto utf8Error;
			}
            ch = ((s[0] & 0x1f) << 6) + (s[1] & 0x3f);
            if (ch < 0x80) 
			{
                errmsg = "illegal encoding";
				goto utf8Error;
			}
			else
			{
				*p++ = (Py_UNICODE)ch;
            }
			break;

        case 3:
            if ((s[1] & 0xc0) != 0x80 || 
                (s[2] & 0xc0) != 0x80) 
			{
                errmsg = "invalid data";
				goto utf8Error;
			}
            ch = ((s[0] & 0x0f) << 12) + ((s[1] & 0x3f) << 6) + (s[2] & 0x3f);
            if (ch < 0x0800) 
			{
                errmsg = "illegal encoding";
				goto utf8Error;
			}
			else
			{
				*p++ = (Py_UNICODE)ch;
            }
			break;

        case 4:
            if ((s[1] & 0xc0) != 0x80 ||
                (s[2] & 0xc0) != 0x80 ||
                (s[3] & 0xc0) != 0x80) 
			{
                errmsg = "invalid data";
				goto utf8Error;
			}
            ch = ((s[0] & 0x7) << 18) + ((s[1] & 0x3f) << 12) +
                 ((s[2] & 0x3f) << 6) + (s[3] & 0x3f);
            if ((ch < 0x10000)
                || (ch > 0x10ffff))
			{
                errmsg = "illegal encoding";
				goto utf8Error;
			}
            ch -= 0x10000;
            *p++ = (Py_UNICODE)(0xD800 + (ch >> 10));
            *p++ = (Py_UNICODE)(0xDC00 + (ch & 0x03FF));
            break;

        default:
            errmsg = "unsupported Unicode code range";
			goto utf8Error;
        }
        s += n;
		continue;
	
utf8Error:
		if (utf8_decoding_error(&s, &p, errors, errmsg))
		{
			goto onError;
		}
    }

    if (_PyUnicode_Resize(&unicode, p - unicode->str))
	{
		goto onError;
	}

    return (PyObject *)unicode;

onError:
    Py_DECREF(unicode);
    return NULL;
}

PyObject *PyUnicode_EncodeUTF8(const Py_UNICODE *s,
		     int size,
		     const char *errors)
{
#define MAX_SHORT_UNICHARS 300 
    int i;             
    PyObject *v;       
    char *p;           
    int nallocated;    
    int nneeded;       
    char stackbuf[MAX_SHORT_UNICHARS * 4];

    assert(s != NULL);
    assert(size >= 0);

    if (size <= MAX_SHORT_UNICHARS) 
	{
        nallocated = Py_SAFE_DOWNCAST(sizeof(stackbuf), size_t, int);
        v = NULL;   
        p = stackbuf;
    }
    else 
	{
        nallocated = size * 4;
        if (nallocated / 4 != size) 
		{
            return PyErr_NoMemory();
        }
		v = PyString_FromStringAndSize(NULL, nallocated);
        if (v == NULL)
		{
			return NULL;
        }
		p = PyString_AS_STRING(v);
    }

    for (i = 0; i < size;) 
	{
        Py_UCS4 ch = s[i++];

        if (ch < 0x80)
		{
			*p++ = (char) ch;
		}
        else if (ch < 0x0800) 
		{
            *p++ = (char)(0xc0 | (ch >> 6));
            *p++ = (char)(0x80 | (ch & 0x3f));
        }
        else 
		{
            if (ch < 0x10000) 
			{
                if (0xD800 <= ch && ch <= 0xDBFF && i != size) 
				{
                    Py_UCS4 ch2 = s[i];
                    if (0xDC00 <= ch2 && ch2 <= 0xDFFF) {
                        ch = ((ch - 0xD800) << 10 | (ch2 - 0xDC00)) + 0x10000;
                        i++;
                        goto encodeUCS4;
                    }
                }
                *p++ = (char)(0xe0 | (ch >> 12));
                *p++ = (char)(0x80 | ((ch >> 6) & 0x3f));
                *p++ = (char)(0x80 | (ch & 0x3f));
                continue;
    	    }
encodeUCS4:
            *p++ = (char)(0xf0 | (ch >> 18));
            *p++ = (char)(0x80 | ((ch >> 12) & 0x3f));
            *p++ = (char)(0x80 | ((ch >> 6) & 0x3f));
            *p++ = (char)(0x80 | (ch & 0x3f));
        }
    }

    if (v == NULL) 
	{
        nneeded = Py_SAFE_DOWNCAST(p - stackbuf, long, int);
        assert(nneeded <= nallocated);
        v = PyString_FromStringAndSize(stackbuf, nneeded);
    }
    else 
	{
    	nneeded = Py_SAFE_DOWNCAST(p - PyString_AS_STRING(v), long, int);
        assert(nneeded <= nallocated);
        _PyString_Resize(&v, nneeded);
    }
    return v;

#undef MAX_SHORT_UNICHARS
}

PyObject *PyUnicode_AsUTF8String(PyObject *unicode)
{
    if (!PyUnicode_Check(unicode)) 
	{
        PyErr_BadArgument();
        return NULL;
    }
    return PyUnicode_EncodeUTF8(PyUnicode_AS_UNICODE(unicode),
				PyUnicode_GET_SIZE(unicode),
				NULL);
}

static int utf16_decoding_error(Py_UNICODE **dest,
			 const char *errors,
			 const char *details) 
{
    if ((errors == NULL) ||
        (strcmp(errors, "strict") == 0)) 
	{
        PyErr_Format(PyExc_UnicodeError,
                     "UTF-16 decoding error: %.400s",
                     details);
        return -1;
    }
    else if (strcmp(errors, "ignore") == 0) 
	{
        return 0;
    }
    else if (strcmp(errors, "replace") == 0) 
	{
		if (dest) 
		{
			**dest = Py_UNICODE_REPLACEMENT_CHARACTER;
			(*dest)++;
		}
        return 0;
    }
    else 
	{
        PyErr_Format(PyExc_ValueError,
             "UTF-16 decoding error; "
		     "unknown error handling code: %.400s",
             errors);
        return -1;
    }
}

PyObject *PyUnicode_DecodeUTF16(const char *s,
		      int size,
		      const char *errors,
		      int *byteorder)
{
    PyUnicodeObject *unicode;
    Py_UNICODE *p;
    const unsigned char *q, *e;
    int bo = 0;       
    const char *errmsg = "";
    int ihi = 1, ilo = 0;
	if (size & 1) 
	{
        if (utf16_decoding_error(NULL, errors, "truncated data"))
        {
			return NULL;
        }
		--size; 
    }

    unicode = _PyUnicode_New(size);
    if (!unicode)
    {
		return NULL;
    }
	if (size == 0)
    {
		return (PyObject *)unicode;
	}

    p = unicode->str;
    q = (unsigned char *)s;
    e = q + size;

    if (byteorder)
	{
        bo = *byteorder;
	}

    if (bo == 0) 
	{
        const Py_UNICODE bom = (q[ihi] << 8) | q[ilo];
		if (bom == 0xFEFF) 
		{
			q += 2;
			bo = -1;
		}
        else if (bom == 0xFFFE) 
		{
			q += 2;
			bo = 1;
		}
    }
    if (bo == -1) 
	{
        ihi = 1;
        ilo = 0;
    }
    else if (bo == 1) 
	{
        ihi = 0;
        ilo = 1;
    }

    while (q < e) 
	{
		Py_UNICODE ch = (q[ihi] << 8) | q[ilo];
		q += 2;

		if (ch < 0xD800 || ch > 0xDFFF) 
		{
			*p++ = ch;
			continue;
		}

		if (q >= e) 
		{
			errmsg = "unexpected end of data";
			goto utf16Error;
		}
		if (0xD800 <= ch && ch <= 0xDBFF) 
		{
			Py_UNICODE ch2 = (q[ihi] << 8) | q[ilo];
			q += 2;
			if (0xDC00 <= ch2 && ch2 <= 0xDFFF) 
			{
				*p++ = ch;
				*p++ = ch2;
				continue;
			}
			else 
			{
                errmsg = "illegal UTF-16 surrogate";
				goto utf16Error;
			}
		}
		errmsg = "illegal encoding";
utf16Error:
		if (utf16_decoding_error(&p, errors, errmsg))
		{
			goto onError;
		}
	}
	if (byteorder)
	{
        *byteorder = bo;
	}

    if (_PyUnicode_Resize(&unicode, p - unicode->str))
	{
        goto onError;
	}
    return (PyObject *)unicode;

onError:
    Py_DECREF(unicode);
    return NULL;
}

PyObject *PyUnicode_EncodeUTF16(const Py_UNICODE *s,
		      int size,
		      const char *errors,
		      int byteorder)
{
    PyObject *v;
    unsigned char *p;
    int i, pairs;
    int ihi = 1, ilo = 0;

#define STORECHAR(CH)                   \
    do {                                \
        p[ihi] = ((CH) >> 8) & 0xff;    \
        p[ilo] = (CH) & 0xff;           \
        p += 2;                         \
    } while(0)

    for (i = pairs = 0; i < size; i++)
	{
		if (s[i] >= 0x10000)
		{
			pairs++;
		}
    }
	v = PyString_FromStringAndSize(NULL, 
		  2 * (size + pairs + (byteorder == 0)));
    if (v == NULL)
    {
		return NULL;
	}

    p = (unsigned char *)PyString_AS_STRING(v);
    if (byteorder == 0)
	{
		STORECHAR(0xFEFF);
    }
	if (size == 0)
    {
		return v;
	}

    if (byteorder == -1) 
	{
        ihi = 1;
        ilo = 0;
    }
    else if (byteorder == 1) 
	{
        ihi = 0;
        ilo = 1;
    }

    while (size-- > 0) 
	{
		Py_UNICODE ch = *s++;
		Py_UNICODE ch2 = 0;
		if (ch >= 0x10000) 
		{
			ch2 = 0xDC00 | ((ch-0x10000) & 0x3FF);
			ch  = 0xD800 | ((ch-0x10000) >> 10);
		}
        STORECHAR(ch);
        if (ch2)
        {
			STORECHAR(ch2);
		}
    }
    return v;
#undef STORECHAR
}

PyObject *PyUnicode_AsUTF16String(PyObject *unicode)
{
    if (!PyUnicode_Check(unicode)) 
	{
        PyErr_BadArgument();
        return NULL;
    }
    return PyUnicode_EncodeUTF16(PyUnicode_AS_UNICODE(unicode),
				 PyUnicode_GET_SIZE(unicode),
				 NULL,
				 0);
}

static int unicodeescape_decoding_error(Py_UNICODE **x,
                                 const char *errors,
                                 const char *details) 
{
    if ((errors == NULL) ||
        (strcmp(errors, "strict") == 0)) 
	{
        PyErr_Format(PyExc_UnicodeError,
                     "Unicode-Escape decoding error: %.400s",
                     details);
        return -1;
    }
    else if (strcmp(errors, "ignore") == 0) 
	{
        return 0;
    }
    else if (strcmp(errors,"replace") == 0) 
	{
        **x = Py_UNICODE_REPLACEMENT_CHARACTER;
		(*x)++;
        return 0;
    }
    else 
	{
        PyErr_Format(PyExc_ValueError,
                     "Unicode-Escape decoding error; "
                     "unknown error handling code: %.400s",
                     errors);
        return -1;
    }
}

static _PyUnicode_Name_CAPI *ucnhash_CAPI = NULL;

PyObject *PyUnicode_DecodeUnicodeEscape(const char *s,
					int size,
					const char *errors)
{
    PyUnicodeObject *v;
    Py_UNICODE *p, *buf;
    const char *end;
    char* message;
    Py_UCS4 chr = 0xffffffff;

    v = _PyUnicode_New(size);
    if (v == NULL)
    {
		goto onError;
    }
	if (size == 0)
    {
		return (PyObject *)v;
	}

    p = buf = PyUnicode_AS_UNICODE(v);
    end = s + size;

    while (s < end) 
	{
        unsigned char c;
        Py_UNICODE x;
        int i, digits;

        if (*s != '\\') 
		{
            *p++ = (unsigned char) *s++;
            continue;
        }

        s++;
        switch (*s++) 
		{
		case '\n': 
			break;
        
		case '\\': 
			*p++ = '\\'; 
			break;
        
		case '\'': 
			*p++ = '\''; 
			break;
        
		case '\"': 
			*p++ = '\"'; 
			break;
        
		case 'b': 
			*p++ = '\b'; 
			break;
        
		case 'f': 
			*p++ = '\014'; 
			break;
        
		case 't': 
			*p++ = '\t'; 
			break;
        
		case 'n': 
			*p++ = '\n'; 
			break;
        
		case 'r': 
			*p++ = '\r'; 
			break;
        
		case 'v': 
			*p++ = '\013'; 
			break;
        
		case 'a': 
			*p++ = '\007'; 
			break;

        case '0': case '1': case '2': case '3':
        case '4': case '5': case '6': case '7':
            x = s[-1] - '0';
            if ('0' <= *s && *s <= '7') 
			{
                x = (x<<3) + *s++ - '0';
                if ('0' <= *s && *s <= '7')
                {
					x = (x<<3) + *s++ - '0';
				}
            }
            *p++ = x;
            break;

        case 'x':
            digits = 2;
            message = "truncated \\xXX escape";
            goto hexescape;

        case 'u':
            digits = 4;
            message = "truncated \\uXXXX escape";
            goto hexescape;

        case 'U':
            digits = 8;
            message = "truncated \\UXXXXXXXX escape";
hexescape:
            chr = 0;
            for (i = 0; i < digits; i++) 
			{
                c = (unsigned char) s[i];
                if (!isxdigit(c)) 
				{
                    if (unicodeescape_decoding_error(&p, errors, message))
                    {
						goto onError;
                    }
					chr = 0xffffffff;
                    i++;
                    break;
                }
                chr = (chr<<4) & ~0xF;
                if (c >= '0' && c <= '9')
                {
					chr += c - '0';
                }
				else if (c >= 'a' && c <= 'f')
                {
					chr += 10 + c - 'a';
                }
				else
                {
					chr += 10 + c - 'A';
				}
            }
            s += i;
			if (chr == 0xffffffff)
			{
				break;
			}
store:
            if (chr <= 0xffff)
            {
				*p++ = (Py_UNICODE) chr;
            }
			else if (chr <= 0x10ffff) 
			{
                chr -= 0x10000L;
                *p++ = 0xD800 + (Py_UNICODE) (chr >> 10);
                *p++ = 0xDC00 + (Py_UNICODE) (chr & 0x03FF);
            } 
			else 
			{
                if (unicodeescape_decoding_error(
						&p, errors,
						"illegal Unicode character")
					)
				{
                    goto onError;
				}
            }
            break;

        case 'N':
            message = "malformed \\N character escape";
            if (ucnhash_CAPI == NULL) 
			{
                PyObject *m, *v;
                m = PyImport_ImportModule("unicodedata");
                if (m == NULL)
                {
					goto ucnhashError;
                }
				v = PyObject_GetAttrString(m, "ucnhash_CAPI");
                Py_DECREF(m);
                if (v == NULL)
                {
					goto ucnhashError;
                }
				ucnhash_CAPI = PyCObject_AsVoidPtr(v);
                Py_DECREF(v);
                if (ucnhash_CAPI == NULL)
                {
					goto ucnhashError;
				}
            }
            if (*s == '{') 
			{
                const char *start = s+1;
                while (*s != '}' && s < end)
                {
					s++;
                }
				if (s > start && s < end && *s == '}') 
				{
                    message = "unknown Unicode character name";
                    s++;
                    if (ucnhash_CAPI->getcode(start, s-start-1, &chr))
                    {
						goto store;
					}
                }
            }
            if (unicodeescape_decoding_error(&p, errors, message))
            {
				goto onError;
            }
			break;

        default:
			if (s > end) 
			{
				if (unicodeescape_decoding_error(&p, errors, "\\ at end of string"))
				{
					goto onError;
				}
			}
			else 
			{
				*p++ = '\\';
				*p++ = (unsigned char)s[-1];
			}
            break;
        }
    }
    if (_PyUnicode_Resize(&v, (int)(p - buf)))
	{
		goto onError;
    }
	return (PyObject *)v;
    
ucnhashError:
    PyErr_SetString(
        PyExc_UnicodeError,
        "\\N escapes not supported (can't load unicodedata module)"
        );
    return NULL;

onError:
    Py_XDECREF(v);
    return NULL;
}

static const Py_UNICODE *findchar(const Py_UNICODE *s,
				  int size,
				  Py_UNICODE ch);

static PyObject *unicodeescape_string(const Py_UNICODE *s,
                               int size,
                               int quotes)
{
    PyObject *repr;
    char *p;

    static const char *hexdigit = "0123456789abcdef";

    repr = PyString_FromStringAndSize(NULL, 2 + 6*size + 1);
    if (repr == NULL)
    {
		return NULL;
	}

    p = PyString_AS_STRING(repr);

    if (quotes) 
	{
        *p++ = 'u';
        *p++ = (findchar(s, size, '\'') && 
                !findchar(s, size, '"')) ? '"' : '\'';
    }
    while (size-- > 0) 
	{
        Py_UNICODE ch = *s++;

        if (quotes && 
			(ch == (Py_UNICODE) PyString_AS_STRING(repr)[1] || ch == '\\')) 
		{
            *p++ = '\\';
            *p++ = (char) ch;
			continue;
        }
		else if (ch >= 0xD800 && ch < 0xDC00) 
		{
			Py_UNICODE ch2;
			Py_UCS4 ucs;
			
			ch2 = *s++;
			size--;
			if (ch2 >= 0xDC00 && ch2 <= 0xDFFF) 
			{
				ucs = (((ch & 0x03FF) << 10) | (ch2 & 0x03FF)) + 0x00010000;
				*p++ = '\\';
				*p++ = 'U';
				*p++ = hexdigit[(ucs >> 28) & 0x0000000F];
				*p++ = hexdigit[(ucs >> 24) & 0x0000000F];
				*p++ = hexdigit[(ucs >> 20) & 0x0000000F];
				*p++ = hexdigit[(ucs >> 16) & 0x0000000F];
				*p++ = hexdigit[(ucs >> 12) & 0x0000000F];
				*p++ = hexdigit[(ucs >> 8) & 0x0000000F];
				*p++ = hexdigit[(ucs >> 4) & 0x0000000F];
				*p++ = hexdigit[ucs & 0x0000000F];
				continue;
			}
			s--;
			size++;
		}

		if (ch >= 256) 
		{
			*p++ = '\\';
			*p++ = 'u';
			*p++ = hexdigit[(ch >> 12) & 0x000F];
			*p++ = hexdigit[(ch >> 8) & 0x000F];
			*p++ = hexdigit[(ch >> 4) & 0x000F];
			*p++ = hexdigit[ch & 0x000F];
		}
		else if (ch == '\t') 
		{
			*p++ = '\\';
			*p++ = 't';
		}
		else if (ch == '\n') 
		{
			*p++ = '\\';
			*p++ = 'n';
		}
		else if (ch == '\r') 
		{
			*p++ = '\\';
			*p++ = 'r';
		}
		else if (ch < ' ' || ch >= 0x7F) 
		{
			*p++ = '\\';
			*p++ = 'x';
			*p++ = hexdigit[(ch >> 4) & 0x000F];
			*p++ = hexdigit[ch & 0x000F];
		} 
		else
		{
			*p++ = (char) ch;
		}
    }
    if (quotes)
	{
		*p++ = PyString_AS_STRING(repr)[1];
	}

    *p = '\0';
    _PyString_Resize(&repr, p - PyString_AS_STRING(repr));
    return repr;
}

PyObject *PyUnicode_EncodeUnicodeEscape(const Py_UNICODE *s,
					int size)
{
    return unicodeescape_string(s, size, 0);
}

PyObject *PyUnicode_AsUnicodeEscapeString(PyObject *unicode)
{
    if (!PyUnicode_Check(unicode)) 
	{
        PyErr_BadArgument();
        return NULL;
    }
    return PyUnicode_EncodeUnicodeEscape(PyUnicode_AS_UNICODE(unicode),
					 PyUnicode_GET_SIZE(unicode));
}

PyObject *PyUnicode_DecodeRawUnicodeEscape(const char *s,
					   int size,
					   const char *errors)
{
    PyUnicodeObject *v;
    Py_UNICODE *p, *buf;
    const char *end;
    const char *bs;
    
    v = _PyUnicode_New(size);
    if (v == NULL)
	{
		goto onError;
    }
	if (size == 0)
	{
		return (PyObject *)v;
    }
	p = buf = PyUnicode_AS_UNICODE(v);
    end = s + size;
    while (s < end) 
	{
		unsigned char c;
		Py_UCS4 x;
		int i;

		if (*s != '\\') 
		{
			*p++ = (unsigned char)*s++;
			continue;
		}

		bs = s;
		for (;s < end;) 
		{
			if (*s != '\\')
			{
				break;
			}
			*p++ = (unsigned char)*s++;
		}
		if (((s - bs) & 1) == 0 ||
			s >= end ||
			*s != 'u') 
		{
			continue;
		}
		p--;
		s++;

		for (x = 0, i = 0; i < 4; i++) 
		{
			c = (unsigned char)s[i];
			if (!isxdigit(c)) 
			{
				if (unicodeescape_decoding_error(&p, errors,
					"truncated \\uXXXX"))
				{
					goto onError;
				}
				x = 0xffffffff;
				i++;
				break;
			}
			x = (x<<4) & ~0xF;
			if (c >= '0' && c <= '9')
			{
				x += c - '0';
			}
			else if (c >= 'a' && c <= 'f')
			{
				x += 10 + c - 'a';
			}
			else
			{
				x += 10 + c - 'A';
			}
		}
		s += i;
		if (x != 0xffffffff)
		{
			*p++ = x;
		}
    }
    if (_PyUnicode_Resize(&v, (int)(p - buf)))
	{
		goto onError;
    }
	return (PyObject *)v;
    
onError:
    Py_XDECREF(v);
    return NULL;
}

PyObject *PyUnicode_EncodeRawUnicodeEscape(const Py_UNICODE *s,
					   int size)
{
    PyObject *repr;
    char *p;
    char *q;

    static const char *hexdigit = "0123456789abcdef";

    repr = PyString_FromStringAndSize(NULL, 6 * size);
    if (repr == NULL)
	{
		return NULL;
    }
	if (size == 0)
	{
		return repr;
	}

    p = q = PyString_AS_STRING(repr);
    while (size-- > 0) 
	{
        Py_UNICODE ch = *s++;
		if (ch >= 256) 
		{
            *p++ = '\\';
            *p++ = 'u';
            *p++ = hexdigit[(ch >> 12) & 0xf];
            *p++ = hexdigit[(ch >> 8) & 0xf];
            *p++ = hexdigit[(ch >> 4) & 0xf];
            *p++ = hexdigit[ch & 15];
        }
		else
        {
			*p++ = (char) ch;
		}
    }
    *p = '\0';
    _PyString_Resize(&repr, p - q);
    return repr;
}

PyObject *PyUnicode_AsRawUnicodeEscapeString(PyObject *unicode)
{
    if (!PyUnicode_Check(unicode)) 
	{
		PyErr_BadArgument();
		return NULL;
    }
    return PyUnicode_EncodeRawUnicodeEscape(PyUnicode_AS_UNICODE(unicode),
					    PyUnicode_GET_SIZE(unicode));
}

PyObject *PyUnicode_DecodeLatin1(const char *s,
				 int size,
				 const char *errors)
{
    PyUnicodeObject *v;
    Py_UNICODE *p;
    
    if (size == 1 && *(unsigned char*)s < 256) 
	{
		Py_UNICODE r = *(unsigned char*)s;
		return PyUnicode_FromUnicode(&r, 1);
    }

    v = _PyUnicode_New(size);
    if (v == NULL)
	{
		goto onError;
    }
	if (size == 0)
	{
		return (PyObject *)v;
    }
	p = PyUnicode_AS_UNICODE(v);
    while (size-- > 0)
	{
		*p++ = (unsigned char)*s++;
    }
	return (PyObject *)v;
    
onError:
    Py_XDECREF(v);
    return NULL;
}

static int latin1_encoding_error(const Py_UNICODE **source,
			  char **dest,
			  const char *errors,
			  const char *details) 
{
    if ((errors == NULL) ||
		(strcmp(errors, "strict") == 0)) 
	{
		PyErr_Format(PyExc_UnicodeError,
		     "Latin-1 encoding error: %.400s",
		     details);
		return -1;
    }
    else if (strcmp(errors, "ignore") == 0) 
	{
		return 0;
    }
    else if (strcmp(errors,"replace") == 0) 
	{
		**dest = '?';
		(*dest)++;
		return 0;
    }
    else 
	{
		PyErr_Format(PyExc_ValueError,
		     "Latin-1 encoding error; "
		     "unknown error handling code: %.400s",
		     errors);
		return -1;
    }
}

PyObject *PyUnicode_EncodeLatin1(const Py_UNICODE *p,
				 int size,
				 const char *errors)
{
    PyObject *repr;
    char *s, *start;

    repr = PyString_FromStringAndSize(NULL, size);
    if (repr == NULL)
    {
		return NULL;
    }
	if (size == 0)
	{
		return repr;
	}

    s = PyString_AS_STRING(repr);
    start = s;
    while (size-- > 0) 
	{
        Py_UNICODE ch = *p++;
		if (ch >= 256) 
		{
			if (latin1_encoding_error(&p, &s, errors, 
				      "ordinal not in range(256)"))
			{
				goto onError;
			}
		}
		else
		{
            *s++ = (char)ch;
		}
    }
    if (s - start < PyString_GET_SIZE(repr))
	{
		_PyString_Resize(&repr, s - start);
    }
	return repr;

onError:
    Py_DECREF(repr);
    return NULL;
}

PyObject *PyUnicode_AsLatin1String(PyObject *unicode)
{
    if (!PyUnicode_Check(unicode)) 
	{
		PyErr_BadArgument();
		return NULL;
    }
    return PyUnicode_EncodeLatin1(PyUnicode_AS_UNICODE(unicode),
				  PyUnicode_GET_SIZE(unicode),
				  NULL);
}

static int ascii_decoding_error(const char **source,
			 Py_UNICODE **dest,
			 const char *errors,
			 const char *details) 
{
    if ((errors == NULL) ||
		(strcmp(errors, "strict") == 0)) 
	{
		PyErr_Format(PyExc_UnicodeError,
		     "ASCII decoding error: %.400s",
		     details);
		return -1;
    }
    else if (strcmp(errors, "ignore") == 0) 
	{
		return 0;
    }
    else if (strcmp(errors, "replace") == 0) 
	{
		**dest = Py_UNICODE_REPLACEMENT_CHARACTER;
		(*dest)++;
		return 0;
    }
    else 
	{
		PyErr_Format(PyExc_ValueError,
		     "ASCII decoding error; "
		     "unknown error handling code: %.400s",
		     errors);
		return -1;
    }
}

PyObject *PyUnicode_DecodeASCII(const char *s,
				int size,
				const char *errors)
{
    PyUnicodeObject *v;
    Py_UNICODE *p;
    
    if (size == 1 && *(unsigned char*)s < 128) 
	{
		Py_UNICODE r = *(unsigned char*)s;
		return PyUnicode_FromUnicode(&r, 1);
    }
    
    v = _PyUnicode_New(size);
    if (v == NULL)
	{
		goto onError;
    }
	if (size == 0)
	{
		return (PyObject *)v;
    }
	p = PyUnicode_AS_UNICODE(v);
    while (size-- > 0) 
	{
		unsigned char c;

		c = (unsigned char)*s++;
		if (c < 128)
		{
			*p++ = c;
		}
		else if (ascii_decoding_error(&s, &p, errors, 
						  "ordinal not in range(128)"))
		{
			goto onError;
		}
    }
    if (p - PyUnicode_AS_UNICODE(v) < PyString_GET_SIZE(v))
	{
		if (_PyUnicode_Resize(&v, (int)(p - PyUnicode_AS_UNICODE(v))))
		{
			goto onError;
		}
    }
	return (PyObject *)v;
    
onError:
    Py_XDECREF(v);
    return NULL;
}

static int ascii_encoding_error(const Py_UNICODE **source,
			 char **dest,
			 const char *errors,
			 const char *details) 
{
    if ((errors == NULL) ||
		(strcmp(errors, "strict") == 0)) 
	{
		PyErr_Format(PyExc_UnicodeError,
		     "ASCII encoding error: %.400s",
		     details);
		return -1;
    }
    else if (strcmp(errors, "ignore") == 0) 
	{
		return 0;
    }
    else if (strcmp(errors, "replace") == 0) 
	{
		**dest = '?';
		(*dest)++;
		return 0;
    }
    else 
	{
		PyErr_Format(PyExc_ValueError,
		     "ASCII encoding error; "
		     "unknown error handling code: %.400s",
		     errors);
		return -1;
    }
}

PyObject *PyUnicode_EncodeASCII(const Py_UNICODE *p,
				int size,
				const char *errors)
{
    PyObject *repr;
    char *s, *start;

    repr = PyString_FromStringAndSize(NULL, size);
    if (repr == NULL)
	{
		return NULL;
    }
	if (size == 0)
	{
		return repr;
	}

    s = PyString_AS_STRING(repr);
    start = s;
    while (size-- > 0) 
	{
        Py_UNICODE ch = *p++;
		if (ch >= 128) 
		{
			if (ascii_encoding_error(&p, &s, errors, 
				      "ordinal not in range(128)"))
			{
				goto onError;
			}
		}
		else
		{
			*s++ = (char)ch;
		}
	}
    if (s - start < PyString_GET_SIZE(repr))
	{
		_PyString_Resize(&repr, s - start);
    }
	return repr;

onError:
    Py_DECREF(repr);
    return NULL;
}

PyObject *PyUnicode_AsASCIIString(PyObject *unicode)
{
    if (!PyUnicode_Check(unicode)) 
	{
		PyErr_BadArgument();
		return NULL;
    }
    return PyUnicode_EncodeASCII(PyUnicode_AS_UNICODE(unicode),
				 PyUnicode_GET_SIZE(unicode),
				 NULL);
}

static int charmap_decoding_error(const char **source,
			 Py_UNICODE **dest,
			 const char *errors,
			 const char *details) 
{
    if ((errors == NULL) ||
		(strcmp(errors,"strict") == 0)) 
	{
		PyErr_Format(PyExc_UnicodeError,
		     "charmap decoding error: %.400s",
		     details);
		return -1;
    }
    else if (strcmp(errors,"ignore") == 0) 
	{
		return 0;
    }
    else if (strcmp(errors,"replace") == 0) 
	{
		**dest = Py_UNICODE_REPLACEMENT_CHARACTER;
		(*dest)++;
		return 0;
    }
    else 
	{
		PyErr_Format(PyExc_ValueError,
		     "charmap decoding error; "
		     "unknown error handling code: %.400s",
		     errors);
		return -1;
    }
}

PyObject *PyUnicode_DecodeCharmap(const char *s,
				  int size,
				  PyObject *mapping,
				  const char *errors)
{
    PyUnicodeObject *v;
    Py_UNICODE *p;
    int extrachars = 0;
    
    if (mapping == NULL)
	{
		return PyUnicode_DecodeLatin1(s, size, errors);
	}

    v = _PyUnicode_New(size);
    if (v == NULL)
	{
		goto onError;
    }
	if (size == 0)
	{
		return (PyObject *)v;
    }
	p = PyUnicode_AS_UNICODE(v);
    while (size-- > 0) 
	{
		unsigned char ch = *s++;
		PyObject *w, *x;

		w = PyInt_FromLong((long)ch);
		if (w == NULL)
		{
			goto onError;
		}
		x = PyObject_GetItem(mapping, w);
		Py_DECREF(w);
		if (x == NULL) 
		{
			if (PyErr_ExceptionMatches(PyExc_LookupError)) 
			{
				PyErr_Clear();
				x = Py_None;
				Py_INCREF(x);
			} 
			else
			{
				goto onError;
			}
		}

		if (PyInt_Check(x)) 
		{
			long value = PyInt_AS_LONG(x);
			if (value < 0 || value > 65535) 
			{
				PyErr_SetString(PyExc_TypeError,
						"character mapping must be in range(65536)");
				Py_DECREF(x);
				goto onError;
			}
			*p++ = (Py_UNICODE)value;
		}
		else if (x == Py_None) 
		{
			if (charmap_decoding_error(&s, &p, errors, 
						   "character maps to <undefined>")) 
			{
				Py_DECREF(x);
				goto onError;
			}
		}
		else if (PyUnicode_Check(x)) 
		{
			int targetsize = PyUnicode_GET_SIZE(x);

			if (targetsize == 1)
			{
				*p++ = *PyUnicode_AS_UNICODE(x);
			}
			else if (targetsize > 1) 
			{
				if (targetsize > extrachars) 
				{
					int oldpos = (int)(p - PyUnicode_AS_UNICODE(v));
					int needed = (targetsize - extrachars) + \
							 (targetsize << 2);
					extrachars += needed;
					if (_PyUnicode_Resize(&v, 
							 PyUnicode_GET_SIZE(v) + needed)) 
					{
						Py_DECREF(x);
						goto onError;
					}
					p = PyUnicode_AS_UNICODE(v) + oldpos;
				}
				Py_UNICODE_COPY(p,
						PyUnicode_AS_UNICODE(x),
						targetsize);
				p += targetsize;
				extrachars -= targetsize;
			}
		}
		else 
		{
			PyErr_SetString(PyExc_TypeError,
			  "character mapping must return integer, None or unicode");
			Py_DECREF(x);
			goto onError;
		}
		Py_DECREF(x);
    }
    if (p - PyUnicode_AS_UNICODE(v) < PyUnicode_GET_SIZE(v))
	{
		if (_PyUnicode_Resize(&v, (int)(p - PyUnicode_AS_UNICODE(v))))
		{
			goto onError;
		}
	}
	return (PyObject *)v;
    
onError:
    Py_XDECREF(v);
    return NULL;
}

static int charmap_encoding_error(const Py_UNICODE **source,
			   char **dest,
			   const char *errors,
			   const char *details) 
{
    if ((errors == NULL) ||
		(strcmp(errors,"strict") == 0)) 
	{
		PyErr_Format(PyExc_UnicodeError,
		     "charmap encoding error: %.400s",
		     details);
		return -1;
    }
    else if (strcmp(errors,"ignore") == 0) 
	{
		return 0;
    }
    else if (strcmp(errors,"replace") == 0) 
	{
		**dest = '?';
		(*dest)++;
		return 0;
    }
    else 
	{
		PyErr_Format(PyExc_ValueError,
		     "charmap encoding error; "
		     "unknown error handling code: %.400s",
		     errors);
		return -1;
    }
}

PyObject *PyUnicode_EncodeCharmap(const Py_UNICODE *p,
				  int size,
				  PyObject *mapping,
				  const char *errors)
{
    PyObject *v;
    char *s;
    int extrachars = 0;

    if (mapping == NULL)
	{
		return PyUnicode_EncodeLatin1(p, size, errors);
	}

    v = PyString_FromStringAndSize(NULL, size);
    if (v == NULL)
    {
		return NULL;
    }
	if (size == 0)
	{
		return v;
    }
	s = PyString_AS_STRING(v);
    while (size-- > 0) 
	{
		Py_UNICODE ch = *p++;
		PyObject *w, *x;

		w = PyInt_FromLong((long)ch);
		if (w == NULL)
		{
			goto onError;
		}
		x = PyObject_GetItem(mapping, w);
		Py_DECREF(w);
		if (x == NULL) 
		{
			if (PyErr_ExceptionMatches(PyExc_LookupError)) 
			{
				PyErr_Clear();
				x = Py_None;
				Py_INCREF(x);
			} 
			else
			{
				goto onError;
			}
		}

		if (PyInt_Check(x)) 
		{
			long value = PyInt_AS_LONG(x);
			if (value < 0 || value > 255) 
			{
				PyErr_SetString(PyExc_TypeError,
						"character mapping must be in range(256)");
				Py_DECREF(x);
				goto onError;
			}
			*s++ = (char)value;
		}
		else if (x == Py_None) 
		{
			if (charmap_encoding_error(&p, &s, errors, 
						   "character maps to <undefined>")) 
			{
				Py_DECREF(x);
				goto onError;
			}
		}
		else if (PyString_Check(x)) 
		{
			int targetsize = PyString_GET_SIZE(x);

			if (targetsize == 1)
			{
				*s++ = *PyString_AS_STRING(x);
			}
			else if (targetsize > 1) 
			{
				if (targetsize > extrachars) 
				{
					int oldpos = (int)(s - PyString_AS_STRING(v));
					int needed = (targetsize - extrachars) + (targetsize << 2);
					extrachars += needed;
					if (_PyString_Resize(&v, PyString_GET_SIZE(v) + needed)) 
					{
						Py_DECREF(x);
						goto onError;
					}
					s = PyString_AS_STRING(v) + oldpos;
				}
				memcpy(s, PyString_AS_STRING(x), targetsize);
				s += targetsize;
				extrachars -= targetsize;
			}
		}
		else 
		{
			PyErr_SetString(PyExc_TypeError,
				"character mapping must return integer, None or unicode");
			Py_DECREF(x);
			goto onError;
		}
		Py_DECREF(x);
    }
    if (s - PyString_AS_STRING(v) < PyString_GET_SIZE(v))
	{
		_PyString_Resize(&v, (int)(s - PyString_AS_STRING(v)));
    }
	return v;

onError:
    Py_XDECREF(v);
    return NULL;
}

PyObject *PyUnicode_AsCharmapString(PyObject *unicode,
				    PyObject *mapping)
{
    if (!PyUnicode_Check(unicode) || mapping == NULL) 
	{
		PyErr_BadArgument();
		return NULL;
    }
    return PyUnicode_EncodeCharmap(PyUnicode_AS_UNICODE(unicode),
				   PyUnicode_GET_SIZE(unicode),
				   mapping,
				   NULL);
}

static int translate_error(const Py_UNICODE **source,
		    Py_UNICODE **dest,
		    const char *errors,
		    const char *details) 
{
    if ((errors == NULL) ||
		(strcmp(errors, "strict") == 0)) 
	{
		PyErr_Format(PyExc_UnicodeError,
		     "translate error: %.400s",
		     details);
		return -1;
    }
    else if (strcmp(errors, "ignore") == 0) 
	{
		return 0;
    }
    else if (strcmp(errors, "replace") == 0) 
	{
		**dest = '?';
		(*dest)++;
		return 0;
    }
    else 
	{
		PyErr_Format(PyExc_ValueError,
		     "translate error; "
		     "unknown error handling code: %.400s",
		     errors);
		return -1;
    }
}

PyObject *PyUnicode_TranslateCharmap(const Py_UNICODE *s,
				     int size,
				     PyObject *mapping,
				     const char *errors)
{
    PyUnicodeObject *v;
    Py_UNICODE *p;
    
    if (mapping == NULL) 
	{
		PyErr_BadArgument();
		return NULL;
    }
    
    v = _PyUnicode_New(size);
    if (v == NULL)
	{
		goto onError;
    }
	if (size == 0)
	{
		goto done;
    }
	p = PyUnicode_AS_UNICODE(v);
    while (size-- > 0) 
	{
		Py_UNICODE ch = *s++;
		PyObject *w, *x;

		w = PyInt_FromLong(ch);
		if (w == NULL)
		{
			goto onError;
		}
		x = PyObject_GetItem(mapping, w);
		Py_DECREF(w);
		if (x == NULL) 
		{
			if (PyErr_ExceptionMatches(PyExc_LookupError)) 
			{
				PyErr_Clear();
				*p++ = ch;
				continue;
			}
			goto onError;
		}

		if (PyInt_Check(x))
		{
			*p++ = (Py_UNICODE)PyInt_AS_LONG(x);
		}
		else if (x == Py_None) 
		{
			if (translate_error(&s, &p, errors, 
					"character maps to <undefined>")) 
			{
				Py_DECREF(x);
				goto onError;
			}
		}
		else if (PyUnicode_Check(x)) 
		{
			if (PyUnicode_GET_SIZE(x) != 1) 
			{
				PyErr_SetString(PyExc_NotImplementedError,
						"1-n mappings are currently not implemented");
				Py_DECREF(x);
				goto onError;
			}
			*p++ = *PyUnicode_AS_UNICODE(x);
		}
		else 
		{
			PyErr_SetString(PyExc_TypeError,
				"translate mapping must return integer, None or unicode");
			Py_DECREF(x);
			goto onError;
		}
		Py_DECREF(x);
    }
    if (p - PyUnicode_AS_UNICODE(v) < PyUnicode_GET_SIZE(v))
	{
		if (_PyUnicode_Resize(&v, (int)(p - PyUnicode_AS_UNICODE(v))))
		{
			goto onError;
		}
	}

done:
    return (PyObject *)v;
    
onError:
    Py_XDECREF(v);
    return NULL;
}

PyObject *PyUnicode_Translate(PyObject *str,
			      PyObject *mapping,
			      const char *errors)
{
    PyObject *result;
    
    str = PyUnicode_FromObject(str);
    if (str == NULL)
	{
		goto onError;
    }
	result = PyUnicode_TranslateCharmap(PyUnicode_AS_UNICODE(str),
					PyUnicode_GET_SIZE(str),
					mapping,
					errors);
    Py_DECREF(str);
    return result;
    
onError:
    Py_XDECREF(str);
    return NULL;
}

int PyUnicode_EncodeDecimal(Py_UNICODE *s,
			    int length,
			    char *output,
			    const char *errors)
{
    Py_UNICODE *p, *end;

    if (output == NULL) 
	{
		PyErr_BadArgument();
		return -1;
    }

    p = s;
    end = s + length;
    while (p < end) 
	{
		Py_UNICODE ch = *p++;
		int decimal;
		
		if (Py_UNICODE_ISSPACE(ch)) 
		{
			*output++ = ' ';
			continue;
		}
		decimal = Py_UNICODE_TODECIMAL(ch);
		if (decimal >= 0) 
		{
			*output++ = '0' + decimal;
			continue;
		}
		if (0 < ch && ch < 256) 
		{
			*output++ = (char)ch;
			continue;
		}
		if (errors == NULL || strcmp(errors, "strict") == 0) 
		{
			PyErr_SetString(PyExc_ValueError,
					"invalid decimal Unicode string");
			goto onError;
		}
		else if (strcmp(errors, "ignore") == 0)
		{
			continue;
		}
		else if (strcmp(errors, "replace") == 0) 
		{
			*output++ = '?';
			continue;
		}
    }
    *output++ = '\0';
    return 0;

onError:
    return -1;
}

static int count(PyUnicodeObject *self,
	  int start,
	  int end,
	  PyUnicodeObject *substring)
{
    int count = 0;

    if (start < 0)
	{
        start += self->length;
    }
	if (start < 0)
    {
		start = 0;
    }
	if (end > self->length)
    {
		end = self->length;
    }
	if (end < 0)
    {
		end += self->length;
    }
	if (end < 0)
    {
		end = 0;
	}

    if (substring->length == 0)
	{
		return (end - start + 1);
	}

    end -= substring->length;

    while (start <= end)
    {
		if (Py_UNICODE_MATCH(self, start, substring)) 
		{
            count++;
            start += substring->length;
        } 
		else
        {
			start++;
		}
	}

    return count;
}

int PyUnicode_Count(PyObject *str,
		    PyObject *substr,
		    int start,
		    int end)
{
    int result;
    
    str = PyUnicode_FromObject(str);
    if (str == NULL)
	{
		return -1;
    }
	substr = PyUnicode_FromObject(substr);
    if (substr == NULL) 
	{
		Py_DECREF(str);
		return -1;
    }
    
    result = count((PyUnicodeObject *)str,
		   start, end,
		   (PyUnicodeObject *)substr);
    
    Py_DECREF(str);
    Py_DECREF(substr);
    return result;
}

static int findstring(PyUnicodeObject *self,
	       PyUnicodeObject *substring,
	       int start,
	       int end,
	       int direction)
{
    if (start < 0)
    {
		start += self->length;
    }
	if (start < 0)
    {
		start = 0;
	}

    if (end > self->length)
    {
		end = self->length;
    }
	if (end < 0)
    {
		end += self->length;
    }
	if (end < 0)
    {
		end = 0;
	}

    if (substring->length == 0)
	{
		return (direction > 0) ? start : end;
	}

    end -= substring->length;

    if (direction < 0) 
	{
        for (; end >= start; end--)
        {
			if (Py_UNICODE_MATCH(self, end, substring))
			{
				return end;
			}
		}
    } 
	else 
	{
        for (; start <= end; start++)
		{
			if (Py_UNICODE_MATCH(self, start, substring))
			{
				return start;
			}
		}
    }

    return -1;
}

int PyUnicode_Find(PyObject *str,
		   PyObject *substr,
		   int start,
		   int end,
		   int direction)
{
    int result;
    
    str = PyUnicode_FromObject(str);
    if (str == NULL)
	{
		return -1;
    }
	substr = PyUnicode_FromObject(substr);
    if (substr == NULL) 
	{
		Py_DECREF(substr);
		return -1;
    }
    
    result = findstring((PyUnicodeObject *)str,
			(PyUnicodeObject *)substr,
			start, end, direction);
    Py_DECREF(str);
    Py_DECREF(substr);
    return result;
}

static int tailmatch(PyUnicodeObject *self,
	      PyUnicodeObject *substring,
	      int start,
	      int end,
	      int direction)
{
    if (start < 0)
	{
		start += self->length;
    }
	if (start < 0)
    {
		start = 0;
	}

    if (substring->length == 0)
    {
		return 1;
	}

    if (end > self->length)
    {
		end = self->length;
    }
	if (end < 0)
    {
		end += self->length;
    }
	if (end < 0)
    {
		end = 0;
	}

    end -= substring->length;
    if (end < start)
	{
		return 0;
	}

    if (direction > 0) 
	{
		if (Py_UNICODE_MATCH(self, end, substring))
		{
			return 1;
		}
    } 
	else 
	{
        if (Py_UNICODE_MATCH(self, start, substring))
		{
			return 1;
		}
    }

    return 0;
}

int PyUnicode_Tailmatch(PyObject *str,
			PyObject *substr,
			int start,
			int end,
			int direction)
{
    int result;
    
    str = PyUnicode_FromObject(str);
    if (str == NULL)
	{
		return -1;
    }
	substr = PyUnicode_FromObject(substr);
    if (substr == NULL) 
	{
		Py_DECREF(substr);
		return -1;
    }
    
    result = tailmatch((PyUnicodeObject *)str,
		       (PyUnicodeObject *)substr,
		       start, end, direction);
    Py_DECREF(str);
    Py_DECREF(substr);
    return result;
}

static const Py_UNICODE *findchar(const Py_UNICODE *s,
		     int size,
		     Py_UNICODE ch)
{
    while (size-- > 0) 
	{
        if (*s == ch)
		{
			return s;
		}
        s++;
    }

    return NULL;
}

static PyObject *fixup(PyUnicodeObject *self,
		int (*fixfct)(PyUnicodeObject *s))
{

    PyUnicodeObject *u;

    u = (PyUnicodeObject*) PyUnicode_FromUnicode(NULL, self->length);
    if (u == NULL)
	{
		return NULL;
	}

    Py_UNICODE_COPY(u->str, self->str, self->length);

    if (!fixfct(u) && PyUnicode_CheckExact(self)) 
	{
		Py_INCREF(self);
		Py_DECREF(u);
		return (PyObject*) self;
    }
    return (PyObject*) u;
}

static int fixupper(PyUnicodeObject *self)
{
    int len = self->length;
    Py_UNICODE *s = self->str;
    int status = 0;
    
    while (len-- > 0) 
	{
		Py_UNICODE ch;
		
		ch = Py_UNICODE_TOUPPER(*s);
		if (ch != *s) 
		{
			status = 1;
			*s = ch;
		}
        s++;
    }

    return status;
}

static int fixlower(PyUnicodeObject *self)
{
    int len = self->length;
    Py_UNICODE *s = self->str;
    int status = 0;
    
    while (len-- > 0) 
	{
		Py_UNICODE ch;
		
		ch = Py_UNICODE_TOLOWER(*s);
		if (ch != *s) 
		{
			status = 1;
			*s = ch;
		}
        s++;
    }

    return status;
}

static int fixswapcase(PyUnicodeObject *self)
{
    int len = self->length;
    Py_UNICODE *s = self->str;
    int status = 0;
    
    while (len-- > 0) 
	{
        if (Py_UNICODE_ISUPPER(*s)) 
		{
            *s = Py_UNICODE_TOLOWER(*s);
            status = 1;
        } 
		else if (Py_UNICODE_ISLOWER(*s)) 
		{
            *s = Py_UNICODE_TOUPPER(*s);
            status = 1;
        }
        s++;
    }

    return status;
}

static int fixcapitalize(PyUnicodeObject *self)
{
    int len = self->length;
    Py_UNICODE *s = self->str;
    int status = 0;
    
    if (len == 0)
	{
		return 0;
    }
	if (Py_UNICODE_ISLOWER(*s)) 
	{
		*s = Py_UNICODE_TOUPPER(*s);
		status = 1;
    }
    s++;
    while (--len > 0) 
	{
        if (Py_UNICODE_ISUPPER(*s)) 
		{
            *s = Py_UNICODE_TOLOWER(*s);
            status = 1;
        }
        s++;
    }
    return status;
}

static int fixtitle(PyUnicodeObject *self)
{
    Py_UNICODE *p = PyUnicode_AS_UNICODE(self);
    Py_UNICODE *e;
    int previous_is_cased;

    if (PyUnicode_GET_SIZE(self) == 1) 
	{
		Py_UNICODE ch = Py_UNICODE_TOTITLE(*p);
		if (*p != ch) 
		{
			*p = ch;
			return 1;
		}
		else
		{
			return 0;
		}
	}
    
	e = p + PyUnicode_GET_SIZE(self);
	previous_is_cased = 0;
	for (; p < e; p++) 
	{
		const Py_UNICODE ch = *p;
		
		if (previous_is_cased)
		{
			*p = Py_UNICODE_TOLOWER(ch);
		}
		else
		{
			*p = Py_UNICODE_TOTITLE(ch);
		}

		if (Py_UNICODE_ISLOWER(ch) || 
			Py_UNICODE_ISUPPER(ch) || 
			Py_UNICODE_ISTITLE(ch))
		{
			previous_is_cased = 1;
		}
		else
		{
			previous_is_cased = 0;
		}
    }
    return 1;
}

PyObject *PyUnicode_Join(PyObject *separator,
			 PyObject *seq)
{
    Py_UNICODE *sep;
    int seplen;
    PyUnicodeObject *res = NULL;
    int reslen = 0;
    Py_UNICODE *p;
    int sz = 100;
    int i;
    PyObject *it;

    it = PyObject_GetIter(seq);
    if (it == NULL)
    {
		return NULL;
	}

    if (separator == NULL) 
	{
		Py_UNICODE blank = ' ';
		sep = &blank;
		seplen = 1;
    }
    else 
	{
		separator = PyUnicode_FromObject(separator);
		if (separator == NULL)
		{
			goto onError;
		}
		sep = PyUnicode_AS_UNICODE(separator);
		seplen = PyUnicode_GET_SIZE(separator);
    }
    
    res = _PyUnicode_New(sz);
    if (res == NULL)
	{
		goto onError;
    }
	p = PyUnicode_AS_UNICODE(res);
    reslen = 0;

    for (i = 0; ; ++i) 
	{
		int itemlen;
		PyObject *item = PyIter_Next(it);
		if (item == NULL) 
		{
			if (PyErr_Occurred())
			{
				goto onError;
			}
			break;
		}
		if (!PyUnicode_Check(item)) 
		{
			PyObject *v;
			if (!PyString_Check(item)) 
			{
				PyErr_Format(PyExc_TypeError,
					 "sequence item %i: expected string or Unicode,"
					 " %.80s found",
					 i, item->ob_type->tp_name);
				Py_DECREF(item);
				goto onError;
			}
			v = PyUnicode_FromObject(item);
			Py_DECREF(item);
			item = v;
			if (item == NULL)
			{
				goto onError;
			}
		}
		itemlen = PyUnicode_GET_SIZE(item);
		while (reslen + itemlen + seplen >= sz) 
		{
			if (_PyUnicode_Resize(&res, sz * 2)) 
			{
				Py_DECREF(item);
				goto onError;
			}
			sz *= 2;
			p = PyUnicode_AS_UNICODE(res) + reslen;
		}
		if (i > 0) 
		{
			Py_UNICODE_COPY(p, sep, seplen);
			p += seplen;
			reslen += seplen;
		}
		Py_UNICODE_COPY(p, PyUnicode_AS_UNICODE(item), itemlen);
		p += itemlen;
		reslen += itemlen;
		Py_DECREF(item);
    }
    if (_PyUnicode_Resize(&res, reslen))
	{
		goto onError;
	}

    Py_XDECREF(separator);
    Py_DECREF(it);
    return (PyObject *)res;

onError:
    Py_XDECREF(separator);
    Py_XDECREF(res);
    Py_DECREF(it);
    return NULL;
}

static 
PyUnicodeObject *pad(PyUnicodeObject *self, 
		     int left, 
		     int right,
		     Py_UNICODE fill)
{
    PyUnicodeObject *u;

    if (left < 0)
    {
		left = 0;
    }
	if (right < 0)
    {
		right = 0;
	}

    if (left == 0 && right == 0 && PyUnicode_CheckExact(self)) 
	{
        Py_INCREF(self);
        return self;
    }

    u = _PyUnicode_New(left + self->length + right);
    if (u) 
	{
        if (left)
		{
			Py_UNICODE_FILL(u->str, fill, left);
        }
		Py_UNICODE_COPY(u->str + left, self->str, self->length);
        if (right)
        {
			Py_UNICODE_FILL(u->str + left + self->length, fill, right);
		}
    }

    return u;
}

#define SPLIT_APPEND(data, left, right)					\
	str = PyUnicode_FromUnicode(data + left, right - left);		\
	if (!str)							\
	    goto onError;						\
	if (PyList_Append(list, str)) {					\
	    Py_DECREF(str);						\
	    goto onError;						\
	}								\
        else								\
            Py_DECREF(str);

static PyObject *split_whitespace(PyUnicodeObject *self,
			   PyObject *list,
			   int maxcount)
{
    int i;
    int j;
    int len = self->length;
    PyObject *str;

    for (i = j = 0; i < len; ) 
	{
		while (i < len && Py_UNICODE_ISSPACE(self->str[i]))
		{
			i++;
		}
		j = i;
		while (i < len && !Py_UNICODE_ISSPACE(self->str[i]))
		{
			i++;
		}
		if (j < i) 
		{
			if (maxcount-- <= 0)
			{
				break;
			}
			SPLIT_APPEND(self->str, j, i);
			while (i < len && Py_UNICODE_ISSPACE(self->str[i]))
			{
				i++;
			}
			j = i;
		}
    }
    if (j < len) 
	{
		SPLIT_APPEND(self->str, j, len);
    }
    return list;

onError:
    Py_DECREF(list);
    return NULL;
}

PyObject *PyUnicode_Splitlines(PyObject *string,
			       int keepends)
{
    int i;
    int j;
    int len;
    PyObject *list;
    PyObject *str;
    Py_UNICODE *data;

    string = PyUnicode_FromObject(string);
    if (string == NULL)
	{
		return NULL;
    }
	data = PyUnicode_AS_UNICODE(string);
    len = PyUnicode_GET_SIZE(string);

    list = PyList_New(0);
    if (!list)
    {
		goto onError;
	}

    for (i = j = 0; i < len; ) 
	{
		int eol;
		
		while (i < len && !Py_UNICODE_ISLINEBREAK(data[i]))
		{
			i++;
		}

		eol = i;
		if (i < len) 
		{
			if (data[i] == '\r' && i + 1 < len &&
				data[i+1] == '\n')
			{
				i += 2;
			}
			else
			{
				i++;
			}
			if (keepends)
			{
				eol = i;
			}
		}
		SPLIT_APPEND(data, j, eol);
		j = i;
    }
    if (j < len) 
	{
		SPLIT_APPEND(data, j, len);
    }

    Py_DECREF(string);
    return list;

onError:
    Py_DECREF(list);
    Py_DECREF(string);
    return NULL;
}

static PyObject *split_char(PyUnicodeObject *self,
		     PyObject *list,
		     Py_UNICODE ch,
		     int maxcount)
{
    int i;
    int j;
    int len = self->length;
    PyObject *str;

    for (i = j = 0; i < len; ) 
	{
		if (self->str[i] == ch) 
		{
			if (maxcount-- <= 0)
			{
				break;
			}
			SPLIT_APPEND(self->str, j, i);
			i = j = i + 1;
		} 
		else
		{
			i++;
		}
	}
	if (j <= len) 
	{
		SPLIT_APPEND(self->str, j, len);
    }
    return list;

onError:
    Py_DECREF(list);
    return NULL;
}

static PyObject *split_substring(PyUnicodeObject *self,
			  PyObject *list,
			  PyUnicodeObject *substring,
			  int maxcount)
{
    int i;
    int j;
    int len = self->length;
    int sublen = substring->length;
    PyObject *str;

    for (i = j = 0; i <= len - sublen; ) 
	{
		if (Py_UNICODE_MATCH(self, i, substring)) 
		{
			if (maxcount-- <= 0)
			{
				break;
			}
			SPLIT_APPEND(self->str, j, i);
			i = j = i + sublen;
		} 
		else
		{
			i++;
		}
	}
	if (j <= len) 
	{
		SPLIT_APPEND(self->str, j, len);
    }
    return list;

onError:
    Py_DECREF(list);
    return NULL;
}

#undef SPLIT_APPEND

static PyObject *split(PyUnicodeObject *self,
		PyUnicodeObject *substring,
		int maxcount)
{
    PyObject *list;

    if (maxcount < 0)
    {
		maxcount = INT_MAX;
	}

    list = PyList_New(0);
    if (!list)
    {
		return NULL;
	}

    if (substring == NULL)
	{
		return split_whitespace(self,list,maxcount);
	}
	else if (substring->length == 1)
	{
		return split_char(self,list,substring->str[0],maxcount);
	}
    else if (substring->length == 0) 
	{
		Py_DECREF(list);
		PyErr_SetString(PyExc_ValueError, "empty separator");
		return NULL;
    }
    else
	{
		return split_substring(self,list,substring,maxcount);
	}
}

static PyObject *replace(PyUnicodeObject *self,
		  PyUnicodeObject *str1,
		  PyUnicodeObject *str2,
		  int maxcount)
{
    PyUnicodeObject *u;

    if (maxcount < 0)
	{
		maxcount = INT_MAX;
	}

    if (str1->length == 1 && str2->length == 1) 
	{
        int i;

        if (!findchar(self->str, self->length, str1->str[0]) &&
            PyUnicode_CheckExact(self)) 
		{
            Py_INCREF(self);
            u = self;
        } 
		else 
		{
			Py_UNICODE u1 = str1->str[0];
			Py_UNICODE u2 = str2->str[0];
	    
            u = (PyUnicodeObject*) PyUnicode_FromUnicode(
                NULL,
                self->length
                );
            if (u != NULL) 
			{
				Py_UNICODE_COPY(u->str, self->str, 
					self->length);
                for (i = 0; i < u->length; i++)
                {
					if (u->str[i] == u1) 
					{
                        if (--maxcount < 0)
						{
							break;
                        }
						u->str[i] = u2;
                    }
				}
			}
        }
    } 
	else 
	{
        int n, i;
        Py_UNICODE *p;

        n = count(self, 0, self->length, str1);
        if (n > maxcount)
        {
			n = maxcount;
        }
		if (n == 0) 
		{
            if (PyUnicode_CheckExact(self)) 
			{
                Py_INCREF(self);
                u = self;
            }
            else 
			{
                u = (PyUnicodeObject *)
                    PyUnicode_FromUnicode(self->str, self->length);
			}
        } 
		else 
		{
            u = _PyUnicode_New(
                self->length + n * (str2->length - str1->length));
            if (u) 
			{
                i = 0;
                p = u->str;
                while (i <= self->length - str1->length)
				{
                    if (Py_UNICODE_MATCH(self, i, str1)) 
					{
                        Py_UNICODE_COPY(p, str2->str, str2->length);
                        p += str2->length;
                        i += str1->length;
                        if (--n <= 0) 
						{
                            Py_UNICODE_COPY(p, self->str+i, self->length-i);
                            break;
                        }
                    } 
					else
                    {
						*p++ = self->str[i++];
					}
				}
            }
        }
    }
    
    return (PyObject *) u;
}


static char title__doc__[] =
	"S.title() -> unicode\n"
	"\n"
	"Return a titlecased version of S, i.e. words start with title case\n"
	"characters, all remaining cased characters have lower case.";

static PyObject *unicode_title(PyUnicodeObject *self)
{
    return fixup(self, fixtitle);
}

static char capitalize__doc__[] =
	"S.capitalize() -> unicode\n"
	"\n"
	"Return a capitalized version of S, i.e. make the first character\n"
	"have upper case.";

static PyObject *unicode_capitalize(PyUnicodeObject *self)
{
    return fixup(self, fixcapitalize);
}

static char center__doc__[] =
	"S.center(width) -> unicode\n"
	"\n"
	"Return S centered in a Unicode string of length width. Padding is done\n"
	"using spaces.";

static PyObject *unicode_center(PyUnicodeObject *self, PyObject *args)
{
    int marg, left;
    int width;

    if (!PyArg_ParseTuple(args, "i:center", &width))
	{
		return NULL;
	}

    if (self->length >= width && PyUnicode_CheckExact(self)) 
	{
        Py_INCREF(self);
        return (PyObject*) self;
    }

    marg = width - self->length;
    left = marg / 2 + (marg & width & 1);

    return (PyObject*) pad(self, left, marg - left, ' ');
}

static int unicode_compare(PyUnicodeObject *str1, PyUnicodeObject *str2)
{
    int len1, len2;

    Py_UNICODE *s1 = str1->str;
    Py_UNICODE *s2 = str2->str;

    len1 = str1->length;
    len2 = str2->length;
    
    while (len1 > 0 && len2 > 0) 
	{
        Py_UNICODE c1, c2;     

        c1 = *s1++;
        c2 = *s2++;

        if (c1 != c2)
		{
			return (c1 < c2) ? -1 : 1;
		}

        len1--; 
		len2--;
    }

    return (len1 < len2) ? -1 : (len1 != len2);
}

int PyUnicode_Compare(PyObject *left,
		      PyObject *right)
{
    PyUnicodeObject *u = NULL, *v = NULL;
    int result;

    u = (PyUnicodeObject *)PyUnicode_FromObject(left);
    if (u == NULL)
	{
		goto onError;
    }
	v = (PyUnicodeObject *)PyUnicode_FromObject(right);
    if (v == NULL)
	{
		goto onError;
	}

    if (v == u)
	{
		Py_DECREF(u);
		Py_DECREF(v);
		return 0;
    }

    result = unicode_compare(u, v);

    Py_DECREF(u);
    Py_DECREF(v);
    return result;

onError:
    Py_XDECREF(u);
    Py_XDECREF(v);
    return -1;
}

int PyUnicode_Contains(PyObject *container,
		       PyObject *element)
{
    PyUnicodeObject *u = NULL, *v = NULL;
    int result;
    const Py_UNICODE *p, *e;
    Py_UNICODE ch;

    v = (PyUnicodeObject *)PyUnicode_FromObject(element);
    if (v == NULL) 
	{
		PyErr_SetString(PyExc_TypeError,
			"'in <string>' requires character as left operand");
		goto onError;
    }
    u = (PyUnicodeObject *)PyUnicode_FromObject(container);
    if (u == NULL) 
	{
		Py_DECREF(v);
		goto onError;
    }

    if (PyUnicode_GET_SIZE(v) != 1) 
	{
		PyErr_SetString(PyExc_TypeError,
			"'in <string>' requires character as left operand");
		goto onError;
    }
    ch = *PyUnicode_AS_UNICODE(v);
    p = PyUnicode_AS_UNICODE(u);
    e = p + PyUnicode_GET_SIZE(u);
    result = 0;
    while (p < e) 
	{
		if (*p++ == ch) 
		{
			result = 1;
			break;
		}
    }

    Py_DECREF(u);
    Py_DECREF(v);
    return result;

onError:
    Py_XDECREF(u);
    Py_XDECREF(v);
    return -1;
}

PyObject *PyUnicode_Concat(PyObject *left,
			   PyObject *right)
{
    PyUnicodeObject *u = NULL, *v = NULL, *w;

    u = (PyUnicodeObject *)PyUnicode_FromObject(left);
    if (u == NULL)
	{
		goto onError;
    }
	v = (PyUnicodeObject *)PyUnicode_FromObject(right);
    if (v == NULL)
	{
		goto onError;
	}

    if (v == unicode_empty) 
	{
		Py_DECREF(v);
		return (PyObject *)u;
    }
    if (u == unicode_empty) 
	{
		Py_DECREF(u);
		return (PyObject *)v;
    }

    w = _PyUnicode_New(u->length + v->length);
    if (w == NULL)
	{
		goto onError;
    }
	Py_UNICODE_COPY(w->str, u->str, u->length);
    Py_UNICODE_COPY(w->str + u->length, v->str, v->length);

    Py_DECREF(u);
    Py_DECREF(v);
    return (PyObject *)w;

onError:
    Py_XDECREF(u);
    Py_XDECREF(v);
    return NULL;
}

static char count__doc__[] =
	"S.count(sub[, start[, end]]) -> int\n"
	"\n"
	"Return the number of occurrences of substring sub in Unicode string\n"
	"S[start:end].  Optional arguments start and end are\n"
	"interpreted as in slice notation.";

static PyObject *unicode_count(PyUnicodeObject *self, PyObject *args)
{
    PyUnicodeObject *substring;
    int start = 0;
    int end = INT_MAX;
    PyObject *result;

    if (!PyArg_ParseTuple(args, "O|O&O&:count", &substring,
			_PyEval_SliceIndex, &start, _PyEval_SliceIndex, &end))
    {
		return NULL;
	}

    substring = (PyUnicodeObject *)PyUnicode_FromObject(
						(PyObject *)substring);
    if (substring == NULL)
	{
		return NULL;
    }

    if (start < 0)
    {
		start += self->length;
    }
	if (start < 0)
    {
		start = 0;
    }
	if (end > self->length)
    {
		end = self->length;
    }
	if (end < 0)
    {
		end += self->length;
    }
	if (end < 0)
    {
		end = 0;
	}

    result = PyInt_FromLong((long) count(self, start, end, substring));

    Py_DECREF(substring);
    return result;
}

static char encode__doc__[] =
	"S.encode([encoding[,errors]]) -> string\n"
	"\n"
	"Return an encoded string version of S. Default encoding is the current\n"
	"default string encoding. errors may be given to set a different error\n"
	"handling scheme. Default is 'strict' meaning that encoding errors raise\n"
	"a ValueError. Other possible values are 'ignore' and 'replace'.";

static PyObject *unicode_encode(PyUnicodeObject *self, PyObject *args)
{
    char *encoding = NULL;
    char *errors = NULL;
    if (!PyArg_ParseTuple(args, "|ss:encode", &encoding, &errors))
	{
		return NULL;
    }
	return PyUnicode_AsEncodedString((PyObject *)self, encoding, errors);
}

static char expandtabs__doc__[] =
	"S.expandtabs([tabsize]) -> unicode\n"
	"\n"
	"Return a copy of S where all tab characters are expanded using spaces.\n"
	"If tabsize is not given, a tab size of 8 characters is assumed.";

static PyObject *unicode_expandtabs(PyUnicodeObject *self, PyObject *args)
{
    Py_UNICODE *e;
    Py_UNICODE *p;
    Py_UNICODE *q;
    int i, j;
    PyUnicodeObject *u;
    int tabsize = 8;

    if (!PyArg_ParseTuple(args, "|i:expandtabs", &tabsize))
	{
		return NULL;
	}

    i = j = 0;
    e = self->str + self->length;
    for (p = self->str; p < e; p++)
    {
		if (*p == '\t') 
		{
			if (tabsize > 0)
			{
				j += tabsize - (j % tabsize);
			}
		}
        else 
		{
            j++;
            if (*p == '\n' || *p == '\r') 
			{
                i += j;
                j = 0;
            }
        }
	}

    u = _PyUnicode_New(i + j);
    if (!u)
	{
		return NULL;
	}

    j = 0;
    q = u->str;

    for (p = self->str; p < e; p++)
	{
        if (*p == '\t') 
		{
			if (tabsize > 0) 
			{
				i = tabsize - (j % tabsize);
				j += i;
				while (i--)
				{
					*q++ = ' ';
				}
			}
		}
		else 
		{
			j++;
			*q++ = *p;
			if (*p == '\n' || *p == '\r')
			{
				j = 0;
			}
		}
	}

    return (PyObject*) u;
}

static char find__doc__[] =
	"S.find(sub [,start [,end]]) -> int\n"
	"\n"
	"Return the lowest index in S where substring sub is found,\n"
	"such that sub is contained within s[start,end].  Optional\n"
	"arguments start and end are interpreted as in slice notation.\n"
	"\n"
	"Return -1 on failure.";

static PyObject *unicode_find(PyUnicodeObject *self, PyObject *args)
{
    PyUnicodeObject *substring;
    int start = 0;
    int end = INT_MAX;
    PyObject *result;

    if (!PyArg_ParseTuple(args, "O|O&O&:find", &substring,
			_PyEval_SliceIndex, &start, _PyEval_SliceIndex, &end))
    {
		return NULL;
    }
	substring = (PyUnicodeObject *)PyUnicode_FromObject(
						(PyObject *)substring);
    if (substring == NULL)
	{
		return NULL;
	}

    result = PyInt_FromLong(findstring(self, substring, start, end, 1));

    Py_DECREF(substring);
    return result;
}

static PyObject *unicode_getitem(PyUnicodeObject *self, int index)
{
    if (index < 0 || index >= self->length) 
	{
        PyErr_SetString(PyExc_IndexError, "string index out of range");
        return NULL;
    }

    return (PyObject*) PyUnicode_FromUnicode(&self->str[index], 1);
}

static long unicode_hash(PyUnicodeObject *self)
{
    int len;
    Py_UNICODE *p;
    long x;

    if (self->hash != -1)
	{
		return self->hash;
    }
	len = PyUnicode_GET_SIZE(self);
    p = PyUnicode_AS_UNICODE(self);
    x = *p << 7;
    while (--len >= 0)
	{
		x = (1000003 * x) ^ *p++;
    }
	x ^= PyUnicode_GET_SIZE(self);
    if (x == -1)
	{
		x = -2;
    }
	self->hash = x;
    return x;
}

static char index__doc__[] =
	"S.index(sub [,start [,end]]) -> int\n"
	"\n"
	"Like S.find() but raise ValueError when the substring is not found.";

static PyObject *unicode_index(PyUnicodeObject *self, PyObject *args)
{
    int result;
    PyUnicodeObject *substring;
    int start = 0;
    int end = INT_MAX;

    if (!PyArg_ParseTuple(args, "O|O&O&:index", &substring,
			_PyEval_SliceIndex, &start, _PyEval_SliceIndex, &end))
	{
        return NULL;
    }

    substring = (PyUnicodeObject *)PyUnicode_FromObject(
						(PyObject *)substring);
    if (substring == NULL)
	{
		return NULL;
	}

    result = findstring(self, substring, start, end, 1);

    Py_DECREF(substring);
    if (result < 0) 
	{
        PyErr_SetString(PyExc_ValueError, "substring not found");
        return NULL;
    }
    return PyInt_FromLong(result);
}

static char islower__doc__[] =
	"S.islower() -> int\n"
	"\n"
	"Return 1 if  all cased characters in S are lowercase and there is\n"
	"at least one cased character in S, 0 otherwise.";

static PyObject *unicode_islower(PyUnicodeObject *self)
{
    const Py_UNICODE *p = PyUnicode_AS_UNICODE(self);
    const Py_UNICODE *e;
    int cased;

    if (PyUnicode_GET_SIZE(self) == 1)
	{
		return PyInt_FromLong(Py_UNICODE_ISLOWER(*p) != 0);
	}

    if (PyString_GET_SIZE(self) == 0)
	{
		return PyInt_FromLong(0);
	}

    e = p + PyUnicode_GET_SIZE(self);
    cased = 0;
    for (; p < e; p++) 
	{
		const Py_UNICODE ch = *p;
		
		if (Py_UNICODE_ISUPPER(ch) || Py_UNICODE_ISTITLE(ch))
		{
			return PyInt_FromLong(0);
		}
		else if (!cased && Py_UNICODE_ISLOWER(ch))
		{
			cased = 1;
		}
    }
    return PyInt_FromLong(cased);
}

static char isupper__doc__[] =
	"S.isupper() -> int\n"
	"\n"
	"Return 1 if  all cased characters in S are uppercase and there is\n"
	"at least one cased character in S, 0 otherwise.";

static PyObject *unicode_isupper(PyUnicodeObject *self)
{
    const Py_UNICODE *p = PyUnicode_AS_UNICODE(self);
    const Py_UNICODE *e;
    int cased;

    if (PyUnicode_GET_SIZE(self) == 1)
	{
		return PyInt_FromLong(Py_UNICODE_ISUPPER(*p) != 0);
	}

    if (PyString_GET_SIZE(self) == 0)
	{
		return PyInt_FromLong(0);
	}

    e = p + PyUnicode_GET_SIZE(self);
    cased = 0;
    for (; p < e; p++) 
	{
		const Py_UNICODE ch = *p;
		
		if (Py_UNICODE_ISLOWER(ch) || Py_UNICODE_ISTITLE(ch))
		{
			return PyInt_FromLong(0);
		}
		else if (!cased && Py_UNICODE_ISUPPER(ch))
		{
			cased = 1;
		}
    }
    return PyInt_FromLong(cased);
}

static char istitle__doc__[] =
	"S.istitle() -> int\n"
	"\n"
	"Return 1 if S is a titlecased string, i.e. upper- and titlecase characters\n"
	"may only follow uncased characters and lowercase characters only cased\n"
	"ones. Return 0 otherwise.";

static PyObject *unicode_istitle(PyUnicodeObject *self)
{
    const Py_UNICODE *p = PyUnicode_AS_UNICODE(self);
    const Py_UNICODE *e;
    int cased, previous_is_cased;

    if (PyUnicode_GET_SIZE(self) == 1)
	{
		return PyInt_FromLong((Py_UNICODE_ISTITLE(*p) != 0) ||
			      (Py_UNICODE_ISUPPER(*p) != 0));
	}

    if (PyString_GET_SIZE(self) == 0)
	{
		return PyInt_FromLong(0);
	}

    e = p + PyUnicode_GET_SIZE(self);
    cased = 0;
    previous_is_cased = 0;
    for (; p < e; p++) 
	{
		const Py_UNICODE ch = *p;
		
		if (Py_UNICODE_ISUPPER(ch) || Py_UNICODE_ISTITLE(ch)) 
		{
			if (previous_is_cased)
			{
				return PyInt_FromLong(0);
			}
			previous_is_cased = 1;
			cased = 1;
		}
		else if (Py_UNICODE_ISLOWER(ch)) 
		{
			if (!previous_is_cased)
			{
				return PyInt_FromLong(0);
			}
			previous_is_cased = 1;
			cased = 1;
		}
		else
		{
			previous_is_cased = 0;
		}
    }
    return PyInt_FromLong(cased);
}

static char isspace__doc__[] =
	"S.isspace() -> int\n"
	"\n"
	"Return 1 if there are only whitespace characters in S,\n"
	"0 otherwise.";

static PyObject *unicode_isspace(PyUnicodeObject *self)
{
    const Py_UNICODE *p = PyUnicode_AS_UNICODE(self);
    const Py_UNICODE *e;

    if (PyUnicode_GET_SIZE(self) == 1 &&
		Py_UNICODE_ISSPACE(*p))
	{
		return PyInt_FromLong(1);
	}

    if (PyString_GET_SIZE(self) == 0)
	{
		return PyInt_FromLong(0);
	}

    e = p + PyUnicode_GET_SIZE(self);
    for (; p < e; p++) 
	{
		if (!Py_UNICODE_ISSPACE(*p))
		{
			return PyInt_FromLong(0);
		}
    }
    return PyInt_FromLong(1);
}

static char isalpha__doc__[] =
	"S.isalpha() -> int\n"
	"\n"
	"Return 1 if  all characters in S are alphabetic\n"
	"and there is at least one character in S, 0 otherwise.";

static PyObject *unicode_isalpha(PyUnicodeObject *self)
{
    const Py_UNICODE *p = PyUnicode_AS_UNICODE(self);
    const Py_UNICODE *e;

    if (PyUnicode_GET_SIZE(self) == 1 &&
		Py_UNICODE_ISALPHA(*p))
	{
		return PyInt_FromLong(1);
	}

    if (PyString_GET_SIZE(self) == 0)
	{
		return PyInt_FromLong(0);
	}

    e = p + PyUnicode_GET_SIZE(self);
    for (; p < e; p++) 
	{
		if (!Py_UNICODE_ISALPHA(*p))
		{
			return PyInt_FromLong(0);
		}
    }
    return PyInt_FromLong(1);
}

static char isalnum__doc__[] =
	"S.isalnum() -> int\n"
	"\n"
	"Return 1 if  all characters in S are alphanumeric\n"
	"and there is at least one character in S, 0 otherwise.";

static PyObject *unicode_isalnum(PyUnicodeObject *self)
{
    const Py_UNICODE *p = PyUnicode_AS_UNICODE(self);
    const Py_UNICODE *e;

    if (PyUnicode_GET_SIZE(self) == 1 &&
		Py_UNICODE_ISALNUM(*p))
	{
		return PyInt_FromLong(1);
	}

    if (PyString_GET_SIZE(self) == 0)
	{
		return PyInt_FromLong(0);
	}

    e = p + PyUnicode_GET_SIZE(self);
    for (; p < e; p++) 
	{
		if (!Py_UNICODE_ISALNUM(*p))
		{
			return PyInt_FromLong(0);
		}
    }
    return PyInt_FromLong(1);
}

static char isdecimal__doc__[] =
	"S.isdecimal() -> int\n"
	"\n"
	"Return 1 if there are only decimal characters in S,\n"
	"0 otherwise.";

static PyObject *unicode_isdecimal(PyUnicodeObject *self)
{
    const Py_UNICODE *p = PyUnicode_AS_UNICODE(self);
    const Py_UNICODE *e;

    if (PyUnicode_GET_SIZE(self) == 1 &&
		Py_UNICODE_ISDECIMAL(*p))
	{
		return PyInt_FromLong(1);
	}

    if (PyString_GET_SIZE(self) == 0)
	{
		return PyInt_FromLong(0);
	}

    e = p + PyUnicode_GET_SIZE(self);
    for (; p < e; p++) 
	{
		if (!Py_UNICODE_ISDECIMAL(*p))
		{
			return PyInt_FromLong(0);
		}
    }
    return PyInt_FromLong(1);
}

static char isdigit__doc__[] =
	"S.isdigit() -> int\n"
	"\n"
	"Return 1 if there are only digit characters in S,\n"
	"0 otherwise.";

static PyObject *unicode_isdigit(PyUnicodeObject *self)
{
    const Py_UNICODE *p = PyUnicode_AS_UNICODE(self);
    const Py_UNICODE *e;

    if (PyUnicode_GET_SIZE(self) == 1 &&
		Py_UNICODE_ISDIGIT(*p))
	{
		return PyInt_FromLong(1);
	}

    if (PyString_GET_SIZE(self) == 0)
	{
		return PyInt_FromLong(0);
	}

    e = p + PyUnicode_GET_SIZE(self);
    for (; p < e; p++) 
	{
		if (!Py_UNICODE_ISDIGIT(*p))
		{
			return PyInt_FromLong(0);
		}
    }
    return PyInt_FromLong(1);
}

static char isnumeric__doc__[] =
	"S.isnumeric() -> int\n"
	"\n"
	"Return 1 if there are only numeric characters in S,\n"
	"0 otherwise.";

static PyObject *unicode_isnumeric(PyUnicodeObject *self)
{
    const Py_UNICODE *p = PyUnicode_AS_UNICODE(self);
    const Py_UNICODE *e;

    if (PyUnicode_GET_SIZE(self) == 1 &&
		Py_UNICODE_ISNUMERIC(*p))
	{
		return PyInt_FromLong(1);
	}

    if (PyString_GET_SIZE(self) == 0)
	{
		return PyInt_FromLong(0);
	}

    e = p + PyUnicode_GET_SIZE(self);
    for (; p < e; p++) 
	{
		if (!Py_UNICODE_ISNUMERIC(*p))
		{
			return PyInt_FromLong(0);
		}
    }
    return PyInt_FromLong(1);
}

static char join__doc__[] =
	"S.join(sequence) -> unicode\n"
	"\n"
	"Return a string which is the concatenation of the strings in the\n"
	"sequence.  The separator between elements is S.";

static PyObject *unicode_join(PyObject *self, PyObject *data)
{
    return PyUnicode_Join(self, data);
}

static int unicode_length(PyUnicodeObject *self)
{
    return self->length;
}

static char ljust__doc__[] =
	"S.ljust(width) -> unicode\n"
	"\n"
	"Return S left justified in a Unicode string of length width. Padding is\n"
	"done using spaces.";

static PyObject *unicode_ljust(PyUnicodeObject *self, PyObject *args)
{
    int width;
    if (!PyArg_ParseTuple(args, "i:ljust", &width))
	{
		return NULL;
	}

    if (self->length >= width && PyUnicode_CheckExact(self)) 
	{
        Py_INCREF(self);
        return (PyObject*) self;
    }

    return (PyObject*) pad(self, 0, width - self->length, ' ');
}

static char lower__doc__[] =
	"S.lower() -> unicode\n"
	"\n"
	"Return a copy of the string S converted to lowercase.";

static PyObject *unicode_lower(PyUnicodeObject *self)
{
    return fixup(self, fixlower);
}

#define LEFTSTRIP 0
#define RIGHTSTRIP 1
#define BOTHSTRIP 2

static const char *stripformat[] = {"|O:lstrip", "|O:rstrip", "|O:strip"};

#define STRIPNAME(i) (stripformat[i]+3)

static const Py_UNICODE *unicode_memchr(const Py_UNICODE *s, Py_UNICODE c, size_t n)
{
	size_t i;
	for (i = 0; i < n; ++i)
	{
		if (s[i] == c)
		{
			return s + i;
		}
	}
	return NULL;
}

PyObject *_PyUnicode_XStrip(PyUnicodeObject *self, int striptype, PyObject *sepobj)
{
	Py_UNICODE *s = PyUnicode_AS_UNICODE(self);
	int len = PyUnicode_GET_SIZE(self);
	Py_UNICODE *sep = PyUnicode_AS_UNICODE(sepobj);
	int seplen = PyUnicode_GET_SIZE(sepobj);
	int i, j;

	i = 0;
	if (striptype != RIGHTSTRIP) 
	{
		while (i < len && unicode_memchr(sep, s[i], seplen)) 
		{
			i++;
		}
	}

	j = len;
	if (striptype != LEFTSTRIP) 
	{
		do 
		{
			j--;
		} while (j >= i && unicode_memchr(sep, s[j], seplen));
		j++;
	}

	if (i == 0 && j == len && PyUnicode_CheckExact(self)) 
	{
		Py_INCREF(self);
		return (PyObject*)self;
	}
	else
	{
		return PyUnicode_FromUnicode(s + i, j - i);
	}
}


static PyObject *do_strip(PyUnicodeObject *self, int striptype)
{
	Py_UNICODE *s = PyUnicode_AS_UNICODE(self);
	int len = PyUnicode_GET_SIZE(self), i, j;

	i = 0;
	if (striptype != RIGHTSTRIP) 
	{
		while (i < len && Py_UNICODE_ISSPACE(s[i])) 
		{
			i++;
		}
	}

	j = len;
	if (striptype != LEFTSTRIP) 
	{
		do 
		{
			j--;
		} while (j >= i && Py_UNICODE_ISSPACE(s[j]));
		j++;
	}

	if (i == 0 && j == len && PyUnicode_CheckExact(self)) 
	{
		Py_INCREF(self);
		return (PyObject*)self;
	}
	else
	{
		return PyUnicode_FromUnicode(s + i, j - i);
	}
}


static PyObject *do_argstrip(PyUnicodeObject *self, int striptype, PyObject *args)
{
	PyObject *sep = NULL;

	if (!PyArg_ParseTuple(args, (char *)stripformat[striptype], &sep))
	{
		return NULL;
	}

	if (sep != NULL && sep != Py_None) 
	{
		if (PyUnicode_Check(sep))
		{
			return _PyUnicode_XStrip(self, striptype, sep);
		}
		else if (PyString_Check(sep)) 
		{
			PyObject *res;
			sep = PyUnicode_FromObject(sep);
			if (sep == NULL)
			{
				return NULL;
			}
			res = _PyUnicode_XStrip(self, striptype, sep);
			Py_DECREF(sep);
			return res;
		}
		else 
		{
			PyErr_Format(PyExc_TypeError,
				     "%s arg must be None, unicode or str",
				     STRIPNAME(striptype));
			return NULL;
		}
	}

	return do_strip(self, striptype);
}


static char strip__doc__[] =
	"S.strip([sep]) -> unicode\n"
	"\n"
	"Return a copy of the string S with leading and trailing\n"
	"whitespace removed.\n"
	"If sep is given and not None, remove characters in sep instead.\n"
	"If sep is a str, it will be converted to unicode before stripping";

static PyObject *unicode_strip(PyUnicodeObject *self, PyObject *args)
{
	if (PyTuple_GET_SIZE(args) == 0)
	{
		return do_strip(self, BOTHSTRIP);
	}
	else
	{
		return do_argstrip(self, BOTHSTRIP, args);
	}
}


static char lstrip__doc__[] =
	"S.lstrip([sep]) -> unicode\n"
	"\n"
	"Return a copy of the string S with leading whitespace removed.\n"
	"If sep is given and not None, remove characters in sep instead.\n"
	"If sep is a str, it will be converted to unicode before stripping";

static PyObject *unicode_lstrip(PyUnicodeObject *self, PyObject *args)
{
	if (PyTuple_GET_SIZE(args) == 0)
	{
		return do_strip(self, LEFTSTRIP);
	}
	else
	{
		return do_argstrip(self, LEFTSTRIP, args);
	}
}


static char rstrip__doc__[] =
	"S.rstrip([sep]) -> unicode\n"
	"\n"
	"Return a copy of the string S with trailing whitespace removed.\n"
	"If sep is given and not None, remove characters in sep instead.\n"
	"If sep is a str, it will be converted to unicode before stripping";

static PyObject *unicode_rstrip(PyUnicodeObject *self, PyObject *args)
{
	if (PyTuple_GET_SIZE(args) == 0)
	{
		return do_strip(self, RIGHTSTRIP);
	}
	else
	{
		return do_argstrip(self, RIGHTSTRIP, args);
	}
}


static PyObject *unicode_repeat(PyUnicodeObject *str, int len)
{
    PyUnicodeObject *u;
    Py_UNICODE *p;
    int nchars;
    size_t nbytes;

    if (len < 0)
	{
        len = 0;
	}

    if (len == 1 && PyUnicode_CheckExact(str)) 
	{
        Py_INCREF(str);
        return (PyObject*) str;
    }

    nchars = len * str->length;
    if (len && nchars / len != str->length) 
	{
        PyErr_SetString(PyExc_OverflowError,
                        "repeated string is too long");
        return NULL;
    }
    nbytes = (nchars + 1) * sizeof(Py_UNICODE);
    if (nbytes / sizeof(Py_UNICODE) != (size_t)(nchars + 1)) 
	{
        PyErr_SetString(PyExc_OverflowError,
                        "repeated string is too long");
        return NULL;
    }
    u = _PyUnicode_New(nchars);
    if (!u)
    {
		return NULL;
	}

    p = u->str;

    while (len-- > 0) 
	{
        Py_UNICODE_COPY(p, str->str, str->length);
        p += str->length;
    }

    return (PyObject*) u;
}

PyObject *PyUnicode_Replace(PyObject *obj,
			    PyObject *subobj,
			    PyObject *replobj,
			    int maxcount)
{
    PyObject *self;
    PyObject *str1;
    PyObject *str2;
    PyObject *result;

    self = PyUnicode_FromObject(obj);
    if (self == NULL)
	{
		return NULL;
    }
	str1 = PyUnicode_FromObject(subobj);
    if (str1 == NULL) 
	{
		Py_DECREF(self);
		return NULL;
    }
    str2 = PyUnicode_FromObject(replobj);
    if (str2 == NULL) 
	{
		Py_DECREF(self);
		Py_DECREF(str1);
		return NULL;
    }
    result = replace((PyUnicodeObject *)self, 
		     (PyUnicodeObject *)str1, 
		     (PyUnicodeObject *)str2, 
		     maxcount);
    Py_DECREF(self);
    Py_DECREF(str1);
    Py_DECREF(str2);
    return result;
}

static char replace__doc__[] =
	"S.replace (old, new[, maxsplit]) -> unicode\n"
	"\n"
	"Return a copy of S with all occurrences of substring\n"
	"old replaced by new.  If the optional argument maxsplit is\n"
	"given, only the first maxsplit occurrences are replaced.";

static PyObject *unicode_replace(PyUnicodeObject *self, PyObject *args)
{
    PyUnicodeObject *str1;
    PyUnicodeObject *str2;
    int maxcount = -1;
    PyObject *result;

    if (!PyArg_ParseTuple(args, "OO|i:replace", &str1, &str2, &maxcount))
    {
		return NULL;
    }
	str1 = (PyUnicodeObject *)PyUnicode_FromObject((PyObject *)str1);
    if (str1 == NULL)
	{
		return NULL;
    }
	str2 = (PyUnicodeObject *)PyUnicode_FromObject((PyObject *)str2);
    if (str2 == NULL)
	{
		return NULL;
	}

    result = replace(self, str1, str2, maxcount);

    Py_DECREF(str1);
    Py_DECREF(str2);
    return result;
}

static PyObject *unicode_repr(PyObject *unicode)
{
    return unicodeescape_string(PyUnicode_AS_UNICODE(unicode),
				PyUnicode_GET_SIZE(unicode),
				1);
}

static char rfind__doc__[] =
	"S.rfind(sub [,start [,end]]) -> int\n"
	"\n"
	"Return the highest index in S where substring sub is found,\n"
	"such that sub is contained within s[start,end].  Optional\n"
	"arguments start and end are interpreted as in slice notation.\n"
	"\n"
	"Return -1 on failure.";

static PyObject *unicode_rfind(PyUnicodeObject *self, PyObject *args)
{
    PyUnicodeObject *substring;
    int start = 0;
    int end = INT_MAX;
    PyObject *result;

    if (!PyArg_ParseTuple(args, "O|O&O&:rfind", &substring,
		_PyEval_SliceIndex, &start, _PyEval_SliceIndex, &end))
    {
		return NULL;
    }
	substring = (PyUnicodeObject *)PyUnicode_FromObject(
						(PyObject *)substring);
    if (substring == NULL)
	{
		return NULL;
	}

    result = PyInt_FromLong(findstring(self, substring, start, end, -1));

    Py_DECREF(substring);
    return result;
}

static char rindex__doc__[] =
	"S.rindex(sub [,start [,end]]) -> int\n"
	"\n"
	"Like S.rfind() but raise ValueError when the substring is not found.";

static PyObject *unicode_rindex(PyUnicodeObject *self, PyObject *args)
{
    int result;
    PyUnicodeObject *substring;
    int start = 0;
    int end = INT_MAX;

    if (!PyArg_ParseTuple(args, "O|O&O&:rindex", &substring,
		_PyEval_SliceIndex, &start, _PyEval_SliceIndex, &end))
    {
		return NULL;
    }
	substring = (PyUnicodeObject *)PyUnicode_FromObject(
						(PyObject *)substring);
    if (substring == NULL)
	{
		return NULL;
	}

    result = findstring(self, substring, start, end, -1);

    Py_DECREF(substring);
    if (result < 0) 
	{
        PyErr_SetString(PyExc_ValueError, "substring not found");
        return NULL;
    }
    return PyInt_FromLong(result);
}

static char rjust__doc__[] =
	"S.rjust(width) -> unicode\n"
	"\n"
	"Return S right justified in a Unicode string of length width. Padding is\n"
	"done using spaces.";

static PyObject *unicode_rjust(PyUnicodeObject *self, PyObject *args)
{
    int width;
    if (!PyArg_ParseTuple(args, "i:rjust", &width))
	{
		return NULL;
	}

    if (self->length >= width && PyUnicode_CheckExact(self)) 
	{
        Py_INCREF(self);
        return (PyObject*) self;
    }

    return (PyObject*) pad(self, width - self->length, 0, ' ');
}

static PyObject *unicode_slice(PyUnicodeObject *self, int start, int end)
{
    if (start < 0)
	{
		start = 0;
    }
	if (end < 0)
    {
		end = 0;
    }
	if (end > self->length)
    {
		end = self->length;
    }
	if (start == 0 && end == self->length && PyUnicode_CheckExact(self)) 
	{
        Py_INCREF(self);
        return (PyObject*) self;
    }
    if (start > end)
    {
		start = end;
    }
	return (PyObject*) PyUnicode_FromUnicode(self->str + start,
					     end - start);
}

PyObject *PyUnicode_Split(PyObject *s,
			  PyObject *sep,
			  int maxsplit)
{
    PyObject *result;
    
    s = PyUnicode_FromObject(s);
    if (s == NULL)
	{
		return NULL;
    }
	if (sep != NULL) 
	{
		sep = PyUnicode_FromObject(sep);
		if (sep == NULL) 
		{
			Py_DECREF(s);
			return NULL;
		}
    }

    result = split((PyUnicodeObject *)s, (PyUnicodeObject *)sep, maxsplit);

    Py_DECREF(s);
    Py_XDECREF(sep);
    return result;
}

static char split__doc__[] =
	"S.split([sep [,maxsplit]]) -> list of strings\n"
	"\n"
	"Return a list of the words in S, using sep as the\n"
	"delimiter string.  If maxsplit is given, at most maxsplit\n"
	"splits are done. If sep is not specified, any whitespace string\n"
	"is a separator.";

static PyObject *unicode_split(PyUnicodeObject *self, PyObject *args)
{
    PyObject *substring = Py_None;
    int maxcount = -1;

    if (!PyArg_ParseTuple(args, "|Oi:split", &substring, &maxcount))
    {
		return NULL;
	}

    if (substring == Py_None)
	{
		return split(self, NULL, maxcount);
    }
	else if (PyUnicode_Check(substring))
	{
		return split(self, (PyUnicodeObject *)substring, maxcount);
    }
	else
	{
		return PyUnicode_Split((PyObject *)self, substring, maxcount);
	}
}

static char splitlines__doc__[] =
	"S.splitlines([keepends]]) -> list of strings\n"
	"\n"
	"Return a list of the lines in S, breaking at line boundaries.\n"
	"Line breaks are not included in the resulting list unless keepends\n"
	"is given and true.";

static PyObject *unicode_splitlines(PyUnicodeObject *self, PyObject *args)
{
    int keepends = 0;

    if (!PyArg_ParseTuple(args, "|i:splitlines", &keepends))
    {
		return NULL;
	}

    return PyUnicode_Splitlines((PyObject *)self, keepends);
}

static PyObject *unicode_str(PyUnicodeObject *self)
{
    return PyUnicode_AsEncodedString((PyObject *)self, NULL, NULL);
}

static char swapcase__doc__[] =
	"S.swapcase() -> unicode\n"
	"\n"
	"Return a copy of S with uppercase characters converted to lowercase\n"
	"and vice versa.";

static PyObject *unicode_swapcase(PyUnicodeObject *self)
{
    return fixup(self, fixswapcase);
}

static char translate__doc__[] =
	"S.translate(table) -> unicode\n"
	"\n"
	"Return a copy of the string S, where all characters have been mapped\n"
	"through the given translation table, which must be a mapping of\n"
	"Unicode ordinals to Unicode ordinals or None. Unmapped characters\n"
	"are left untouched. Characters mapped to None are deleted.";

static PyObject *unicode_translate(PyUnicodeObject *self, PyObject *table)
{
    return PyUnicode_TranslateCharmap(self->str, 
				      self->length,
				      table, 
				      "ignore");
}

static char upper__doc__[] =
	"S.upper() -> unicode\n"
	"\n"
	"Return a copy of S converted to uppercase.";

static PyObject *unicode_upper(PyUnicodeObject *self)
{
    return fixup(self, fixupper);
}

static char zfill__doc__[] =
	"S.zfill(width) -> unicode\n"
	"\n"
	"Pad a numeric string x with zeros on the left, to fill a field\n"
	"of the specified width. The string x is never truncated.";

static PyObject *unicode_zfill(PyUnicodeObject *self, PyObject *args)
{
    int fill;
    PyUnicodeObject *u;

    int width;
    if (!PyArg_ParseTuple(args, "i:zfill", &width))
    {
		return NULL;
	}

    if (self->length >= width) 
	{
        if (PyUnicode_CheckExact(self)) 
		{
            Py_INCREF(self);
            return (PyObject*) self;
        }
        else
        {
			return PyUnicode_FromUnicode(
                PyUnicode_AS_UNICODE(self),
                PyUnicode_GET_SIZE(self)
            );
		}
    }

    fill = width - self->length;

    u = pad(self, fill, 0, '0');

    if (u == NULL)
    {
		return NULL;
	}

    if (u->str[fill] == '+' || u->str[fill] == '-') 
	{
        u->str[0] = u->str[fill];
        u->str[fill] = '0';
    }

    return (PyObject*) u;
}

static char startswith__doc__[] =
	"S.startswith(prefix[, start[, end]]) -> int\n"
	"\n"
	"Return 1 if S starts with the specified prefix, otherwise return 0.  With\n"
	"optional start, test S beginning at that position.  With optional end, stop\n"
	"comparing S at that position.";

static PyObject *unicode_startswith(PyUnicodeObject *self,
		   PyObject *args)
{
    PyUnicodeObject *substring;
    int start = 0;
    int end = INT_MAX;
    PyObject *result;

    if (!PyArg_ParseTuple(args, "O|O&O&:startswith", &substring,
		_PyEval_SliceIndex, &start, _PyEval_SliceIndex, &end))
	{
		return NULL;
    }
	substring = (PyUnicodeObject *)PyUnicode_FromObject(
						(PyObject *)substring);
    if (substring == NULL)
	{
		return NULL;
	}

    result = PyInt_FromLong(tailmatch(self, substring, start, end, -1));

    Py_DECREF(substring);
    return result;
}

static char endswith__doc__[] =
	"S.endswith(suffix[, start[, end]]) -> int\n"
	"\n"
	"Return 1 if S ends with the specified suffix, otherwise return 0.  With\n"
	"optional start, test S beginning at that position.  With optional end, stop\n"
	"comparing S at that position.";

static PyObject *unicode_endswith(PyUnicodeObject *self,
		 PyObject *args)
{
    PyUnicodeObject *substring;
    int start = 0;
    int end = INT_MAX;
    PyObject *result;

    if (!PyArg_ParseTuple(args, "O|O&O&:endswith", &substring,
		_PyEval_SliceIndex, &start, _PyEval_SliceIndex, &end))
	{
		return NULL;
    }
	substring = (PyUnicodeObject *)PyUnicode_FromObject(
						(PyObject *)substring);
    if (substring == NULL)
	{
		return NULL;
	}

    result = PyInt_FromLong(tailmatch(self, substring, start, end, +1));

    Py_DECREF(substring);
    return result;
}

static PyMethodDef unicode_methods[] = {
    {"encode", (PyCFunction) unicode_encode, METH_VARARGS, encode__doc__},
    {"replace", (PyCFunction) unicode_replace, METH_VARARGS, replace__doc__},
    {"split", (PyCFunction) unicode_split, METH_VARARGS, split__doc__},
    {"join", (PyCFunction) unicode_join, METH_O, join__doc__},
    {"capitalize", (PyCFunction) unicode_capitalize, METH_NOARGS, capitalize__doc__},
    {"title", (PyCFunction) unicode_title, METH_NOARGS, title__doc__},
    {"center", (PyCFunction) unicode_center, METH_VARARGS, center__doc__},
    {"count", (PyCFunction) unicode_count, METH_VARARGS, count__doc__},
    {"expandtabs", (PyCFunction) unicode_expandtabs, METH_VARARGS, expandtabs__doc__},
    {"find", (PyCFunction) unicode_find, METH_VARARGS, find__doc__},
    {"index", (PyCFunction) unicode_index, METH_VARARGS, index__doc__},
    {"ljust", (PyCFunction) unicode_ljust, METH_VARARGS, ljust__doc__},
    {"lower", (PyCFunction) unicode_lower, METH_NOARGS, lower__doc__},
    {"lstrip", (PyCFunction) unicode_lstrip, METH_VARARGS, lstrip__doc__},
    {"rfind", (PyCFunction) unicode_rfind, METH_VARARGS, rfind__doc__},
    {"rindex", (PyCFunction) unicode_rindex, METH_VARARGS, rindex__doc__},
    {"rjust", (PyCFunction) unicode_rjust, METH_VARARGS, rjust__doc__},
    {"rstrip", (PyCFunction) unicode_rstrip, METH_VARARGS, rstrip__doc__},
    {"splitlines", (PyCFunction) unicode_splitlines, METH_VARARGS, splitlines__doc__},
    {"strip", (PyCFunction) unicode_strip, METH_VARARGS, strip__doc__},
    {"swapcase", (PyCFunction) unicode_swapcase, METH_NOARGS, swapcase__doc__},
    {"translate", (PyCFunction) unicode_translate, METH_O, translate__doc__},
    {"upper", (PyCFunction) unicode_upper, METH_NOARGS, upper__doc__},
    {"startswith", (PyCFunction) unicode_startswith, METH_VARARGS, startswith__doc__},
    {"endswith", (PyCFunction) unicode_endswith, METH_VARARGS, endswith__doc__},
    {"islower", (PyCFunction) unicode_islower, METH_NOARGS, islower__doc__},
    {"isupper", (PyCFunction) unicode_isupper, METH_NOARGS, isupper__doc__},
    {"istitle", (PyCFunction) unicode_istitle, METH_NOARGS, istitle__doc__},
    {"isspace", (PyCFunction) unicode_isspace, METH_NOARGS, isspace__doc__},
    {"isdecimal", (PyCFunction) unicode_isdecimal, METH_NOARGS, isdecimal__doc__},
    {"isdigit", (PyCFunction) unicode_isdigit, METH_NOARGS, isdigit__doc__},
    {"isnumeric", (PyCFunction) unicode_isnumeric, METH_NOARGS, isnumeric__doc__},
    {"isalpha", (PyCFunction) unicode_isalpha, METH_NOARGS, isalpha__doc__},
    {"isalnum", (PyCFunction) unicode_isalnum, METH_NOARGS, isalnum__doc__},
    {"zfill", (PyCFunction) unicode_zfill, METH_VARARGS, zfill__doc__},
    {NULL, NULL}
};

static PySequenceMethods unicode_as_sequence = {
    (inquiry) unicode_length, 		
    (binaryfunc) PyUnicode_Concat, 	
    (intargfunc) unicode_repeat, 	
    (intargfunc) unicode_getitem, 	
    (intintargfunc) unicode_slice, 	
    0, 					
    0, 					
    (objobjproc)PyUnicode_Contains,
};

static int unicode_buffer_getreadbuf(PyUnicodeObject *self,
			  int index,
			  const void **ptr)
{
    if (index != 0) 
	{
        PyErr_SetString(PyExc_SystemError,
			"accessing non-existent unicode segment");
        return -1;
    }
    *ptr = (void *) self->str;
    return PyUnicode_GET_DATA_SIZE(self);
}

static int unicode_buffer_getwritebuf(PyUnicodeObject *self, int index,
			   const void **ptr)
{
    PyErr_SetString(PyExc_TypeError,
		    "cannot use unicode as modifyable buffer");
    return -1;
}

static int unicode_buffer_getsegcount(PyUnicodeObject *self,
			   int *lenp)
{
    if (lenp)
    {
		*lenp = PyUnicode_GET_DATA_SIZE(self);
    }
	return 1;
}

static int unicode_buffer_getcharbuf(PyUnicodeObject *self,
			  int index,
			  const void **ptr)
{
    PyObject *str;
    
    if (index != 0) 
	{
        PyErr_SetString(PyExc_SystemError,
			"accessing non-existent unicode segment");
        return -1;
    }
    str = _PyUnicode_AsDefaultEncodedString((PyObject *)self, NULL);
    if (str == NULL)
	{
		return -1;
    }
	*ptr = (void *) PyString_AS_STRING(str);
    return PyString_GET_SIZE(str);
}

static PyObject *getnextarg(PyObject *args, int arglen, int *p_argidx)
{
    int argidx = *p_argidx;
    if (argidx < arglen) 
	{
		(*p_argidx)++;
		if (arglen < 0)
		{
			return args;
		}
		else
		{
			return PyTuple_GetItem(args, argidx);
		}
    }
    PyErr_SetString(PyExc_TypeError,
		    "not enough arguments for format string");
    return NULL;
}

#define F_LJUST (1<<0)
#define F_SIGN	(1<<1)
#define F_BLANK (1<<2)
#define F_ALT	(1<<3)
#define F_ZERO	(1<<4)

static int usprintf(Py_UNICODE *buffer, char *format, ...)
{
    int i;
    int len;
    va_list va;
    char *charbuffer;
    va_start(va, format);

    charbuffer = (char *)buffer;
    len = vsprintf(charbuffer, format, va);
    for (i = len - 1; i >= 0; i--)
	{
		buffer[i] = (Py_UNICODE) charbuffer[i];
	}

    va_end(va);
    return len;
}

static int formatfloat(Py_UNICODE *buf,
	    size_t buflen,
	    int flags,
	    int prec,
	    int type,
	    PyObject *v)
{
    char fmt[20];
    double x;
    
    x = PyFloat_AsDouble(v);
    if (x == -1.0 && PyErr_Occurred())
	{
		return -1;
    }
	if (prec < 0)
	{
		prec = 6;
    }
	if (type == 'f' && (fabs(x) / 1e25) >= 1e25)
	{
		type = 'g';
    }
	PyOS_snprintf(fmt, sizeof(fmt), "%%%s.%d%c",
		  (flags & F_ALT) ? "#" : "", prec, type);
    if (buflen <= (size_t)10 + (size_t)prec) 
	{
		PyErr_SetString(PyExc_OverflowError,
			"formatted float is too long (precision too long?)");
		return -1;
    }
    return usprintf(buf, fmt, x);
}

static PyObject *formatlong(PyObject *val, int flags, int prec, int type)
{
	char *buf;
	int i, len;
	PyObject *str;
	PyUnicodeObject *result;

	str = _PyString_FormatLong(val, flags, prec, type, &buf, &len);
	if (!str)
	{
		return NULL;
	}
	result = _PyUnicode_New(len);
	for (i = 0; i < len; i++)
	{
		result->str[i] = buf[i];
	}
	result->str[len] = 0;
	Py_DECREF(str);
	return (PyObject*)result;
}

static int formatint(Py_UNICODE *buf,
	  size_t buflen,
	  int flags,
	  int prec,
	  int type,
	  PyObject *v)
{
    char fmt[64];
    long x;
    int use_native_c_format = 1;

    x = PyInt_AsLong(v);
    if (x == -1 && PyErr_Occurred())
	{
		return -1;
    }
	if (prec < 0)
	{
		prec = 1;
	}
	if (buflen <= 13 || buflen <= (size_t)2+(size_t)prec) 
	{
        PyErr_SetString(PyExc_OverflowError,
            "formatted integer is too long (precision too long?)");
        return -1;
    }
    if (x == 0 && (flags & F_ALT) && (type == 'x' || type == 'X')) 
	{
        PyOS_snprintf(fmt, sizeof(fmt), type == 'x' ? "%#x" : "%#X", 0);
        if (fmt[1] != (char)type) 
		{
            use_native_c_format = 0;
            PyOS_snprintf(fmt, sizeof(fmt), "0%c%%#.%dl%c", type, prec, type);
        }
    }
    if (use_native_c_format)
    {
		PyOS_snprintf(fmt, sizeof(fmt), "%%%s.%dl%c",
		       (flags & F_ALT) ? "#" : "", prec, type);
    }
	return usprintf(buf, fmt, x);
}

static int
formatchar(Py_UNICODE *buf,
           size_t buflen,
           PyObject *v)
{
    if (PyUnicode_Check(v)) 
	{
		if (PyUnicode_GET_SIZE(v) != 1)
		{
			goto onError;
		}
		buf[0] = PyUnicode_AS_UNICODE(v)[0];
    }
	else if (PyString_Check(v)) 
	{
		if (PyString_GET_SIZE(v) != 1) 
		{
			goto onError;
		}
		buf[0] = (Py_UNICODE)PyString_AS_STRING(v)[0];
    }
    else 
	{
        long x;
		x = PyInt_AsLong(v);
		if (x == -1 && PyErr_Occurred())
		{
			goto onError;
		}
		if (x < 0 || x > 0xffff) 
		{
			PyErr_SetString(PyExc_ValueError,
					"%c arg not in range(0x10000) "
					"(narrow Python build)");
			return -1;
		}
		buf[0] = (Py_UNICODE) x;
    }
    buf[1] = '\0';
    return 1;

onError:
    PyErr_SetString(PyExc_TypeError,
		    "%c requires int or char");
    return -1;
}

#define FORMATBUFLEN (size_t)120

PyObject *PyUnicode_Format(PyObject *format,
			   PyObject *args)
{
    Py_UNICODE *fmt, *res;
    int fmtcnt, rescnt, reslen, arglen, argidx;
    int args_owned = 0;
    PyUnicodeObject *result = NULL;
    PyObject *dict = NULL;
    PyObject *uformat;
	
    if (format == NULL || args == NULL) 
	{
		PyErr_BadInternalCall();
		return NULL;
    }
    uformat = PyUnicode_FromObject(format);
    if (uformat == NULL)
	{
		return NULL;
    }
	fmt = PyUnicode_AS_UNICODE(uformat);
    fmtcnt = PyUnicode_GET_SIZE(uformat);

    reslen = rescnt = fmtcnt + 100;
    result = _PyUnicode_New(reslen);
    if (result == NULL)
	{
		goto onError;
    }
	res = PyUnicode_AS_UNICODE(result);

    if (PyTuple_Check(args)) 
	{
		arglen = PyTuple_Size(args);
		argidx = 0;
    }
    else 
	{
		arglen = -1;
		argidx = -2;
    }
    if (args->ob_type->tp_as_mapping)
	{
		dict = args;
	}

    while (--fmtcnt >= 0) 
	{
		if (*fmt != '%') 
		{
			if (--rescnt < 0) 
			{
				rescnt = fmtcnt + 100;
				reslen += rescnt;
				if (_PyUnicode_Resize(&result, reslen) < 0)
				{
					return NULL;
				}
				res = PyUnicode_AS_UNICODE(result) + reslen - rescnt;
				--rescnt;
			}
			*res++ = *fmt++;
		}
		else 
		{
			int flags = 0;
			int width = -1;
			int prec = -1;
			Py_UNICODE c = '\0';
			Py_UNICODE fill;
			PyObject *v = NULL;
			PyObject *temp = NULL;
			Py_UNICODE *pbuf;
			Py_UNICODE sign;
			int len;
			Py_UNICODE formatbuf[FORMATBUFLEN];

			fmt++;
			if (*fmt == '(') 
			{
				Py_UNICODE *keystart;
				int keylen;
				PyObject *key;
				int pcount = 1;

				if (dict == NULL) 
				{
					PyErr_SetString(PyExc_TypeError,
							"format requires a mapping"); 
					goto onError;
				}
				++fmt;
				--fmtcnt;
				keystart = fmt;
				while (pcount > 0 && --fmtcnt >= 0) 
				{
					if (*fmt == ')')
					{
						--pcount;
					}
					else if (*fmt == '(')
					{
						++pcount;
					}
					fmt++;
				}
				keylen = fmt - keystart - 1;
				if (fmtcnt < 0 || pcount > 0) 
				{
					PyErr_SetString(PyExc_ValueError,
							"incomplete format key");
					goto onError;
				}
				key = PyUnicode_FromUnicode(keystart, keylen);
				if (key == NULL)
				{
					goto onError;
				}
				if (args_owned) 
				{
					Py_DECREF(args);
					args_owned = 0;
				}
				args = PyObject_GetItem(dict, key);
				Py_DECREF(key);
				if (args == NULL) 
				{
					goto onError;
				}
				args_owned = 1;
				arglen = -1;
				argidx = -2;
			}
			while (--fmtcnt >= 0) 
			{
				switch (c = *fmt++) 
				{
				case '-': 
					flags |= F_LJUST; 
					continue;
				
				case '+': 
					flags |= F_SIGN; 
					continue;
				
				case ' ': 
					flags |= F_BLANK; 
					continue;
				
				case '#': 
					flags |= F_ALT; 
					continue;
				
				case '0': 
					flags |= F_ZERO; 
					continue;
				}
				break;
			}
			if (c == '*') 
			{
				v = getnextarg(args, arglen, &argidx);
				if (v == NULL)
				{
					goto onError;
				}
				if (!PyInt_Check(v)) 
				{
					PyErr_SetString(PyExc_TypeError,
							"* wants int");
					goto onError;
				}
				width = PyInt_AsLong(v);
				if (width < 0) 
				{
					flags |= F_LJUST;
					width = -width;
				}
				if (--fmtcnt >= 0)
				{
					c = *fmt++;
				}
			}
			else if (c >= '0' && c <= '9') 
			{
				width = c - '0';
				while (--fmtcnt >= 0) 
				{
					c = *fmt++;
					if (c < '0' || c > '9')
					{
						break;
					}
					if ((width*10) / 10 != width) 
					{
						PyErr_SetString(PyExc_ValueError,
								"width too big");
						goto onError;
					}
					width = width*10 + (c - '0');
				}
			}
			if (c == '.') 
			{
				prec = 0;
				if (--fmtcnt >= 0)
				{
					c = *fmt++;
				}
				if (c == '*') 
				{
					v = getnextarg(args, arglen, &argidx);
					if (v == NULL)
					{
						goto onError;
					}
					if (!PyInt_Check(v)) 
					{
						PyErr_SetString(PyExc_TypeError,
								"* wants int");
						goto onError;
					}
					prec = PyInt_AsLong(v);
					if (prec < 0)
					{
						prec = 0;
					}
					if (--fmtcnt >= 0)
					{
						c = *fmt++;
					}
				}
				else if (c >= '0' && c <= '9') 
				{
					prec = c - '0';
					while (--fmtcnt >= 0) 
					{
						c = Py_CHARMASK(*fmt++);
						if (c < '0' || c > '9')
						{
							break;
						}
						if ((prec*10) / 10 != prec) 
						{
							PyErr_SetString(PyExc_ValueError,
									"prec too big");
							goto onError;
						}
						prec = prec*10 + (c - '0');
					}
				}
			}
			if (fmtcnt >= 0) 
			{
				if (c == 'h' || c == 'l' || c == 'L') 
				{
					if (--fmtcnt >= 0)
					c = *fmt++;
				}
			}
			if (fmtcnt < 0) 
			{
				PyErr_SetString(PyExc_ValueError,
						"incomplete format");
				goto onError;
			}
			if (c != '%') 
			{
				v = getnextarg(args, arglen, &argidx);
				if (v == NULL)
				{
					goto onError;
				}
			}
			sign = 0;
			fill = ' ';
			switch (c) 
			{
			case '%':
				pbuf = formatbuf;
				pbuf[0] = '%';
				len = 1;
				break;

			case 's':
			case 'r':
				if (PyUnicode_Check(v) && c == 's') 
				{
					temp = v;
					Py_INCREF(temp);
				}
				else 
				{
					PyObject *unicode;
					if (c == 's')
					{
						temp = PyObject_Str(v);
					}
					else
					{
						temp = PyObject_Repr(v);
					}
					if (temp == NULL)
					{
						goto onError;
					}
					if (!PyString_Check(temp)) 
					{
						Py_DECREF(temp);
						PyErr_SetString(PyExc_TypeError,
								"%s argument has non-string str()");
						goto onError;
					}
					unicode = PyUnicode_Decode(PyString_AS_STRING(temp),
								   PyString_GET_SIZE(temp),
								   NULL,
								   "strict");
					Py_DECREF(temp);
					temp = unicode;
					if (temp == NULL)
					{
						goto onError;
					}
				}
				pbuf = PyUnicode_AS_UNICODE(temp);
				len = PyUnicode_GET_SIZE(temp);
				if (prec >= 0 && len > prec)
				{
					len = prec;
				}
				break;

			case 'i':
			case 'd':
			case 'u':
			case 'o':
			case 'x':
			case 'X':
				if (c == 'i')
				{
					c = 'd';
				}
				if (PyLong_Check(v)) 
				{
					temp = formatlong(v, flags, prec, c);
					if (!temp)
					{
						goto onError;
					}
					pbuf = PyUnicode_AS_UNICODE(temp);
					len = PyUnicode_GET_SIZE(temp);
					sign = 1;
				}
				else 
				{
					pbuf = formatbuf;
					len = formatint(pbuf, sizeof(formatbuf)/sizeof(Py_UNICODE),
							flags, prec, c, v);
					if (len < 0)
					{
						goto onError;
					}
					sign = c == 'd';
				}
				if (flags & F_ZERO)
				{
					fill = '0';
				}
				break;

			case 'e':
			case 'E':
			case 'f':
			case 'g':
			case 'G':
				pbuf = formatbuf;
				len = formatfloat(pbuf, sizeof(formatbuf)/sizeof(Py_UNICODE),
					flags, prec, c, v);
				if (len < 0)
				{
					goto onError;
				}
				sign = 1;
				if (flags & F_ZERO)
				{
					fill = '0';
				}
				break;

			case 'c':
				pbuf = formatbuf;
				len = formatchar(pbuf, sizeof(formatbuf)/sizeof(Py_UNICODE), v);
				if (len < 0)
				{
					goto onError;
				}
				break;

			default:
				PyErr_Format(PyExc_ValueError,
								 "unsupported format character '%c' (0x%x) "
								 "at index %i",
								 (31<=c && c<=126) ? (char)c : '?', 
								 (int)c,
								 (int)(fmt -1 - PyUnicode_AS_UNICODE(uformat)));
				goto onError;
			}
			if (sign) 
			{
				if (*pbuf == '-' || *pbuf == '+') 
				{
					sign = *pbuf++;
					len--;
				}
				else if (flags & F_SIGN)
				{
					sign = '+';
				}
				else if (flags & F_BLANK)
				{
					sign = ' ';
				}
				else
				{
					sign = 0;
				}
			}
			if (width < len)
			{
				width = len;
			}
			if (rescnt - (sign != 0) < width) 
			{
				reslen -= rescnt;
				rescnt = width + fmtcnt + 100;
				reslen += rescnt;
				if (reslen < 0) 
				{
					Py_DECREF(result);
					return PyErr_NoMemory();
				}
				if (_PyUnicode_Resize(&result, reslen) < 0)
				{
					return NULL;
				}
				res = PyUnicode_AS_UNICODE(result)
					+ reslen - rescnt;
			}
			if (sign) 
			{
				if (fill != ' ')
				{
					*res++ = sign;
				}
				rescnt--;
				if (width > len)
				{
					width--;
				}
			}
			if ((flags & F_ALT) && (c == 'x' || c == 'X')) 
			{
				assert(pbuf[0] == '0');
				assert(pbuf[1] == c);
				if (fill != ' ') 
				{
					*res++ = *pbuf++;
					*res++ = *pbuf++;
				}
				rescnt -= 2;
				width -= 2;
				if (width < 0)
				{
					width = 0;
				}
				len -= 2;
			}
			if (width > len && !(flags & F_LJUST)) 
			{
				do 
				{
					--rescnt;
					*res++ = fill;
				} while (--width > len);
			}
			if (fill == ' ') 
			{
				if (sign)
				{
					*res++ = sign;
				}
				if ((flags & F_ALT) && (c == 'x' || c == 'X')) 
				{
					assert(pbuf[0] == '0');
					assert(pbuf[1] == c);
					*res++ = *pbuf++;
					*res++ = *pbuf++;
				}
			}
			Py_UNICODE_COPY(res, pbuf, len);
			res += len;
			rescnt -= len;
			while (--width >= len) 
			{
				--rescnt;
				*res++ = ' ';
			}
			if (dict && (argidx < arglen) && c != '%') 
			{
				PyErr_SetString(PyExc_TypeError,
						"not all arguments converted");
				goto onError;
			}
			Py_XDECREF(temp);
		}
    }
    if (argidx < arglen && !dict) 
	{
		PyErr_SetString(PyExc_TypeError,
				"not all arguments converted");
		goto onError;
    }

    if (args_owned) 
	{
		Py_DECREF(args);
    }
    Py_DECREF(uformat);
    if (_PyUnicode_Resize(&result, reslen - rescnt))
	{
		goto onError;
    }
	return (PyObject *)result;

onError:
    Py_XDECREF(result);
    Py_DECREF(uformat);
    if (args_owned) 
	{
		Py_DECREF(args);
    }
    return NULL;
}

static PyBufferProcs unicode_as_buffer = {
    (getreadbufferproc) unicode_buffer_getreadbuf,
    (getwritebufferproc) unicode_buffer_getwritebuf,
    (getsegcountproc) unicode_buffer_getsegcount,
    (getcharbufferproc) unicode_buffer_getcharbuf,
};

staticforward PyObject *unicode_subtype_new(PyTypeObject *type, PyObject *args, PyObject *kwds);

static PyObject *unicode_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    PyObject *x = NULL;
	static char *kwlist[] = {"string", "encoding", "errors", 0};
	char *encoding = NULL;
	char *errors = NULL;

	if (type != &PyUnicode_Type)
	{
		return unicode_subtype_new(type, args, kwds);
	}
	if (!PyArg_ParseTupleAndKeywords(args, kwds, "|Oss:unicode",
					  kwlist, &x, &encoding, &errors))
	{
		return NULL;
	}
	if (x == NULL)
	{
		return (PyObject *)_PyUnicode_New(0);
	}
	if (encoding == NULL && errors == NULL)
	{
		return PyObject_Unicode(x);
	}
	else
	{
		return PyUnicode_FromEncodedObject(x, encoding, errors);
	}
}

static PyObject *unicode_subtype_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	PyUnicodeObject *tmp, *pnew;
	int n;

	assert(PyType_IsSubtype(type, &PyUnicode_Type));
	tmp = (PyUnicodeObject *)unicode_new(&PyUnicode_Type, args, kwds);
	if (tmp == NULL)
	{
		return NULL;
	}
	assert(PyUnicode_Check(tmp));
	pnew = (PyUnicodeObject *) type->tp_alloc(type, n = tmp->length);
	if (pnew == NULL)
	{
		return NULL;
	}
	pnew->str = PyMem_NEW(Py_UNICODE, n+1);
	if (pnew->str == NULL) 
	{
		_Py_ForgetReference((PyObject *)pnew);
		PyObject_DEL(pnew);
		return NULL;
	}
	Py_UNICODE_COPY(pnew->str, tmp->str, n+1);
	pnew->length = n;
	pnew->hash = tmp->hash;
	Py_DECREF(tmp);
	return (PyObject *)pnew;
}

static char unicode_doc[] =
	"unicode(string [, encoding[, errors]]) -> object\n"
	"\n"
	"Create a new Unicode object from the given encoded string.\n"
	"encoding defaults to the current default string encoding and \n"
	"errors, defining the error handling, to 'strict'.";

PyTypeObject PyUnicode_Type = {
    PyObject_HEAD_INIT(&PyType_Type)
    0, 					
    "unicode", 			
    sizeof(PyUnicodeObject), 		
    0, 
    (destructor)unicode_dealloc, 	
    0, 					
    0,				 	
    0, 					
    (cmpfunc) unicode_compare, 		
    (reprfunc) unicode_repr, 		
    0, 					
    &unicode_as_sequence, 		
    0, 					
    (hashfunc) unicode_hash,
    0, 					
    (reprfunc) unicode_str,	 	
    PyObject_GenericGetAttr, 	
    0,			 		
    &unicode_as_buffer,			
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    unicode_doc,		
    0,					
    0,					
    0,					
    0,					
    0,					
    0,					
    unicode_methods,	
    0,					
    0,					
    0,					
    0,					
    0,					
    0,					
    0,					
    0,					
    0,					
    unicode_new,		
    _PyObject_Del,		
};

void _PyUnicode_Init()
{
    int i;

    unicode_freelist = NULL;
    unicode_freelist_size = 0;
    unicode_empty = _PyUnicode_New(0);
    strcpy(unicode_default_encoding, "ascii");
    for (i = 0; i < 256; i++)
	{
		unicode_latin1[i] = NULL;
	}
}

void _PyUnicode_Fini()
{
    PyUnicodeObject *u;
    int i;

    Py_XDECREF(unicode_empty);
    unicode_empty = NULL;

    for (i = 0; i < 256; i++) 
	{
		if (unicode_latin1[i]) 
		{
			Py_DECREF(unicode_latin1[i]);
			unicode_latin1[i] = NULL;
		}
    }

    for (u = unicode_freelist; u != NULL;) 
	{
		PyUnicodeObject *v = u;
		u = *(PyUnicodeObject **)u;
		if (v->str)
		{
			PyMem_DEL(v->str);
		}
		Py_XDECREF(v->defenc);
		PyObject_DEL(v);
    }
    unicode_freelist = NULL;
    unicode_freelist_size = 0;
}
