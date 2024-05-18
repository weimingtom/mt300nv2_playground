//20180324
#pragma once

struct _frame;

int PyTraceBack_Here(struct _frame *);
int PyTraceBack_Print(PyObject *, PyObject *);
extern PyTypeObject PyTraceBack_Type;
#define PyTraceBack_Check(v) ((v)->ob_type == &PyTraceBack_Type)
