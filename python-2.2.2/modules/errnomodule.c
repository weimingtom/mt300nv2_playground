//20180406

#include "python.h"


static PyMethodDef errno_methods[] = {
	{NULL,	      	NULL}
};

static void _inscode(PyObject *d, PyObject *de, char *name, int code)
{
	PyObject *u = PyString_FromString(name);
	PyObject *v = PyInt_FromLong((long) code);

	if (u && v) 
	{
		PyDict_SetItem(d, u, v);
		PyDict_SetItem(de, v, u);
	}
	Py_XDECREF(u);
	Py_XDECREF(v);
}

static char errno__doc__ [] =
	"This module makes available standard errno system symbols.\n"
	"\n"
	"The value of each symbol is the corresponding integer value,\n"
	"e.g., on most systems, errno.ENOENT equals the integer 2.\n"
	"\n"
	"The dictionary errno.errorcode maps numeric codes to symbol names,\n"
	"e.g., errno.errorcode[2] could be the string 'ENOENT'.\n"
	"\n"
	"Symbols that are not relevant to the underlying system are not defined.\n"
	"\n"
	"To map error codes to error messages, use the function os.strerror(),\n"
	"e.g. os.strerror(2) could return 'No such file or directory'.";

void initerrno()
{
	PyObject *m, *d, *de;
	m = Py_InitModule3("errno", errno_methods, errno__doc__);
	d = PyModule_GetDict(m);
	de = PyDict_New();
	if (!d || !de || PyDict_SetItemString(d, "errorcode", de) < 0)
	{
		return;
	}

#define inscode(d, ds, de, name, code, comment) _inscode(d, de, name, code)

	inscode(d, ds, de, "ENODEV", ENODEV, "No such device");
	inscode(d, ds, de, "ENOSYS", ENOSYS, "Function not implemented");
	inscode(d, ds, de, "EPIPE", EPIPE, "Broken pipe");
	inscode(d, ds, de, "EINVAL", EINVAL, "Invalid argument");
	inscode(d, ds, de, "EINTR", EINTR, "Interrupted system call");
	inscode(d, ds, de, "ENOTEMPTY", ENOTEMPTY, "Directory not empty");
	inscode(d, ds, de, "ECHILD", ECHILD, "No child processes");
	inscode(d, ds, de, "EXDEV", EXDEV, "Cross-device link");
	inscode(d, ds, de, "E2BIG", E2BIG, "Arg list too long");
	inscode(d, ds, de, "ESRCH", ESRCH, "No such process");
	inscode(d, ds, de, "EBUSY", EBUSY, "Device or resource busy");
	inscode(d, ds, de, "EBADF", EBADF, "Bad file number");
	inscode(d, ds, de, "EIO", EIO, "I/O error");
	inscode(d, ds, de, "ENOSPC", ENOSPC, "No space left on device");
	inscode(d, ds, de, "ENOEXEC", ENOEXEC, "Exec format error");
	inscode(d, ds, de, "EACCES", EACCES, "Permission denied");
	inscode(d, ds, de, "EILSEQ", EILSEQ, "Illegal byte sequence");
	inscode(d, ds, de, "ENOTDIR", ENOTDIR, "Not a directory");
	inscode(d, ds, de, "EPERM", EPERM, "Operation not permitted");
	inscode(d, ds, de, "EDOM", EDOM, "Math argument out of domain of func");
	inscode(d, ds, de, "EISDIR", EISDIR, "Is a directory");
	inscode(d, ds, de, "EROFS", EROFS, "Read-only file system");
	inscode(d, ds, de, "ENFILE", ENFILE, "File table overflow");
	inscode(d, ds, de, "ESPIPE", ESPIPE, "Illegal seek");
	inscode(d, ds, de, "ENOENT", ENOENT, "No such file or directory");
	inscode(d, ds, de, "EEXIST", EEXIST, "File exists");
	inscode(d, ds, de, "EFAULT", EFAULT, "Bad address");
	inscode(d, ds, de, "EFBIG", EFBIG, "File too large");
	inscode(d, ds, de, "EDEADLK", EDEADLK, "Resource deadlock would occur");
	inscode(d, ds, de, "ENOLCK", ENOLCK, "No record locks available");
	inscode(d, ds, de, "ENOMEM", ENOMEM, "Out of memory");
	inscode(d, ds, de, "EMLINK", EMLINK, "Too many links");
	inscode(d, ds, de, "ERANGE", ERANGE, "Math result not representable");
	inscode(d, ds, de, "EAGAIN", EAGAIN, "Try again");
	inscode(d, ds, de, "ENAMETOOLONG", ENAMETOOLONG, "File name too long");
	inscode(d, ds, de, "ENOTTY", ENOTTY, "Not a typewriter");
	inscode(d, ds, de, "EDEADLOCK", EDEADLOCK, "Error EDEADLOCK");
	inscode(d, ds, de, "EMFILE", EMFILE, "Too many open files");
	inscode(d, ds, de, "ENXIO", ENXIO, "No such device or address");
	Py_DECREF(de);
}
