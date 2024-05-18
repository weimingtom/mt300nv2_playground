//20180125

#include "python.h"
#include "node.h"
#include "token.h"
#include "errcode.h"
#include "marshal.h"
#include "compile.h"
#include "eval.h"
#include "osdefs.h"
#include "importdl.h"

#include <fcntl.h>

#define CANT_WRITE(int_value)	(int_value > 32767)

extern time_t PyOS_GetLastModificationTime(char *, FILE *);

#define MAGIC (60717 | ((long)'\r'<<16) | ((long)'\n'<<24))

static long pyc_magic = MAGIC;
static PyObject *extensions = NULL;

extern struct _inittab _PyImport_Inittab[];

struct _inittab *PyImport_Inittab = _PyImport_Inittab;

struct filedescr * _PyImport_Filetab = NULL;

static const struct filedescr _PyImport_StandardFiletab[] = {
	{".py", "r", PY_SOURCE},
	{".pyw", "r", PY_SOURCE},
	{".pyc", "rb", PY_COMPILED},
	{0, 0}
};

void _PyImport_Init()
{
	const struct filedescr *scan;
	struct filedescr *filetab;
	int countD = 0;
	int countS = 0;

	for (scan = _PyImport_DynLoadFiletab; scan->suffix != NULL; ++scan)
	{
		++countD;
	}
	for (scan = _PyImport_StandardFiletab; scan->suffix != NULL; ++scan)
	{
		++countS;
	}
	filetab = PyMem_NEW(struct filedescr, countD + countS + 1);
	memcpy(filetab, _PyImport_DynLoadFiletab, countD * sizeof(struct filedescr));
	memcpy(filetab + countD, _PyImport_StandardFiletab, countS * sizeof(struct filedescr));
	filetab[countD + countS].suffix = NULL;

	_PyImport_Filetab = filetab;

	if (Py_OptimizeFlag) 
	{
		for (; filetab->suffix != NULL; filetab++) 
		{
			if (strcmp(filetab->suffix, ".pyc") == 0)
			{
				filetab->suffix = ".pyo";
			}
		}
	}

	if (Py_UnicodeFlag) 
	{
		pyc_magic = MAGIC + 1;
	}
}

void _PyImport_Fini()
{
	Py_XDECREF(extensions);
	extensions = NULL;
	PyMem_DEL(_PyImport_Filetab);
	_PyImport_Filetab = NULL;
}

#include "pythread.h"

static PyThread_type_lock import_lock = 0;
static long import_lock_thread = -1;
static int import_lock_level = 0;

static void lock_import()
{
	long me = PyThread_get_thread_ident();
	if (me == -1)
	{
		return;
	}
	if (import_lock == NULL)
	{
		import_lock = PyThread_allocate_lock();
	}
	if (import_lock_thread == me) 
	{
		import_lock_level++;
		return;
	}
	if (import_lock_thread != -1 || !PyThread_acquire_lock(import_lock, 0)) 
	{
		PyThreadState *tstate = PyEval_SaveThread();
		PyThread_acquire_lock(import_lock, 1);
		PyEval_RestoreThread(tstate);
	}
	import_lock_thread = me;
	import_lock_level = 1;
}

static void unlock_import()
{
	long me = PyThread_get_thread_ident();
	if (me == -1)
	{
		return;
	}
	if (import_lock_thread != me)
	{
		Py_FatalError("unlock_import: not holding the import lock");
	}
	import_lock_level--;
	if (import_lock_level == 0) 
	{
		import_lock_thread = -1;
		PyThread_release_lock(import_lock);
	}
}

static PyObject *imp_lock_held(PyObject *self, PyObject *args)
{
	if (!PyArg_ParseTuple(args, ":lock_held"))
	{
		return NULL;
	}
	return PyInt_FromLong(import_lock_thread != -1);
}

PyObject *PyImport_GetModuleDict()
{
	PyInterpreterState *interp = PyThreadState_Get()->interp;
	if (interp->modules == NULL)
	{
		Py_FatalError("PyImport_GetModuleDict: no module dictionary!");
	}
	return interp->modules;
}

static char* sys_deletes[] = {
	"path", "argv", "ps1", "ps2", "exitfunc",
	"exc_type", "exc_value", "exc_traceback",
	"last_type", "last_value", "last_traceback",
	NULL
};

static char* sys_files[] = {
	"stdin", "__stdin__",
	"stdout", "__stdout__",
	"stderr", "__stderr__",
	NULL
};

void PyImport_Cleanup()
{
	int pos, ndone;
	char *name;
	PyObject *key, *value, *dict;
	PyInterpreterState *interp = PyThreadState_Get()->interp;
	PyObject *modules = interp->modules;

	if (modules == NULL)
	{
		return;
	}

	value = PyDict_GetItemString(modules, "__builtin__");
	if (value != NULL && PyModule_Check(value)) 
	{
		dict = PyModule_GetDict(value);
		if (Py_VerboseFlag)
		{
			PySys_WriteStderr("# clear __builtin__._\n");
		}
		PyDict_SetItemString(dict, "_", Py_None);
	}
	value = PyDict_GetItemString(modules, "sys");
	if (value != NULL && PyModule_Check(value)) 
	{
		char **p;
		PyObject *v;
		dict = PyModule_GetDict(value);
		for (p = sys_deletes; *p != NULL; p++) 
		{
			if (Py_VerboseFlag)
			{
				PySys_WriteStderr("# clear sys.%s\n", *p);
			}
			PyDict_SetItemString(dict, *p, Py_None);
		}
		for (p = sys_files; *p != NULL; p += 2) 
		{
			if (Py_VerboseFlag)
			{
				PySys_WriteStderr("# restore sys.%s\n", *p);
			}
			v = PyDict_GetItemString(dict, *(p+1));
			if (v == NULL)
			{
				v = Py_None;
			}
			PyDict_SetItemString(dict, *p, v);
		}
	}

	value = PyDict_GetItemString(modules, "__main__");
	if (value != NULL && PyModule_Check(value)) 
	{
		if (Py_VerboseFlag)
		{
			PySys_WriteStderr("# cleanup __main__\n");
		}
		_PyModule_Clear(value);
		PyDict_SetItemString(modules, "__main__", Py_None);
	}

	do 
	{
		ndone = 0;
		pos = 0;
		while (PyDict_Next(modules, &pos, &key, &value)) 
		{
			if (value->ob_refcnt != 1)
			{
				continue;
			}
			if (PyString_Check(key) && PyModule_Check(value)) 
			{
				name = PyString_AS_STRING(key);
				if (strcmp(name, "__builtin__") == 0)
				{
					continue;
				}
				if (strcmp(name, "sys") == 0)
				{
					continue;
				}
				if (Py_VerboseFlag)
				{
					PySys_WriteStderr(
						"# cleanup[1] %s\n", name);
				}
				_PyModule_Clear(value);
				PyDict_SetItem(modules, key, Py_None);
				ndone++;
			}
		}
	} while (ndone > 0);

	pos = 0;
	while (PyDict_Next(modules, &pos, &key, &value)) 
	{
		if (PyString_Check(key) && PyModule_Check(value)) 
		{
			name = PyString_AS_STRING(key);
			if (strcmp(name, "__builtin__") == 0)
			{
				continue;
			}
			if (strcmp(name, "sys") == 0)
			{
				continue;
			}
			if (Py_VerboseFlag)
			{
				PySys_WriteStderr("# cleanup[2] %s\n", name);
			}
			_PyModule_Clear(value);
			PyDict_SetItem(modules, key, Py_None);
		}
	}

	value = PyDict_GetItemString(modules, "sys");
	if (value != NULL && PyModule_Check(value)) 
	{
		if (Py_VerboseFlag)
		{
			PySys_WriteStderr("# cleanup sys\n");
		}
		_PyModule_Clear(value);
		PyDict_SetItemString(modules, "sys", Py_None);
	}
	value = PyDict_GetItemString(modules, "__builtin__");
	if (value != NULL && PyModule_Check(value)) 
	{
		if (Py_VerboseFlag)
		{
			PySys_WriteStderr("# cleanup __builtin__\n");
		}
		_PyModule_Clear(value);
		PyDict_SetItemString(modules, "__builtin__", Py_None);
	}

	PyDict_Clear(modules);
	interp->modules = NULL;
	Py_DECREF(modules);
}

long PyImport_GetMagicNumber()
{
	return pyc_magic;
}

PyObject *_PyImport_FixupExtension(char *name, char *filename)
{
	PyObject *modules, *mod, *dict, *copy;
	if (extensions == NULL) 
	{
		extensions = PyDict_New();
		if (extensions == NULL)
		{
			return NULL;
		}
	}
	modules = PyImport_GetModuleDict();
	mod = PyDict_GetItemString(modules, name);
	if (mod == NULL || !PyModule_Check(mod)) 
	{
		PyErr_Format(PyExc_SystemError,
		  "_PyImport_FixupExtension: module %.200s not loaded", name);
		return NULL;
	}
	dict = PyModule_GetDict(mod);
	if (dict == NULL)
	{
		return NULL;
	}
	copy = PyDict_Copy(dict);
	if (copy == NULL)
	{
		return NULL;
	}
	PyDict_SetItemString(extensions, filename, copy);
	Py_DECREF(copy);
	return copy;
}

PyObject *_PyImport_FindExtension(char *name, char *filename)
{
	PyObject *dict, *mod, *mdict;
	if (extensions == NULL)
	{
		return NULL;
	}
	dict = PyDict_GetItemString(extensions, filename);
	if (dict == NULL)
	{
		return NULL;
	}
	mod = PyImport_AddModule(name);
	if (mod == NULL)
	{
		return NULL;
	}
	mdict = PyModule_GetDict(mod);
	if (mdict == NULL)
	{
		return NULL;
	}
	if (PyDict_Update(mdict, dict))
	{
		return NULL;
	}
	if (Py_VerboseFlag)
	{
		PySys_WriteStderr("import %s # previously loaded (%s)\n",
			name, filename);
	}
	return mod;
}

PyObject *PyImport_AddModule(char *name)
{
	PyObject *modules = PyImport_GetModuleDict();
	PyObject *m;

	if ((m = PyDict_GetItemString(modules, name)) != NULL &&
	    PyModule_Check(m))
	{
		return m;
	}
	m = PyModule_New(name);
	if (m == NULL)
	{
		return NULL;
	}
	if (PyDict_SetItemString(modules, name, m) != 0) 
	{
		Py_DECREF(m);
		return NULL;
	}
	Py_DECREF(m);

	return m;
}

PyObject *PyImport_ExecCodeModule(char *name, PyObject *co)
{
	return PyImport_ExecCodeModuleEx(name, co, (char *)NULL);
}

PyObject *PyImport_ExecCodeModuleEx(char *name, PyObject *co, char *pathname)
{
	PyObject *modules = PyImport_GetModuleDict();
	PyObject *m, *d, *v;

	m = PyImport_AddModule(name);
	if (m == NULL)
	{
		return NULL;
	}
	d = PyModule_GetDict(m);
	if (PyDict_GetItemString(d, "__builtins__") == NULL) 
	{
		if (PyDict_SetItemString(d, "__builtins__", PyEval_GetBuiltins()) != 0)
		{
			return NULL;
		}
	}
	v = NULL;
	if (pathname != NULL) 
	{
		v = PyString_FromString(pathname);
		if (v == NULL)
		{
			PyErr_Clear();
		}
	}
	if (v == NULL) 
	{
		v = ((PyCodeObject *)co)->co_filename;
		Py_INCREF(v);
	}
	if (PyDict_SetItemString(d, "__file__", v) != 0)
	{
		PyErr_Clear();
	}
	Py_DECREF(v);

	v = PyEval_EvalCode((PyCodeObject *)co, d, d);
	if (v == NULL)
	{
		return NULL;
	}
	Py_DECREF(v);

	if ((m = PyDict_GetItemString(modules, name)) == NULL) 
	{
		PyErr_Format(PyExc_ImportError,
			     "Loaded module %.200s not found in sys.modules",
			     name);
		return NULL;
	}

	Py_INCREF(m);

	return m;
}

static char *make_compiled_pathname(char *pathname, char *buf, size_t buflen)
{
	size_t len = strlen(pathname);
	if (len + 2 > buflen)
	{
		return NULL;
	}

	if (len >= 4 && strcmp(&pathname[len-4], ".pyw") == 0)
	{
		--len;
	}
	memcpy(buf, pathname, len);
	buf[len] = Py_OptimizeFlag ? 'o' : 'c';
	buf[len+1] = '\0';

	return buf;
}

static FILE *check_compiled_module(char *pathname, long mtime, char *cpathname)
{
	FILE *fp;
	long magic;
	long pyc_mtime;

	fp = fopen(cpathname, "rb");
	if (fp == NULL)
	{
		return NULL;
	}
	magic = PyMarshal_ReadLongFromFile(fp);
	if (magic != pyc_magic) 
	{
		if (Py_VerboseFlag)
		{
			PySys_WriteStderr("# %s has bad magic\n", cpathname);
		}
		fclose(fp);
		return NULL;
	}
	pyc_mtime = PyMarshal_ReadLongFromFile(fp);
	if (pyc_mtime != mtime) 
	{
		if (Py_VerboseFlag)
		{
			PySys_WriteStderr("# %s has bad mtime\n", cpathname);
		}
		fclose(fp);
		return NULL;
	}
	if (Py_VerboseFlag)
	{
		PySys_WriteStderr("# %s matches %s\n", cpathname, pathname);
	}
	return fp;
}

static PyCodeObject *read_compiled_module(char *cpathname, FILE *fp)
{
	PyObject *co;

	co = PyMarshal_ReadLastObjectFromFile(fp);
	if (co == NULL || !PyCode_Check(co)) 
	{
		if (!PyErr_Occurred())
		{
			PyErr_Format(PyExc_ImportError, "Non-code object in %.200s", cpathname);
		}
		Py_XDECREF(co);
		return NULL;
	}
	return (PyCodeObject *)co;
}

static PyObject *load_compiled_module(char *name, char *cpathname, FILE *fp)
{
	long magic;
	PyCodeObject *co;
	PyObject *m;

	magic = PyMarshal_ReadLongFromFile(fp);
	if (magic != pyc_magic) 
	{
		PyErr_Format(PyExc_ImportError, "Bad magic number in %.200s", cpathname);
		return NULL;
	}
	PyMarshal_ReadLongFromFile(fp);
	co = read_compiled_module(cpathname, fp);
	if (co == NULL)
	{
		return NULL;
	}
	if (Py_VerboseFlag)
	{
		PySys_WriteStderr("import %s # precompiled from %s\n", name, cpathname);
	}
	m = PyImport_ExecCodeModuleEx(name, (PyObject *)co, cpathname);
	Py_DECREF(co);

	return m;
}

static PyCodeObject *parse_source_module(char *pathname, FILE *fp)
{
	PyCodeObject *co;
	node *n;

	n = PyParser_SimpleParseFile(fp, pathname, Py_file_input);
	if (n == NULL)
	{
		return NULL;
	}
	co = PyNode_Compile(n, pathname);
	PyNode_Free(n);

	return co;
}

static FILE *open_exclusive(char *filename)
{
	return fopen(filename, "wb");
}

static void write_compiled_module(PyCodeObject *co, char *cpathname, long mtime)
{
	FILE *fp;

	if (CANT_WRITE(co->co_argcount) ||
	    CANT_WRITE(co->co_nlocals) ||
	    CANT_WRITE(co->co_stacksize) ||
	    CANT_WRITE(co->co_flags) ||
	    CANT_WRITE(co->co_firstlineno)) 
	{
		if (Py_VerboseFlag)
		{
			PySys_WriteStderr("# code too large: can't write %s\n", cpathname);
		}
		return;
	}

	fp = open_exclusive(cpathname);
	if (fp == NULL) 
	{
		if (Py_VerboseFlag)
		{
			PySys_WriteStderr("# can't create %s\n", cpathname);
		}
		return;
	}
	PyMarshal_WriteLongToFile(pyc_magic, fp);
	PyMarshal_WriteLongToFile(0L, fp);
	PyMarshal_WriteObjectToFile((PyObject *)co, fp);
	if (ferror(fp)) 
	{
		if (Py_VerboseFlag)
		{
			PySys_WriteStderr("# can't write %s\n", cpathname);
		}
		fclose(fp);
		unlink(cpathname);
		return;
	}
	fseek(fp, 4L, 0);
	PyMarshal_WriteLongToFile(mtime, fp);
	fflush(fp);
	fclose(fp);
	if (Py_VerboseFlag)
	{
		PySys_WriteStderr("# wrote %s\n", cpathname);
	}
}

static PyObject *load_source_module(char *name, char *pathname, FILE *fp)
{
	time_t mtime;
	FILE *fpc;
	char buf[MAXPATHLEN+1];
	char *cpathname;
	PyCodeObject *co;
	PyObject *m;

	mtime = PyOS_GetLastModificationTime(pathname, fp);
	if (mtime == (time_t)(-1))
	{
		return NULL;
	}
	cpathname = make_compiled_pathname(pathname, buf, (size_t)MAXPATHLEN + 1);
	if (cpathname != NULL &&
	    (fpc = check_compiled_module(pathname, (long)mtime, cpathname))) 
	{
		co = read_compiled_module(cpathname, fpc);
		fclose(fpc);
		if (co == NULL)
		{
			return NULL;
		}
		if (Py_VerboseFlag)
		{
			PySys_WriteStderr("import %s # precompiled from %s\n", name, cpathname);
		}
		pathname = cpathname;
	}
	else 
	{
		co = parse_source_module(pathname, fp);
		if (co == NULL)
		{
			return NULL;
		}
		if (Py_VerboseFlag)
		{
			PySys_WriteStderr("import %s # from %s\n", name, pathname);
		}
		write_compiled_module(co, cpathname, (long)mtime);
	}
	m = PyImport_ExecCodeModuleEx(name, (PyObject *)co, pathname);
	Py_DECREF(co);

	return m;
}


static PyObject *load_module(char *, FILE *, char *, int);
static struct filedescr *find_module(char *, PyObject *,
				     char *, size_t, FILE **);
static struct _frozen *find_frozen(char *name);

static PyObject *load_package(char *name, char *pathname)
{
	PyObject *m, *d, *file, *path;
	int err;
	char buf[MAXPATHLEN+1];
	FILE *fp = NULL;
	struct filedescr *fdp;

	m = PyImport_AddModule(name);
	if (m == NULL)
	{
		return NULL;
	}
	if (Py_VerboseFlag)
	{
		PySys_WriteStderr("import %s # directory %s\n", name, pathname);
	}
	d = PyModule_GetDict(m);
	file = PyString_FromString(pathname);
	if (file == NULL)
	{
		return NULL;
	}
	path = Py_BuildValue("[O]", file);
	if (path == NULL) 
	{
		Py_DECREF(file);
		return NULL;
	}
	err = PyDict_SetItemString(d, "__file__", file);
	if (err == 0)
	{
		err = PyDict_SetItemString(d, "__path__", path);
	}
	if (err != 0) 
	{
		m = NULL;
		goto cleanup;
	}
	buf[0] = '\0';
	fdp = find_module("__init__", path, buf, sizeof(buf), &fp);
	if (fdp == NULL) 
	{
		if (PyErr_ExceptionMatches(PyExc_ImportError)) 
		{
			PyErr_Clear();
		}
		else
		{
			m = NULL;
		}
		goto cleanup;
	}
	m = load_module(name, fp, buf, fdp->type);
	if (fp != NULL)
	{
		fclose(fp);
	}
cleanup:
	Py_XDECREF(path);
	Py_XDECREF(file);
	return m;
}

static int is_builtin(char *name)
{
	int i;
	for (i = 0; PyImport_Inittab[i].name != NULL; i++) 
	{
		if (strcmp(name, PyImport_Inittab[i].name) == 0) 
		{
			if (PyImport_Inittab[i].initfunc == NULL)
			{
				return -1;
			}
			else
			{
				return 1;
			}
		}
	}
	return 0;
}

static int case_ok(char *, int, int, char *);
static int find_init_module(char *);

static struct filedescr *find_module(char *realname, PyObject *path, char *buf, size_t buflen, FILE **p_fp)
{
	int i, npath;
	size_t len, namelen;
	struct filedescr *fdp = NULL;
	FILE *fp = NULL;
	struct stat statbuf;
	static struct filedescr fd_frozen = {"", "", PY_FROZEN};
	static struct filedescr fd_builtin = {"", "", C_BUILTIN};
	static struct filedescr fd_package = {"", "", PKG_DIRECTORY};
	char name[MAXPATHLEN+1];

	if (strlen(realname) > MAXPATHLEN) 
	{
		PyErr_SetString(PyExc_OverflowError,
				"module name is too long");
		return NULL;
	}
	strcpy(name, realname);

	if (path != NULL && PyString_Check(path)) 
	{
		if (PyString_Size(path) + 1 + strlen(name) >= (size_t)buflen) 
		{
			PyErr_SetString(PyExc_ImportError,
					"full frozen module name too long");
			return NULL;
		}
		strcpy(buf, PyString_AsString(path));
		strcat(buf, ".");
		strcat(buf, name);
		strcpy(name, buf);
		if (find_frozen(name) != NULL) 
		{
			strcpy(buf, name);
			return &fd_frozen;
		}
		PyErr_Format(PyExc_ImportError, "No frozen submodule named %.200s", name);
		return NULL;
	}
	if (path == NULL) 
	{
		if (is_builtin(name)) 
		{
			strcpy(buf, name);
			return &fd_builtin;
		}
		if ((find_frozen(name)) != NULL) 
		{
			strcpy(buf, name);
			return &fd_frozen;
		}

		path = PySys_GetObject("path");
	}
	if (path == NULL || !PyList_Check(path)) 
	{
		PyErr_SetString(PyExc_ImportError,
				"sys.path must be a list of directory names");
		return NULL;
	}
	npath = PyList_Size(path);
	namelen = strlen(name);
	for (i = 0; i < npath; i++) 
	{
		PyObject *v = PyList_GetItem(path, i);
		if (!PyString_Check(v))
		{
			continue;
		}
		len = PyString_Size(v);
		if (len + 2 + namelen + MAXSUFFIXSIZE >= buflen)
		{
			continue;
		}
		strcpy(buf, PyString_AsString(v));
		if (strlen(buf) != len)
		{
			continue;
		}
		if (len > 0 && buf[len-1] != SEP
		    && buf[len-1] != ALTSEP
		    )
		{
			buf[len++] = SEP;
		}
		strcpy(buf+len, name);
		len += namelen;

		if (stat(buf, &statbuf) == 0 &&
		    S_ISDIR_(statbuf.st_mode) &&
		    find_init_module(buf) && 
		    case_ok(buf, len, namelen, name))
		{
			return &fd_package;
		}
		for (fdp = _PyImport_Filetab; fdp->suffix != NULL; fdp++) 
		{
			strcpy(buf + len, fdp->suffix);
			if (Py_VerboseFlag > 1)
			{
				PySys_WriteStderr("# trying %s\n", buf);
			}
			fp = fopen(buf, fdp->mode);
			if (fp != NULL) 
			{
				if (case_ok(buf, len, namelen, name))
				{
					break;
				}
				else 
				{
					fclose(fp);
					fp = NULL;
				}
			}
		}
		if (fp != NULL)
		{
			break;
		}
	}
	if (fp == NULL) 
	{
		PyErr_Format(PyExc_ImportError, "No module named %.200s", name);
		return NULL;
	}
	*p_fp = fp;
	return fdp;
}

static int case_ok(char *buf, int len, int namelen, char *name)
{
	if (Py_GETENV("PYTHONCASEOK") != NULL)
	{
		return 1;
	}

	return 1;
}

static int find_init_module(char *buf)
{
	const size_t save_len = strlen(buf);
	size_t i = save_len;
	char *pname; 
	struct stat statbuf;

	if (save_len + 13 >= MAXPATHLEN)
	{
		return 0;
	}
	buf[i++] = SEP;
	pname = buf + i;
	strcpy(pname, "__init__.py");
	if (stat(buf, &statbuf) == 0) 
	{
		if (case_ok(buf, save_len + 9, 8, pname)) 
		{
			buf[save_len] = '\0';
			return 1;
		}
	}
	i += strlen(pname);
	strcpy(buf+i, Py_OptimizeFlag ? "o" : "c");
	if (stat(buf, &statbuf) == 0) 
	{
		if (case_ok(buf, save_len + 9, 8, pname)) 
		{
			buf[save_len] = '\0';
			return 1;
		}
	}
	buf[save_len] = '\0';
	return 0;
}

static int init_builtin(char *);

static PyObject *load_module(char *name, FILE *fp, char *buf, int type)
{
	PyObject *modules;
	PyObject *m;
	int err;

	switch (type) 
	{
	case PY_SOURCE:
	case PY_COMPILED:
		if (fp == NULL) 
		{
			PyErr_Format(PyExc_ValueError, "file object required for import (type code %d)", type);
			return NULL;
		}
	}

	switch (type) 
	{
	case PY_SOURCE:
		m = load_source_module(name, buf, fp);
		break;

	case PY_COMPILED:
		m = load_compiled_module(name, buf, fp);
		break;

	case C_EXTENSION:
		m = _PyImport_LoadDynamicModule(name, buf, fp);
		break;

	case PKG_DIRECTORY:
		m = load_package(name, buf);
		break;

	case C_BUILTIN:
	case PY_FROZEN:
		if (buf != NULL && buf[0] != '\0')
		{
			name = buf;
		}
		if (type == C_BUILTIN)
		{
			err = init_builtin(name);
		}
		else
		{
			err = PyImport_ImportFrozenModule(name);
		}
		if (err < 0)
		{
			return NULL;
		}
		if (err == 0) 
		{
			PyErr_Format(PyExc_ImportError,
				     "Purported %s module %.200s not found",
				     type == C_BUILTIN ?
						"builtin" : "frozen",
				     name);
			return NULL;
		}
		modules = PyImport_GetModuleDict();
		m = PyDict_GetItemString(modules, name);
		if (m == NULL) 
		{
			PyErr_Format(
				PyExc_ImportError,
				"%s module %.200s not properly initialized",
				type == C_BUILTIN ?
					"builtin" : "frozen",
				name);
			return NULL;
		}
		Py_INCREF(m);
		break;

	default:
		PyErr_Format(PyExc_ImportError, "Don't know how to import %.200s (type code %d)", name, type);
		m = NULL;

	}

	return m;
}

static int init_builtin(char *name)
{
	struct _inittab *p;

	if (_PyImport_FindExtension(name, name) != NULL)
	{
		return 1;
	}

	for (p = PyImport_Inittab; p->name != NULL; p++) 
	{
		if (strcmp(name, p->name) == 0) 
		{
			if (p->initfunc == NULL) 
			{
				PyErr_Format(PyExc_ImportError,
				    "Cannot re-init internal module %.200s",
				    name);
				return -1;
			}
			if (Py_VerboseFlag)
			{
				PySys_WriteStderr("import %s # builtin\n", name);
			}
			(*p->initfunc)();
			if (PyErr_Occurred())
			{
				return -1;
			}
			if (_PyImport_FixupExtension(name, name) == NULL)
			{
				return -1;
			}
			return 1;
		}
	}
	return 0;
}

static struct _frozen *find_frozen(char *name)
{
	struct _frozen *p;

	for (p = PyImport_FrozenModules; ; p++) 
	{
		if (p->name == NULL)
		{
			return NULL;
		}
		if (strcmp(p->name, name) == 0)
		{
			break;
		}
	}
	return p;
}

static PyObject *get_frozen_object(char *name)
{
	struct _frozen *p = find_frozen(name);
	int size;

	if (p == NULL) 
	{
		PyErr_Format(PyExc_ImportError,
			     "No such frozen object named %.200s",
			     name);
		return NULL;
	}
	if (p->code == NULL) 
	{
		PyErr_Format(PyExc_ImportError,
			     "Excluded frozen object named %.200s",
			     name);
		return NULL;
	}
	size = p->size;
	if (size < 0)
	{
		size = -size;
	}
	return PyMarshal_ReadObjectFromString((char *)p->code, size);
}

int PyImport_ImportFrozenModule(char *name)
{
	struct _frozen *p = find_frozen(name);
	PyObject *co;
	PyObject *m;
	int ispackage;
	int size;

	if (p == NULL)
	{
		return 0;
	}
	if (p->code == NULL) 
	{
		PyErr_Format(PyExc_ImportError,
			     "Excluded frozen object named %.200s",
			     name);
		return -1;
	}
	size = p->size;
	ispackage = (size < 0);
	if (ispackage)
	{
		size = -size;
	}
	if (Py_VerboseFlag)
	{
		PySys_WriteStderr("import %s # frozen%s\n",
			name, ispackage ? " package" : "");
	}
	co = PyMarshal_ReadObjectFromString((char *)p->code, size);
	if (co == NULL)
	{
		return -1;
	}
	if (!PyCode_Check(co)) 
	{
		Py_DECREF(co);
		PyErr_Format(PyExc_TypeError,
			     "frozen object %.200s is not a code object",
			     name);
		return -1;
	}
	if (ispackage) 
	{
		PyObject *d, *s;
		int err;
		m = PyImport_AddModule(name);
		if (m == NULL)
		{
			return -1;
		}
		d = PyModule_GetDict(m);
		s = PyString_InternFromString(name);
		if (s == NULL)
		{
			return -1;
		}
		err = PyDict_SetItemString(d, "__path__", s);
		Py_DECREF(s);
		if (err != 0)
		{
			return err;
		}
	}
	m = PyImport_ExecCodeModuleEx(name, co, "<frozen>");
	Py_DECREF(co);
	if (m == NULL)
	{
		return -1;
	}
	Py_DECREF(m);
	return 1;
}

PyObject *PyImport_ImportModule(char *name)
{
	PyObject *pname;
	PyObject *result;

	pname = PyString_FromString(name);
	result = PyImport_Import(pname);
	Py_DECREF(pname);
	return result;
}

static PyObject *get_parent(PyObject *globals, char *buf, int *p_buflen);
static PyObject *load_next(PyObject *mod, PyObject *altmod,
			   char **p_name, char *buf, int *p_buflen);
static int mark_miss(char *name);
static int ensure_fromlist(PyObject *mod, PyObject *fromlist,
			   char *buf, int buflen, int recursive);
static PyObject * import_submodule(PyObject *mod, char *name, char *fullname);

static PyObject *import_module_ex(char *name, PyObject *globals, PyObject *locals,
		 PyObject *fromlist)
{
	char buf[MAXPATHLEN+1];
	int buflen = 0;
	PyObject *parent, *head, *next, *tail;

	parent = get_parent(globals, buf, &buflen);
	if (parent == NULL)
	{
		return NULL;
	}

	head = load_next(parent, Py_None, &name, buf, &buflen);
	if (head == NULL)
	{
		return NULL;
	}

	tail = head;
	Py_INCREF(tail);
	while (name) 
	{
		next = load_next(tail, tail, &name, buf, &buflen);
		Py_DECREF(tail);
		if (next == NULL) 
		{
			Py_DECREF(head);
			return NULL;
		}
		tail = next;
	}

	if (fromlist != NULL) 
	{
		if (fromlist == Py_None || !PyObject_IsTrue(fromlist))
		{
			fromlist = NULL;
		}
	}

	if (fromlist == NULL) 
	{
		Py_DECREF(tail);
		return head;
	}

	Py_DECREF(head);
	if (!ensure_fromlist(tail, fromlist, buf, buflen, 0)) 
	{
		Py_DECREF(tail);
		return NULL;
	}

	return tail;
}

PyObject *PyImport_ImportModuleEx(char *name, PyObject *globals, PyObject *locals,
			PyObject *fromlist)
{
	PyObject *result;
	lock_import();
	result = import_module_ex(name, globals, locals, fromlist);
	unlock_import();
	return result;
}

static PyObject *get_parent(PyObject *globals, char *buf, int *p_buflen)
{
	static PyObject *namestr = NULL;
	static PyObject *pathstr = NULL;
	PyObject *modname, *modpath, *modules, *parent;

	if (globals == NULL || !PyDict_Check(globals))
	{
		return Py_None;
	}

	if (namestr == NULL) 
	{
		namestr = PyString_InternFromString("__name__");
		if (namestr == NULL)
		{
			return NULL;
		}
	}
	if (pathstr == NULL) 
	{
		pathstr = PyString_InternFromString("__path__");
		if (pathstr == NULL)
		{
			return NULL;
		}
	}

	*buf = '\0';
	*p_buflen = 0;
	modname = PyDict_GetItem(globals, namestr);
	if (modname == NULL || !PyString_Check(modname))
	{
		return Py_None;
	}

	modpath = PyDict_GetItem(globals, pathstr);
	if (modpath != NULL) 
	{
		int len = PyString_GET_SIZE(modname);
		if (len > MAXPATHLEN) 
		{
			PyErr_SetString(PyExc_ValueError,
					"Module name too long");
			return NULL;
		}
		strcpy(buf, PyString_AS_STRING(modname));
		*p_buflen = len;
	}
	else 
	{
		char *start = PyString_AS_STRING(modname);
		char *lastdot = strrchr(start, '.');
		size_t len;
		if (lastdot == NULL)
		{
			return Py_None;
		}
		len = lastdot - start;
		if (len >= MAXPATHLEN) 
		{
			PyErr_SetString(PyExc_ValueError,
					"Module name too long");
			return NULL;
		}
		strncpy(buf, start, len);
		buf[len] = '\0';
		*p_buflen = len;
	}

	modules = PyImport_GetModuleDict();
	parent = PyDict_GetItemString(modules, buf);
	if (parent == NULL)
	{
		parent = Py_None;
	}
	return parent;
}

static PyObject *load_next(PyObject *mod, PyObject *altmod, char **p_name, char *buf,
	  int *p_buflen)
{
	char *name = *p_name;
	char *dot = strchr(name, '.');
	size_t len;
	char *p;
	PyObject *result;

	if (dot == NULL) 
	{
		*p_name = NULL;
		len = strlen(name);
	}
	else 
	{
		*p_name = dot+1;
		len = dot-name;
	}
	if (len == 0) 
	{
		PyErr_SetString(PyExc_ValueError,
				"Empty module name");
		return NULL;
	}

	p = buf + *p_buflen;
	if (p != buf)
	{
		*p++ = '.';
	}
	if (p+len-buf >= MAXPATHLEN) 
	{
		PyErr_SetString(PyExc_ValueError,
				"Module name too long");
		return NULL;
	}
	strncpy(p, name, len);
	p[len] = '\0';
	*p_buflen = p+len-buf;

	result = import_submodule(mod, p, buf);
	if (result == Py_None && altmod != mod) 
	{
		Py_DECREF(result);
		result = import_submodule(altmod, p, p);
		if (result != NULL && result != Py_None) 
		{
			if (mark_miss(buf) != 0) 
			{
				Py_DECREF(result);
				return NULL;
			}
			strncpy(buf, name, len);
			buf[len] = '\0';
			*p_buflen = len;
		}
	}
	if (result == NULL)
	{
		return NULL;
	}

	if (result == Py_None) 
	{
		Py_DECREF(result);
		PyErr_Format(PyExc_ImportError,
			     "No module named %.200s", name);
		return NULL;
	}

	return result;
}

static int mark_miss(char *name)
{
	PyObject *modules = PyImport_GetModuleDict();
	return PyDict_SetItemString(modules, name, Py_None);
}

static int ensure_fromlist(PyObject *mod, PyObject *fromlist, char *buf, int buflen,
		int recursive)
{
	int i;

	if (!PyObject_HasAttrString(mod, "__path__"))
	{
		return 1;
	}

	for (i = 0; ; i++) 
	{
		PyObject *item = PySequence_GetItem(fromlist, i);
		int hasit;
		if (item == NULL) 
		{
			if (PyErr_ExceptionMatches(PyExc_IndexError)) 
			{
				PyErr_Clear();
				return 1;
			}
			return 0;
		}
		if (!PyString_Check(item)) 
		{
			PyErr_SetString(PyExc_TypeError,
					"Item in ``from list'' not a string");
			Py_DECREF(item);
			return 0;
		}
		if (PyString_AS_STRING(item)[0] == '*') 
		{
			PyObject *all;
			Py_DECREF(item);
			if (recursive)
			{
				continue;
			}
			all = PyObject_GetAttrString(mod, "__all__");
			if (all == NULL)
			{
				PyErr_Clear();
			}
			else 
			{
				if (!ensure_fromlist(mod, all, buf, buflen, 1))
				{
					return 0;
				}
				Py_DECREF(all);
			}
			continue;
		}
		hasit = PyObject_HasAttr(mod, item);
		if (!hasit) 
		{
			char *subname = PyString_AS_STRING(item);
			PyObject *submod;
			char *p;
			if (buflen + strlen(subname) >= MAXPATHLEN) 
			{
				PyErr_SetString(PyExc_ValueError,
						"Module name too long");
				Py_DECREF(item);
				return 0;
			}
			p = buf + buflen;
			*p++ = '.';
			strcpy(p, subname);
			submod = import_submodule(mod, subname, buf);
			Py_XDECREF(submod);
			if (submod == NULL) 
			{
				Py_DECREF(item);
				return 0;
			}
		}
		Py_DECREF(item);
	}
}

static PyObject *import_submodule(PyObject *mod, char *subname, char *fullname)
{
	PyObject *modules = PyImport_GetModuleDict();
	PyObject *m, *res = NULL;

	if ((m = PyDict_GetItemString(modules, fullname)) != NULL) 
	{
		Py_INCREF(m);
	}
	else 
	{
		PyObject *path;
		char buf[MAXPATHLEN+1];
		struct filedescr *fdp;
		FILE *fp = NULL;

		if (mod == Py_None)
		{
			path = NULL;
		}
		else 
		{
			path = PyObject_GetAttrString(mod, "__path__");
			if (path == NULL) 
			{
				PyErr_Clear();
				Py_INCREF(Py_None);
				return Py_None;
			}
		}

		buf[0] = '\0';
		fdp = find_module(subname, path, buf, MAXPATHLEN+1, &fp);
		Py_XDECREF(path);
		if (fdp == NULL) 
		{
			if (!PyErr_ExceptionMatches(PyExc_ImportError))
			{
				return NULL;
			}
			PyErr_Clear();
			Py_INCREF(Py_None);
			return Py_None;
		}
		m = load_module(fullname, fp, buf, fdp->type);
		if (fp)
		{
			fclose(fp);
		}
		if (mod != Py_None) 
		{
			res = m;
			if (res == NULL)
			{
				res = PyDict_GetItemString(modules, fullname);
			}
			if (res != NULL &&
			    PyObject_SetAttrString(mod, subname, res) < 0) 
			{
				Py_XDECREF(m);
				m = NULL;
			}
		}
	}

	return m;
}

PyObject *PyImport_ReloadModule(PyObject *m)
{
	PyObject *modules = PyImport_GetModuleDict();
	PyObject *path = NULL;
	char *name, *subname;
	char buf[MAXPATHLEN + 1];
	struct filedescr *fdp;
	FILE *fp = NULL;

	if (m == NULL || !PyModule_Check(m)) 
	{
		PyErr_SetString(PyExc_TypeError,
				"reload() argument must be module");
		return NULL;
	}
	name = PyModule_GetName(m);
	if (name == NULL)
	{
		return NULL;
	}
	if (m != PyDict_GetItemString(modules, name)) 
	{
		PyErr_Format(PyExc_ImportError,
			     "reload(): module %.200s not in sys.modules",
			     name);
		return NULL;
	}
	subname = strrchr(name, '.');
	if (subname == NULL)
	{
		subname = name;
	}
	else 
	{
		PyObject *parentname, *parent;
		parentname = PyString_FromStringAndSize(name, (subname - name));
		if (parentname == NULL)
		{
			return NULL;
		}
		parent = PyDict_GetItem(modules, parentname);
		Py_DECREF(parentname);
		if (parent == NULL) 
		{
			PyErr_Format(PyExc_ImportError,
			    "reload(): parent %.200s not in sys.modules",
			    name);
			return NULL;
		}
		subname++;
		path = PyObject_GetAttrString(parent, "__path__");
		if (path == NULL)
		{
			PyErr_Clear();
		}
	}
	buf[0] = '\0';
	fdp = find_module(subname, path, buf, MAXPATHLEN+1, &fp);
	Py_XDECREF(path);
	if (fdp == NULL)
	{
		return NULL;
	}
	m = load_module(name, fp, buf, fdp->type);
	if (fp)
	{
		fclose(fp);
	}
	return m;
}

PyObject *PyImport_Import(PyObject *module_name)
{
	static PyObject *silly_list = NULL;
	static PyObject *builtins_str = NULL;
	static PyObject *import_str = NULL;
	PyObject *globals = NULL;
	PyObject *import = NULL;
	PyObject *builtins = NULL;
	PyObject *r = NULL;

	if (silly_list == NULL) 
	{
		import_str = PyString_InternFromString("__import__");
		if (import_str == NULL)
		{
			return NULL;
		}
		builtins_str = PyString_InternFromString("__builtins__");
		if (builtins_str == NULL)
		{
			return NULL;
		}
		silly_list = Py_BuildValue("[s]", "__doc__");
		if (silly_list == NULL)
		{
			return NULL;
		}
	}

	globals = PyEval_GetGlobals();
	if (globals != NULL) 
	{
	    Py_INCREF(globals);
		builtins = PyObject_GetItem(globals, builtins_str);
		if (builtins == NULL)
		{
			goto err;
		}
	}
	else 
	{
		PyErr_Clear();

		builtins = PyImport_ImportModuleEx("__builtin__",
						   NULL, NULL, NULL);
		if (builtins == NULL)
		{
			return NULL;
		}
		globals = Py_BuildValue("{OO}", builtins_str, builtins);
		if (globals == NULL)
		{
			goto err;
		}
	}

	if (PyDict_Check(builtins)) 
	{
		import = PyObject_GetItem(builtins, import_str);
		if (import == NULL)
		{
			PyErr_SetObject(PyExc_KeyError, import_str);
		}
	}
	else
	{
		import = PyObject_GetAttr(builtins, import_str);
	}
	if (import == NULL)
	{
		goto err;
	}

	r = PyObject_CallFunction(import, "OOOO",
				  module_name, globals, globals, silly_list);

err:
	Py_XDECREF(globals);
	Py_XDECREF(builtins);
	Py_XDECREF(import);

	return r;
}

static PyObject *imp_get_magic(PyObject *self, PyObject *args)
{
	char buf[4];

	if (!PyArg_ParseTuple(args, ":get_magic"))
	{
		return NULL;
	}
	buf[0] = (char) ((pyc_magic >>  0) & 0xff);
	buf[1] = (char) ((pyc_magic >>  8) & 0xff);
	buf[2] = (char) ((pyc_magic >> 16) & 0xff);
	buf[3] = (char) ((pyc_magic >> 24) & 0xff);

	return PyString_FromStringAndSize(buf, 4);
}

static PyObject *imp_get_suffixes(PyObject *self, PyObject *args)
{
	PyObject *list;
	struct filedescr *fdp;

	if (!PyArg_ParseTuple(args, ":get_suffixes"))
	{
		return NULL;
	}
	list = PyList_New(0);
	if (list == NULL)
	{
		return NULL;
	}
	for (fdp = _PyImport_Filetab; fdp->suffix != NULL; fdp++) 
	{
		PyObject *item = Py_BuildValue("ssi",
				       fdp->suffix, fdp->mode, fdp->type);
		if (item == NULL) 
		{
			Py_DECREF(list);
			return NULL;
		}
		if (PyList_Append(list, item) < 0) 
		{
			Py_DECREF(list);
			Py_DECREF(item);
			return NULL;
		}
		Py_DECREF(item);
	}
	return list;
}

static PyObject *call_find_module(char *name, PyObject *path)
{
	extern int fclose(FILE *);
	PyObject *fob, *ret;
	struct filedescr *fdp;
	char pathname[MAXPATHLEN+1];
	FILE *fp = NULL;

	pathname[0] = '\0';
	if (path == Py_None)
	{
		path = NULL;
	}
	fdp = find_module(name, path, pathname, MAXPATHLEN+1, &fp);
	if (fdp == NULL)
	{
		return NULL;
	}
	if (fp != NULL) 
	{
		fob = PyFile_FromFile(fp, pathname, fdp->mode, fclose);
		if (fob == NULL) 
		{
			fclose(fp);
			return NULL;
		}
	}
	else 
	{
		fob = Py_None;
		Py_INCREF(fob);
	}
	ret = Py_BuildValue("Os(ssi)",
		      fob, pathname, fdp->suffix, fdp->mode, fdp->type);
	Py_DECREF(fob);
	return ret;
}

static PyObject *imp_find_module(PyObject *self, PyObject *args)
{
	char *name;
	PyObject *path = NULL;
	if (!PyArg_ParseTuple(args, "s|O:find_module", &name, &path))
	{
		return NULL;
	}
	return call_find_module(name, path);
}

static PyObject *imp_init_builtin(PyObject *self, PyObject *args)
{
	char *name;
	int ret;
	PyObject *m;
	if (!PyArg_ParseTuple(args, "s:init_builtin", &name))
	{
		return NULL;
	}
	ret = init_builtin(name);
	if (ret < 0)
	{
		return NULL;
	}
	if (ret == 0) 
	{
		Py_INCREF(Py_None);
		return Py_None;
	}
	m = PyImport_AddModule(name);
	Py_XINCREF(m);
	return m;
}

static PyObject *imp_init_frozen(PyObject *self, PyObject *args)
{
	char *name;
	int ret;
	PyObject *m;
	if (!PyArg_ParseTuple(args, "s:init_frozen", &name))
	{
		return NULL;
	}
	ret = PyImport_ImportFrozenModule(name);
	if (ret < 0)
	{
		return NULL;
	}
	if (ret == 0) 
	{
		Py_INCREF(Py_None);
		return Py_None;
	}
	m = PyImport_AddModule(name);
	Py_XINCREF(m);
	return m;
}

static PyObject *imp_get_frozen_object(PyObject *self, PyObject *args)
{
	char *name;

	if (!PyArg_ParseTuple(args, "s:get_frozen_object", &name))
	{
		return NULL;
	}
	return get_frozen_object(name);
}

static PyObject *imp_is_builtin(PyObject *self, PyObject *args)
{
	char *name;
	if (!PyArg_ParseTuple(args, "s:is_builtin", &name))
	{
		return NULL;
	}
	return PyInt_FromLong(is_builtin(name));
}

static PyObject *imp_is_frozen(PyObject *self, PyObject *args)
{
	char *name;
	struct _frozen *p;
	if (!PyArg_ParseTuple(args, "s:is_frozen", &name))
	{
		return NULL;
	}
	p = find_frozen(name);
	return PyInt_FromLong((long) (p == NULL ? 0 : p->size));
}

static FILE *get_file(char *pathname, PyObject *fob, char *mode)
{
	FILE *fp;
	if (fob == NULL) 
	{
		fp = fopen(pathname, mode);
		if (fp == NULL)
		{
			PyErr_SetFromErrno(PyExc_IOError);
		}
	}
	else 
	{
		fp = PyFile_AsFile(fob);
		if (fp == NULL)
		{
			PyErr_SetString(PyExc_ValueError, "bad/closed file object");
		}
	}
	return fp;
}

static PyObject *imp_load_compiled(PyObject *self, PyObject *args)
{
	char *name;
	char *pathname;
	PyObject *fob = NULL;
	PyObject *m;
	FILE *fp;
	if (!PyArg_ParseTuple(args, "ss|O!:load_compiled", &name, &pathname,
			      &PyFile_Type, &fob))
	{
		return NULL;
	}
	fp = get_file(pathname, fob, "rb");
	if (fp == NULL)
	{
		return NULL;
	}
	m = load_compiled_module(name, pathname, fp);
	if (fob == NULL)
	{
		fclose(fp);
	}
	return m;
}

static PyObject *imp_load_dynamic(PyObject *self, PyObject *args)
{
	char *name;
	char *pathname;
	PyObject *fob = NULL;
	PyObject *m;
	FILE *fp = NULL;
	if (!PyArg_ParseTuple(args, "ss|O!:load_dynamic", &name, &pathname,
			      &PyFile_Type, &fob))
	{
		return NULL;
	}
	if (fob) 
	{
		fp = get_file(pathname, fob, "r");
		if (fp == NULL)
		{
			return NULL;
		}
	}
	m = _PyImport_LoadDynamicModule(name, pathname, fp);
	return m;
}

static PyObject *imp_load_source(PyObject *self, PyObject *args)
{
	char *name;
	char *pathname;
	PyObject *fob = NULL;
	PyObject *m;
	FILE *fp;
	if (!PyArg_ParseTuple(args, "ss|O!:load_source", &name, &pathname,
			      &PyFile_Type, &fob))
	{
		return NULL;
	}
	fp = get_file(pathname, fob, "r");
	if (fp == NULL)
	{
		return NULL;
	}
	m = load_source_module(name, pathname, fp);
	if (fob == NULL)
	{
		fclose(fp);
	}
	return m;
}

static PyObject *imp_load_module(PyObject *self, PyObject *args)
{
	char *name;
	PyObject *fob;
	char *pathname;
	char *suffix;
	char *mode;
	int type;
	FILE *fp;

	if (!PyArg_ParseTuple(args, "sOs(ssi):load_module",
			      &name, &fob, &pathname,
			      &suffix, &mode, &type))
	{
		return NULL;
	}
	if (*mode && (*mode != 'r' || strchr(mode, '+') != NULL)) 
	{
		PyErr_Format(PyExc_ValueError,
			     "invalid file open mode %.200s", mode);
		return NULL;
	}
	if (fob == Py_None)
	{
		fp = NULL;
	}
	else 
	{
		if (!PyFile_Check(fob)) 
		{
			PyErr_SetString(PyExc_ValueError,
				"load_module arg#2 should be a file or None");
			return NULL;
		}
		fp = get_file(pathname, fob, mode);
		if (fp == NULL)
		{
			return NULL;
		}
	}
	return load_module(name, fp, pathname, type);
}

static PyObject *imp_load_package(PyObject *self, PyObject *args)
{
	char *name;
	char *pathname;
	if (!PyArg_ParseTuple(args, "ss:load_package", &name, &pathname))
	{
		return NULL;
	}
	return load_package(name, pathname);
}

static PyObject *imp_new_module(PyObject *self, PyObject *args)
{
	char *name;
	if (!PyArg_ParseTuple(args, "s:new_module", &name))
	{
		return NULL;
	}
	return PyModule_New(name);
}

static char doc_imp[] = ""
	"This module provides the components needed to build your own\n"
	"__import__ function.  Undocumented functions are obsolete.\n"
	"";

static char doc_find_module[] = ""
	"find_module(name, [path]) -> (file, filename, (suffix, mode, type))\n"
	"Search for a module.  If path is omitted or None, search for a\n"
	"built-in, frozen or special module and continue search in sys.path.\n"
	"The module name cannot contain '.'; to search for a submodule of a\n"
	"package, pass the submodule name and the package's __path__."
	"";

static char doc_load_module[] = ""
	"load_module(name, file, filename, (suffix, mode, type)) -> module\n"
	"Load a module, given information returned by find_module().\n"
	"The module name must include the full package name, if any."
	"";

static char doc_get_magic[] = ""
	"get_magic() -> string\n"
	"Return the magic number for .pyc or .pyo files."
	"";

static char doc_get_suffixes[] = ""
	"get_suffixes() -> [(suffix, mode, type), ...]\n"
	"Return a list of (suffix, mode, type) tuples describing the files\n"
	"that find_module() looks for."
	"";

static char doc_new_module[] = ""
	"new_module(name) -> module\n"
	"Create a new module.  Do not enter it in sys.modules.\n"
	"The module name must include the full package name, if any."
	"";

static char doc_lock_held[] = ""
	"lock_held() -> 0 or 1\n"
	"Return 1 if the import lock is currently held.\n"
	"On platforms without threads, return 0."
	"";

static PyMethodDef imp_methods[] = {
	{"find_module",		imp_find_module,	1, doc_find_module},
	{"get_magic",		imp_get_magic,		1, doc_get_magic},
	{"get_suffixes",	imp_get_suffixes,	1, doc_get_suffixes},
	{"load_module",		imp_load_module,	1, doc_load_module},
	{"new_module",		imp_new_module,		1, doc_new_module},
	{"lock_held",		imp_lock_held,		1, doc_lock_held},
	{"get_frozen_object",	imp_get_frozen_object,	1},
	{"init_builtin",	imp_init_builtin,	1},
	{"init_frozen",		imp_init_frozen,	1},
	{"is_builtin",		imp_is_builtin,		1},
	{"is_frozen",		imp_is_frozen,		1},
	{"load_compiled",	imp_load_compiled,	1},
	{"load_dynamic",	imp_load_dynamic,	1},
	{"load_package",	imp_load_package,	1},
	{"load_source",		imp_load_source,	1},
	{NULL,			NULL}
};

static int setint(PyObject *d, char *name, int value)
{
	PyObject *v;
	int err;

	v = PyInt_FromLong((long)value);
	err = PyDict_SetItemString(d, name, v);
	Py_XDECREF(v);
	return err;
}

void initimp()
{
	PyObject *m, *d;

	m = Py_InitModule4("imp", imp_methods, doc_imp, NULL, PYTHON_API_VERSION);
	d = PyModule_GetDict(m);

	if (setint(d, "SEARCH_ERROR", SEARCH_ERROR) < 0) 
	{
		goto failure;
	}
	if (setint(d, "PY_SOURCE", PY_SOURCE) < 0) 
	{
		goto failure;
	}
	if (setint(d, "PY_COMPILED", PY_COMPILED) < 0) 
	{
		goto failure;
	}
	if (setint(d, "C_EXTENSION", C_EXTENSION) < 0) 
	{
		goto failure;
	}
	if (setint(d, "PY_RESOURCE", PY_RESOURCE) < 0) 
	{
		goto failure;
	}
	if (setint(d, "PKG_DIRECTORY", PKG_DIRECTORY) < 0) 
	{
		goto failure;
	}
	if (setint(d, "C_BUILTIN", C_BUILTIN) < 0) 
	{
		goto failure;
	}
	if (setint(d, "PY_FROZEN", PY_FROZEN) < 0) 
	{
		goto failure;
	}
	if (setint(d, "PY_CODERESOURCE", PY_CODERESOURCE) < 0) 
	{
		goto failure;
	}

failure:
	;
}

int PyImport_ExtendInittab(struct _inittab *newtab)
{
	static struct _inittab *our_copy = NULL;
	struct _inittab *p;
	int i, n;

	for (n = 0; newtab[n].name != NULL; n++)
	{
		;
	}
	if (n == 0)
	{
		return 0;
	}
	for (i = 0; PyImport_Inittab[i].name != NULL; i++)
	{
		;
	}

	p = our_copy;
	PyMem_RESIZE(p, struct _inittab, i+n+1);
	if (p == NULL)
	{
		return -1;
	}

	if (our_copy != PyImport_Inittab)
	{
		memcpy(p, PyImport_Inittab, (i + 1) * sizeof(struct _inittab));
	}
	PyImport_Inittab = our_copy = p;
	memcpy(p+i, newtab, (n + 1) * sizeof(struct _inittab));

	return 0;
}

int PyImport_AppendInittab(char *name, void (*initfunc)())
{
	struct _inittab newtab[2];

	memset(newtab, '\0', sizeof newtab);

	newtab[0].name = name;
	newtab[0].initfunc = initfunc;

	return PyImport_ExtendInittab(newtab);
}
