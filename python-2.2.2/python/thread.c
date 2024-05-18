//20180114
#include "python.h"
#include <stdio.h>
#include <stdlib.h>
#include "pythread.h"

#ifdef _DEBUG
static int thread_debug = 0;
#define dprintf(args)	((thread_debug & 1) && printf args)
#define d2printf(args)	((thread_debug & 8) && printf args)
#else
#define dprintf(args)
#define d2printf(args)
#endif

static int initialized;

static void PyThread__init_thread();

void PyThread_init_thread()
{
#ifdef _DEBUG
	char *p = getenv("THREADDEBUG");

	if (p) 
	{
		if (*p)
		{
			thread_debug = atoi(p);
		}
		else
		{
			thread_debug = 1;
		}
	}
#endif
	if (initialized)
	{
		return;
	}
	initialized = 1;
	dprintf(("PyThread_init_thread called\n"));
	PyThread__init_thread();
}




















static void PyThread__init_thread()
{

}

long PyThread_get_thread_ident()
{
	if (!initialized)
	{
		PyThread_init_thread();
	}

	return 12345;
}

int PyThread_acquire_lock(PyThread_type_lock aLock, int waitflag)
{
	int success;
	dprintf(("%ld: PyThread_acquire_lock(%p, %d) called\n", PyThread_get_thread_ident(),aLock, waitflag));
	success = 1;
	dprintf(("%ld: PyThread_acquire_lock(%p, %d) -> %d\n", PyThread_get_thread_ident(),aLock, waitflag, success));
	return success;
}

void PyThread_release_lock(PyThread_type_lock aLock)
{
	dprintf(("%ld: PyThread_release_lock(%p) called\n", PyThread_get_thread_ident(),aLock));
}

void PyThread_free_lock(PyThread_type_lock aLock)
{
	dprintf(("%ld: PyThread_free_lock(%p) called\n", PyThread_get_thread_ident(),aLock));
}

long PyThread_start_new_thread(void (*func)(void *), void *arg)
{
	return 23456;
}

static void do_PyThread_exit_thread(int no_cleanup)
{
	dprintf(("%ld: PyThread_exit_thread called\n", PyThread_get_thread_ident()));
	if (!initialized)
	{
		exit(0);
	}
}

void PyThread_exit_thread()
{
	do_PyThread_exit_thread(0);
}

PyThread_type_lock PyThread_allocate_lock()
{
	int aLock;

	dprintf(("PyThread_allocate_lock called\n"));
	if (!initialized)
	{
		PyThread_init_thread();
	}

	aLock = 3456;

	dprintf(("%ld: PyThread_allocate_lock() -> %p\n", PyThread_get_thread_ident(), aLock));

	return (PyThread_type_lock) aLock;
}
