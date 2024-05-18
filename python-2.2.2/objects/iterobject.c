//20170418
#include "python.h"

typedef struct {
	PyObject_HEAD
	long      it_index;
	PyObject *it_seq;
} seqiterobject;

PyObject *PySeqIter_New(PyObject *seq)
{
	seqiterobject *it;
	it = PyObject_NEW(seqiterobject, &PySeqIter_Type);
	if (it == NULL)
	{
		return NULL;
	}
	it->it_index = 0;
	Py_INCREF(seq);
	it->it_seq = seq;
	return (PyObject *)it;
}

static void iter_dealloc(seqiterobject *it)
{
	Py_DECREF(it->it_seq);
	PyObject_DEL(it);
}

static int iter_traverse(seqiterobject *it, visitproc visit, void *arg)
{
	return visit(it->it_seq, arg);
}

static PyObject *iter_next(seqiterobject *it)
{
	PyObject *seq = it->it_seq;
	PyObject *result = PySequence_GetItem(seq, it->it_index++);

	if (result == NULL && PyErr_ExceptionMatches(PyExc_IndexError))
	{
		PyErr_SetObject(PyExc_StopIteration, Py_None);
	}
	return result;
}

static PyObject *iter_getiter(PyObject *it)
{
	Py_INCREF(it);
	return it;
}

static PyObject *iter_iternext(PyObject *iterator)
{
	seqiterobject *it;
	PyObject *seq;

	assert(PySeqIter_Check(iterator));
	it = (seqiterobject *)iterator;
	seq = it->it_seq;

	if (PyList_Check(seq)) 
	{
		PyObject *item;
		if (it->it_index >= PyList_GET_SIZE(seq)) 
		{
			return NULL;
		}
		item = PyList_GET_ITEM(seq, it->it_index);
		it->it_index++;
		Py_INCREF(item);
		return item;
	}
	else 
	{
		PyObject *result = PySequence_GetItem(seq, it->it_index++);
		if (result != NULL) 
		{
			return result;
		}
		if (PyErr_ExceptionMatches(PyExc_IndexError) ||
			PyErr_ExceptionMatches(PyExc_StopIteration)) 
		{
			PyErr_Clear();
			return NULL;
		}
		else 
		{
			return NULL;
		}
	}
}

static PyMethodDef iter_methods[] = {
	{"next",	(PyCFunction)iter_next,	METH_NOARGS,
	 "it.next() -- get the next value, or raise StopIteration"},
	{NULL,		NULL}
};

PyTypeObject PySeqIter_Type = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,				
	"iterator",			
	sizeof(seqiterobject),	
	0,			
	(destructor)iter_dealloc, 
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
	PyObject_GenericGetAttr,	
	0,				
	0,				
	Py_TPFLAGS_DEFAULT,	
 	0,			
 	(traverseproc)iter_traverse,
 	0,				
	0,			
	0,				
	(getiterfunc)iter_getiter,	
	(iternextfunc)iter_iternext,	
	iter_methods,		
	0,				
	0,				
	0,				
	0,				
	0,		
	0,	
};

typedef struct {
	PyObject_HEAD
	PyObject *it_callable;
	PyObject *it_sentinel;
} calliterobject;

PyObject *PyCallIter_New(PyObject *callable, PyObject *sentinel)
{
	calliterobject *it;
	it = PyObject_NEW(calliterobject, &PyCallIter_Type);
	if (it == NULL)
	{
		return NULL;
	}
	Py_INCREF(callable);
	it->it_callable = callable;
	Py_INCREF(sentinel);
	it->it_sentinel = sentinel;
	return (PyObject *)it;
}

static void calliter_dealloc(calliterobject *it)
{
	Py_DECREF(it->it_callable);
	Py_DECREF(it->it_sentinel);
	PyObject_DEL(it);
}

static int calliter_traverse(calliterobject *it, visitproc visit, void *arg)
{
	int err;
	if ((err = visit(it->it_callable, arg)))
	{
		return err;
	}
	if ((err = visit(it->it_sentinel, arg)))
	{
		return err;
	}
	return 0;
}

static PyObject *calliter_next(calliterobject *it, PyObject *args)
{
	PyObject *result = PyObject_CallObject(it->it_callable, NULL);
	if (result != NULL) 
	{
		if (PyObject_RichCompareBool(result, it->it_sentinel, Py_EQ)) 
		{
			PyErr_SetObject(PyExc_StopIteration, Py_None);
			Py_DECREF(result);
			result = NULL;
		}
	}
	return result;
}

static PyMethodDef calliter_methods[] = {
	{"next",	(PyCFunction)calliter_next,	METH_VARARGS,
	 "it.next() -- get the next value, or raise StopIteration"},
	{NULL,		NULL}
};

static PyObject *calliter_iternext(calliterobject *it)
{
	PyObject *result = PyObject_CallObject(it->it_callable, NULL);
	if (result != NULL) 
	{
		if (PyObject_RichCompareBool(result, it->it_sentinel, Py_EQ)) 
		{
			Py_DECREF(result);
			result = NULL;
		}
	}
	else if (PyErr_ExceptionMatches(PyExc_StopIteration)) 
	{
		PyErr_Clear();
	}
	return result;
}

PyTypeObject PyCallIter_Type = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,					
	"callable-iterator",			
	sizeof(calliterobject),			
	0,					
	(destructor)calliter_dealloc, 
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
	PyObject_GenericGetAttr,		
	0,					
	0,					
	Py_TPFLAGS_DEFAULT,			
 	0,					
 	(traverseproc)calliter_traverse,
 	0,					
	0,					
	0,					
	(getiterfunc)iter_getiter,		
	(iternextfunc)calliter_iternext,	
	calliter_methods,			
	0,					
	0,					
	0,					
	0,					
	0,					
	0,					
};
