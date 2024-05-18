//20171119
//20180109
#include "python.h"
#include "importdl.h"

const struct filedescr _PyImport_DynLoadFiletab[] = {
#ifdef _DEBUG
	{"_d.pyd", "rb", C_EXTENSION},
	{"_d.dll", "rb", C_EXTENSION},
#else
	{".pyd", "rb", C_EXTENSION},
	{".dll", "rb", C_EXTENSION},
#endif
	{0, 0}
};
