//20180318
#pragma once

typedef unsigned short digit;
typedef unsigned int wdigit;
#define BASE_TWODIGITS_TYPE long
typedef unsigned BASE_TWODIGITS_TYPE twodigits;
typedef BASE_TWODIGITS_TYPE stwodigits;

#define SHIFT 15
#define BASE ((digit)1 << SHIFT)
#define MASK ((int)(BASE - 1))

struct _longobject {
	PyObject_HEAD
	int ob_size;
	digit ob_digit[1];
};

PyLongObject * _PyLong_New(int);
PyObject * _PyLong_Copy(PyLongObject *src);
