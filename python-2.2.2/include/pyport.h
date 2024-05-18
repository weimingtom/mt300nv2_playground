//20180324
#pragma once

#include "pyconfig.h" 

#define ANY void
#define Py_PROTO(x) x
#define Py_FPROTO(x) Py_PROTO(x)

typedef unsigned int	Py_uintptr_t;
typedef int		Py_intptr_t;

#include <stdlib.h>

#include <math.h> 

#include <time.h>

#define HAVE_STAT

#define HAVE_FSTAT

#include <sys/stat.h>

#define S_ISREG_(x) (((x) & S_IFMT) == S_IFREG)
#define S_ISDIR_(x) (((x) & S_IFMT) == S_IFDIR)

#define Py_ARITHMETIC_RIGHT_SHIFT(TYPE, I, J) ((I) >> (J))

#define Py_FORCE_EXPANSION(X) X

#ifdef _DEBUG
#define Py_SAFE_DOWNCAST(VALUE, WIDE, NARROW) \
	(assert((WIDE)(NARROW)(VALUE) == (VALUE)), (NARROW)(VALUE))
#else
#define Py_SAFE_DOWNCAST(VALUE, WIDE, NARROW) (NARROW)(VALUE)
#endif

#define Py_IS_INFINITY(X) ((X) && (X)*0.5 == (X))

#define Py_HUGE_VAL HUGE_VAL

#define Py_OVERFLOWED(X) ((X) != 0.0 && (errno == ERANGE ||    \
					 (X) == Py_HUGE_VAL || \
					 (X) == -Py_HUGE_VAL))

#define Py_SET_ERANGE_IF_OVERFLOW(X) \
	do { \
		if (errno == 0 && ((X) == Py_HUGE_VAL ||  \
				   (X) == -Py_HUGE_VAL))  \
			errno = ERANGE; \
	} while(0)

#define Py_ADJUST_ERANGE1(X)						\
	do {								\
		if (errno == 0) {					\
			if ((X) == Py_HUGE_VAL || (X) == -Py_HUGE_VAL)	\
				errno = ERANGE;				\
		}							\
		else if (errno == ERANGE && (X) == 0.0)			\
			errno = 0;					\
	} while(0)

#define Py_ADJUST_ERANGE2(X, Y)						\
	do {								\
		if ((X) == Py_HUGE_VAL || (X) == -Py_HUGE_VAL ||	\
		    (Y) == Py_HUGE_VAL || (Y) == -Py_HUGE_VAL) {	\
				if (errno == 0)				\
					errno = ERANGE;			\
		}							\
		else if (errno == ERANGE)				\
			errno = 0;					\
	} while(0)

#define _PyMem_EXTRA 0
