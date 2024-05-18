//20180119
#include "python.h"
#include "importdl.h"

PyObject *_PyImport_LoadDynamicModule(char *name, char *pathname, FILE *fp)
{
	PyObject *m;

	if ((m = _PyImport_FindExtension(name, pathname)) != NULL) 
	{
		Py_INCREF(m);
		return m;
	}
	return NULL;
}
