//20170419
#include "python.h"
#include "longintrepr.h"

#include <ctype.h>

#define ABS(x) ((x) < 0 ? -(x) : (x))

static PyLongObject *long_normalize(PyLongObject *);
static PyLongObject *mul1(PyLongObject *, wdigit);
static PyLongObject *muladd1(PyLongObject *, wdigit, wdigit);
static PyLongObject *divrem1(PyLongObject *, digit, digit *);
static PyObject *long_format(PyObject *aa, int base, int addL);

static int ticker;	

#define SIGCHECK(PyTryBlock) \
	if (--ticker < 0) { \
		ticker = 100; \
		if (PyErr_CheckSignals()) { PyTryBlock; } \
	}

static PyLongObject *long_normalize(PyLongObject *v)
{
	int j = ABS(v->ob_size);
	int i = j;
	
	while (i > 0 && v->ob_digit[i-1] == 0)
	{
		--i;
	}
	if (i != j)
	{
		v->ob_size = (v->ob_size < 0) ? -(i) : i;
	}
	return v;
}

PyLongObject *_PyLong_New(int size)
{
	return PyObject_NEW_VAR(PyLongObject, &PyLong_Type, size);
}

PyObject *_PyLong_Copy(PyLongObject *src)
{
	PyLongObject *result;
	int i;

	assert(src != NULL);
	i = src->ob_size;
	if (i < 0)
	{
		i = -(i);
	}
	result = _PyLong_New(i);
	if (result != NULL) 
	{
		result->ob_size = src->ob_size;
		while (--i >= 0)
		{
			result->ob_digit[i] = src->ob_digit[i];
		}
	}
	return (PyObject *)result;
}

PyObject *PyLong_FromLong(long ival)
{
	PyLongObject *v;
	unsigned long t;
	int ndigits = 0;
	int negative = 0;

	if (ival < 0) 
	{
		ival = -ival;
		negative = 1;
	}

	t = (unsigned long)ival;
	while (t) 
	{
		++ndigits;
		t >>= SHIFT;
	}
	v = _PyLong_New(ndigits);
	if (v != NULL) 
	{
		digit *p = v->ob_digit;
		v->ob_size = negative ? -ndigits : ndigits;
		t = (unsigned long)ival;
		while (t) 
		{
			*p++ = (digit)(t & MASK);
			t >>= SHIFT;
		}
	}
	return (PyObject *)v;
}

PyObject *PyLong_FromUnsignedLong(unsigned long ival)
{
	PyLongObject *v;
	unsigned long t;
	int ndigits = 0;

	t = (unsigned long)ival;
	while (t) 
	{
		++ndigits;
		t >>= SHIFT;
	}
	v = _PyLong_New(ndigits);
	if (v != NULL) 
	{
		digit *p = v->ob_digit;
		v->ob_size = ndigits;
		while (ival) 
		{
			*p++ = (digit)(ival & MASK);
			ival >>= SHIFT;
		}
	}
	return (PyObject *)v;
}

PyObject *PyLong_FromDouble(double dval)
{
	PyLongObject *v;
	double frac;
	int i, ndig, expo, neg;
	neg = 0;
	if (Py_IS_INFINITY(dval)) 
	{
		PyErr_SetString(PyExc_OverflowError,
			"cannot convert float infinity to long");
		return NULL;
	}
	if (dval < 0.0) 
	{
		neg = 1;
		dval = -dval;
	}
	frac = frexp(dval, &expo);
	if (expo <= 0)
	{
		return PyLong_FromLong(0L);
	}
	ndig = (expo-1) / SHIFT + 1;
	v = _PyLong_New(ndig);
	if (v == NULL)
	{
		return NULL;
	}
	frac = ldexp(frac, (expo-1) % SHIFT + 1);
	for (i = ndig; --i >= 0; ) 
	{
		long bits = (long)frac;
		v->ob_digit[i] = (digit) bits;
		frac = frac - (double)bits;
		frac = ldexp(frac, SHIFT);
	}
	if (neg)
	{
		v->ob_size = -(v->ob_size);
	}
	return (PyObject *)v;
}

long PyLong_AsLong(PyObject *vv)
{
	PyLongObject *v;
	unsigned long x, prev;
	int i, sign;

	if (vv == NULL || !PyLong_Check(vv)) 
	{
		if (vv != NULL && PyInt_Check(vv))
		{
			return PyInt_AsLong(vv);
		}
		PyErr_BadInternalCall();
		return -1;
	}
	v = (PyLongObject *)vv;
	i = v->ob_size;
	sign = 1;
	x = 0;
	if (i < 0) 
	{
		sign = -1;
		i = -(i);
	}
	while (--i >= 0) 
	{
		prev = x;
		x = (x << SHIFT) + v->ob_digit[i];
		if ((x >> SHIFT) != prev)
		{
			goto overflow;
		}
	}
	if ((long)x < 0 && (sign > 0 || (x << 1) != 0))
	{
		goto overflow;
	}
	return (long)x * sign;

overflow:
	PyErr_SetString(PyExc_OverflowError,
			"long int too large to convert to int");
	return -1;
}

unsigned long PyLong_AsUnsignedLong(PyObject *vv)
{
	PyLongObject *v;
	unsigned long x, prev;
	int i;
	
	if (vv == NULL || !PyLong_Check(vv)) 
	{
		PyErr_BadInternalCall();
		return (unsigned long) -1;
	}
	v = (PyLongObject *)vv;
	i = v->ob_size;
	x = 0;
	if (i < 0) 
	{
		PyErr_SetString(PyExc_OverflowError,
			   "can't convert negative value to unsigned long");
		return (unsigned long) -1;
	}
	while (--i >= 0) 
	{
		prev = x;
		x = (x << SHIFT) + v->ob_digit[i];
		if ((x >> SHIFT) != prev) 
		{
			PyErr_SetString(PyExc_OverflowError,
				"long int too large to convert");
			return (unsigned long) -1;
		}
	}
	return x;
}

PyObject *_PyLong_FromByteArray(const unsigned char* bytes, size_t n,
		      int little_endian, int is_signed)
{
	const unsigned char* pstartbyte;
	int incr;		
	const unsigned char* pendbyte;	
	size_t numsignificantbytes;	
	size_t ndigits;			
	PyLongObject* v;		
	int idigit = 0;  		

	if (n == 0)
	{
		return PyLong_FromLong(0L);
	}

	if (little_endian) 
	{
		pstartbyte = bytes;
		pendbyte = bytes + n - 1;
		incr = 1;
	}
	else 
	{
		pstartbyte = bytes + n - 1;
		pendbyte = bytes;
		incr = -1;
	}

	if (is_signed)
	{
		is_signed = *pendbyte >= 0x80;
	}

	{
		size_t i;
		const unsigned char* p = pendbyte;
		const int pincr = -incr;  
		const unsigned char insignficant = is_signed ? 0xff : 0x00;

		for (i = 0; i < n; ++i, p += pincr) 
		{
			if (*p != insignficant)
			{
				break;
			}
		}
		numsignificantbytes = n - i;
		if (is_signed && numsignificantbytes < n)
		{
			++numsignificantbytes;
		}
	}

	ndigits = (numsignificantbytes * 8 + SHIFT - 1) / SHIFT;
	if (ndigits > (size_t)INT_MAX)
	{
		return PyErr_NoMemory();
	}
	v = _PyLong_New((int)ndigits);
	if (v == NULL)
	{
		return NULL;
	}

	{
		size_t i;
		twodigits carry = 1;		
		twodigits accum = 0;		
		unsigned int accumbits = 0; 
		const unsigned char* p = pstartbyte;

		for (i = 0; i < numsignificantbytes; ++i, p += incr) 
		{
			twodigits thisbyte = *p;
			if (is_signed) 
			{
				thisbyte = (0xff ^ thisbyte) + carry;
				carry = thisbyte >> 8;
				thisbyte &= 0xff;
			}
			accum |= thisbyte << accumbits;
			accumbits += 8;
			if (accumbits >= SHIFT) 
			{
				assert(idigit < (int)ndigits);
				v->ob_digit[idigit] = (digit)(accum & MASK);
				++idigit;
				accum >>= SHIFT;
				accumbits -= SHIFT;
				assert(accumbits < SHIFT);
			}
		}
		assert(accumbits < SHIFT);
		if (accumbits) 
		{
			assert(idigit < (int)ndigits);
			v->ob_digit[idigit] = (digit)accum;
			++idigit;
		}
	}

	v->ob_size = is_signed ? -idigit : idigit;
	return (PyObject *)long_normalize(v);
}

int _PyLong_AsByteArray(PyLongObject* v,
		    unsigned char* bytes, size_t n,
		    int little_endian, int is_signed)
{
	int i;			
	int ndigits;	
	twodigits accum;	
	unsigned int accumbits; 
	int do_twos_comp;
	twodigits carry;
	size_t j;
	unsigned char* p;
	int pincr;

	assert(v != NULL && PyLong_Check(v));

	if (v->ob_size < 0) 
	{
		ndigits = -(v->ob_size);
		if (!is_signed) 
		{
			PyErr_SetString(PyExc_TypeError,
				"can't convert negative long to unsigned");
			return -1;
		}
		do_twos_comp = 1;
	}
	else 
	{
		ndigits = v->ob_size;
		do_twos_comp = 0;
	}

	if (little_endian) 
	{
		p = bytes;
		pincr = 1;
	}
	else 
	{
		p = bytes + n - 1;
		pincr = -1;
	}

	assert(ndigits == 0 || v->ob_digit[ndigits - 1] != 0);
	j = 0;
	accum = 0;
	accumbits = 0;
	carry = do_twos_comp ? 1 : 0;
	for (i = 0; i < ndigits; ++i) 
	{
		twodigits thisdigit = v->ob_digit[i];
		if (do_twos_comp) 
		{
			thisdigit = (thisdigit ^ MASK) + carry;
			carry = thisdigit >> SHIFT;
			thisdigit &= MASK;
		}
		accum |= thisdigit << accumbits;
		accumbits += SHIFT;

		if (i == ndigits - 1) 
		{
			stwodigits s = (stwodigits)(thisdigit <<
				(8 * sizeof(stwodigits) - SHIFT));
			unsigned int nsignbits = 0;
			while ((s < 0) == do_twos_comp && nsignbits < SHIFT) 
			{
				++nsignbits;
				s <<= 1;
			}
			accumbits -= nsignbits;
		}

		while (accumbits >= 8) 
		{
			if (j >= n)
			{
				goto Overflow;
			}
			++j;
			*p = (unsigned char)(accum & 0xff);
			p += pincr;
			accumbits -= 8;
			accum >>= 8;
		}
	}

	assert(accumbits < 8);
	assert(carry == 0);  
	if (accumbits > 0) 
	{
		if (j >= n)
		{
			goto Overflow;
		}
		++j;
		if (do_twos_comp) 
		{
			accum |= (~(twodigits)0) << accumbits;
		}
		*p = (unsigned char)(accum & 0xff);
		p += pincr;
	}
	else if (j == n && n > 0 && is_signed) 
	{
		unsigned char msb = *(p - pincr);
		int sign_bit_set = msb >= 0x80;
		assert(accumbits == 0);
		if (sign_bit_set == do_twos_comp)
		{
			return 0;
		}
		else
		{
			goto Overflow;
		}
	}

	{
		unsigned char signbyte = do_twos_comp ? 0xffU : 0U;
		for ( ; j < n; ++j, p += pincr)
		{
			*p = signbyte;
		}
	}

	return 0;

Overflow:
	PyErr_SetString(PyExc_OverflowError, "long too big to convert");
	return -1;
	
}

double _PyLong_AsScaledDouble(PyObject *vv, int *exponent)
{
#define NBITS_WANTED 57
	PyLongObject *v;
	double x;
	const double multiplier = (double)(1L << SHIFT);
	int i, sign;
	int nbitsneeded;

	if (vv == NULL || !PyLong_Check(vv)) 
	{
		PyErr_BadInternalCall();
		return -1;
	}
	v = (PyLongObject *)vv;
	i = v->ob_size;
	sign = 1;
	if (i < 0) 
	{
		sign = -1;
		i = -(i);
	}
	else if (i == 0) 
	{
		*exponent = 0;
		return 0.0;
	}
	--i;
	x = (double)v->ob_digit[i];
	nbitsneeded = NBITS_WANTED - 1;
	while (i > 0 && nbitsneeded > 0) 
	{
		--i;
		x = x * multiplier + (double)v->ob_digit[i];
		nbitsneeded -= SHIFT;
	}
	*exponent = i;
	assert(x > 0.0);
	return x * sign;
#undef NBITS_WANTED
}

double PyLong_AsDouble(PyObject *vv)
{
	int e;
	double x;

	if (vv == NULL || !PyLong_Check(vv)) 
	{
		PyErr_BadInternalCall();
		return -1;
	}
	x = _PyLong_AsScaledDouble(vv, &e);
	if (x == -1.0 && PyErr_Occurred())
	{
		return -1.0;
	}
	if (e > INT_MAX / SHIFT)
	{
		goto overflow;
	}
	errno = 0;
	x = ldexp(x, e * SHIFT);
	if (Py_OVERFLOWED(x))
	{
		goto overflow;
	}
	return x;

overflow:
	PyErr_SetString(PyExc_OverflowError,
		"long int too large to convert to float");
	return -1.0;
}

PyObject *PyLong_FromVoidPtr(void *p)
{
	return PyInt_FromLong((long)p);
}

void *PyLong_AsVoidPtr(PyObject *vv)
{
	long x;

	if (PyInt_Check(vv))
	{
		x = PyInt_AS_LONG(vv);
	}
	else
	{
		x = PyLong_AsLong(vv);
	}

	if (x == -1 && PyErr_Occurred())
	{
		return NULL;
	}
	return (void *)x;
}

#define IS_LITTLE_ENDIAN (int)*(unsigned char*)&one

PyObject *PyLong_FromLongLong(LONG_LONG ival)
{
	LONG_LONG bytes = ival;
	int one = 1;
	return _PyLong_FromByteArray(
			(unsigned char *)&bytes,
			SIZEOF_LONG_LONG, IS_LITTLE_ENDIAN, 1);
}

PyObject *PyLong_FromUnsignedLongLong(unsigned LONG_LONG ival)
{
	unsigned LONG_LONG bytes = ival;
	int one = 1;
	return _PyLong_FromByteArray(
			(unsigned char *)&bytes,
			SIZEOF_LONG_LONG, IS_LITTLE_ENDIAN, 0);
}

LONG_LONG PyLong_AsLongLong(PyObject *vv)
{
	LONG_LONG bytes;
	int one = 1;
	int res;

	if (vv == NULL) 
	{
		PyErr_BadInternalCall();
		return -1;
	}
	if (!PyLong_Check(vv)) 
	{
		if (PyInt_Check(vv))
		{
			return (LONG_LONG)PyInt_AsLong(vv);
		}
		PyErr_BadInternalCall();
		return -1;
	}

	res = _PyLong_AsByteArray(
			(PyLongObject *)vv, (unsigned char *)&bytes,
			SIZEOF_LONG_LONG, IS_LITTLE_ENDIAN, 1);

	return res < 0 ? (LONG_LONG)res : bytes;
}

unsigned LONG_LONG PyLong_AsUnsignedLongLong(PyObject *vv)
{
	unsigned LONG_LONG bytes;
	int one = 1;
	int res;

	if (vv == NULL || !PyLong_Check(vv)) 
	{
		PyErr_BadInternalCall();
		return -1;
	}

	res = _PyLong_AsByteArray(
			(PyLongObject *)vv, (unsigned char *)&bytes,
			SIZEOF_LONG_LONG, IS_LITTLE_ENDIAN, 0);

	return res < 0 ? (unsigned LONG_LONG)res : bytes;
}

#undef IS_LITTLE_ENDIAN


static int convert_binop(PyObject *v, PyObject *w, PyLongObject **a, PyLongObject **b) 
{
	if (PyLong_Check(v)) 
	{ 
		*a = (PyLongObject *) v;
		Py_INCREF(v);
	}
	else if (PyInt_Check(v)) 
	{
		*a = (PyLongObject *) PyLong_FromLong(PyInt_AS_LONG(v));
	}
	else 
	{
		return 0;
	}
	if (PyLong_Check(w)) 
	{ 
		*b = (PyLongObject *) w;
		Py_INCREF(w);
	}
	else if (PyInt_Check(w)) 
	{
		*b = (PyLongObject *) PyLong_FromLong(PyInt_AS_LONG(w));
	}
	else 
	{
		Py_DECREF(*a);
		return 0;
	}
	return 1;
}

#define CONVERT_BINOP(v, w, a, b) \
	if (!convert_binop(v, w, a, b)) { \
		Py_INCREF(Py_NotImplemented); \
		return Py_NotImplemented; \
	}


static PyLongObject *mul1(PyLongObject *a, wdigit n)
{
	return muladd1(a, n, (digit)0);
}

static PyLongObject *muladd1(PyLongObject *a, wdigit n, wdigit extra)
{
	int size_a = ABS(a->ob_size);
	PyLongObject *z = _PyLong_New(size_a+1);
	twodigits carry = extra;
	int i;
	
	if (z == NULL)
	{
		return NULL;
	}
	for (i = 0; i < size_a; ++i) 
	{
		carry += (twodigits)a->ob_digit[i] * n;
		z->ob_digit[i] = (digit) (carry & MASK);
		carry >>= SHIFT;
	}
	z->ob_digit[i] = (digit) carry;
	return long_normalize(z);
}

static digit inplace_divrem1(digit *pout, digit *pin, int size, digit n)
{
	twodigits rem = 0;

	assert(n > 0 && n <= MASK);
	pin += size;
	pout += size;
	while (--size >= 0) 
	{
		digit hi;
		rem = (rem << SHIFT) + *--pin;
		*--pout = hi = (digit)(rem / n);
		rem -= hi * n;
	}
	return (digit)rem;
}

static PyLongObject *divrem1(PyLongObject *a, digit n, digit *prem)
{
	const int size = ABS(a->ob_size);
	PyLongObject *z;
	
	assert(n > 0 && n <= MASK);
	z = _PyLong_New(size);
	if (z == NULL)
	{
		return NULL;
	}
	*prem = inplace_divrem1(z->ob_digit, a->ob_digit, size, n);
	return long_normalize(z);
}

static PyObject *long_format(PyObject *aa, int base, int addL)
{
	PyLongObject *a = (PyLongObject *)aa;
	PyStringObject *str;
	int i;
	const int size_a = ABS(a->ob_size);
	char *p;
	int bits;
	char sign = '\0';

	if (a == NULL || !PyLong_Check(a)) 
	{
		PyErr_BadInternalCall();
		return NULL;
	}
	assert(base >= 2 && base <= 36);
	
	i = base;
	bits = 0;
	while (i > 1) 
	{
		++bits;
		i >>= 1;
	}
	i = 5 + (addL ? 1 : 0) + (size_a*SHIFT + bits-1) / bits;
	str = (PyStringObject *) PyString_FromStringAndSize((char *)0, i);
	if (str == NULL)
	{
		return NULL;
	}
	p = PyString_AS_STRING(str) + i;
	*p = '\0';
    if (addL)
	{
		*--p = 'L';
	}
	if (a->ob_size < 0)
	{
		sign = '-';
	}

	if (a->ob_size == 0) 
	{
		*--p = '0';
	}
	else if ((base & (base - 1)) == 0) 
	{
		twodigits accum = 0;
		int accumbits = 0;	
		int basebits = 1;	
		i = base;
		while ((i >>= 1) > 1)
		{
			++basebits;
		}

		for (i = 0; i < size_a; ++i) 
		{
			accum |= a->ob_digit[i] << accumbits;
			accumbits += SHIFT;
			assert(accumbits >= basebits);
			do 
			{
				char cdigit = (char)(accum & (base - 1));
				cdigit += (cdigit < 10) ? '0' : 'A'-10;
				assert(p > PyString_AS_STRING(str));
				*--p = cdigit;
				accumbits -= basebits;
				accum >>= basebits;
			} while (i < size_a-1 ? accumbits >= basebits :
					 	accum > 0);
		}
	}
	else 
	{
		int size = size_a;
		digit *pin = a->ob_digit;
		PyLongObject *scratch;
		digit powbase = base;
		int power = 1;
		for (;;) 
		{
			unsigned long newpow = powbase * (unsigned long)base;
			if (newpow >> SHIFT)
			{
				break;
			}
			powbase = (digit)newpow;
			++power;
		}

		scratch = _PyLong_New(size);
		if (scratch == NULL) 
		{
			Py_DECREF(str);
			return NULL;
		}

		do 
		{
			int ntostore = power;
			digit rem = inplace_divrem1(scratch->ob_digit,
						     pin, size, powbase);
			pin = scratch->ob_digit;
			if (pin[size - 1] == 0)
			{
				--size;
			}
			SIGCHECK({
				Py_DECREF(scratch);
				Py_DECREF(str);
				return NULL;
			})

			assert(ntostore > 0);
			do 
			{
				digit nextrem = (digit)(rem / base);
				char c = (char)(rem - nextrem * base);
				assert(p > PyString_AS_STRING(str));
				c += (c < 10) ? '0' : 'A'-10;
				*--p = c;
				rem = nextrem;
				--ntostore;
			} while (ntostore && (size || rem));
		} while (size != 0);
		Py_DECREF(scratch);
	}

	if (base == 8) 
	{
		if (size_a != 0)
		{
			*--p = '0';
		}
	}
	else if (base == 16) 
	{
		*--p = 'x';
		*--p = '0';
	}
	else if (base != 10) 
	{
		*--p = '#';
		*--p = '0' + base % 10;
		if (base > 10)
		{
			*--p = '0' + base / 10;
		}
	}
	if (sign)
	{
		*--p = sign;
	}
	if (p != PyString_AS_STRING(str)) 
	{
		char *q = PyString_AS_STRING(str);
		assert(p > q);
		do 
		{
			;
		} while ((*q++ = *p++) != '\0');
		q--;
		_PyString_Resize((PyObject **)&str,
				 (int) (q - PyString_AS_STRING(str)));
	}
	return (PyObject *)str;
}

PyObject *PyLong_FromString(char *str, char **pend, int base)
{
	int sign = 1;
	char *start, *orig_str = str;
	PyLongObject *z;
	
	if ((base != 0 && base < 2) || base > 36) 
	{
		PyErr_SetString(PyExc_ValueError,
				"long() arg 2 must be >= 2 and <= 36");
		return NULL;
	}
	while (*str != '\0' && isspace(Py_CHARMASK(*str)))
	{
		str++;
	}
	if (*str == '+')
	{
		++str;
	}
	else if (*str == '-') 
	{
		++str;
		sign = -1;
	}
	while (*str != '\0' && isspace(Py_CHARMASK(*str)))
	{
		str++;
	}
	if (base == 0) 
	{
		if (str[0] != '0')
		{
			base = 10;
		}
		else if (str[1] == 'x' || str[1] == 'X')
		{
			base = 16;
		}
		else
		{
			base = 8;
		}
	}
	if (base == 16 && str[0] == '0' && (str[1] == 'x' || str[1] == 'X'))
	{
		str += 2;
	}
	z = _PyLong_New(0);
	start = str;
	for ( ; z != NULL; ++str) 
	{
		int k = -1;
		PyLongObject *temp;
		
		if (*str <= '9')
		{
			k = *str - '0';
		}
		else if (*str >= 'a')
		{
			k = *str - 'a' + 10;
		}
		else if (*str >= 'A')
		{
			k = *str - 'A' + 10;
		}
		if (k < 0 || k >= base)
		{
			break;
		}
		temp = muladd1(z, (digit)base, (digit)k);
		Py_DECREF(z);
		z = temp;
	}
	if (z == NULL)
	{
		return NULL;
	}
	if (str == start)
	{
		goto onError;
	}
	if (sign < 0 && z != NULL && z->ob_size != 0)
	{
		z->ob_size = -(z->ob_size);
	}
	if (*str == 'L' || *str == 'l')
	{
		str++;
	}
	while (*str && isspace(Py_CHARMASK(*str)))
	{
		str++;
	}
	if (*str != '\0')
	{
		goto onError;
	}
	if (pend)
	{
		*pend = str;
	}
	return (PyObject *) z;

onError:
	PyErr_Format(PyExc_ValueError, 
		     "invalid literal for long(): %.200s", orig_str);
	Py_XDECREF(z);
	return NULL;
}

PyObject *PyLong_FromUnicode(Py_UNICODE *u, int length, int base)
{
	char buffer[256];

	if (length >= sizeof(buffer)) 
	{
		PyErr_SetString(PyExc_ValueError,
				"long() literal too large to convert");
		return NULL;
	}
	if (PyUnicode_EncodeDecimal(u, length, buffer, NULL))
	{
		return NULL;
	}

	return PyLong_FromString(buffer, NULL, base);
}

static PyLongObject *x_divrem(PyLongObject *, PyLongObject *, PyLongObject **);
static PyObject *long_pos(PyLongObject *);
static int long_divrem(PyLongObject *, PyLongObject *, PyLongObject **, PyLongObject **);

static int long_divrem(PyLongObject *a, PyLongObject *b,
	    PyLongObject **pdiv, PyLongObject **prem)
{
	int size_a = ABS(a->ob_size), size_b = ABS(b->ob_size);
	PyLongObject *z;
	
	if (size_b == 0) 
	{
		PyErr_SetString(PyExc_ZeroDivisionError,
				"long division or modulo by zero");
		return -1;
	}
	if (size_a < size_b ||
	    (size_a == size_b &&
	     a->ob_digit[size_a-1] < b->ob_digit[size_b-1])) 
	{
		*pdiv = _PyLong_New(0);
		Py_INCREF(a);
		*prem = (PyLongObject *) a;
		return 0;
	}
	if (size_b == 1) 
	{
		digit rem = 0;
		z = divrem1(a, b->ob_digit[0], &rem);
		if (z == NULL)
		{
			return -1;
		}
		*prem = (PyLongObject *) PyLong_FromLong((long)rem);
	}
	else 
	{
		z = x_divrem(a, b, prem);
		if (z == NULL)
		{
			return -1;
		}
	}
	if ((a->ob_size < 0) != (b->ob_size < 0))
	{
		z->ob_size = -(z->ob_size);
	}
	if (a->ob_size < 0 && (*prem)->ob_size != 0)
	{
		(*prem)->ob_size = -((*prem)->ob_size);
	}
	*pdiv = z;
	return 0;
}

static PyLongObject *x_divrem(PyLongObject *v1, PyLongObject *w1, PyLongObject **prem)
{
	int size_v = ABS(v1->ob_size), size_w = ABS(w1->ob_size);
	digit d = (digit) ((twodigits)BASE / (w1->ob_digit[size_w-1] + 1));
	PyLongObject *v = mul1(v1, d);
	PyLongObject *w = mul1(w1, d);
	PyLongObject *a;
	int j, k;
	
	if (v == NULL || w == NULL) 
	{
		Py_XDECREF(v);
		Py_XDECREF(w);
		return NULL;
	}
	
	assert(size_v >= size_w && size_w > 1); 
	assert(v->ob_refcnt == 1); 
	assert(size_w == ABS(w->ob_size)); 
	
	size_v = ABS(v->ob_size);
	a = _PyLong_New(size_v - size_w + 1);
	
	for (j = size_v, k = a->ob_size-1; a != NULL && k >= 0; --j, --k) 
	{
		digit vj = (j >= size_v) ? 0 : v->ob_digit[j];
		twodigits q;
		stwodigits carry = 0;
		int i;
		
		SIGCHECK({
			Py_DECREF(a);
			a = NULL;
			break;
		})
		if (vj == w->ob_digit[size_w-1])
		{
			q = MASK;
		}
		else
		{
			q = (((twodigits)vj << SHIFT) + v->ob_digit[j-1]) /
				w->ob_digit[size_w-1];
		}

		while (w->ob_digit[size_w-2]*q >
				((
					((twodigits)vj << SHIFT)
					+ v->ob_digit[j-1]
					- q*w->ob_digit[size_w-1]
								) << SHIFT)
				+ v->ob_digit[j-2])
		{
			--q;
		}

		for (i = 0; i < size_w && i+k < size_v; ++i) 
		{
			twodigits z = w->ob_digit[i] * q;
			digit zz = (digit) (z >> SHIFT);
			carry += v->ob_digit[i+k] - z
				+ ((twodigits)zz << SHIFT);
			v->ob_digit[i+k] = carry & MASK;
			carry = Py_ARITHMETIC_RIGHT_SHIFT(BASE_TWODIGITS_TYPE,
							  carry, SHIFT);
			carry -= zz;
		}
		
		if (i + k < size_v) 
		{
			carry += v->ob_digit[i+k];
			v->ob_digit[i+k] = 0;
		}
		
		if (carry == 0)
		{
			a->ob_digit[k] = (digit) q;
		}
		else 
		{
			assert(carry == -1);
			a->ob_digit[k] = (digit) q-1;
			carry = 0;
			for (i = 0; i < size_w && i+k < size_v; ++i)
			{
				carry += v->ob_digit[i+k] + w->ob_digit[i];
				v->ob_digit[i+k] = carry & MASK;
				carry = Py_ARITHMETIC_RIGHT_SHIFT(
						BASE_TWODIGITS_TYPE,
						carry, SHIFT);
			}
		}
	}
	
	if (a == NULL)
	{
		*prem = NULL;
	}
	else 
	{
		a = long_normalize(a);
		*prem = divrem1(v, d, &d);
		if (*prem == NULL) 
		{
			Py_DECREF(a);
			a = NULL;
		}
	}
	Py_DECREF(v);
	Py_DECREF(w);
	return a;
}

static void long_dealloc(PyObject *v)
{
	v->ob_type->tp_free(v);
}

static PyObject *long_repr(PyObject *v)
{
	return long_format(v, 10, 1);
}

static PyObject *long_str(PyObject *v)
{
	return long_format(v, 10, 0);
}

static int long_compare(PyLongObject *a, PyLongObject *b)
{
	int sign;
	
	if (a->ob_size != b->ob_size) 
	{
		if (ABS(a->ob_size) == 0 && ABS(b->ob_size) == 0)
		{
			sign = 0;
		}
		else
		{
			sign = a->ob_size - b->ob_size;
		}
	}
	else 
	{
		int i = ABS(a->ob_size);
		while (--i >= 0 && a->ob_digit[i] == b->ob_digit[i])
		{
			;
		}
		if (i < 0)
		{
			sign = 0;
		}
		else 
		{
			sign = (int)a->ob_digit[i] - (int)b->ob_digit[i];
			if (a->ob_size < 0)
			{
				sign = -sign;
			}
		}
	}
	return sign < 0 ? -1 : sign > 0 ? 1 : 0;
}

static long long_hash(PyLongObject *v)
{
	long x;
	int i, sign;

	i = v->ob_size;
	sign = 1;
	x = 0;
	if (i < 0) 
	{
		sign = -1;
		i = -(i);
	}
	while (--i >= 0) 
	{
		x = ((x << SHIFT) & ~MASK) | ((x >> (32-SHIFT)) & MASK);
		x += v->ob_digit[i];
	}
	x = x * sign;
	if (x == -1)
	{
		x = -2;
	}
	return x;
}


static PyLongObject *x_add(PyLongObject *a, PyLongObject *b)
{
	int size_a = ABS(a->ob_size), size_b = ABS(b->ob_size);
	PyLongObject *z;
	int i;
	digit carry = 0;

	if (size_a < size_b) 
	{
		{ 
			PyLongObject *temp = a; 
			a = b; 
			b = temp; 
		}
		{ 
			int size_temp = size_a;
			size_a = size_b;
			size_b = size_temp; 
		}
	}
	z = _PyLong_New(size_a+1);
	if (z == NULL)
	{
		return NULL;
	}
	for (i = 0; i < size_b; ++i) 
	{
		carry += a->ob_digit[i] + b->ob_digit[i];
		z->ob_digit[i] = carry & MASK;
		carry >>= SHIFT;
	}
	for (; i < size_a; ++i) 
	{
		carry += a->ob_digit[i];
		z->ob_digit[i] = carry & MASK;
		carry >>= SHIFT;
	}
	z->ob_digit[i] = carry;
	return long_normalize(z);
}

static PyLongObject *x_sub(PyLongObject *a, PyLongObject *b)
{
	int size_a = ABS(a->ob_size), size_b = ABS(b->ob_size);
	PyLongObject *z;
	int i;
	int sign = 1;
	digit borrow = 0;

	if (size_a < size_b) 
	{
		sign = -1;
		{ 
			PyLongObject *temp = a; 
			a = b; 
			b = temp; 
		}
		{ 
			int size_temp = size_a;
			size_a = size_b;
			size_b = size_temp; 
		}
	}
	else if (size_a == size_b) 
	{
		i = size_a;
		while (--i >= 0 && a->ob_digit[i] == b->ob_digit[i])
		{
			;
		}
		if (i < 0)
		{
			return _PyLong_New(0);
		}
		if (a->ob_digit[i] < b->ob_digit[i]) 
		{
			sign = -1;
			{ 
				PyLongObject *temp = a; 
				a = b; 
				b = temp; 
			}
		}
		size_a = size_b = i+1;
	}
	z = _PyLong_New(size_a);
	if (z == NULL)
	{
		return NULL;
	}
	for (i = 0; i < size_b; ++i) 
	{
		borrow = a->ob_digit[i] - b->ob_digit[i] - borrow;
		z->ob_digit[i] = borrow & MASK;
		borrow >>= SHIFT;
		borrow &= 1;
	}
	for (; i < size_a; ++i) 
	{
		borrow = a->ob_digit[i] - borrow;
		z->ob_digit[i] = borrow & MASK;
		borrow >>= SHIFT;
		borrow &= 1; 
	}
	assert(borrow == 0);
	if (sign < 0)
	{
		z->ob_size = -(z->ob_size);
	}
	return long_normalize(z);
}

static PyObject *long_add(PyLongObject *v, PyLongObject *w)
{
	PyLongObject *a, *b, *z;

	CONVERT_BINOP((PyObject *)v, (PyObject *)w, &a, &b);

	if (a->ob_size < 0) 
	{
		if (b->ob_size < 0) 
		{
			z = x_add(a, b);
			if (z != NULL && z->ob_size != 0)
			{
				z->ob_size = -(z->ob_size);
			}
		}
		else
		{
			z = x_sub(b, a);
		}
	}
	else 
	{
		if (b->ob_size < 0)
		{
			z = x_sub(a, b);
		}
		else
		{
			z = x_add(a, b);
		}
	}
	Py_DECREF(a);
	Py_DECREF(b);
	return (PyObject *)z;
}

static PyObject *long_sub(PyLongObject *v, PyLongObject *w)
{
	PyLongObject *a, *b, *z;
	
	CONVERT_BINOP((PyObject *)v, (PyObject *)w, &a, &b);

	if (a->ob_size < 0) 
	{
		if (b->ob_size < 0)
		{
			z = x_sub(a, b);
		}
		else
		{
			z = x_add(a, b);
		}
		if (z != NULL && z->ob_size != 0)
		{
			z->ob_size = -(z->ob_size);
		}
	}
	else 
	{
		if (b->ob_size < 0)
		{
			z = x_add(a, b);
		}
		else
		{
			z = x_sub(a, b);
		}
	}
	Py_DECREF(a);
	Py_DECREF(b);
	return (PyObject *)z;
}

static PyObject *long_repeat(PyObject *v, PyLongObject *w)
{
	long n = PyLong_AsLong((PyObject *) w);
	if (n == -1 && PyErr_Occurred())
	{
		return NULL;
	}
	else
	{
		return (*v->ob_type->tp_as_sequence->sq_repeat)(v, n);
	}
}

static PyObject *long_mul(PyLongObject *v, PyLongObject *w)
{
	PyLongObject *a, *b, *z;
	int size_a;
	int size_b;
	int i;

	if (!convert_binop((PyObject *)v, (PyObject *)w, &a, &b)) 
	{
		if (!PyLong_Check(v) &&
		    v->ob_type->tp_as_sequence &&
		    v->ob_type->tp_as_sequence->sq_repeat)
		{
			return long_repeat((PyObject *)v, w);
		}
		if (!PyLong_Check(w) &&
			 w->ob_type->tp_as_sequence &&
			 w->ob_type->tp_as_sequence->sq_repeat)
		{
			return long_repeat((PyObject *)w, v);
		}
		Py_INCREF(Py_NotImplemented);
		return Py_NotImplemented;
	}

	size_a = ABS(a->ob_size);
	size_b = ABS(b->ob_size);
	if (size_a > size_b) 
	{
		int hold_sa = size_a;
		PyLongObject *hold_a = a;
		size_a = size_b;
		size_b = hold_sa;
		a = b;
		b = hold_a;
	}
	z = _PyLong_New(size_a + size_b);
	if (z == NULL) 
	{
		Py_DECREF(a);
		Py_DECREF(b);
		return NULL;
	}
	for (i = 0; i < z->ob_size; ++i)
	{
		z->ob_digit[i] = 0;
	}
	for (i = 0; i < size_a; ++i) 
	{
		twodigits carry = 0;
		twodigits f = a->ob_digit[i];
		int j;
		
		SIGCHECK({
			Py_DECREF(a);
			Py_DECREF(b);
			Py_DECREF(z);
			return NULL;
		})
		for (j = 0; j < size_b; ++j) 
		{
			carry += z->ob_digit[i+j] + b->ob_digit[j] * f;
			z->ob_digit[i+j] = (digit) (carry & MASK);
			carry >>= SHIFT;
		}
		for (; carry != 0; ++j) 
		{
			assert(i+j < z->ob_size);
			carry += z->ob_digit[i+j];
			z->ob_digit[i+j] = (digit) (carry & MASK);
			carry >>= SHIFT;
		}
	}
	if (a->ob_size < 0)
	{
		z->ob_size = -(z->ob_size);
	}
	if (b->ob_size < 0)
	{
		z->ob_size = -(z->ob_size);
	}
	Py_DECREF(a);
	Py_DECREF(b);
	return (PyObject *) long_normalize(z);
}

static int l_divmod(PyLongObject *v, PyLongObject *w, 
	 PyLongObject **pdiv, PyLongObject **pmod)
{
	PyLongObject *div, *mod;
	
	if (long_divrem(v, w, &div, &mod) < 0)
		return -1;
	if ((mod->ob_size < 0 && w->ob_size > 0) ||
	    (mod->ob_size > 0 && w->ob_size < 0)) {
		PyLongObject *temp;
		PyLongObject *one;
		temp = (PyLongObject *) long_add(mod, w);
		Py_DECREF(mod);
		mod = temp;
		if (mod == NULL) {
			Py_DECREF(div);
			return -1;
		}
		one = (PyLongObject *) PyLong_FromLong(1L);
		if (one == NULL ||
		    (temp = (PyLongObject *) long_sub(div, one)) == NULL) {
			Py_DECREF(mod);
			Py_DECREF(div);
			Py_XDECREF(one);
			return -1;
		}
		Py_DECREF(one);
		Py_DECREF(div);
		div = temp;
	}
	*pdiv = div;
	*pmod = mod;
	return 0;
}

static PyObject *long_div(PyObject *v, PyObject *w)
{
	PyLongObject *a, *b, *div, *mod;

	CONVERT_BINOP(v, w, &a, &b);

	if (l_divmod(a, b, &div, &mod) < 0) 
	{
		Py_DECREF(a);
		Py_DECREF(b);
		return NULL;
	}
	Py_DECREF(a);
	Py_DECREF(b);
	Py_DECREF(mod);
	return (PyObject *)div;
}

static PyObject *long_classic_div(PyObject *v, PyObject *w)
{
	PyLongObject *a, *b, *div, *mod;

	CONVERT_BINOP(v, w, &a, &b);

	if (Py_DivisionWarningFlag &&
	    PyErr_Warn(PyExc_DeprecationWarning, "classic long division") < 0)
	{
		div = NULL;
	}
	else if (l_divmod(a, b, &div, &mod) < 0)
	{
		div = NULL;
	}
	else
	{
		Py_DECREF(mod);
	}

	Py_DECREF(a);
	Py_DECREF(b);
	return (PyObject *)div;
}

static PyObject *long_true_divide(PyObject *v, PyObject *w)
{
	PyLongObject *a, *b;
	double ad, bd;
	int aexp, bexp, failed;

	CONVERT_BINOP(v, w, &a, &b);
	ad = _PyLong_AsScaledDouble((PyObject *)a, &aexp);
	bd = _PyLong_AsScaledDouble((PyObject *)b, &bexp);
	failed = (ad == -1.0 || bd == -1.0) && PyErr_Occurred();
	Py_DECREF(a);
	Py_DECREF(b);
	if (failed)
	{
		return NULL;
	}

	if (bd == 0.0) 
	{
		PyErr_SetString(PyExc_ZeroDivisionError,
			"long division or modulo by zero");
		return NULL;
	}

	ad /= bd;	
	aexp -= bexp;
	if (aexp > INT_MAX / SHIFT)
	{
		goto overflow;
	}
	else if (aexp < -(INT_MAX / SHIFT))
	{
		return PyFloat_FromDouble(0.0);
	}
	errno = 0;
	ad = ldexp(ad, aexp * SHIFT);
	if (Py_OVERFLOWED(ad))
	{
		goto overflow;
	}
	return PyFloat_FromDouble(ad);

overflow:
	PyErr_SetString(PyExc_OverflowError,
		"long/long too large for a float");
	return NULL;
	
}

static PyObject *long_mod(PyObject *v, PyObject *w)
{
	PyLongObject *a, *b, *div, *mod;

	CONVERT_BINOP(v, w, &a, &b);

	if (l_divmod(a, b, &div, &mod) < 0) 
	{
		Py_DECREF(a);
		Py_DECREF(b);
		return NULL;
	}
	Py_DECREF(a);
	Py_DECREF(b);
	Py_DECREF(div);
	return (PyObject *)mod;
}

static PyObject *long_divmod(PyObject *v, PyObject *w)
{
	PyLongObject *a, *b, *div, *mod;
	PyObject *z;

	CONVERT_BINOP(v, w, &a, &b);

	if (l_divmod(a, b, &div, &mod) < 0) 
	{
		Py_DECREF(a);
		Py_DECREF(b);
		return NULL;
	}
	z = PyTuple_New(2);
	if (z != NULL) 
	{
		PyTuple_SetItem(z, 0, (PyObject *) div);
		PyTuple_SetItem(z, 1, (PyObject *) mod);
	}
	else 
	{
		Py_DECREF(div);
		Py_DECREF(mod);
	}
	Py_DECREF(a);
	Py_DECREF(b);
	return z;
}

static PyObject *long_pow(PyObject *v, PyObject *w, PyObject *x)
{
	PyLongObject *a, *b;
	PyObject *c;
	PyLongObject *z, *div, *mod;
	int size_b, i;

	CONVERT_BINOP(v, w, &a, &b);
	if (PyLong_Check(x) || Py_None == x) 
	{ 
		c = x;
		Py_INCREF(x);
	}
	else if (PyInt_Check(x)) 
	{
		c = PyLong_FromLong(PyInt_AS_LONG(x));
	}
	else 
	{
		Py_DECREF(a);
		Py_DECREF(b);
		Py_INCREF(Py_NotImplemented);
		return Py_NotImplemented;
	}

	if (c != Py_None && ((PyLongObject *)c)->ob_size == 0) 
	{
		PyErr_SetString(PyExc_ValueError,
				"pow() 3rd argument cannot be 0");
		z = NULL;
		goto error;
	}

	size_b = b->ob_size;
	if (size_b < 0) 
	{
		Py_DECREF(a);
		Py_DECREF(b);
		Py_DECREF(c);
		if (x != Py_None) 
		{
			PyErr_SetString(PyExc_TypeError, "pow() 2nd argument "
			     "cannot be negative when 3rd argument specified");
			return NULL;
		}
		return PyFloat_Type.tp_as_number->nb_power(v, w, x);
	}
	z = (PyLongObject *)PyLong_FromLong(1L);
	for (i = 0; i < size_b; ++i) 
	{
		digit bi = b->ob_digit[i];
		int j;
	
		for (j = 0; j < SHIFT; ++j) 
		{
			PyLongObject *temp;
		
			if (bi & 1) 
			{
				temp = (PyLongObject *)long_mul(z, a);
				Py_DECREF(z);
			 	if (c != Py_None && temp != NULL) 
				{
			 		if (l_divmod(temp,(PyLongObject *)c,
							&div,&mod) < 0) 
					{
						Py_DECREF(temp);
						z = NULL;
						goto error;
					}
				 	Py_XDECREF(div);
				 	Py_DECREF(temp);
				 	temp = mod;
				}
			 	z = temp;
				if (z == NULL)
				{
					break;
				}
			}
			bi >>= 1;
			if (bi == 0 && i+1 == size_b)
			{
				break;
			}
			temp = (PyLongObject *)long_mul(a, a);
			Py_DECREF(a);
		 	if (c!=Py_None && temp!=NULL) 
			{
			 	if (l_divmod(temp, (PyLongObject *)c, &div,
							&mod) < 0) 
				{
					Py_DECREF(temp);
					z = NULL;
					goto error;
				}
			 	Py_XDECREF(div);
			 	Py_DECREF(temp);
			 	temp = mod;
			}
			a = temp;
			if (a == NULL) 
			{
				Py_DECREF(z);
				z = NULL;
				break;
			}
		}
		if (a == NULL || z == NULL)
		{
			break;
		}
	}
	if (c!=Py_None && z!=NULL) 
	{
		if (l_divmod(z, (PyLongObject *)c, &div, &mod) < 0) 
		{
			Py_DECREF(z);
			z = NULL;
		}
		else 
		{
			Py_XDECREF(div);
			Py_DECREF(z);
			z = mod;
		}
	}
error:
	Py_XDECREF(a);
	Py_DECREF(b);
	Py_DECREF(c);
	return (PyObject *)z;
}

static PyObject *long_invert(PyLongObject *v)
{
	PyLongObject *x;
	PyLongObject *w;
	w = (PyLongObject *)PyLong_FromLong(1L);
	if (w == NULL)
	{
		return NULL;
	}
	x = (PyLongObject *) long_add(v, w);
	Py_DECREF(w);
	if (x == NULL)
	{
		return NULL;
	}
	x->ob_size = -(x->ob_size);
	return (PyObject *)x;
}

static PyObject *long_pos(PyLongObject *v)
{
	if (PyLong_CheckExact(v)) 
	{
		Py_INCREF(v);
		return (PyObject *)v;
	}
	else
	{
		return _PyLong_Copy(v);
	}
}

static PyObject *long_neg(PyLongObject *v)
{
	PyLongObject *z;
	if (v->ob_size == 0 && PyLong_CheckExact(v)) 
	{
		Py_INCREF(v);
		return (PyObject *) v;
	}
	z = (PyLongObject *)_PyLong_Copy(v);
	if (z != NULL)
	{
		z->ob_size = -(v->ob_size);
	}
	return (PyObject *)z;
}

static PyObject *long_abs(PyLongObject *v)
{
	if (v->ob_size < 0)
	{
		return long_neg(v);
	}
	else
	{
		return long_pos(v);
	}
}

static int long_nonzero(PyLongObject *v)
{
	return ABS(v->ob_size) != 0;
}

static PyObject *long_rshift(PyLongObject *v, PyLongObject *w)
{
	PyLongObject *a, *b;
	PyLongObject *z = NULL;
	long shiftby;
	int newsize, wordshift, loshift, hishift, i, j;
	digit lomask, himask;
	
	CONVERT_BINOP((PyObject *)v, (PyObject *)w, &a, &b);

	if (a->ob_size < 0) 
	{
		PyLongObject *a1, *a2;
		a1 = (PyLongObject *) long_invert(a);
		if (a1 == NULL)
		{
			goto rshift_error;
		}
		a2 = (PyLongObject *) long_rshift(a1, b);
		Py_DECREF(a1);
		if (a2 == NULL)
		{
			goto rshift_error;
		}
		z = (PyLongObject *) long_invert(a2);
		Py_DECREF(a2);
	}
	else 
	{	
		shiftby = PyLong_AsLong((PyObject *)b);
		if (shiftby == -1L && PyErr_Occurred())
		{
			goto rshift_error;
		}
		if (shiftby < 0) 
		{
			PyErr_SetString(PyExc_ValueError,
					"negative shift count");
			goto rshift_error;
		}
		wordshift = shiftby / SHIFT;
		newsize = ABS(a->ob_size) - wordshift;
		if (newsize <= 0) 
		{
			z = _PyLong_New(0);
			Py_DECREF(a);
			Py_DECREF(b);
			return (PyObject *)z;
		}
		loshift = shiftby % SHIFT;
		hishift = SHIFT - loshift;
		lomask = ((digit)1 << hishift) - 1;
		himask = MASK ^ lomask;
		z = _PyLong_New(newsize);
		if (z == NULL)
		{
			goto rshift_error;
		}
		if (a->ob_size < 0)
		{
			z->ob_size = -(z->ob_size);
		}
		for (i = 0, j = wordshift; i < newsize; i++, j++) 
		{
			z->ob_digit[i] = (a->ob_digit[j] >> loshift) & lomask;
			if (i+1 < newsize)
			{
				z->ob_digit[i] |=
				  (a->ob_digit[j+1] << hishift) & himask;
			}
		}
		z = long_normalize(z);
	}
rshift_error:
	Py_DECREF(a);
	Py_DECREF(b);
	return (PyObject *) z;

}

static PyObject *long_lshift(PyObject *v, PyObject *w)
{
	PyLongObject *a, *b;
	PyLongObject *z = NULL;
	long shiftby;
	int oldsize, newsize, wordshift, remshift, i, j;
	twodigits accum;
	
	CONVERT_BINOP(v, w, &a, &b);

	shiftby = PyLong_AsLong((PyObject *)b);
	if (shiftby == -1L && PyErr_Occurred())
	{
		goto lshift_error;
	}
	if (shiftby < 0) 
	{
		PyErr_SetString(PyExc_ValueError, "negative shift count");
		goto lshift_error;
	}
	if ((long)(int)shiftby != shiftby) 
	{
		PyErr_SetString(PyExc_ValueError,
				"outrageous left shift count");
		goto lshift_error;
	}
	wordshift = (int)shiftby / SHIFT;
	remshift  = (int)shiftby - wordshift * SHIFT;

	oldsize = ABS(a->ob_size);
	newsize = oldsize + wordshift;
	if (remshift)
	{
		++newsize;
	}
	z = _PyLong_New(newsize);
	if (z == NULL)
	{
		goto lshift_error;
	}
	if (a->ob_size < 0)
	{
		z->ob_size = -(z->ob_size);
	}
	for (i = 0; i < wordshift; i++)
	{
		z->ob_digit[i] = 0;
	}
	accum = 0;	
	for (i = wordshift, j = 0; j < oldsize; i++, j++) 
	{
		accum |= a->ob_digit[j] << remshift;
		z->ob_digit[i] = (digit)(accum & MASK);
		accum >>= SHIFT;
	}
	if (remshift)
	{
		z->ob_digit[newsize-1] = (digit)accum;
	}
	else	
	{
		assert(!accum);
	}
	z = long_normalize(z);
lshift_error:
	Py_DECREF(a);
	Py_DECREF(b);
	return (PyObject *) z;
}

#define MAX(x, y) ((x) < (y) ? (y) : (x))
#define MIN(x, y) ((x) > (y) ? (y) : (x))

static PyObject *long_bitwise(PyLongObject *a,
	     int op, 
	     PyLongObject *b)
{
	digit maska, maskb;
	int negz;
	int size_a, size_b, size_z;
	PyLongObject *z;
	int i;
	digit diga, digb;
	PyObject *v;
	
	if (a->ob_size < 0) 
	{
		a = (PyLongObject *) long_invert(a);
		maska = MASK;
	}
	else 
	{
		Py_INCREF(a);
		maska = 0;
	}
	if (b->ob_size < 0) 
	{
		b = (PyLongObject *) long_invert(b);
		maskb = MASK;
	}
	else 
	{
		Py_INCREF(b);
		maskb = 0;
	}
	
	negz = 0;
	switch (op) 
	{
	case '^':
		if (maska != maskb) 
		{
			maska ^= MASK;
			negz = -1;
		}
		break;
	
	case '&':
		if (maska && maskb)
		{
			op = '|';
			maska ^= MASK;
			maskb ^= MASK;
			negz = -1;
		}
		break;
	
	case '|':
		if (maska || maskb) 
		{
			op = '&';
			maska ^= MASK;
			maskb ^= MASK;
			negz = -1;
		}
		break;
	}
	
	size_a = a->ob_size;
	size_b = b->ob_size;
	size_z = op == '&'
		? (maska
		   ? size_b
		   : (maskb ? size_a : MIN(size_a, size_b)))
		: MAX(size_a, size_b);
	z = _PyLong_New(size_z);
	if (a == NULL || b == NULL || z == NULL) 
	{
		Py_XDECREF(a);
		Py_XDECREF(b);
		Py_XDECREF(z);
		return NULL;
	}
	
	for (i = 0; i < size_z; ++i) 
	{
		diga = (i < size_a ? a->ob_digit[i] : 0) ^ maska;
		digb = (i < size_b ? b->ob_digit[i] : 0) ^ maskb;
		switch (op) 
		{
		case '&': 
			z->ob_digit[i] = diga & digb; 
			break;

		case '|': 
			z->ob_digit[i] = diga | digb; 
			break;
		
		case '^': 
			z->ob_digit[i] = diga ^ digb; 
			break;
		}
	}
	
	Py_DECREF(a);
	Py_DECREF(b);
	z = long_normalize(z);
	if (negz == 0)
	{
		return (PyObject *) z;
	}
	v = long_invert(z);
	Py_DECREF(z);
	return v;
}

static PyObject *long_and(PyObject *v, PyObject *w)
{
	PyLongObject *a, *b;
	PyObject *c;
	CONVERT_BINOP(v, w, &a, &b);
	c = long_bitwise(a, '&', b);
	Py_DECREF(a);
	Py_DECREF(b);
	return c;
}

static PyObject *long_xor(PyObject *v, PyObject *w)
{
	PyLongObject *a, *b;
	PyObject *c;
	CONVERT_BINOP(v, w, &a, &b);
	c = long_bitwise(a, '^', b);
	Py_DECREF(a);
	Py_DECREF(b);
	return c;
}

static PyObject *long_or(PyObject *v, PyObject *w)
{
	PyLongObject *a, *b;
	PyObject *c;
	CONVERT_BINOP(v, w, &a, &b);
	c = long_bitwise(a, '|', b);
	Py_DECREF(a);
	Py_DECREF(b);
	return c;
}

static int long_coerce(PyObject **pv, PyObject **pw)
{
	if (PyInt_Check(*pw)) 
	{
		*pw = PyLong_FromLong(PyInt_AS_LONG(*pw));
		Py_INCREF(*pv);
		return 0;
	}
	else if (PyLong_Check(*pw)) 
	{
		Py_INCREF(*pv);
		Py_INCREF(*pw);
		return 0;
	}
	return 1;
}

static PyObject *long_int(PyObject *v)
{
	long x;
	x = PyLong_AsLong(v);
	if (PyErr_Occurred())
	{
		return NULL;
	}
	return PyInt_FromLong(x);
}

static PyObject *long_long(PyObject *v)
{
	Py_INCREF(v);
	return v;
}

static PyObject *long_float(PyObject *v)
{
	double result;
	result = PyLong_AsDouble(v);
	if (result == -1.0 && PyErr_Occurred())
	{
		return NULL;
	}
	return PyFloat_FromDouble(result);
}

static PyObject *long_oct(PyObject *v)
{
	return long_format(v, 8, 1);
}

static PyObject *long_hex(PyObject *v)
{
	return long_format(v, 16, 1);
}

staticforward PyObject *long_subtype_new(PyTypeObject *type, PyObject *args, PyObject *kwds);

static PyObject *long_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	PyObject *x = NULL;
	int base = -909;
	static char *kwlist[] = {"x", "base", 0};

	if (type != &PyLong_Type)
	{
		return long_subtype_new(type, args, kwds);
	}
	if (!PyArg_ParseTupleAndKeywords(args, kwds, "|Oi:long", kwlist,
					 &x, &base))
	{
		return NULL;
	}
	if (x == NULL)
	{
		return PyLong_FromLong(0L);
	}
	if (base == -909)
	{
		return PyNumber_Long(x);
	}
	else if (PyString_Check(x))
	{
		return PyLong_FromString(PyString_AS_STRING(x), NULL, base);
	}
	else if (PyUnicode_Check(x))
	{
		return PyLong_FromUnicode(PyUnicode_AS_UNICODE(x),
					  PyUnicode_GET_SIZE(x),
					  base);
	}
	else 
	{
		PyErr_SetString(PyExc_TypeError,
			"long() can't convert non-string with explicit base");
		return NULL;
	}
}

static PyObject *long_subtype_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	PyLongObject *tmp, *new;
	int i, n;

	assert(PyType_IsSubtype(type, &PyLong_Type));
	tmp = (PyLongObject *)long_new(&PyLong_Type, args, kwds);
	if (tmp == NULL)
	{
		return NULL;
	}
	assert(PyLong_CheckExact(tmp));
	n = tmp->ob_size;
	if (n < 0)
	{
		n = -n;
	}
	new = (PyLongObject *)type->tp_alloc(type, n);
	if (new == NULL)
	{
		return NULL;
	}
	assert(PyLong_Check(new));
	new->ob_size = tmp->ob_size;
	for (i = 0; i < n; i++)
	{
		new->ob_digit[i] = tmp->ob_digit[i];
	}
	Py_DECREF(tmp);
	return (PyObject *)new;
}

static char long_doc[] =
	"long(x[, base]) -> integer\n"
	"\n"
	"Convert a string or number to a long integer, if possible.  A floating\n"
	"point argument will be truncated towards zero (this does not include a\n"
	"string representation of a floating point number!)  When converting a\n"
	"string, use the optional base.  It is an error to supply a base when\n"
	"converting a non-string.";

static PyNumberMethods long_as_number = {
	(binaryfunc)	long_add,	
	(binaryfunc)	long_sub,	
	(binaryfunc)	long_mul,	
	(binaryfunc)	long_classic_div, 
	(binaryfunc)	long_mod,	
	(binaryfunc)	long_divmod,
	(ternaryfunc)	long_pow,	
	(unaryfunc) 	long_neg,	
	(unaryfunc) 	long_pos,	
	(unaryfunc) 	long_abs,	
	(inquiry)	long_nonzero,	
	(unaryfunc)	long_invert,	
	(binaryfunc)	long_lshift,
	(binaryfunc)	long_rshift,
	(binaryfunc)	long_and,	
	(binaryfunc)	long_xor,	
	(binaryfunc)	long_or,	
	(coercion)	long_coerce,	
	(unaryfunc)	long_int,	
	(unaryfunc)	long_long,	
	(unaryfunc)	long_float,	
	(unaryfunc)	long_oct,	
	(unaryfunc)	long_hex,	
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
	(binaryfunc)long_div,	
	long_true_divide,		
	0,				
	0,				
};

PyTypeObject PyLong_Type = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,					
	"long",				
	sizeof(PyLongObject) - sizeof(digit),	
	sizeof(digit),		
	(destructor)long_dealloc,	
	0,					
	0,					
	0,					
	(cmpfunc)long_compare,			
	(reprfunc)long_repr,			
	&long_as_number,			
	0,					
	0,					
	(hashfunc)long_hash,		
        0,              		
        (reprfunc)long_str,		
	PyObject_GenericGetAttr,	
	0,					
	0,					
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_CHECKTYPES |
		Py_TPFLAGS_BASETYPE,	
	long_doc,			
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
	long_new,			
	_PyObject_Del,			
};
