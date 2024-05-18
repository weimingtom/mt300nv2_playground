//20180406
#include "python.h"

#include <stdio.h>

#define DATE __DATE__

#define TIME __TIME__

#define BUILD 0

const char *Py_GetBuildInfo()
{
	static char buildinfo[50];
	PyOS_snprintf(buildinfo, sizeof(buildinfo),
		      "#%d, %.20s, %.9s", BUILD, DATE, TIME);
	return buildinfo;
}
