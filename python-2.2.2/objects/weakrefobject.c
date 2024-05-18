//20170427
#include "python.h"
#include "structmember.h"

#define GET_WEAKREFS_LISTPTR(o) \
        ((PyWeakReference **) PyObject_GET_WEAKREFS_LISTPTR(o))

static PyWeakReference *
free_list = NULL;

long _PyWeakref_GetWeakrefCount(PyWeakReference *head)
{
    long count = 0;

    while (head != NULL) 
	{
        ++count;
        head = head->wr_next;
    }
    return count;
}

static PyWeakReference *new_weakref()
{
    PyWeakReference *result;

    if (free_list != NULL) 
	{
        result = free_list;
        free_list = result->wr_next;
        result->ob_type = &_PyWeakref_RefType;
        _Py_NewReference((PyObject *)result);
    }
    else 
	{
        result = PyObject_GC_New(PyWeakReference, &_PyWeakref_RefType);
    }
    if (result)
	{
		result->hash = -1;
	}
	return result;
}

static void clear_weakref(PyWeakReference *self)
{
    PyObject *callback = self->wr_callback;

    if (PyWeakref_GET_OBJECT(self) != Py_None) 
	{
        PyWeakReference **list = GET_WEAKREFS_LISTPTR(
            PyWeakref_GET_OBJECT(self));

        if (*list == self)
		{
            *list = self->wr_next;
        }
		self->wr_object = Py_None;
        self->wr_callback = NULL;
        if (self->wr_prev != NULL)
        {
			self->wr_prev->wr_next = self->wr_next;
        }
		if (self->wr_next != NULL)
        {
			self->wr_next->wr_prev = self->wr_prev;
        }
		self->wr_prev = NULL;
        self->wr_next = NULL;
        Py_XDECREF(callback);
    }
}

static void weakref_dealloc(PyWeakReference *self)
{
    PyObject_GC_UnTrack((PyObject *)self);
    clear_weakref(self);
    self->wr_next = free_list;
    free_list = self;
}

static int gc_traverse(PyWeakReference *self, visitproc visit, void *arg)
{
    if (self->wr_callback != NULL)
	{
        return visit(self->wr_callback, arg);
    }
	return 0;
}


static int gc_clear(PyWeakReference *self)
{
    clear_weakref(self);
    return 0;
}

static PyObject *weakref_call(PyWeakReference *self, PyObject *args, PyObject *kw)
{
    static char *argnames[] = {NULL};

    if (PyArg_ParseTupleAndKeywords(args, kw, ":__call__", argnames)) 
	{
        PyObject *object = PyWeakref_GET_OBJECT(self);
        Py_INCREF(object);
        return (object);
    }
    return NULL;
}

static long weakref_hash(PyWeakReference *self)
{
    if (self->hash != -1)
	{
        return self->hash;
    }
	if (PyWeakref_GET_OBJECT(self) == Py_None) 
	{
        PyErr_SetString(PyExc_TypeError, "weak object has gone away");
        return -1;
    }
    self->hash = PyObject_Hash(PyWeakref_GET_OBJECT(self));
    return self->hash;
}

static PyObject *weakref_repr(PyWeakReference *self)
{
    char buffer[256];
    if (PyWeakref_GET_OBJECT(self) == Py_None) 
	{
        PyOS_snprintf(buffer, sizeof(buffer), "<weakref at %lx; dead>",
		      (long)(self));
    }
    else 
	{
        PyOS_snprintf(buffer, sizeof(buffer),
		      "<weakref at %#lx; to '%.50s' at %#lx>",
		      (long)(self),
		      PyWeakref_GET_OBJECT(self)->ob_type->tp_name,
		      (long)(PyWeakref_GET_OBJECT(self)));
    }
    return PyString_FromString(buffer);
}

static PyObject *weakref_richcompare(PyWeakReference* self, PyWeakReference* other, int op)
{
    if (op != Py_EQ || self->ob_type != other->ob_type) 
	{
        Py_INCREF(Py_NotImplemented);
        return Py_NotImplemented;
    }
    if (PyWeakref_GET_OBJECT(self) == Py_None
        || PyWeakref_GET_OBJECT(other) == Py_None) 
	{
        PyObject *res = self==other ? Py_True : Py_False;
        Py_INCREF(res);
        return res;
    }
    return PyObject_RichCompare(PyWeakref_GET_OBJECT(self),
                                PyWeakref_GET_OBJECT(other), op);
}

PyTypeObject _PyWeakref_RefType = {
    PyObject_HEAD_INIT(&PyType_Type)
    0,
    "weakref",
    sizeof(PyWeakReference),
    0,
    (destructor)weakref_dealloc,
    0,	                    
    0,                         
    0,                         
    0,	                       
    (reprfunc)weakref_repr,    
    0,                         
    0,                         
    0,                         
    (hashfunc)weakref_hash,    
    (ternaryfunc)weakref_call, 
    0,                         
    0,                         
    0,                         
    0,                         
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_HAVE_RICHCOMPARE,
    0,                         
    (traverseproc)gc_traverse, 
    (inquiry)gc_clear,         
    (richcmpfunc)weakref_richcompare,
    0,			
};


static int proxy_checkref(PyWeakReference *proxy)
{
    if (PyWeakref_GET_OBJECT(proxy) == Py_None) 
	{
        PyErr_SetString(PyExc_ReferenceError,
                        "weakly-referenced object no longer exists");
        return 0;
    }
    return 1;
}

#define UNWRAP(o) \
        if (PyWeakref_CheckProxy(o)) { \
            if (!proxy_checkref((PyWeakReference *)o)) \
                return NULL; \
            o = PyWeakref_GET_OBJECT(o); \
        }

#define UNWRAP_I(o) \
        if (PyWeakref_CheckProxy(o)) { \
            if (!proxy_checkref((PyWeakReference *)o)) \
                return -1; \
            o = PyWeakref_GET_OBJECT(o); \
        }

#define WRAP_UNARY(method, generic) \
    static PyObject * \
    method(PyObject *proxy) { \
        UNWRAP(proxy); \
        return generic(proxy); \
    }

#define WRAP_BINARY(method, generic) \
    static PyObject * \
    method(PyObject *x, PyObject *y) { \
        UNWRAP(x); \
        UNWRAP(y); \
        return generic(x, y); \
    }

#define WRAP_TERNARY(method, generic) \
    static PyObject * \
    method(PyObject *proxy, PyObject *v, PyObject *w) { \
        UNWRAP(proxy); \
        UNWRAP(v); \
        if (w != NULL) \
            UNWRAP(w); \
        return generic(proxy, v, w); \
    }

WRAP_BINARY(proxy_getattr, PyObject_GetAttr)
WRAP_UNARY(proxy_str, PyObject_Str)
WRAP_TERNARY(proxy_call, PyEval_CallObjectWithKeywords)

static int proxy_print(PyWeakReference *proxy, FILE *fp, int flags)
{
    if (!proxy_checkref(proxy))
    {
		return -1;
    }
	return PyObject_Print(PyWeakref_GET_OBJECT(proxy), fp, flags);
}

static PyObject *proxy_repr(PyWeakReference *proxy)
{
    char buf[160];
    PyOS_snprintf(buf, sizeof(buf),
		  "<weakref at %p to %.100s at %p>", proxy,
		  PyWeakref_GET_OBJECT(proxy)->ob_type->tp_name,
		  PyWeakref_GET_OBJECT(proxy));
    return PyString_FromString(buf);
}

static int proxy_setattr(PyWeakReference *proxy, PyObject *name, PyObject *value)
{
    if (!proxy_checkref(proxy))
    {
		return -1;
    }
	return PyObject_SetAttr(PyWeakref_GET_OBJECT(proxy), name, value);
}

static int proxy_compare(PyObject *proxy, PyObject *v)
{
    UNWRAP_I(proxy);
    UNWRAP_I(v);
    return PyObject_Compare(proxy, v);
}

WRAP_BINARY(proxy_add, PyNumber_Add)
WRAP_BINARY(proxy_sub, PyNumber_Subtract)
WRAP_BINARY(proxy_mul, PyNumber_Multiply)
WRAP_BINARY(proxy_div, PyNumber_Divide)
WRAP_BINARY(proxy_mod, PyNumber_Remainder)
WRAP_BINARY(proxy_divmod, PyNumber_Divmod)
WRAP_TERNARY(proxy_pow, PyNumber_Power)
WRAP_UNARY(proxy_neg, PyNumber_Negative)
WRAP_UNARY(proxy_pos, PyNumber_Positive)
WRAP_UNARY(proxy_abs, PyNumber_Absolute)
WRAP_UNARY(proxy_invert, PyNumber_Invert)
WRAP_BINARY(proxy_lshift, PyNumber_Lshift)
WRAP_BINARY(proxy_rshift, PyNumber_Rshift)
WRAP_BINARY(proxy_and, PyNumber_And)
WRAP_BINARY(proxy_xor, PyNumber_Xor)
WRAP_BINARY(proxy_or, PyNumber_Or)
WRAP_UNARY(proxy_int, PyNumber_Int)
WRAP_UNARY(proxy_long, PyNumber_Long)
WRAP_UNARY(proxy_float, PyNumber_Float)
WRAP_BINARY(proxy_iadd, PyNumber_InPlaceAdd)
WRAP_BINARY(proxy_isub, PyNumber_InPlaceSubtract)
WRAP_BINARY(proxy_imul, PyNumber_InPlaceMultiply)
WRAP_BINARY(proxy_idiv, PyNumber_InPlaceDivide)
WRAP_BINARY(proxy_imod, PyNumber_InPlaceRemainder)
WRAP_TERNARY(proxy_ipow, PyNumber_InPlacePower)
WRAP_BINARY(proxy_ilshift, PyNumber_InPlaceLshift)
WRAP_BINARY(proxy_irshift, PyNumber_InPlaceRshift)
WRAP_BINARY(proxy_iand, PyNumber_InPlaceAnd)
WRAP_BINARY(proxy_ixor, PyNumber_InPlaceXor)
WRAP_BINARY(proxy_ior, PyNumber_InPlaceOr)

static int proxy_nonzero(PyWeakReference *proxy)
{
    PyObject *o = PyWeakref_GET_OBJECT(proxy);
    if (!proxy_checkref(proxy))
	{
		return 1;
    }
	if (o->ob_type->tp_as_number &&
        o->ob_type->tp_as_number->nb_nonzero)
	{
        return (*o->ob_type->tp_as_number->nb_nonzero)(o);
    }
	else
    {
		return 1;
	}
}

static PyObject *proxy_slice(PyWeakReference *proxy, int i, int j)
{
    if (!proxy_checkref(proxy))
    {
		return NULL;
    }
	return PySequence_GetSlice(PyWeakref_GET_OBJECT(proxy), i, j);
}

static int proxy_ass_slice(PyWeakReference *proxy, int i, int j, PyObject *value)
{
    if (!proxy_checkref(proxy))
    {
		return -1;
    }
	return PySequence_SetSlice(PyWeakref_GET_OBJECT(proxy), i, j, value);
}

static int proxy_contains(PyWeakReference *proxy, PyObject *value)
{
    if (!proxy_checkref(proxy))
    {
		return -1;
    }
	return PySequence_Contains(PyWeakref_GET_OBJECT(proxy), value);
}

static int proxy_length(PyWeakReference *proxy)
{
    if (!proxy_checkref(proxy))
	{
		return -1;
    }
	return PyObject_Length(PyWeakref_GET_OBJECT(proxy));
}

WRAP_BINARY(proxy_getitem, PyObject_GetItem)

static int proxy_setitem(PyWeakReference *proxy, PyObject *key, PyObject *value)
{
    if (!proxy_checkref(proxy))
	{
		return -1;
    }
	return PyObject_SetItem(PyWeakref_GET_OBJECT(proxy), key, value);
}

static PyNumberMethods proxy_as_number = {
    (binaryfunc)proxy_add,      
    (binaryfunc)proxy_sub,      
    (binaryfunc)proxy_mul,      
    (binaryfunc)proxy_div,      
    (binaryfunc)proxy_mod,      
    (binaryfunc)proxy_divmod,   
    (ternaryfunc)proxy_pow,     
    (unaryfunc)proxy_neg,       
    (unaryfunc)proxy_pos,       
    (unaryfunc)proxy_abs,       
    (inquiry)proxy_nonzero,     
    (unaryfunc)proxy_invert,    
    (binaryfunc)proxy_lshift,   
    (binaryfunc)proxy_rshift,   
    (binaryfunc)proxy_and,      
    (binaryfunc)proxy_xor,      
    (binaryfunc)proxy_or,       
    (coercion)0,                
    (unaryfunc)proxy_int,       
    (unaryfunc)proxy_long,      
    (unaryfunc)proxy_float,     
    (unaryfunc)0,               
    (unaryfunc)0,               
    (binaryfunc)proxy_iadd,     
    (binaryfunc)proxy_isub,     
    (binaryfunc)proxy_imul,     
    (binaryfunc)proxy_idiv,     
    (binaryfunc)proxy_imod,     
    (ternaryfunc)proxy_ipow,    
    (binaryfunc)proxy_ilshift,  
    (binaryfunc)proxy_irshift,  
    (binaryfunc)proxy_iand,     
    (binaryfunc)proxy_ixor,     
    (binaryfunc)proxy_ior,      
};

static PySequenceMethods proxy_as_sequence = {
    (inquiry)proxy_length,      
    0,                          
    0,                          
    0,                          
    (intintargfunc)proxy_slice, 
    0,                          
    (intintobjargproc)proxy_ass_slice,
    (objobjproc)proxy_contains, 
};

static PyMappingMethods proxy_as_mapping = {
    (inquiry)proxy_length,      
    (binaryfunc)proxy_getitem,  
    (objobjargproc)proxy_setitem, 
};

PyTypeObject _PyWeakref_ProxyType = {
    PyObject_HEAD_INIT(&PyType_Type)
    0,
    "weakproxy",
    sizeof(PyWeakReference),
    0,
    (destructor)weakref_dealloc,
    (printfunc)proxy_print,     
    0,				
    0, 				
    proxy_compare,		
    (unaryfunc)proxy_repr,	
    &proxy_as_number,		
    &proxy_as_sequence,		
    &proxy_as_mapping,		
    0,	                    
    (ternaryfunc)0,	        
    (unaryfunc)proxy_str,	
    (getattrofunc)proxy_getattr,
    (setattrofunc)proxy_setattr,
    0,				
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC
    |Py_TPFLAGS_CHECKTYPES,   
    0,                        
    (traverseproc)gc_traverse,
    (inquiry)gc_clear,      
};

PyTypeObject _PyWeakref_CallableProxyType = {
    PyObject_HEAD_INIT(&PyType_Type)
    0,
    "weakcallableproxy",
    sizeof(PyWeakReference),
    0,
    (destructor)weakref_dealloc,
    (printfunc)proxy_print,   
    0,				
    0, 				
    proxy_compare,		
    (unaryfunc)proxy_repr,	
    &proxy_as_number,		
    &proxy_as_sequence,		
    &proxy_as_mapping,		
    0,	                    
    (ternaryfunc)proxy_call,
    (unaryfunc)proxy_str,	
    (getattrofunc)proxy_getattr,
    (setattrofunc)proxy_setattr,
    0,	
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC
    |Py_TPFLAGS_CHECKTYPES,     
    0,                          
    (traverseproc)gc_traverse,  
    (inquiry)gc_clear,        
};

static void get_basic_refs(PyWeakReference *head,
               PyWeakReference **refp, PyWeakReference **proxyp)
{
    *refp = NULL;
    *proxyp = NULL;

    if (head != NULL && head->wr_callback == NULL) 
	{
        if (head->ob_type == &_PyWeakref_RefType) 
		{
            *refp = head;
            head = head->wr_next;
        }
        if (head != NULL && head->wr_callback == NULL) 
		{
            *proxyp = head;
            head = head->wr_next;
        }
    }
}

static void insert_after(PyWeakReference *newref, PyWeakReference *prev)
{
    newref->wr_prev = prev;
    newref->wr_next = prev->wr_next;
    if (prev->wr_next != NULL)
    {
		prev->wr_next->wr_prev = newref;
    }
	prev->wr_next = newref;
}

static void insert_head(PyWeakReference *newref, PyWeakReference **list)
{
    PyWeakReference *next = *list;

    newref->wr_prev = NULL;
    newref->wr_next = next;
    if (next != NULL)
	{
		next->wr_prev = newref;
	}
	*list = newref;
}

PyObject *PyWeakref_NewRef(PyObject *ob, PyObject *callback)
{
    PyWeakReference *result = NULL;
    PyWeakReference **list;
    PyWeakReference *ref, *proxy;

    if (!PyType_SUPPORTS_WEAKREFS(ob->ob_type)) 
	{
        PyErr_Format(PyExc_TypeError,
		     "cannot create weak reference to '%s' object",
                     ob->ob_type->tp_name);
        return NULL;
    }
    list = GET_WEAKREFS_LISTPTR(ob);
    get_basic_refs(*list, &ref, &proxy);
    if (callback == NULL || callback == Py_None)
    {
		result = ref;
    }
	if (result != NULL)
    {
		Py_XINCREF(result);
    }
	else 
	{
        result = new_weakref();
        if (result != NULL) 
		{
            Py_XINCREF(callback);
            result->wr_callback = callback;
            result->wr_object = ob;
            if (callback == NULL) 
			{
                insert_head(result, list);
            }
            else 
			{
                PyWeakReference *prev = (proxy == NULL) ? ref : proxy;

                if (prev == NULL)
				{
					insert_head(result, list);
                }
				else
                {
					insert_after(result, prev);
				}
            }
            PyObject_GC_Track(result);
        }
    }
    return (PyObject *) result;
}

PyObject *PyWeakref_NewProxy(PyObject *ob, PyObject *callback)
{
    PyWeakReference *result = NULL;
    PyWeakReference **list;
    PyWeakReference *ref, *proxy;

    if (!PyType_SUPPORTS_WEAKREFS(ob->ob_type)) 
	{
        PyErr_Format(PyExc_TypeError,
		     "cannot create weak reference to '%s' object",
                     ob->ob_type->tp_name);
        return NULL;
    }
    list = GET_WEAKREFS_LISTPTR(ob);
    get_basic_refs(*list, &ref, &proxy);
    if (callback == NULL)
    {
		result = proxy;
    }
	if (result != NULL)
	{
		Py_XINCREF(result);
    }
	else 
	{
        result = new_weakref();
        if (result != NULL) 
		{
            PyWeakReference *prev;

            if (PyCallable_Check(ob))
			{
				result->ob_type = &_PyWeakref_CallableProxyType;
            }
			else
            {
				result->ob_type = &_PyWeakref_ProxyType;
            }
			result->wr_object = ob;
            Py_XINCREF(callback);
            result->wr_callback = callback;
            if (callback == NULL)
            {
				prev = ref;
            }
			else
            {
				prev = (proxy == NULL) ? ref : proxy;
			}

            if (prev == NULL)
            {
				insert_head(result, list);
            }
			else
            {
				insert_after(result, prev);
            }
			PyObject_GC_Track(result);
        }
    }
    return (PyObject *) result;
}

PyObject *PyWeakref_GetObject(PyObject *ref)
{
    if (ref == NULL || !PyWeakref_Check(ref)) 
	{
        PyErr_BadInternalCall();
        return NULL;
    }
    return PyWeakref_GET_OBJECT(ref);
}


static void handle_callback(PyWeakReference *ref, PyObject *callback)
{
    PyObject *cbresult = PyObject_CallFunction(callback, "O", ref);

    if (cbresult == NULL)
	{
		PyErr_WriteUnraisable(callback);
    }
	else
    {
		Py_DECREF(cbresult);
	}
}

void PyObject_ClearWeakRefs(PyObject *object)
{
    PyWeakReference **list;

    if (object == NULL
        || !PyType_SUPPORTS_WEAKREFS(object->ob_type)
        || object->ob_refcnt != 0) 
	{
        PyErr_BadInternalCall();
        return;
    }
    list = GET_WEAKREFS_LISTPTR(object);
    if (*list != NULL && (*list)->wr_callback == NULL) 
	{
        clear_weakref(*list);
        if (*list != NULL && (*list)->wr_callback == NULL)
		{
			clear_weakref(*list);
		}
    }
    if (*list != NULL) 
	{
        PyWeakReference *current = *list;
        int count = _PyWeakref_GetWeakrefCount(current);
        int restore_error = PyErr_Occurred() ? 1 : 0;
        PyObject *err_type, *err_value, *err_tb;

        if (restore_error)
        {
			PyErr_Fetch(&err_type, &err_value, &err_tb);
        }
		if (count == 1) 
		{
            PyObject *callback = current->wr_callback;

            current->wr_callback = NULL;
            clear_weakref(current);
            handle_callback(current, callback);
            Py_DECREF(callback);
        }
        else 
		{
            PyObject *tuple = PyTuple_New(count * 2);
            int i = 0;

            for (i = 0; i < count; ++i) 
			{
                PyWeakReference *next = current->wr_next;

                Py_INCREF(current);
                PyTuple_SET_ITEM(tuple, i * 2, (PyObject *) current);
                PyTuple_SET_ITEM(tuple, i * 2 + 1, current->wr_callback);
                current->wr_callback = NULL;
                clear_weakref(current);
                current = next;
            }
            for (i = 0; i < count; ++i) 
			{
                PyObject *current = PyTuple_GET_ITEM(tuple, i * 2);
                PyObject *callback = PyTuple_GET_ITEM(tuple, i * 2 + 1);

                handle_callback((PyWeakReference *)current, callback);
            }
            Py_DECREF(tuple);
        }
        if (restore_error)
		{
			PyErr_Restore(err_type, err_value, err_tb);
		}
    }
}
