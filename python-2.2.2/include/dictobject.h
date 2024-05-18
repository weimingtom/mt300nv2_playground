//20180318
#pragma once

#define PyDict_MINSIZE 8

typedef struct {
	long me_hash;
	PyObject *me_key;
	PyObject *me_value;
} PyDictEntry;

typedef struct _dictobject PyDictObject;
struct _dictobject {
	PyObject_HEAD
	int ma_fill;
	int ma_used;
	int ma_mask;
	PyDictEntry *ma_table;
	PyDictEntry *(*ma_lookup)(PyDictObject *mp, PyObject *key, long hash);
	PyDictEntry ma_smalltable[PyDict_MINSIZE];
};

extern PyTypeObject PyDict_Type;
#define PyDict_Check(op) PyObject_TypeCheck(op, &PyDict_Type)
extern PyObject * PyDict_New();
extern PyObject * PyDict_GetItem(PyObject *mp, PyObject *key);
extern int PyDict_SetItem(PyObject *mp, PyObject *key, PyObject *item);
extern int PyDict_DelItem(PyObject *mp, PyObject *key);
extern void PyDict_Clear(PyObject *mp);
extern int PyDict_Next(PyObject *mp, int *pos, PyObject **key, PyObject **value);
extern PyObject * PyDict_Keys(PyObject *mp);
extern PyObject * PyDict_Values(PyObject *mp);
extern PyObject * PyDict_Items(PyObject *mp);
extern int PyDict_Size(PyObject *mp);
extern PyObject * PyDict_Copy(PyObject *mp);
extern int PyDict_Update(PyObject *mp, PyObject *other);
extern int PyDict_Merge(PyObject *mp, PyObject *other, int override);
extern int PyDict_MergeFromSeq2(PyObject *d, PyObject *seq2, int override);
extern PyObject * PyDict_GetItemString(PyObject *dp, char *key);
extern int PyDict_SetItemString(PyObject *dp, char *key, PyObject *item);
extern int PyDict_DelItemString(PyObject *dp, char *key);
