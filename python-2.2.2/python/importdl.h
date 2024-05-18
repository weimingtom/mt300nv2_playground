//20180119
#pragma once

enum filetype {
	SEARCH_ERROR,
	PY_SOURCE,
	PY_COMPILED,
	C_EXTENSION,
	PY_RESOURCE,
	PKG_DIRECTORY,
	C_BUILTIN,
	PY_FROZEN,
	PY_CODERESOURCE
};

struct filedescr {
	char *suffix;
	char *mode;
	enum filetype type;
};

extern struct filedescr * _PyImport_Filetab;
extern const struct filedescr _PyImport_DynLoadFiletab[];
extern PyObject *_PyImport_LoadDynamicModule(char *name, char *pathname, FILE *);

#define MAXSUFFIXSIZE 12
