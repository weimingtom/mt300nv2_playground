//20180111
#include "python.h"

static unsigned char M___hello__[] = {
	99,0,0,0,0,1,0,0,0,115,15,0,0,0,127,0,
	0,127,1,0,100,0,0,71,72,100,1,0,83,40,2,0,
	0,0,115,14,0,0,0,72,101,108,108,111,32,119,111,114,
	108,100,46,46,46,78,40,0,0,0,0,40,0,0,0,0,
	40,0,0,0,0,40,0,0,0,0,115,8,0,0,0,104,
	101,108,108,111,46,112,121,115,1,0,0,0,63,1,0,115,
	0,0,0,0,
};

#define SIZE (int)sizeof(M___hello__)

static struct _frozen _PyImport_FrozenModules[] = {
	{"__hello__", M___hello__, SIZE},
	{"__phello__", M___hello__, -SIZE},
	{"__phello__.spam", M___hello__, SIZE},
	{0, 0, 0}
};

struct _frozen *PyImport_FrozenModules = _PyImport_FrozenModules;
