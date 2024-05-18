//20180318
#pragma once

#define PyObject_DelAttrString(O,A) PyObject_SetAttrString((O), (A), NULL)
#define PyObject_DelAttr(O,A) PyObject_SetAttr((O), (A), NULL)
int PyObject_Cmp(PyObject *o1, PyObject *o2, int *result);

int PyCallable_Check(PyObject *o);

PyObject * PyObject_Call(PyObject *callable_object, PyObject *args, PyObject *kw);
PyObject * PyObject_CallObject(PyObject *callable_object, PyObject *args);
PyObject * PyObject_CallFunction(PyObject *callable_object, char *format, ...);
PyObject * PyObject_CallMethod(PyObject *o, char *m, char *format, ...);
PyObject * PyObject_CallFunctionObjArgs(PyObject *callable, ...);
PyObject * PyObject_CallMethodObjArgs(PyObject *o, PyObject *m, ...);
PyObject * PyObject_Type(PyObject *o);
int PyObject_Size(PyObject *o);
#undef PyObject_Length
int PyObject_Length(PyObject *o);
#define PyObject_Length PyObject_Size
PyObject * PyObject_GetItem(PyObject *o, PyObject *key);
int PyObject_SetItem(PyObject *o, PyObject *key, PyObject *v);
int PyObject_DelItemString(PyObject *o, char *key);
int PyObject_DelItem(PyObject *o, PyObject *key);
int PyObject_AsCharBuffer(PyObject *obj, const char **buffer, int *buffer_len);
int PyObject_CheckReadBuffer(PyObject *obj);
int PyObject_AsReadBuffer(PyObject *obj, const void **buffer, int *buffer_len);
int PyObject_AsWriteBuffer(PyObject *obj, void **buffer, int *buffer_len);
PyObject * PyObject_GetIter(PyObject *);

#define PyIter_Check(obj) (PyType_HasFeature((obj)->ob_type, Py_TPFLAGS_HAVE_ITER) && (obj)->ob_type->tp_iternext != NULL)
PyObject * PyIter_Next(PyObject *);

int PyNumber_Check(PyObject *o);
PyObject * PyNumber_Add(PyObject *o1, PyObject *o2);
PyObject * PyNumber_Subtract(PyObject *o1, PyObject *o2);
PyObject * PyNumber_Multiply(PyObject *o1, PyObject *o2);
PyObject * PyNumber_Divide(PyObject *o1, PyObject *o2);
PyObject * PyNumber_FloorDivide(PyObject *o1, PyObject *o2);
PyObject * PyNumber_TrueDivide(PyObject *o1, PyObject *o2);
PyObject * PyNumber_Remainder(PyObject *o1, PyObject *o2);
PyObject * PyNumber_Divmod(PyObject *o1, PyObject *o2);
PyObject * PyNumber_Power(PyObject *o1, PyObject *o2, PyObject *o3);
PyObject * PyNumber_Negative(PyObject *o);
PyObject * PyNumber_Positive(PyObject *o);
PyObject * PyNumber_Absolute(PyObject *o);
PyObject * PyNumber_Invert(PyObject *o);
PyObject * PyNumber_Lshift(PyObject *o1, PyObject *o2);
PyObject * PyNumber_Rshift(PyObject *o1, PyObject *o2);
PyObject * PyNumber_And(PyObject *o1, PyObject *o2);
PyObject * PyNumber_Xor(PyObject *o1, PyObject *o2);
PyObject * PyNumber_Or(PyObject *o1, PyObject *o2);
PyObject * PyNumber_Int(PyObject *o);
PyObject * PyNumber_Long(PyObject *o);
PyObject * PyNumber_Float(PyObject *o);
PyObject * PyNumber_InPlaceAdd(PyObject *o1, PyObject *o2);
PyObject * PyNumber_InPlaceSubtract(PyObject *o1, PyObject *o2);
PyObject * PyNumber_InPlaceMultiply(PyObject *o1, PyObject *o2);
PyObject * PyNumber_InPlaceDivide(PyObject *o1, PyObject *o2);
PyObject * PyNumber_InPlaceFloorDivide(PyObject *o1, PyObject *o2);
PyObject * PyNumber_InPlaceTrueDivide(PyObject *o1, PyObject *o2);
PyObject * PyNumber_InPlaceRemainder(PyObject *o1, PyObject *o2);
PyObject * PyNumber_InPlacePower(PyObject *o1, PyObject *o2, PyObject *o3);
PyObject * PyNumber_InPlaceLshift(PyObject *o1, PyObject *o2);
PyObject * PyNumber_InPlaceRshift(PyObject *o1, PyObject *o2);
PyObject * PyNumber_InPlaceAnd(PyObject *o1, PyObject *o2);
PyObject * PyNumber_InPlaceXor(PyObject *o1, PyObject *o2);
PyObject * PyNumber_InPlaceOr(PyObject *o1, PyObject *o2);

int PySequence_Check(PyObject *o);
int PySequence_Size(PyObject *o);
#undef PySequence_Length
int PySequence_Length(PyObject *o);
#define PySequence_Length PySequence_Size
PyObject * PySequence_Concat(PyObject *o1, PyObject *o2);
PyObject * PySequence_Repeat(PyObject *o, int count);
PyObject * PySequence_GetItem(PyObject *o, int i);
PyObject * PySequence_GetSlice(PyObject *o, int i1, int i2);
int PySequence_SetItem(PyObject *o, int i, PyObject *v);
int PySequence_DelItem(PyObject *o, int i);
int PySequence_SetSlice(PyObject *o, int i1, int i2, PyObject *v);
int PySequence_DelSlice(PyObject *o, int i1, int i2);
PyObject * PySequence_Tuple(PyObject *o);
PyObject * PySequence_List(PyObject *o);
PyObject * PySequence_Fast(PyObject *o, const char* m);
#define PySequence_Fast_GET_SIZE(o) (PyList_Check(o) ? PyList_GET_SIZE(o) : PyTuple_GET_SIZE(o))
#define PySequence_Fast_GET_ITEM(o, i) (PyList_Check(o) ? PyList_GET_ITEM(o, i) : PyTuple_GET_ITEM(o, i))
int PySequence_Count(PyObject *o, PyObject *value);
int PySequence_Contains(PyObject *seq, PyObject *ob);
#define PY_ITERSEARCH_COUNT 1
#define PY_ITERSEARCH_INDEX 2
#define PY_ITERSEARCH_CONTAINS 3
int _PySequence_IterSearch(PyObject *seq, PyObject *obj, int operation);
#undef PySequence_In
int PySequence_In(PyObject *o, PyObject *value);
#define PySequence_In PySequence_Contains
int PySequence_Index(PyObject *o, PyObject *value);
PyObject * PySequence_InPlaceConcat(PyObject *o1, PyObject *o2);
PyObject * PySequence_InPlaceRepeat(PyObject *o, int count);

int PyMapping_Check(PyObject *o);
int PyMapping_Size(PyObject *o);
#undef PyMapping_Length
int PyMapping_Length(PyObject *o);
#define PyMapping_Length PyMapping_Size
#define PyMapping_DelItemString(O,K) PyObject_DelItemString((O), (K))
#define PyMapping_DelItem(O,K) PyObject_DelItem((O), (K))
int PyMapping_HasKeyString(PyObject *o, char *key);
#define PyMapping_Keys(O) PyObject_CallMethod(O, "keys", NULL)
#define PyMapping_Values(O) PyObject_CallMethod(O, "values", NULL)
#define PyMapping_Items(O) PyObject_CallMethod(O, "items", NULL)
PyObject * PyMapping_GetItemString(PyObject *o, char *key);
int PyMapping_SetItemString(PyObject *o, char *key, PyObject *value);

int PyObject_IsInstance(PyObject *object, PyObject *typeorclass);
int PyObject_IsSubclass(PyObject *object, PyObject *typeorclass);

