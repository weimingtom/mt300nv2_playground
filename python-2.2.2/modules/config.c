//20180406

#include "python.h"

extern void initthread();
extern void initsignal();
extern void initposix();
extern void initerrno();
extern void init_sre();
extern void initnew();
extern void init_symtable();
extern void initxxsubtype();

extern void PyMarshal_Init();
extern void initimp();
extern void initgc();

struct _inittab _PyImport_Inittab[] = {
	{"thread", initthread},
	{"signal", initsignal},
	{"errno", initerrno},
	{"_sre", init_sre},
	{"new", initnew},
	{"_symtable", init_symtable},
	{"xxsubtype", initxxsubtype},

	{"marshal", PyMarshal_Init},

	{"imp", initimp},

	{"__main__", NULL},
	{"__builtin__", NULL},
	{"sys", NULL},
	{"exceptions", NULL},

	{"gc", initgc},

	{0, 0}
};
