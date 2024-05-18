//20180318
#pragma once

PyObject * PyEval_EvalCode(PyCodeObject *, PyObject *, PyObject *);
PyObject * PyEval_EvalCodeEx(PyCodeObject *co, PyObject *globals, PyObject *locals, PyObject **args, int argc, PyObject **kwds, int kwdc, PyObject **defs, int defc, PyObject *closure);

