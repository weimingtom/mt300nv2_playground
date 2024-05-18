//20180318
#pragma once

typedef PyObject *(*getter)(PyObject *, void *);
typedef int (*setter)(PyObject *, PyObject *, void *);

typedef struct PyGetSetDef {
	char *name;
	getter get;
	setter set;
	char *doc;
	void *closure;
} PyGetSetDef;

typedef PyObject *(*wrapperfunc)(PyObject *self, PyObject *args, void *wrapped);
typedef PyObject *(*wrapperfunc_kwds)(PyObject *self, PyObject *args, void *wrapped, PyObject *kwds);

struct wrapperbase {
	char *name;
	int offset;
	void *function;
	wrapperfunc wrapper;
	char *doc;
	int flags;
	PyObject *name_strobj;
};

#define PyWrapperFlag_KEYWORDS 1

#define PyDescr_COMMON PyObject_HEAD PyTypeObject *d_type; PyObject *d_name

typedef struct {
	PyDescr_COMMON;
} PyDescrObject;

typedef struct {
	PyDescr_COMMON;
	PyMethodDef *d_method;
} PyMethodDescrObject;

typedef struct {
	PyDescr_COMMON;
	struct PyMemberDef *d_member;
} PyMemberDescrObject;

typedef struct {
	PyDescr_COMMON;
	PyGetSetDef *d_getset;
} PyGetSetDescrObject;

typedef struct {
	PyDescr_COMMON;
	struct wrapperbase *d_base;
	void *d_wrapped;
} PyWrapperDescrObject;

extern PyTypeObject PyWrapperDescr_Type;
extern PyObject * PyDescr_NewMethod(PyTypeObject *, PyMethodDef *);
extern PyObject * PyDescr_NewMember(PyTypeObject *, struct PyMemberDef *);
extern PyObject * PyDescr_NewGetSet(PyTypeObject *, struct PyGetSetDef *);
extern PyObject * PyDescr_NewWrapper(PyTypeObject *, struct wrapperbase *, void *);
extern int PyDescr_IsData(PyObject *);
extern PyObject * PyDictProxy_New(PyObject *);
extern PyObject * PyWrapper_New(PyObject *, PyObject *);
extern PyTypeObject PyProperty_Type;

