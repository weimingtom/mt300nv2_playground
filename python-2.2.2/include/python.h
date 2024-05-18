//20180324
#pragma once

#define _GNU_SOURCE	1

#include "patchlevel.h"
#include "pyconfig.h"

#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <assert.h>

#include "pyport.h"
#include "pymem.h"
#include "object.h"
#include "objimpl.h"
#include "pydebug.h"
#include "unicodeobject.h"
#include "intobject.h"
#include "longobject.h"
#include "floatobject.h"
#include "complexobject.h"
#include "rangeobject.h"
#include "stringobject.h"
#include "bufferobject.h"
#include "tupleobject.h"
#include "listobject.h"
#include "dictobject.h"
#include "methodobject.h"
#include "moduleobject.h"
#include "funcobject.h"
#include "classobject.h"
#include "fileobject.h"
#include "cobject.h"
#include "traceback.h"
#include "sliceobject.h"
#include "cellobject.h"
#include "iterobject.h"
#include "descrobject.h"
#include "weakrefobject.h"
#include "codecs.h"
#include "pyerrors.h"
#include "pystate.h"
#include "modsupport.h"
#include "pythonrun.h"
#include "ceval.h"
#include "sysmodule.h"
#include "intrcheck.h"
#include "import.h"
#include "abstract.h"

#define PyArg_GetInt(v, a) PyArg_Parse((v), "i", (a))
#define PyArg_NoArgs(v) PyArg_Parse(v, "")
#define Py_CHARMASK(c) ((c) & 0xff)

#include "pyfpe.h"

#define Py_single_input 256
#define Py_file_input 257
#define Py_eval_input 258
