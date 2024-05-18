//20180324
#pragma once

extern PyTypeObject PyRange_Type;

#define PyRange_Check(op) ((op)->ob_type == &PyRange_Type)

extern PyObject * PyRange_New(long, long, long, int);
