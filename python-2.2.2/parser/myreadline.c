//20170403

#include "python.h"

int (*PyOS_InputHook)() = NULL;

static int my_fgets(char *buf, int len, FILE *fp)
{
	char *p;
	for (;;) 
	{
		if (PyOS_InputHook != NULL)
		{
			(PyOS_InputHook)();
		}
		errno = 0;
		p = fgets(buf, len, fp);
		if (p != NULL)
		{
			return 0;
		}
		if (feof(fp)) 
		{
			return -1;
		}
		if (errno == EINTR) 
		{
			if (PyOS_InterruptOccurred()) 
			{
				return 1;
			}
			continue;
		}
		if (PyOS_InterruptOccurred()) 
		{
			return 1;
		}
		return -2;
	}
}

char *PyOS_StdioReadline(char *prompt)
{
	size_t n;
	char *p;

	n = 100;
	if ((p = PyMem_MALLOC(n)) == NULL)
	{
		return NULL;
	}
	fflush(stdout);
	if (prompt)
	{
		fprintf(stderr, "%s", prompt);
	}
	fflush(stderr);
	switch (my_fgets(p, (int)n, stdin)) 
	{
	case 0:
		break;
	
	case 1:
		PyMem_FREE(p);
		return NULL;
	
	case -1:
	case -2:
	default:
		*p = '\0';
		break;
	}
	n = strlen(p);
	while (n > 0 && p[n-1] != '\n') 
	{
		size_t incr = n+2;
		p = PyMem_REALLOC(p, n + incr);
		if (p == NULL)
		{
			return NULL;
		}
		if (incr > INT_MAX) 
		{
			PyErr_SetString(PyExc_OverflowError, "input line too long");
		}
		if (my_fgets(p+n, (int)incr, stdin) != 0)
		{
			break;
		}
		n += strlen(p+n);
	}
	return PyMem_REALLOC(p, n+1);
}

char *(*PyOS_ReadlineFunctionPointer)(char *);

char *PyOS_Readline(char *prompt)
{
	char *rv;
	if (PyOS_ReadlineFunctionPointer == NULL) 
	{
		PyOS_ReadlineFunctionPointer = PyOS_StdioReadline;
	}

	{ 
		PyThreadState *_save; 
		_save = PyEval_SaveThread();
	
		rv = (*PyOS_ReadlineFunctionPointer)(prompt);
		
		PyEval_RestoreThread(_save); 
	}
	
	return rv;
}
