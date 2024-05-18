//20180324
#pragma once

#include "pyport.h"

#undef PyCore_REALLOC_FUNC
#undef PyCore_FREE_FUNC
#define PyCore_MALLOC_FUNC      malloc
#define PyCore_REALLOC_FUNC     realloc
#define PyCore_FREE_FUNC        free

#undef PyCore_REALLOC_PROTO
#undef PyCore_FREE_PROTO
#define PyCore_MALLOC_PROTO    (size_t)
#define PyCore_REALLOC_PROTO   (void *, size_t)
#define PyCore_FREE_PROTO      (void *)

#undef PyCore_REALLOC
#undef PyCore_FREE
#define PyCore_MALLOC(n)        PyCore_MALLOC_FUNC(n)
#define PyCore_REALLOC(p, n)    PyCore_REALLOC_FUNC((p), (n))
#define PyCore_FREE(p)          PyCore_FREE_FUNC(p)

extern void * PyMem_Malloc(size_t);
extern void * PyMem_Realloc(void *, size_t);
extern void PyMem_Free(void *);

#define PyMem_MALLOC(n)         PyCore_MALLOC(n)
#define PyMem_REALLOC(p, n)     PyCore_REALLOC((void *)(p), (n))
#define PyMem_FREE(p)           PyCore_FREE((void *)(p))

#define PyMem_New(type, n) \
	( (type *) PyMem_Malloc((n) * sizeof(type)) )
#define PyMem_Resize(p, type, n) \
	( (p) = (type *) PyMem_Realloc((p), (n) * sizeof(type)) )
#define PyMem_Del(p) PyMem_Free(p)

#define PyMem_NEW(type, n) \
	( (type *) PyMem_MALLOC(_PyMem_EXTRA + (n) * sizeof(type)) )

#define PyMem_RESIZE(p, type, n)			\
	do {						\
		size_t _sum = (n) * sizeof(type);	\
		if (!_sum)				\
			_sum = 1;			\
		(p) = (type *)((p) ?			\
			       PyMem_REALLOC(p, _sum) :	\
			       PyMem_MALLOC(_sum));	\
	} while (0)

#define PyMem_DEL(p) PyMem_FREE(p)
