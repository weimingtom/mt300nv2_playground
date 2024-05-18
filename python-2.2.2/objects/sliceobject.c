//20170425
#include "python.h"
#include "structmember.h"

static PyObject *ellipsis_repr(PyObject *op)
{
	return PyString_FromString("Ellipsis");
}

static PyTypeObject PyEllipsis_Type = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,					
	"ellipsis",				
	0,					
	0,					
	0, 
	0,					
	0,					
	0,					
	0,					
	(reprfunc)ellipsis_repr,
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
};

PyObject _Py_EllipsisObject = {
	PyObject_HEAD_INIT(&PyEllipsis_Type)
};

PyObject *PySlice_New(PyObject *start, PyObject *stop, PyObject *step)
{
	PySliceObject *obj = PyObject_NEW(PySliceObject, &PySlice_Type);

	if (obj == NULL)
	{
		return NULL;
	}

	if (step == NULL) 
	{
		step = Py_None;
	}
	Py_INCREF(step);
	
	if (start == NULL) 
	{
		start = Py_None;
	}
	Py_INCREF(start);

	if (stop == NULL) 
	{
		stop = Py_None;
	}
	Py_INCREF(stop);

	obj->step = step;
	obj->start = start;
	obj->stop = stop;

	return (PyObject *) obj;
}

int PySlice_GetIndices(PySliceObject *r, int length,
                   int *start, int *stop, int *step)
{
	if (r->step == Py_None) 
	{
		*step = 1;
	} 
	else 
	{
		if (!PyInt_Check(r->step)) 
		{
			return -1;
		}
		*step = PyInt_AsLong(r->step);
	}
	if (r->start == Py_None) 
	{
		*start = *step < 0 ? length-1 : 0;
	} 
	else 
	{
		if (!PyInt_Check(r->start)) 
		{
			return -1;
		}
		*start = PyInt_AsLong(r->start);
		if (*start < 0) 
		{
			*start += length;
		}
	}
	if (r->stop == Py_None) 
	{
		*stop = *step < 0 ? -1 : length;
	} 
	else 
	{
		if (!PyInt_Check(r->stop)) 
		{
			return -1;
		}
		*stop = PyInt_AsLong(r->stop);
		if (*stop < 0) 
		{
			*stop += length;
		}
	}
	if (*stop > length) 
	{
		return -1;
	}
	if (*start >= length) 
	{
		return -1;
	}
	if (*step == 0) 
	{
		return -1;
	}
	return 0;
}

static void slice_dealloc(PySliceObject *r)
{
	Py_DECREF(r->step);
	Py_DECREF(r->start);
	Py_DECREF(r->stop);
	PyObject_DEL(r);
}

static PyObject *slice_repr(PySliceObject *r)
{
	PyObject *s, *comma;

	s = PyString_FromString("slice(");
	comma = PyString_FromString(", ");
	PyString_ConcatAndDel(&s, PyObject_Repr(r->start));
	PyString_Concat(&s, comma);
	PyString_ConcatAndDel(&s, PyObject_Repr(r->stop));
	PyString_Concat(&s, comma);
	PyString_ConcatAndDel(&s, PyObject_Repr(r->step));
	PyString_ConcatAndDel(&s, PyString_FromString(")"));
	Py_DECREF(comma);
	return s;
}

static PyMemberDef slice_members[] = {
	{"start", T_OBJECT, offsetof(PySliceObject, start), READONLY},
	{"stop", T_OBJECT, offsetof(PySliceObject, stop), READONLY},
	{"step", T_OBJECT, offsetof(PySliceObject, step), READONLY},
	{0}
};

static int slice_compare(PySliceObject *v, PySliceObject *w)
{
	int result = 0;

    if (v == w)
	{
		return 0;
	}

	if (PyObject_Cmp(v->start, w->start, &result) < 0)
	{
		return -2;
	}
	if (result != 0)
	{
		return result;
	}
	if (PyObject_Cmp(v->stop, w->stop, &result) < 0)
	{
		return -2;
	}
	if (result != 0)
	{
		return result;
	}
	if (PyObject_Cmp(v->step, w->step, &result) < 0)
	{
		return -2;
	}
	return result;
}

PyTypeObject PySlice_Type = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,			
	"slice",	
	sizeof(PySliceObject),	
	0,			
	(destructor)slice_dealloc,
	0,			
	0,			
	0,			
	(cmpfunc)slice_compare, 
	(reprfunc)slice_repr,   
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
	0,					
	0,					
	0,					
	0,					
	0,					
	0,					
	0,					
	slice_members,	
	0,					
	0,					
	0,					
};
