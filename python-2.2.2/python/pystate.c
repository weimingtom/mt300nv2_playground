//20180123
#include "python.h"

#define ZAP(x) { \
	PyObject *tmp = (PyObject *)(x); \
	(x) = NULL; \
	Py_XDECREF(tmp); \
}

#include "pythread.h"
static PyThread_type_lock head_mutex = NULL;
#define HEAD_INIT() (head_mutex || (head_mutex = PyThread_allocate_lock()))
#define HEAD_LOCK() PyThread_acquire_lock(head_mutex, WAIT_LOCK)
#define HEAD_UNLOCK() PyThread_release_lock(head_mutex)

static PyInterpreterState *interp_head = NULL;

PyThreadState *_PyThreadState_Current = NULL;
unaryfunc _PyThreadState_GetFrame = NULL;

PyInterpreterState *PyInterpreterState_New()
{
	PyInterpreterState *interp = PyMem_NEW(PyInterpreterState, 1);

	if (interp != NULL) 
	{
		HEAD_INIT();
		interp->modules = NULL;
		interp->sysdict = NULL;
		interp->builtins = NULL;
		interp->checkinterval = 10;
		interp->tstate_head = NULL;
		HEAD_LOCK();
		interp->next = interp_head;
		interp_head = interp;
		HEAD_UNLOCK();
	}

	return interp;
}

void PyInterpreterState_Clear(PyInterpreterState *interp)
{
	PyThreadState *p;
	HEAD_LOCK();
	for (p = interp->tstate_head; p != NULL; p = p->next)
	{
		PyThreadState_Clear(p);
	}
	HEAD_UNLOCK();
	ZAP(interp->modules);
	ZAP(interp->sysdict);
	ZAP(interp->builtins);
}

static void zapthreads(PyInterpreterState *interp)
{
	PyThreadState *p;
	while ((p = interp->tstate_head) != NULL) 
	{
		PyThreadState_Delete(p);
	}
}

void PyInterpreterState_Delete(PyInterpreterState *interp)
{
	PyInterpreterState **p;
	zapthreads(interp);
	HEAD_LOCK();
	for (p = &interp_head; ; p = &(*p)->next) 
	{
		if (*p == NULL)
		{
			Py_FatalError(
				"PyInterpreterState_Delete: invalid interp");
		}
		if (*p == interp)
		{
			break;
		}
	}
	if (interp->tstate_head != NULL)
	{
		Py_FatalError("PyInterpreterState_Delete: remaining threads");
	}
	*p = interp->next;
	HEAD_UNLOCK();
	PyMem_DEL(interp);
}

static struct _frame *threadstate_getframe(PyThreadState *self)
{
	return self->frame;
}

PyThreadState *PyThreadState_New(PyInterpreterState *interp)
{
	PyThreadState *tstate = PyMem_NEW(PyThreadState, 1);
	if (_PyThreadState_GetFrame == NULL)
	{
		_PyThreadState_GetFrame = (unaryfunc)threadstate_getframe;
	}

	if (tstate != NULL) 
	{
		tstate->interp = interp;

		tstate->frame = NULL;
		tstate->recursion_depth = 0;
		tstate->ticker = 0;
		tstate->tracing = 0;
		tstate->use_tracing = 0;
		tstate->tick_counter = 0;

		tstate->dict = NULL;

		tstate->curexc_type = NULL;
		tstate->curexc_value = NULL;
		tstate->curexc_traceback = NULL;

		tstate->exc_type = NULL;
		tstate->exc_value = NULL;
		tstate->exc_traceback = NULL;

		tstate->c_profilefunc = NULL;
		tstate->c_tracefunc = NULL;
		tstate->c_profileobj = NULL;
		tstate->c_traceobj = NULL;

		HEAD_LOCK();
		tstate->next = interp->tstate_head;
		interp->tstate_head = tstate;
		HEAD_UNLOCK();
	}

	return tstate;
}

void PyThreadState_Clear(PyThreadState *tstate)
{
	if (Py_VerboseFlag && tstate->frame != NULL)
	{
		fprintf(stderr,
		  "PyThreadState_Clear: warning: thread still has a frame\n");
	}
	ZAP(tstate->frame);

	ZAP(tstate->dict);

	ZAP(tstate->curexc_type);
	ZAP(tstate->curexc_value);
	ZAP(tstate->curexc_traceback);

	ZAP(tstate->exc_type);
	ZAP(tstate->exc_value);
	ZAP(tstate->exc_traceback);

	tstate->c_profilefunc = NULL;
	tstate->c_tracefunc = NULL;
	ZAP(tstate->c_profileobj);
	ZAP(tstate->c_traceobj);
}

static void tstate_delete_common(PyThreadState *tstate)
{
	PyInterpreterState *interp;
	PyThreadState **p;
	if (tstate == NULL)
	{
		Py_FatalError("PyThreadState_Delete: NULL tstate");
	}
	interp = tstate->interp;
	if (interp == NULL)
	{
		Py_FatalError("PyThreadState_Delete: NULL interp");
	}
	HEAD_LOCK();
	for (p = &interp->tstate_head; ; p = &(*p)->next) 
	{
		if (*p == NULL)
		{
			Py_FatalError(
				"PyThreadState_Delete: invalid tstate");
		}
		if (*p == tstate)
		{
			break;
		}
	}
	*p = tstate->next;
	HEAD_UNLOCK();
	PyMem_DEL(tstate);
}

void PyThreadState_Delete(PyThreadState *tstate)
{
	if (tstate == _PyThreadState_Current)
	{
		Py_FatalError("PyThreadState_Delete: tstate is still current");
	}
	tstate_delete_common(tstate);
}

void PyThreadState_DeleteCurrent()
{
	PyThreadState *tstate = _PyThreadState_Current;
	if (tstate == NULL)
	{
		Py_FatalError(
			"PyThreadState_DeleteCurrent: no current tstate");
	}
	_PyThreadState_Current = NULL;
	tstate_delete_common(tstate);
	PyEval_ReleaseLock();
}

PyThreadState *PyThreadState_Get()
{
	if (_PyThreadState_Current == NULL)
	{
		Py_FatalError("PyThreadState_Get: no current thread");
	}
	return _PyThreadState_Current;
}

PyThreadState *PyThreadState_Swap(PyThreadState *new)
{
	PyThreadState *old = _PyThreadState_Current;

	_PyThreadState_Current = new;

	return old;
}

PyObject *PyThreadState_GetDict()
{
	if (_PyThreadState_Current == NULL)
	{
		Py_FatalError("PyThreadState_GetDict: no current thread");
	}

	if (_PyThreadState_Current->dict == NULL)
	{
		_PyThreadState_Current->dict = PyDict_New();
	}
	return _PyThreadState_Current->dict;
}

PyInterpreterState *PyInterpreterState_Head()
{
	return interp_head;
}

PyInterpreterState *PyInterpreterState_Next(PyInterpreterState *interp) 
{
	return interp->next;
}

PyThreadState *PyInterpreterState_ThreadHead(PyInterpreterState *interp) 
{
	return interp->tstate_head;
}

PyThreadState *PyThreadState_Next(PyThreadState *tstate) 
{
	return tstate->next;
}
