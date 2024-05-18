//20170415
#include "python.h"
#include "structmember.h"

#include <sys/types.h>

#define HAVE_FTRUNCATE

#define BUF(v) PyString_AS_STRING((PyStringObject *)v)

#include <errno.h>

typedef struct {
	PyObject_HEAD
	FILE *f_fp;
	PyObject *f_name;
	PyObject *f_mode;
	int (*f_close)(FILE *);
	int f_softspace;
	int f_binary; 
} PyFileObject;

FILE *PyFile_AsFile(PyObject *f)
{
	if (f == NULL || !PyFile_Check(f))
	{
		return NULL;
	}
	else
	{
		return ((PyFileObject *)f)->f_fp;
	}
}

PyObject *PyFile_Name(PyObject *f)
{
	if (f == NULL || !PyFile_Check(f))
	{
		return NULL;
	}
	else
	{
		return ((PyFileObject *)f)->f_name;
	}
}

static PyObject *fill_file_fields(PyFileObject *f, FILE *fp, char *name, char *mode,
		 int (*close)(FILE *))
{
	assert(f != NULL);
	assert(PyFile_Check(f));
	assert(f->f_fp == NULL);

	Py_DECREF(f->f_name);
	Py_DECREF(f->f_mode);
	f->f_name = PyString_FromString(name);
	f->f_mode = PyString_FromString(mode);

	f->f_close = close;
	f->f_softspace = 0;
	f->f_binary = strchr(mode,'b') != NULL;

	if (f->f_name == NULL || f->f_mode == NULL)
	{
		return NULL;
	}
	f->f_fp = fp;
	return (PyObject *) f;
}

static PyObject *open_the_file(PyFileObject *f, char *name, char *mode)
{
	assert(f != NULL);
	assert(PyFile_Check(f));
	assert(name != NULL);
	assert(mode != NULL);
	assert(f->f_fp == NULL);

	if (PyEval_GetRestricted()) 
	{
		PyErr_SetString(PyExc_IOError,
			"file() constructor not accessible in restricted mode");
		return NULL;
	}
	errno = 0;
	
	{ 
		PyThreadState *_save; 
		_save = PyEval_SaveThread();

		f->f_fp = fopen(name, mode);
		
		PyEval_RestoreThread(_save); 
	}


	if (f->f_fp == NULL) 
	{
		if (errno == 0)	
		{
			errno = EINVAL;
		}
		else if (errno == EINVAL)
		{	
			errno = ENOENT;
		}
		if (errno == EINVAL)
		{
			PyErr_Format(PyExc_IOError, "invalid mode: %s",
				     mode);
		}
		else
		{
			PyErr_SetFromErrnoWithFilename(PyExc_IOError, name);
		}
		f = NULL;
	}
	return (PyObject *)f;
}

PyObject *PyFile_FromFile(FILE *fp, char *name, char *mode, int (*close)(FILE *))
{
	PyFileObject *f = (PyFileObject *)PyFile_Type.tp_new(&PyFile_Type,
							     NULL, NULL);
	if (f != NULL) 
	{
		if (fill_file_fields(f, fp, name, mode, close) == NULL) 
		{
			Py_DECREF(f);
			f = NULL;
		}
	}
	return (PyObject *) f;
}

PyObject *PyFile_FromString(char *name, char *mode)
{
	extern int fclose(FILE *);
	PyFileObject *f;

	f = (PyFileObject *)PyFile_FromFile((FILE *)NULL, name, mode, fclose);
	if (f != NULL) 
	{
		if (open_the_file(f, name, mode) == NULL) 
		{
			Py_DECREF(f);
			f = NULL;
		}
	}
	return (PyObject *)f;
}

void PyFile_SetBufSize(PyObject *f, int bufsize)
{
	if (bufsize >= 0) 
	{
		int type;
		switch (bufsize) 
		{
		case 0:
			type = _IONBF;
			break;

		case 1:
			type = _IOLBF;
			bufsize = BUFSIZ;
			break;

		default:
			type = _IOFBF;
		}
		setvbuf(((PyFileObject *)f)->f_fp, (char *)NULL,
			type, bufsize);
	}
}

static PyObject *err_closed()
{
	PyErr_SetString(PyExc_ValueError, "I/O operation on closed file");
	return NULL;
}

static void file_dealloc(PyFileObject *f)
{
	if (f->f_fp != NULL && f->f_close != NULL) 
	{
		PyThreadState *_save; 
		_save = PyEval_SaveThread();

		(*f->f_close)(f->f_fp);
		
		PyEval_RestoreThread(_save); 
	}
	Py_XDECREF(f->f_name);
	Py_XDECREF(f->f_mode);
	f->ob_type->tp_free((PyObject *)f);
}

static PyObject *file_repr(PyFileObject *f)
{
	return PyString_FromFormat("<%s file '%s', mode '%s' at %p>",
				   f->f_fp == NULL ? "closed" : "open",
				   PyString_AsString(f->f_name),
				   PyString_AsString(f->f_mode),
				   f);
}

static PyObject *file_close(PyFileObject *f)
{
	int sts = 0;
	if (f->f_fp != NULL) 
	{
		if (f->f_close != NULL) 
		{
			PyThreadState *_save; 
			_save = PyEval_SaveThread();

			errno = 0;
			sts = (*f->f_close)(f->f_fp);

			PyEval_RestoreThread(_save); 
		}
		f->f_fp = NULL;
	}
	if (sts == EOF)
	{
		return PyErr_SetFromErrno(PyExc_IOError);
	}
	if (sts != 0)
	{
		return PyInt_FromLong((long)sts);
	}
	Py_INCREF(Py_None);
	return Py_None;
}


typedef off_t Py_off_t;

static int _portable_fseek(FILE *fp, Py_off_t offset, int whence)
{
	return fseek(fp, offset, whence);
}


static Py_off_t _portable_ftell(FILE* fp)
{
	return ftell(fp);
}


static PyObject *file_seek(PyFileObject *f, PyObject *args)
{
	int whence;
	int ret;
	Py_off_t offset;
	PyObject *offobj;

	if (f->f_fp == NULL)
	{
		return err_closed();
	}
	whence = 0;
	if (!PyArg_ParseTuple(args, "O|i:seek", &offobj, &whence))
	{
		return NULL;
	}
	offset = PyInt_AsLong(offobj);
	if (PyErr_Occurred())
	{
		return NULL;
	}

	{ 
		PyThreadState *_save; 
		_save = PyEval_SaveThread();
		
		errno = 0;
		ret = _portable_fseek(f->f_fp, offset, whence);
	
		PyEval_RestoreThread(_save); 
	}

	if (ret != 0) 
	{
		PyErr_SetFromErrno(PyExc_IOError);
		clearerr(f->f_fp);
		return NULL;
	}
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *file_tell(PyFileObject *f)
{
	Py_off_t pos;

	if (f->f_fp == NULL)
	{
		return err_closed();
	}

	{ 
		PyThreadState *_save; 
		_save = PyEval_SaveThread();

		errno = 0;
		pos = _portable_ftell(f->f_fp);

		PyEval_RestoreThread(_save); 
	}
	
	if (pos == -1) 
	{
		PyErr_SetFromErrno(PyExc_IOError);
		clearerr(f->f_fp);
		return NULL;
	}
	return PyInt_FromLong(pos);
}

static PyObject *file_fileno(PyFileObject *f)
{
	if (f->f_fp == NULL)
	{
		return err_closed();
	}
	return PyInt_FromLong((long) fileno(f->f_fp));
}

static PyObject *file_flush(PyFileObject *f)
{
	int res;

	if (f->f_fp == NULL)
	{
		return err_closed();
	}

	{ 
		PyThreadState *_save; 
		_save = PyEval_SaveThread();
		
		errno = 0;
		res = fflush(f->f_fp);

		PyEval_RestoreThread(_save); 
	}
	
	if (res != 0) 
	{
		PyErr_SetFromErrno(PyExc_IOError);
		clearerr(f->f_fp);
		return NULL;
	}
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *file_isatty(PyFileObject *f)
{
	long res;
	if (f->f_fp == NULL)
	{
		return err_closed();
	}

	{ 
		PyThreadState *_save; 
		_save = PyEval_SaveThread();
	
		res = isatty((int)fileno(f->f_fp));

		PyEval_RestoreThread(_save); 
	}
	
	return PyInt_FromLong(res);
}



#define SMALLCHUNK 8192
#define BIGCHUNK  (512 * 1024)

static size_t new_buffersize(PyFileObject *f, size_t currentsize)
{
	off_t pos, end;
	struct stat st;
	if (fstat(fileno(f->f_fp), &st) == 0) 
	{
		end = st.st_size;
		pos = lseek(fileno(f->f_fp), 0L, SEEK_CUR);
		if (pos >= 0) 
		{
			pos = ftell(f->f_fp);
		}
		if (pos < 0)
		{
			clearerr(f->f_fp);
		}
		if (end > pos && pos >= 0)
		{
			return currentsize + end - pos + 1;
		}
	}
	if (currentsize > SMALLCHUNK) 
	{
		if (currentsize <= BIGCHUNK)
		{
			return currentsize + currentsize;
		}
		else
		{
			return currentsize + BIGCHUNK;
		}
	}
	return currentsize + SMALLCHUNK;
}

static PyObject *file_read(PyFileObject *f, PyObject *args)
{
	long bytesrequested = -1;
	size_t bytesread, buffersize, chunksize;
	PyObject *v;

	if (f->f_fp == NULL)
	{
		return err_closed();
	}
	if (!PyArg_ParseTuple(args, "|l:read", &bytesrequested))
	{
		return NULL;
	}
	if (bytesrequested < 0)
	{
		buffersize = new_buffersize(f, (size_t)0);
	}
	else
	{
		buffersize = bytesrequested;
	}
	if (buffersize > INT_MAX) 
	{
		PyErr_SetString(PyExc_OverflowError,
			"requested number of bytes is more than a Python string can hold");
		return NULL;
	}
	v = PyString_FromStringAndSize((char *)NULL, buffersize);
	if (v == NULL)
	{
		return NULL;
	}
	bytesread = 0;
	for (;;) 
	{
		PyThreadState *_save; 
		_save = PyEval_SaveThread();

		errno = 0;
		chunksize = fread(BUF(v) + bytesread, 1,
				  buffersize - bytesread, f->f_fp);
		
		PyEval_RestoreThread(_save); 
		
		
		if (chunksize == 0) 
		{
			if (!ferror(f->f_fp))
			{
				break;
			}
			PyErr_SetFromErrno(PyExc_IOError);
			clearerr(f->f_fp);
			Py_DECREF(v);
			return NULL;
		}
		bytesread += chunksize;
		if (bytesread < buffersize)
		{
			break;
		}
		if (bytesrequested < 0) 
		{
			buffersize = new_buffersize(f, buffersize);
			if (_PyString_Resize(&v, buffersize) < 0)
			{
				return NULL;
			}
		}
	}
	if (bytesread != buffersize)
	{
		_PyString_Resize(&v, bytesread);
	}
	return v;
}

static PyObject *file_readinto(PyFileObject *f, PyObject *args)
{
	char *ptr;
	int ntodo;
	size_t ndone, nnow;

	if (f->f_fp == NULL)
	{
		return err_closed();
	}
	if (!PyArg_Parse(args, "w#", &ptr, &ntodo))
	{
		return NULL;
	}
	ndone = 0;
	while (ntodo > 0) 
	{
		PyThreadState *_save; 
		_save = PyEval_SaveThread();

		errno = 0;
		nnow = fread(ptr+ndone, 1, ntodo, f->f_fp);
		
		PyEval_RestoreThread(_save); 
		

		if (nnow == 0) 
		{
			if (!ferror(f->f_fp))
			{
				break;
			}
			PyErr_SetFromErrno(PyExc_IOError);
			clearerr(f->f_fp);
			return NULL;
		}
		ndone += nnow;
		ntodo -= nnow;
	}
	return PyInt_FromLong((long)ndone);
}


#define USE_FGETS_IN_GETLINE

static PyObject *getline_via_fgets(FILE *fp)
{
#define INITBUFSIZE 100
#define MAXBUFSIZE 300
#define INCBUFSIZE 1000
	char* p;
	char buf[MAXBUFSIZE];
	PyObject* v;
	char* pvfree;
	char* pvend;
	size_t nfree;
	size_t total_v_size;

	total_v_size = INITBUFSIZE;
	pvfree = buf;
	for (;;) 
	{
		PyThreadState *_save; 
		_save = PyEval_SaveThread();

		pvend = buf + total_v_size;
		nfree = pvend - pvfree;
		memset(pvfree, '\n', nfree);
		p = fgets(pvfree, nfree, fp);

		PyEval_RestoreThread(_save); 

		if (p == NULL) 
		{
			clearerr(fp);
			if (PyErr_CheckSignals())
			{
				return NULL;
			}
			v = PyString_FromStringAndSize(buf, pvfree - buf);
			return v;
		}
		p = memchr(pvfree, '\n', nfree);
		if (p != NULL) 
		{
			if (p+1 < pvend && *(p+1) == '\0') 
			{
				++p;
			}
			else 
			{
				assert(p > pvfree && *(p-1) == '\0');
				--p;
			}
			v = PyString_FromStringAndSize(buf, p - buf);
			return v;
		}
		assert(*(pvend-1) == '\0');
		if (pvfree == buf) 
		{
			pvfree = pvend - 1;
			total_v_size = MAXBUFSIZE;
		}
		else
		{
			break;
		}
	}

	total_v_size = MAXBUFSIZE + INCBUFSIZE;
	v = PyString_FromStringAndSize((char*)NULL, (int)total_v_size);
	if (v == NULL)
	{
		return v;
	}
	memcpy(BUF(v), buf, MAXBUFSIZE-1);
	pvfree = BUF(v) + MAXBUFSIZE - 1;

	for (;;) 
	{
		PyThreadState *_save; 
		_save = PyEval_SaveThread();

		pvend = BUF(v) + total_v_size;
		nfree = pvend - pvfree;
		memset(pvfree, '\n', nfree);
		p = fgets(pvfree, nfree, fp);

		PyEval_RestoreThread(_save); 

		if (p == NULL) 
		{
			clearerr(fp);
			if (PyErr_CheckSignals()) 
			{
				Py_DECREF(v);
				return NULL;
			}
			p = pvfree;
			break;
		}
		p = memchr(pvfree, '\n', nfree);
		if (p != NULL) 
		{
			if (p+1 < pvend && *(p+1) == '\0') 
			{
				++p;
				break;
			}
			assert(p > pvfree && *(p-1) == '\0');
			--p;
			break;
		}
		assert(*(pvend-1) == '\0');
		total_v_size += INCBUFSIZE;
		if (total_v_size > INT_MAX) 
		{
			PyErr_SetString(PyExc_OverflowError,
			    "line is longer than a Python string can hold");
			Py_DECREF(v);
			return NULL;
		}
		if (_PyString_Resize(&v, (int)total_v_size) < 0)
		{
			return NULL;
		}
		pvfree = BUF(v) + (total_v_size - INCBUFSIZE - 1);
	}
	if (BUF(v) + total_v_size != p)
	{
		_PyString_Resize(&v, p - BUF(v));
	}
	return v;
#undef INITBUFSIZE
#undef MAXBUFSIZE
#undef INCBUFSIZE
}

#define GETC(f) getc(f)
#define FLOCKFILE(f)
#define FUNLOCKFILE(f)

static PyObject *get_line(PyFileObject *f, int n)
{
	FILE *fp = f->f_fp;
	int c;
	char *buf, *end;
	size_t n1, n2;
	PyObject *v;

	if (n <= 0)
	{
		return getline_via_fgets(fp);
	}
	n2 = n > 0 ? n : 100;
	v = PyString_FromStringAndSize((char *)NULL, n2);
	if (v == NULL)
	{
		return NULL;
	}
	buf = BUF(v);
	end = buf + n2;

	for (;;) 
	{
		PyThreadState *_save; 
		_save = PyEval_SaveThread();

		FLOCKFILE(fp);
		while ((c = GETC(fp)) != EOF &&
		       (*buf++ = c) != '\n' &&
			buf != end)
		{
			;
		}
		FUNLOCKFILE(fp);

		PyEval_RestoreThread(_save); 
		
			
		if (c == '\n')
		{
			break;
		}
		if (c == EOF) 
		{
			if (ferror(fp)) 
			{
				PyErr_SetFromErrno(PyExc_IOError);
				clearerr(fp);
				Py_DECREF(v);
				return NULL;
			}
			clearerr(fp);
			if (PyErr_CheckSignals()) 
			{
				Py_DECREF(v);
				return NULL;
			}
			break;
		}
		if (n > 0)
		{
			break;
		}
		n1 = n2;
		n2 += 1000;
		if (n2 > INT_MAX) 
		{
			PyErr_SetString(PyExc_OverflowError,
			    "line is longer than a Python string can hold");
			Py_DECREF(v);
			return NULL;
		}
		if (_PyString_Resize(&v, n2) < 0)
		{
			return NULL;
		}
		buf = BUF(v) + n1;
		end = BUF(v) + n2;
	}

	n1 = buf - BUF(v);
	if (n1 != n2)
	{
		_PyString_Resize(&v, n1);
	}
	return v;
}

PyObject *PyFile_GetLine(PyObject *f, int n)
{
	PyObject *result;

	if (f == NULL) 
	{
		PyErr_BadInternalCall();
		return NULL;
	}

	if (PyFile_Check(f)) 
	{
		if (((PyFileObject*)f)->f_fp == NULL)
		{
			return err_closed();
		}
		result = get_line((PyFileObject *)f, n);
	}
	else 
	{
		PyObject *reader;
		PyObject *args;

		reader = PyObject_GetAttrString(f, "readline");
		if (reader == NULL)
		{
			return NULL;
		}
		if (n <= 0)
		{
			args = Py_BuildValue("()");
		}
		else
		{
			args = Py_BuildValue("(i)", n);
		}
		if (args == NULL) 
		{
			Py_DECREF(reader);
			return NULL;
		}
		result = PyEval_CallObject(reader, args);
		Py_DECREF(reader);
		Py_DECREF(args);
		if (result != NULL && !PyString_Check(result)) 
		{
			Py_DECREF(result);
			result = NULL;
			PyErr_SetString(PyExc_TypeError,
				   "object.readline() returned non-string");
		}
	}

	if (n < 0 && result != NULL && PyString_Check(result)) 
	{
		char *s = PyString_AS_STRING(result);
		int len = PyString_GET_SIZE(result);
		if (len == 0) 
		{
			Py_DECREF(result);
			result = NULL;
			PyErr_SetString(PyExc_EOFError,
					"EOF when reading a line");
		}
		else if (s[len-1] == '\n') 
		{
			if (result->ob_refcnt == 1)
			{
				_PyString_Resize(&result, len-1);
			}
			else 
			{
				PyObject *v;
				v = PyString_FromStringAndSize(s, len-1);
				Py_DECREF(result);
				result = v;
			}
		}
	}
	return result;
}

static PyObject *file_readline(PyFileObject *f, PyObject *args)
{
	int n = -1;

	if (f->f_fp == NULL)
	{
		return err_closed();
	}
	if (!PyArg_ParseTuple(args, "|i:readline", &n))
	{
		return NULL;
	}
	if (n == 0)
	{
		return PyString_FromString("");
	}
	if (n < 0)
	{
		n = 0;
	}
	return get_line(f, n);
}

static PyObject *file_xreadlines(PyFileObject *f)
{
	static PyObject* xreadlines_function = NULL;

	if (!xreadlines_function) 
	{
		PyObject *xreadlines_module =
			PyImport_ImportModule("xreadlines");
		if (!xreadlines_module)
		{
			return NULL;
		}

		xreadlines_function = PyObject_GetAttrString(xreadlines_module,
							     "xreadlines");
		Py_DECREF(xreadlines_module);
		if (!xreadlines_function)
		{
			return NULL;
		}
	}
	return PyObject_CallFunction(xreadlines_function, "(O)", f);
}

static PyObject *file_readlines(PyFileObject *f, PyObject *args)
{
	long sizehint = 0;
	PyObject *list;
	PyObject *line;
	char small_buffer[SMALLCHUNK];
	char *buffer = small_buffer;
	size_t buffersize = SMALLCHUNK;
	PyObject *big_buffer = NULL;
	size_t nfilled = 0;
	size_t nread;
	size_t totalread = 0;
	char *p, *q, *end;
	int err;
	int shortread = 0;

	if (f->f_fp == NULL)
	{
		return err_closed();
	}
	if (!PyArg_ParseTuple(args, "|l:readlines", &sizehint))
	{
		return NULL;
	}
	if ((list = PyList_New(0)) == NULL)
	{
		return NULL;
	}
	for (;;) 
	{
		if (shortread)
		{
			nread = 0;
		}
		else 
		{
			PyThreadState *_save; 
			_save = PyEval_SaveThread();

			errno = 0;
			nread = fread(buffer+nfilled, 1,
				      buffersize-nfilled, f->f_fp);

			PyEval_RestoreThread(_save); 

			shortread = (nread < buffersize-nfilled);
		}

		if (nread == 0) 
		{
			sizehint = 0;
			if (!ferror(f->f_fp))
			{
				break;
			}
			PyErr_SetFromErrno(PyExc_IOError);
			clearerr(f->f_fp);
error:
			Py_DECREF(list);
			list = NULL;
			goto cleanup;
		}
		totalread += nread;
		p = memchr(buffer+nfilled, '\n', nread);
		if (p == NULL) 
		{
			nfilled += nread;
			buffersize *= 2;
			if (buffersize > INT_MAX) 
			{
				PyErr_SetString(PyExc_OverflowError,
			    "line is longer than a Python string can hold");
				goto error;
			}
			if (big_buffer == NULL) 
			{
				big_buffer = PyString_FromStringAndSize(
					NULL, buffersize);
				if (big_buffer == NULL)
				{
					goto error;
				}
				buffer = PyString_AS_STRING(big_buffer);
				memcpy(buffer, small_buffer, nfilled);
			}
			else 
			{
				_PyString_Resize(&big_buffer, buffersize);
				buffer = PyString_AS_STRING(big_buffer);
			}
			continue;
		}
		end = buffer+nfilled+nread;
		q = buffer;
		do 
		{
			p++;
			line = PyString_FromStringAndSize(q, p-q);
			if (line == NULL)
			{
				goto error;
			}
			err = PyList_Append(list, line);
			Py_DECREF(line);
			if (err != 0)
			{
				goto error;
			}
			q = p;
			p = memchr(q, '\n', end-q);
		} while (p != NULL);
		nfilled = end-q;
		memmove(buffer, q, nfilled);
		if (sizehint > 0)
		{
			if (totalread >= (size_t)sizehint)
			{
				break;
			}
		}
	}
	if (nfilled != 0) 
	{
		line = PyString_FromStringAndSize(buffer, nfilled);
		if (line == NULL)
		{
			goto error;
		}
		if (sizehint > 0) 
		{
			PyObject *rest = get_line(f, 0);
			if (rest == NULL) 
			{
				Py_DECREF(line);
				goto error;
			}
			PyString_Concat(&line, rest);
			Py_DECREF(rest);
			if (line == NULL)
			{
				goto error;
			}
		}
		err = PyList_Append(list, line);
		Py_DECREF(line);
		if (err != 0)
		{
			goto error;
		}
	}
cleanup:
	Py_XDECREF(big_buffer);
	return list;
}

static PyObject *file_write(PyFileObject *f, PyObject *args)
{
	char *s;
	int n, n2;
	if (f->f_fp == NULL)
	{
		return err_closed();
	}
	if (!PyArg_ParseTuple(args, f->f_binary ? "s#" : "t#", &s, &n))
	{
		return NULL;
	}
	f->f_softspace = 0;

	{
		PyThreadState *_save; 
		_save = PyEval_SaveThread();

		errno = 0;
		n2 = fwrite(s, 1, n, f->f_fp);
		
		PyEval_RestoreThread(_save);
	}

	if (n2 != n) 
	{
		PyErr_SetFromErrno(PyExc_IOError);
		clearerr(f->f_fp);
		return NULL;
	}
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *file_writelines(PyFileObject *f, PyObject *seq)
{
#define CHUNKSIZE 1000
	PyObject *list, *line;
	PyObject *it;
	PyObject *result;
	int i, j, index, len, nwritten, islist;

	assert(seq != NULL);
	if (f->f_fp == NULL)
	{
		return err_closed();
	}

	result = NULL;
	list = NULL;
	islist = PyList_Check(seq);
	if  (islist)
	{
		it = NULL;
	}
	else 
	{
		it = PyObject_GetIter(seq);
		if (it == NULL) 
		{
			PyErr_SetString(PyExc_TypeError,
				"writelines() requires an iterable argument");
			return NULL;
		}
		list = PyList_New(CHUNKSIZE);
		if (list == NULL)
		{
			goto error;
		}
	}

	for (index = 0; ; index += CHUNKSIZE) 
	{
		if (islist) 
		{
			Py_XDECREF(list);
			list = PyList_GetSlice(seq, index, index+CHUNKSIZE);
			if (list == NULL)
			{
				goto error;
			}
			j = PyList_GET_SIZE(list);
		}
		else 
		{
			for (j = 0; j < CHUNKSIZE; j++) 
			{
				line = PyIter_Next(it);
				if (line == NULL) 
				{
					if (PyErr_Occurred())
					{
						goto error;
					}
					break;
				}
				PyList_SetItem(list, j, line);
			}
		}
		if (j == 0)
		{
			break;
		}

		for (i = 0; i < j; i++) 
		{
			PyObject *v = PyList_GET_ITEM(list, i);
			if (!PyString_Check(v)) 
			{
			    const char *buffer;
			    int len;
				if (((f->f_binary &&
				      PyObject_AsReadBuffer(v,
					      (const void**)&buffer,
							    &len)) ||
				     PyObject_AsCharBuffer(v,
							   &buffer,
							   &len))) 
				{
					PyErr_SetString(PyExc_TypeError,
						"writelines() argument must be a sequence of strings");
					goto error;
				}
				line = PyString_FromStringAndSize(buffer,
								  len);
				if (line == NULL)
				{
					goto error;
				}
				Py_DECREF(v);
				PyList_SET_ITEM(list, i, line);
			}
		}

		{ 
			PyThreadState *_save; 
			_save = PyEval_SaveThread();
		
			f->f_softspace = 0;
			errno = 0;
			for (i = 0; i < j; i++) 
			{
				line = PyList_GET_ITEM(list, i);
				len = PyString_GET_SIZE(line);
				nwritten = fwrite(PyString_AS_STRING(line),
						  1, len, f->f_fp);
				if (nwritten != len) 
				{
					PyEval_RestoreThread(_save);

					PyErr_SetFromErrno(PyExc_IOError);
					clearerr(f->f_fp);
					goto error;
				}
			}
			
			PyEval_RestoreThread(_save); 
		}

		if (j < CHUNKSIZE)
		{
			break;
		}
	}

	Py_INCREF(Py_None);
	result = Py_None;
  error:
	Py_XDECREF(list);
  	Py_XDECREF(it);
	return result;
#undef CHUNKSIZE
}

static char readline_doc[] =
	"readline([size]) -> next line from the file, as a string.\n"
	"\n"
	"Retain newline.  A non-negative size argument limits the maximum\n"
	"number of bytes to return (an incomplete line may be returned then).\n"
	"Return an empty string at EOF.";

static char read_doc[] =
	"read([size]) -> read at most size bytes, returned as a string.\n"
	"\n"
	"If the size argument is negative or omitted, read until EOF is reached.";

static char write_doc[] =
	"write(str) -> None.  Write string str to file.\n"
	"\n"
	"Note that due to buffering, flush() or close() may be needed before\n"
	"the file on disk reflects the data written.";

static char fileno_doc[] =
	"fileno() -> integer \"file descriptor\".\n"
	"\n"
	"This is needed for lower-level file interfaces, such os.read().";

static char seek_doc[] =
	"seek(offset[, whence]) -> None.  Move to new file position.\n"
	"\n"
	"Argument offset is a byte count.  Optional argument whence defaults to\n"
	"0 (offset from start of file, offset should be >= 0); other values are 1\n"
	"(move relative to current position, positive or negative), and 2 (move\n"
	"relative to end of file, usually negative, although many platforms allow\n"
	"seeking beyond the end of a file).\n"
	"\n"
	"Note that not all file objects are seekable.";

static char truncate_doc[] =
	"truncate([size]) -> None.  Truncate the file to at most size bytes.\n"
	"\n"
	"Size defaults to the current file position, as returned by tell().";

static char tell_doc[] =
	"tell() -> current file position, an integer (may be a long integer).";

static char readinto_doc[] =
	"readinto() -> Undocumented.  Don't use this; it may go away.";

static char readlines_doc[] =
	"readlines([size]) -> list of strings, each a line from the file.\n"
	"\n"
	"Call readline() repeatedly and return a list of the lines so read.\n"
	"The optional size argument, if given, is an approximate bound on the\n"
	"total number of bytes in the lines returned.";

static char xreadlines_doc[] =
	"xreadlines() -> next line from the file, as a string.\n"
	"\n"
	"Equivalent to xreadlines.xreadlines(file).  This is like readlines(), but\n"
	"often quicker, due to reading ahead internally.";

static char writelines_doc[] =
	"writelines(sequence_of_strings) -> None.  Write the strings to the file.\n"
	"\n"
	"Note that newlines are not added.  The sequence can be any iterable object\n"
	"producing strings. This is equivalent to calling write() for each string.";

static char flush_doc[] =
	"flush() -> None.  Flush the internal I/O buffer.";

static char close_doc[] =
	"close() -> None or (perhaps) an integer.  Close the file.\n"
	"\n"
	"Sets data attribute .closed to true.  A closed file cannot be used for\n"
	"further I/O operations.  close() may be called more than once without\n"
	"error.  Some kinds of file objects (for example, opened by popen())\n"
	"may return an exit status upon closing.";

static char isatty_doc[] =
	"isatty() -> true or false.  True if the file is connected to a tty device.";

static PyMethodDef file_methods[] = {
	{"readline",	(PyCFunction)file_readline,   METH_VARARGS, readline_doc},
	{"read",	(PyCFunction)file_read,       METH_VARARGS, read_doc},
	{"write",	(PyCFunction)file_write,      METH_VARARGS, write_doc},
	{"fileno",	(PyCFunction)file_fileno,     METH_NOARGS,  fileno_doc},
	{"seek",	(PyCFunction)file_seek,       METH_VARARGS, seek_doc},
	{"tell",	(PyCFunction)file_tell,       METH_NOARGS,  tell_doc},
	{"readinto",	(PyCFunction)file_readinto,   METH_OLDARGS, readinto_doc},
	{"readlines",	(PyCFunction)file_readlines,  METH_VARARGS, readlines_doc},
	{"xreadlines",	(PyCFunction)file_xreadlines, METH_NOARGS,  xreadlines_doc},
	{"writelines",	(PyCFunction)file_writelines, METH_O,	    writelines_doc},
	{"flush",	(PyCFunction)file_flush,      METH_NOARGS,  flush_doc},
	{"close",	(PyCFunction)file_close,      METH_NOARGS,  close_doc},
	{"isatty",	(PyCFunction)file_isatty,     METH_NOARGS,  isatty_doc},
	{NULL,		NULL}
};

#define OFF(x) offsetof(PyFileObject, x)

static PyMemberDef file_memberlist[] = {
	{"softspace",	T_INT,		OFF(f_softspace), 0,
	 "flag indicating that a space needs to be printed; used by print"},
	{"mode",	T_OBJECT,	OFF(f_mode),	RO,
	 "file mode ('r', 'w', 'a', possibly with 'b' or '+' added)"},
	{"name",	T_OBJECT,	OFF(f_name),	RO,
	 "file name"},
	{NULL}
};

static PyObject *get_closed(PyFileObject *f, void *closure)
{
	return PyInt_FromLong((long)(f->f_fp == 0));
}

static PyGetSetDef file_getsetlist[] = {
	{"closed", (getter)get_closed, NULL, "flag set if the file is closed"},
	{0},
};

static PyObject *file_getiter(PyObject *f)
{
	return PyObject_CallMethod(f, "xreadlines", "");
}

static PyObject *file_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	PyObject *self;
	static PyObject *not_yet_string;

	assert(type != NULL && type->tp_alloc != NULL);

	if (not_yet_string == NULL) 
	{
		not_yet_string = PyString_FromString("<uninitialized file>");
		if (not_yet_string == NULL)
		{
			return NULL;
		}
	}

	self = type->tp_alloc(type, 0);
	if (self != NULL) 
	{
		Py_INCREF(not_yet_string);
		((PyFileObject *)self)->f_name = not_yet_string;
		Py_INCREF(not_yet_string);
		((PyFileObject *)self)->f_mode = not_yet_string;
	}
	return self;
}

static int file_init(PyObject *self, PyObject *args, PyObject *kwds)
{
	PyFileObject *foself = (PyFileObject *)self;
	int ret = 0;
	static char *kwlist[] = {"name", "mode", "buffering", 0};
	char *name = NULL;
	char *mode = "r";
	int bufsize = -1;

	assert(PyFile_Check(self));
	if (foself->f_fp != NULL) 
	{
		PyObject *closeresult = file_close(foself);
		if (closeresult == NULL)
			return -1;
		Py_DECREF(closeresult);
	}

	if (!PyArg_ParseTupleAndKeywords(args, kwds, "et|si:file", kwlist,
					 Py_FileSystemDefaultEncoding, &name,
					 &mode, &bufsize))
	{
		return -1;
	}
	if (fill_file_fields(foself, NULL, name, mode, fclose) == NULL)
	{
		goto Error;
	}
	if (open_the_file(foself, name, mode) == NULL)
	{
		goto Error;
	}
	PyFile_SetBufSize(self, bufsize);
	goto Done;

Error:
	ret = -1;
Done:
	PyMem_Free(name); 
	return ret;
}

static char file_doc[] =
	"file(name[, mode[, buffering]]) -> file object\n"
	"\n"
	"Open a file.  The mode can be 'r', 'w' or 'a' for reading (default),\n"
	"writing or appending.  The file will be created if it doesn't exist\n"
	"when opened for writing or appending; it will be truncated when\n"
	"opened for writing.  Add a 'b' to the mode for binary files.\n"
	"Add a '+' to the mode to allow simultaneous reading and writing.\n"
	"If the buffering argument is given, 0 means unbuffered, 1 means line\n"
	"buffered, and larger numbers specify the buffer size.\n"
	"Note:  open() is an alias for file().\n";

PyTypeObject PyFile_Type = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,
	"file",
	sizeof(PyFileObject),
	0,
	(destructor)file_dealloc,		
	0,					
	0,			 		
	0,			 		
	0,					
	(reprfunc)file_repr,
	0,					
	0,					
	0,					
	0,					
	0,					
	0,					
	PyObject_GenericGetAttr,
	0,					
	0,					
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, 
	file_doc,			
	0,					
	0,					
	0,					
	0,					
	file_getiter,		
	0,					
	file_methods,		
	file_memberlist,	
	file_getsetlist,	
	0,					
	0,					
	0,					
	0,					
	0,					
	(initproc)file_init,	
	PyType_GenericAlloc,	
	file_new,				
	_PyObject_Del,			
};

int PyFile_SoftSpace(PyObject *f, int newflag)
{
	int oldflag = 0;
	if (f == NULL) 
	{
		;
	}
	else if (PyFile_Check(f)) 
	{
		oldflag = ((PyFileObject *)f)->f_softspace;
		((PyFileObject *)f)->f_softspace = newflag;
	}
	else 
	{
		PyObject *v;
		v = PyObject_GetAttrString(f, "softspace");
		if (v == NULL)
		{
			PyErr_Clear();
		}
		else 
		{
			if (PyInt_Check(v))
			{
				oldflag = PyInt_AsLong(v);
			}
			Py_DECREF(v);
		}
		v = PyInt_FromLong((long)newflag);
		if (v == NULL)
		{
			PyErr_Clear();
		}
		else 
		{
			if (PyObject_SetAttrString(f, "softspace", v) != 0)
			{
				PyErr_Clear();
			}
			Py_DECREF(v);
		}
	}
	return oldflag;
}

int PyFile_WriteObject(PyObject *v, PyObject *f, int flags)
{
	PyObject *writer, *value, *args, *result;
	if (f == NULL) 
	{
		PyErr_SetString(PyExc_TypeError, "writeobject with NULL file");
		return -1;
	}
	else if (PyFile_Check(f)) 
	{
		FILE *fp = PyFile_AsFile(f);
		if (fp == NULL) 
		{
			err_closed();
			return -1;
		}
		return PyObject_Print(v, fp, flags);
	}
	writer = PyObject_GetAttrString(f, "write");
	if (writer == NULL)
	{
		return -1;
	}
	if (flags & Py_PRINT_RAW) 
	{
        if (PyUnicode_Check(v)) 
		{
			value = v;
            Py_INCREF(value);
        } 
		else
		{
			value = PyObject_Str(v);
		}
	}
    else
	{
		value = PyObject_Repr(v);
	}
	if (value == NULL) 
	{
		Py_DECREF(writer);
		return -1;
	}
	args = Py_BuildValue("(O)", value);
	if (args == NULL) 
	{
		Py_DECREF(value);
		Py_DECREF(writer);
		return -1;
	}
	result = PyEval_CallObject(writer, args);
	Py_DECREF(args);
	Py_DECREF(value);
	Py_DECREF(writer);
	if (result == NULL)
	{
		return -1;
	}
	Py_DECREF(result);
	return 0;
}

int PyFile_WriteString(const char *s, PyObject *f)
{
	if (f == NULL) 
	{
		if (!PyErr_Occurred()) 
		{
			PyErr_SetString(PyExc_SystemError,
					"null file for PyFile_WriteString");
		}
		return -1;
	}
	else if (PyFile_Check(f)) 
	{
		FILE *fp = PyFile_AsFile(f);
		if (fp == NULL) 
		{
			err_closed();
			return -1;
		}
		fputs(s, fp);
		return 0;
	}
	else if (!PyErr_Occurred()) 
	{
		PyObject *v = PyString_FromString(s);
		int err;
		if (v == NULL)
		{
			return -1;
		}
		err = PyFile_WriteObject(v, f, Py_PRINT_RAW);
		Py_DECREF(v);
		return err;
	}
	else
	{
		return -1;
	}
}

int PyObject_AsFileDescriptor(PyObject *o)
{
	int fd;
	PyObject *meth;

	if (PyInt_Check(o)) 
	{
		fd = PyInt_AsLong(o);
	}
	else if (PyLong_Check(o)) 
	{
		fd = PyLong_AsLong(o);
	}
	else if ((meth = PyObject_GetAttrString(o, "fileno")) != NULL)
	{
		PyObject *fno = PyEval_CallObject(meth, NULL);
		Py_DECREF(meth);
		if (fno == NULL)
		{
			return -1;
		}

		if (PyInt_Check(fno)) 
		{
			fd = PyInt_AsLong(fno);
			Py_DECREF(fno);
		}
		else if (PyLong_Check(fno)) 
		{
			fd = PyLong_AsLong(fno);
			Py_DECREF(fno);
		}
		else 
		{
			PyErr_SetString(PyExc_TypeError,
					"fileno() returned a non-integer");
			Py_DECREF(fno);
			return -1;
		}
	}
	else 
	{
		PyErr_SetString(PyExc_TypeError,
				"argument must be an int, or have a fileno() method.");
		return -1;
	}

	if (fd < 0) 
	{
		PyErr_Format(PyExc_ValueError,
			     "file descriptor cannot be a negative integer (%i)",
			     fd);
		return -1;
	}
	return fd;
}
