//20180318
#pragma once

typedef struct {
    double real;
    double imag;
} Py_complex;

#define c_sum _Py_c_sum
#define c_diff _Py_c_diff
#define c_neg _Py_c_neg
#define c_prod _Py_c_prod
#define c_quot _Py_c_quot
#define c_pow _Py_c_pow

extern Py_complex c_sum(Py_complex, Py_complex);
extern Py_complex c_diff(Py_complex, Py_complex);
extern Py_complex c_neg(Py_complex);
extern Py_complex c_prod(Py_complex, Py_complex);
extern Py_complex c_quot(Py_complex, Py_complex);
extern Py_complex c_pow(Py_complex, Py_complex);

typedef struct {
    PyObject_HEAD
    Py_complex cval;
} PyComplexObject;     

extern PyTypeObject PyComplex_Type;

#define PyComplex_Check(op) PyObject_TypeCheck(op, &PyComplex_Type)
#define PyComplex_CheckExact(op) ((op)->ob_type == &PyComplex_Type)

extern PyObject * PyComplex_FromCComplex(Py_complex);
extern PyObject * PyComplex_FromDoubles(double real, double imag);

extern double PyComplex_RealAsDouble(PyObject *op);
extern double PyComplex_ImagAsDouble(PyObject *op);
extern Py_complex PyComplex_AsCComplex(PyObject *op);
