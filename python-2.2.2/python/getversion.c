//20180118
#include "python.h"
#include "patchlevel.h"

const char *Py_GetVersion()
{
	static char version[250];
	PyOS_snprintf(version, sizeof(version), "%.80s (%.80s) %.80s", 
		      PY_VERSION, Py_GetBuildInfo(), Py_GetCompiler());
	return version;
}
