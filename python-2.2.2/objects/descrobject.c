//20170413
#include "python.h"
#include "structmember.h"

static void descr_dealloc(PyDescrObject *descr)
{
	_PyObject_GC_UNTRACK(descr);
	Py_XDECREF(descr->d_type);
	Py_XDECREF(descr->d_name);
	PyObject_GC_Del(descr);
}

static char *descr_name(PyDescrObject *descr)
{
	if (descr->d_name != NULL && PyString_Check(descr->d_name))
	{
		return PyString_AS_STRING(descr->d_name);
	}
	else
	{
		return "?";
	}
}

static PyObject *descr_repr(PyDescrObject *descr, char *format)
{
	return PyString_FromFormat(format, descr_name(descr),
				   descr->d_type->tp_name);
}

static PyObject *method_repr(PyMethodDescrObject *descr)
{
	return descr_repr((PyDescrObject *)descr, 
			  "<method '%s' of '%s' objects>");
}

static PyObject *member_repr(PyMemberDescrObject *descr)
{
	return descr_repr((PyDescrObject *)descr, 
			  "<member '%s' of '%s' objects>");
}

static PyObject *getset_repr(PyGetSetDescrObject *descr)
{
	return descr_repr((PyDescrObject *)descr, 
			  "<attribute '%s' of '%s' objects>");
}

static PyObject *wrapper_repr(PyWrapperDescrObject *descr)
{
	return descr_repr((PyDescrObject *)descr, 
			  "<slot wrapper '%s' of '%s' objects>");
}

static int descr_check(PyDescrObject *descr, PyObject *obj, PyTypeObject *type,
	    PyObject **pres)
{
	if (obj == NULL || (obj == Py_None && type != Py_None->ob_type)) 
	{
		Py_INCREF(descr);
		*pres = (PyObject *)descr;
		return 1;
	}
	if (!PyObject_IsInstance(obj, (PyObject *)(descr->d_type))) 
	{
		PyErr_Format(PyExc_TypeError,
			     "descriptor '%s' for '%s' objects "
			     "doesn't apply to '%s' object",
			     descr_name((PyDescrObject *)descr),
			     descr->d_type->tp_name,
			     obj->ob_type->tp_name);
		*pres = NULL;
		return 1;
	}
	return 0;
}

static PyObject *method_get(PyMethodDescrObject *descr, PyObject *obj, PyTypeObject *type)
{
	PyObject *res;

	if (descr_check((PyDescrObject *)descr, obj, type, &res))
	{
		return res;
	}
	return PyCFunction_New(descr->d_method, obj);
}

static PyObject *member_get(PyMemberDescrObject *descr, PyObject *obj, PyTypeObject *type)
{
	PyObject *res;

	if (descr_check((PyDescrObject *)descr, obj, type, &res))
	{
		return res;
	}
	return PyMember_GetOne((char *)obj, descr->d_member);
}

static PyObject *getset_get(PyGetSetDescrObject *descr, PyObject *obj, PyTypeObject *type)
{
	PyObject *res;

	if (descr_check((PyDescrObject *)descr, obj, type, &res))
	{
		return res;
	}
	if (descr->d_getset->get != NULL)
	{
		return descr->d_getset->get(obj, descr->d_getset->closure);
	}
	PyErr_Format(PyExc_TypeError,
		     "attribute '%.300s' of '%.100s' objects is not readable",
		     descr_name((PyDescrObject *)descr),
		     descr->d_type->tp_name);
	return NULL;
}

static PyObject *wrapper_get(PyWrapperDescrObject *descr, PyObject *obj, PyTypeObject *type)
{
	PyObject *res;

	if (descr_check((PyDescrObject *)descr, obj, type, &res))
	{
		return res;
	}
	return PyWrapper_New((PyObject *)descr, obj);
}

static int descr_setcheck(PyDescrObject *descr, PyObject *obj, PyObject *value,
	       int *pres)
{
	assert(obj != NULL);
	if (!PyObject_IsInstance(obj, (PyObject *)(descr->d_type))) 
	{
		PyErr_Format(PyExc_TypeError,
			     "descriptor '%.200s' for '%.100s' objects "
			     "doesn't apply to '%.100s' object",
			     descr_name(descr),
			     descr->d_type->tp_name,
			     obj->ob_type->tp_name);
		*pres = -1;
		return 1;
	}
	return 0;
}

static int member_set(PyMemberDescrObject *descr, PyObject *obj, PyObject *value)
{
	int res;

	if (descr_setcheck((PyDescrObject *)descr, obj, value, &res))
	{
		return res;
	}
	return PyMember_SetOne((char *)obj, descr->d_member, value);
}

static int getset_set(PyGetSetDescrObject *descr, PyObject *obj, PyObject *value)
{
	int res;

	if (descr_setcheck((PyDescrObject *)descr, obj, value, &res))
	{
		return res;
	}
	if (descr->d_getset->set != NULL)
	{
		return descr->d_getset->set(obj, value,
					    descr->d_getset->closure);
	}
	PyErr_Format(PyExc_TypeError,
		     "attribute '%.300s' of '%.100s' objects is not writable",
		     descr_name((PyDescrObject *)descr),
		     descr->d_type->tp_name);
	return -1;
}

static PyObject *methoddescr_call(PyMethodDescrObject *descr, PyObject *args, PyObject *kwds)
{
	int argc;
	PyObject *self, *func, *result;

	assert(PyTuple_Check(args));
	argc = PyTuple_GET_SIZE(args);
	if (argc < 1) 
	{
		PyErr_Format(PyExc_TypeError,
			     "descriptor '%.300s' of '%.100s' "
			     "object needs an argument",
			     descr_name((PyDescrObject *)descr),
			     descr->d_type->tp_name);
		return NULL;
	}
	self = PyTuple_GET_ITEM(args, 0);
	if (!PyObject_IsInstance(self, (PyObject *)(descr->d_type))) 
	{
		PyErr_Format(PyExc_TypeError,
			     "descriptor '%.200s' "
			     "requires a '%.100s' object "
			     "but received a '%.100s'",
			     descr_name((PyDescrObject *)descr),
			     descr->d_type->tp_name,
			     self->ob_type->tp_name);
		return NULL;
	}

	func = PyCFunction_New(descr->d_method, self);
	if (func == NULL)
	{
		return NULL;
	}
	args = PyTuple_GetSlice(args, 1, argc);
	if (args == NULL) 
	{
		Py_DECREF(func);
		return NULL;
	}
	result = PyEval_CallObjectWithKeywords(func, args, kwds);
	Py_DECREF(args);
	Py_DECREF(func);
	return result;
}

static PyObject *wrapperdescr_call(PyWrapperDescrObject *descr, PyObject *args, PyObject *kwds)
{
	int argc;
	PyObject *self, *func, *result;

	assert(PyTuple_Check(args));
	argc = PyTuple_GET_SIZE(args);
	if (argc < 1) 
	{
		PyErr_Format(PyExc_TypeError,
			     "descriptor '%.300s' of '%.100s' "
			     "object needs an argument",
			     descr_name((PyDescrObject *)descr),
			     descr->d_type->tp_name);
		return NULL;
	}
	self = PyTuple_GET_ITEM(args, 0);
	if (!PyObject_IsInstance(self, (PyObject *)(descr->d_type))) 
	{
		PyErr_Format(PyExc_TypeError,
			     "descriptor '%.200s' "
			     "requires a '%.100s' object "
			     "but received a '%.100s'",
			     descr_name((PyDescrObject *)descr),
			     descr->d_type->tp_name,
			     self->ob_type->tp_name);
		return NULL;
	}

	func = PyWrapper_New((PyObject *)descr, self);
	if (func == NULL)
	{
		return NULL;
	}
	args = PyTuple_GetSlice(args, 1, argc);
	if (args == NULL) 
	{
		Py_DECREF(func);
		return NULL;
	}
	result = PyEval_CallObjectWithKeywords(func, args, kwds);
	Py_DECREF(args);
	Py_DECREF(func);
	return result;
}

static PyObject *method_get_doc(PyMethodDescrObject *descr, void *closure)
{
	if (descr->d_method->ml_doc == NULL) 
	{
		Py_INCREF(Py_None);
		return Py_None;
	}
	return PyString_FromString(descr->d_method->ml_doc);
}

static PyMemberDef descr_members[] = {
	{"__objclass__", T_OBJECT, offsetof(PyDescrObject, d_type), READONLY},
	{"__name__", T_OBJECT, offsetof(PyDescrObject, d_name), READONLY},
	{0}
};

static PyGetSetDef method_getset[] = {
	{"__doc__", (getter)method_get_doc},
	{0}
};

static PyObject *member_get_doc(PyMemberDescrObject *descr, void *closure)
{
	if (descr->d_member->doc == NULL) 
	{
		Py_INCREF(Py_None);
		return Py_None;
	}
	return PyString_FromString(descr->d_member->doc);
}

static PyGetSetDef member_getset[] = {
	{"__doc__", (getter)member_get_doc},
	{0}
};

static PyObject *getset_get_doc(PyGetSetDescrObject *descr, void *closure)
{
	if (descr->d_getset->doc == NULL) 
	{
		Py_INCREF(Py_None);
		return Py_None;
	}
	return PyString_FromString(descr->d_getset->doc);
}

static PyGetSetDef getset_getset[] = {
	{"__doc__", (getter)getset_get_doc},
	{0}
};

static PyObject *wrapper_get_doc(PyWrapperDescrObject *descr, void *closure)
{
	if (descr->d_base->doc == NULL) 
	{
		Py_INCREF(Py_None);
		return Py_None;
	}
	return PyString_FromString(descr->d_base->doc);
}

static PyGetSetDef wrapper_getset[] = {
	{"__doc__", (getter)wrapper_get_doc},
	{0}
};

static int descr_traverse(PyObject *self, visitproc visit, void *arg)
{
	PyDescrObject *descr = (PyDescrObject *)self;
	int err;

	if (descr->d_type) 
	{
		err = visit((PyObject *)(descr->d_type), arg);
		if (err)
		{
			return err;
		}
	}
	return 0;
}

static PyTypeObject PyMethodDescr_Type = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,
	"method_descriptor",
	sizeof(PyMethodDescrObject),
	0,
	(destructor)descr_dealloc,	
	0,					
	0,					
	0,					
	0,					
	(reprfunc)method_repr,	
	0,					
	0,					
	0,					
	0,					
	(ternaryfunc)methoddescr_call,		
	0,					
	PyObject_GenericGetAttr,		
	0,					
	0,					
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC, 
	0,					
	descr_traverse,				
	0,					
	0,					
	0,					
	0,					
	0,					
	0,					
	descr_members,				
	method_getset,				
	0,					
	0,					
	(descrgetfunc)method_get,		
	0,					
};

static PyTypeObject PyMemberDescr_Type = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,
	"member_descriptor",
	sizeof(PyMemberDescrObject),
	0,
	(destructor)descr_dealloc,	
	0,					
	0,					
	0,					
	0,					
	(reprfunc)member_repr,
	0,					
	0,					
	0,					
	0,					
	(ternaryfunc)0,		
	0,					
	PyObject_GenericGetAttr,
	0,					
	0,					
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC, 
	0,					
	descr_traverse,		
	0,					
	0,					
	0,					
	0,					
	0,					
	0,					
	descr_members,		
	member_getset,		
	0,					
	0,					
	(descrgetfunc)member_get,	
	(descrsetfunc)member_set,
};

static PyTypeObject PyGetSetDescr_Type = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,
	"getset_descriptor",
	sizeof(PyGetSetDescrObject),
	0,
	(destructor)descr_dealloc,	
	0,					
	0,					
	0,					
	0,					
	(reprfunc)getset_repr,
	0,					
	0,					
	0,					
	0,					
	(ternaryfunc)0,		
	0,					
	PyObject_GenericGetAttr,
	0,					
	0,					
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,
	0,					
	descr_traverse,		
	0,					
	0,					
	0,					
	0,					
	0,					
	0,					
	descr_members,		
	getset_getset,		
	0,					
	0,					
	(descrgetfunc)getset_get,
	(descrsetfunc)getset_set,
};

PyTypeObject PyWrapperDescr_Type = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,
	"wrapper_descriptor",
	sizeof(PyWrapperDescrObject),
	0,
	(destructor)descr_dealloc,	
	0,					
	0,					
	0,					
	0,					
	(reprfunc)wrapper_repr,
	0,					
	0,					
	0,					
	0,					
	(ternaryfunc)wrapperdescr_call,
	0,					
	PyObject_GenericGetAttr,
	0,					
	0,					
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC, 
	0,					
	descr_traverse,		
	0,					
	0,					
	0,					
	0,					
	0,					
	0,					
	descr_members,		
	wrapper_getset,		
	0,					
	0,					
	(descrgetfunc)wrapper_get,	
	0,				
};

static PyDescrObject *descr_new(PyTypeObject *descrtype, PyTypeObject *type, char *name)
{
	PyDescrObject *descr;

	descr = (PyDescrObject *)PyType_GenericAlloc(descrtype, 0);
	if (descr != NULL) 
	{
		Py_XINCREF(type);
		descr->d_type = type;
		descr->d_name = PyString_InternFromString(name);
		if (descr->d_name == NULL) 
		{
			Py_DECREF(descr);
			descr = NULL;
		}
	}
	return descr;
}

PyObject *PyDescr_NewMethod(PyTypeObject *type, PyMethodDef *method)
{
	PyMethodDescrObject *descr;

	descr = (PyMethodDescrObject *)descr_new(&PyMethodDescr_Type,
						 type, method->ml_name);
	if (descr != NULL)
	{
		descr->d_method = method;
	}
	return (PyObject *)descr;
}

PyObject *PyDescr_NewMember(PyTypeObject *type, PyMemberDef *member)
{
	PyMemberDescrObject *descr;

	descr = (PyMemberDescrObject *)descr_new(&PyMemberDescr_Type,
						 type, member->name);
	if (descr != NULL)
	{
		descr->d_member = member;
	}
	return (PyObject *)descr;
}

PyObject *PyDescr_NewGetSet(PyTypeObject *type, PyGetSetDef *getset)
{
	PyGetSetDescrObject *descr;

	descr = (PyGetSetDescrObject *)descr_new(&PyGetSetDescr_Type,
						 type, getset->name);
	if (descr != NULL)
	{
		descr->d_getset = getset;
	}
	return (PyObject *)descr;
}

PyObject *PyDescr_NewWrapper(PyTypeObject *type, struct wrapperbase *base, void *wrapped)
{
	PyWrapperDescrObject *descr;

	descr = (PyWrapperDescrObject *)descr_new(&PyWrapperDescr_Type,
						 type, base->name);
	if (descr != NULL) 
	{
		descr->d_base = base;
		descr->d_wrapped = wrapped;
	}
	return (PyObject *)descr;
}

int PyDescr_IsData(PyObject *d)
{
	return d->ob_type->tp_descr_set != NULL;
}

typedef struct {
	PyObject_HEAD
	PyObject *dict;
} proxyobject;

static int proxy_len(proxyobject *pp)
{
	return PyObject_Size(pp->dict);
}

static PyObject *proxy_getitem(proxyobject *pp, PyObject *key)
{
	return PyObject_GetItem(pp->dict, key);
}

static PyMappingMethods proxy_as_mapping = {
	(inquiry)proxy_len,			
	(binaryfunc)proxy_getitem,		
	0,		
};

static int proxy_contains(proxyobject *pp, PyObject *key)
{
	return PySequence_Contains(pp->dict, key);
}

static PySequenceMethods proxy_as_sequence = {
	0,				
	0,				
	0,				
	0,				
	0,				
	0,				
	0,				
	(objobjproc)proxy_contains,	
	0,		
	0,		
};

static PyObject *proxy_has_key(proxyobject *pp, PyObject *key)
{
	return PyInt_FromLong(PySequence_Contains(pp->dict, key));
}

static PyObject *proxy_get(proxyobject *pp, PyObject *args)
{
	PyObject *key, *def = Py_None;

	if (!PyArg_ParseTuple(args, "O|O:get", &key, &def))
	{
		return NULL;
	}
	return PyObject_CallMethod(pp->dict, "get", "(OO)", key, def);
}

static PyObject *proxy_keys(proxyobject *pp)
{
	return PyMapping_Keys(pp->dict);
}

static PyObject *proxy_values(proxyobject *pp)
{
	return PyMapping_Values(pp->dict);
}

static PyObject *proxy_items(proxyobject *pp)
{
	return PyMapping_Items(pp->dict);
}

static PyObject *proxy_copy(proxyobject *pp)
{
	return PyObject_CallMethod(pp->dict, "copy", NULL);
}

static PyMethodDef proxy_methods[] = {
	{"has_key", (PyCFunction)proxy_has_key, METH_O, "XXX"},
	{"get",	    (PyCFunction)proxy_get,     METH_VARARGS, "XXX"},
	{"keys",    (PyCFunction)proxy_keys,    METH_NOARGS, "XXX"},
	{"values",  (PyCFunction)proxy_values,  METH_NOARGS, "XXX"},
	{"items",   (PyCFunction)proxy_items,   METH_NOARGS, "XXX"},
	{"copy",    (PyCFunction)proxy_copy,    METH_NOARGS, "XXX"},
	{0}
};

static void proxy_dealloc(proxyobject *pp)
{
	_PyObject_GC_UNTRACK(pp);
	Py_DECREF(pp->dict);
	PyObject_GC_Del(pp);
}

static PyObject *proxy_getiter(proxyobject *pp)
{
	return PyObject_GetIter(pp->dict);
}

static PyObject *proxy_str(proxyobject *pp)
{
	return PyObject_Str(pp->dict);
}

static int proxy_traverse(PyObject *self, visitproc visit, void *arg)
{
	proxyobject *pp = (proxyobject *)self;
	int err;

	if (pp->dict) 
	{
		err = visit(pp->dict, arg);
		if (err)
		{
			return err;
		}
	}
	return 0;
}

static PyTypeObject proxytype = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,					
	"dict-proxy",			
	sizeof(proxyobject),	
	0,					
	(destructor)proxy_dealloc,
	0,					
	0,					
	0,					
	0,					
	0,					
	0,					
	&proxy_as_sequence,		
	&proxy_as_mapping,		
	0,					
	0,					
	(reprfunc)proxy_str,
	PyObject_GenericGetAttr,	
	0,					
	0,					
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC, 
 	0,					
	proxy_traverse,		
 	0,					
	0,					
	0,					
	(getiterfunc)proxy_getiter,	
	0,				
	proxy_methods,	
	0,				
	0,				
	0,				
	0,				
	0,				
	0,				
};

PyObject *PyDictProxy_New(PyObject *dict)
{
	proxyobject *pp;

	pp = PyObject_GC_New(proxyobject, &proxytype);
	if (pp != NULL) 
	{
		Py_INCREF(dict);
		pp->dict = dict;
		_PyObject_GC_TRACK(pp);
	}
	return (PyObject *)pp;
}

typedef struct {
	PyObject_HEAD
	PyWrapperDescrObject *descr;
	PyObject *self;
} wrapperobject;

static void wrapper_dealloc(wrapperobject *wp)
{
	_PyObject_GC_UNTRACK(wp);
	Py_XDECREF(wp->descr);
	Py_XDECREF(wp->self);
	PyObject_GC_Del(wp);
}

static PyMethodDef wrapper_methods[] = {
	{0}
};

static PyObject *
wrapper_name(wrapperobject *wp)
{
	char *s = wp->descr->d_base->name;

	return PyString_FromString(s);
}

static PyObject *wrapper_doc(wrapperobject *wp)
{
	char *s = wp->descr->d_base->doc;

	if (s == NULL) 
	{
		Py_INCREF(Py_None);
		return Py_None;
	}
	else 
	{
		return PyString_FromString(s);
	}
}

static PyGetSetDef wrapper_getsets[] = {
	{"__name__", (getter)wrapper_name},
	{"__doc__", (getter)wrapper_doc},
	{0}
};

static PyObject *wrapper_call(wrapperobject *wp, PyObject *args, PyObject *kwds)
{
	wrapperfunc wrapper = wp->descr->d_base->wrapper;
	PyObject *self = wp->self;

	if (wp->descr->d_base->flags & PyWrapperFlag_KEYWORDS) 
	{
		wrapperfunc_kwds wk = (wrapperfunc_kwds)wrapper;
		return (*wk)(self, args, wp->descr->d_wrapped, kwds);
	}

	if (kwds != NULL && (!PyDict_Check(kwds) || PyDict_Size(kwds) != 0)) 
	{
		PyErr_Format(PyExc_TypeError,
			     "wrapper %s doesn't take keyword arguments",
			     wp->descr->d_base->name);
		return NULL;
	}
	return (*wrapper)(self, args, wp->descr->d_wrapped);
}

static int wrapper_traverse(PyObject *self, visitproc visit, void *arg)
{
	wrapperobject *wp = (wrapperobject *)self;
	int err;

	if (wp->descr) 
	{
		err = visit((PyObject *)(wp->descr), arg);
		if (err)
		{
			return err;
		}
	}
	if (wp->self) 
	{
		err = visit(wp->self, arg);
		if (err)
		{
			return err;
		}
	}
	return 0;
}

static PyTypeObject wrappertype = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,
	"method-wrapper",
	sizeof(wrapperobject),
	0,
	(destructor)wrapper_dealloc, 
	0,					
	0,					
	0,					
	0,					
	0,					
	0,					
	0,					
	0,		       		
	0,					
	(ternaryfunc)wrapper_call,	
	0,					
	PyObject_GenericGetAttr,
	0,					
	0,					
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC, 
 	0,					
	wrapper_traverse,	
 	0,					
	0,					
	0,					
	0,					
	0,					
	wrapper_methods,	
	0,					
	wrapper_getsets,	
	0,					
	0,					
	0,					
	0,					
};

PyObject *PyWrapper_New(PyObject *d, PyObject *self)
{
	wrapperobject *wp;
	PyWrapperDescrObject *descr;

	assert(PyObject_TypeCheck(d, &PyWrapperDescr_Type));
	descr = (PyWrapperDescrObject *)d;
	assert(PyObject_IsInstance(self, (PyObject *)(descr->d_type)));

	wp = PyObject_GC_New(wrapperobject, &wrappertype);
	if (wp != NULL) 
	{
		Py_INCREF(descr);
		wp->descr = descr;
		Py_INCREF(self);
		wp->self = self;
		_PyObject_GC_TRACK(wp);
	}
	return (PyObject *)wp;
}

typedef struct {
	PyObject_HEAD
	PyObject *prop_get;
	PyObject *prop_set;
	PyObject *prop_del;
	PyObject *prop_doc;
} propertyobject;

static PyMemberDef property_members[] = {
	{"fget", T_OBJECT, offsetof(propertyobject, prop_get), READONLY},
	{"fset", T_OBJECT, offsetof(propertyobject, prop_set), READONLY},
	{"fdel", T_OBJECT, offsetof(propertyobject, prop_del), READONLY},
	{"__doc__",  T_OBJECT, offsetof(propertyobject, prop_doc), READONLY},
	{0}
};


static void property_dealloc(PyObject *self)
{
	propertyobject *gs = (propertyobject *)self;

	_PyObject_GC_UNTRACK(self);
	Py_XDECREF(gs->prop_get);
	Py_XDECREF(gs->prop_set);
	Py_XDECREF(gs->prop_del);
	Py_XDECREF(gs->prop_doc);
	self->ob_type->tp_free(self);
}

static PyObject *property_descr_get(PyObject *self, PyObject *obj, PyObject *type)
{
	propertyobject *gs = (propertyobject *)self;

	if (obj == NULL || obj == Py_None) 
	{
		Py_INCREF(self);
		return self;
	}
	if (gs->prop_get == NULL) 
	{
		PyErr_SetString(PyExc_AttributeError, "unreadable attribute");
		return NULL;
	}
	return PyObject_CallFunction(gs->prop_get, "(O)", obj);
}

static int property_descr_set(PyObject *self, PyObject *obj, PyObject *value)
{
	propertyobject *gs = (propertyobject *)self;
	PyObject *func, *res;

	if (value == NULL)
	{
		func = gs->prop_del;
	}
	else
	{
		func = gs->prop_set;
	}
	if (func == NULL) 
	{
		PyErr_SetString(PyExc_AttributeError,
				value == NULL ?
				"can't delete attribute" :
				"can't set attribute");
		return -1;
	}
	if (value == NULL)
	{
		res = PyObject_CallFunction(func, "(O)", obj);
	}
	else
	{
		res = PyObject_CallFunction(func, "(OO)", obj, value);
	}
	if (res == NULL)
	{
		return -1;
	}
	Py_DECREF(res);
	return 0;
}

static int property_init(PyObject *self, PyObject *args, PyObject *kwds)
{
	PyObject *get = NULL, *set = NULL, *del = NULL, *doc = NULL;
	static char *kwlist[] = {"fget", "fset", "fdel", "doc", 0};
	propertyobject *gs = (propertyobject *)self;

	if (!PyArg_ParseTupleAndKeywords(args, kwds, "|OOOO:property",
	     				 kwlist, &get, &set, &del, &doc))
	{
		return -1;
	}

	if (get == Py_None)
	{
		get = NULL;
	}
	if (set == Py_None)
	{
		set = NULL;
	}
	if (del == Py_None)
	{
		del = NULL;
	}

	Py_XINCREF(get);
	Py_XINCREF(set);
	Py_XINCREF(del);
	Py_XINCREF(doc);

	gs->prop_get = get;
	gs->prop_set = set;
	gs->prop_del = del;
	gs->prop_doc = doc;

	return 0;
}

static char property_doc[] =
	"property(fget=None, fset=None, fdel=None, doc=None) -> property attribute\n"
	"\n"
	"fget is a function to be used for getting an attribute value, and likewise\n"
	"fset is a function for setting, and fdel a function for del'ing, an\n"
	"attribute.  Typical use is to define a managed attribute x:\n"
	"class C(object):\n"
	"    def getx(self): return self.__x\n"
	"    def setx(self, value): self.__x = value\n"
	"    def delx(self): del self.__x\n"
	"    x = property(getx, setx, delx, \"I'm the 'x' property.\")";

static int property_traverse(PyObject *self, visitproc visit, void *arg)
{
	propertyobject *pp = (propertyobject *)self;
	int err;

#define VISIT(SLOT) \
	if (pp->SLOT) { \
		err = visit((PyObject *)(pp->SLOT), arg); \
		if (err) \
			return err; \
	}

	VISIT(prop_get);
	VISIT(prop_set);
	VISIT(prop_del);

	return 0;
}

PyTypeObject PyProperty_Type = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,				
	"property",		
	sizeof(propertyobject),	
	0,
	property_dealloc,
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
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC |
		Py_TPFLAGS_BASETYPE,	
 	property_doc,		
	property_traverse,	
 	0,					
	0,					
	0,					
	0,					
	0,					
	0,					
	property_members,	
	0,					
	0,					
	0,					
	property_descr_get,		
	property_descr_set,		
	0,					
	property_init,				
	PyType_GenericAlloc,		
	PyType_GenericNew,			
	_PyObject_GC_Del,			
};
