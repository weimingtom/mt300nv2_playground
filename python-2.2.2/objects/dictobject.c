//20170413
#include "python.h"

typedef PyDictEntry dictentry;
typedef PyDictObject dictobject;

#undef SHOW_CONVERSION_COUNTS

#define PERTURB_SHIFT 5

static PyObject *dummy;

static dictentry *lookdict_string(dictobject *mp, PyObject *key, long hash);

#define INIT_NONZERO_DICT_SLOTS(mp) do {				\
	(mp)->ma_table = (mp)->ma_smalltable;				\
	(mp)->ma_mask = PyDict_MINSIZE - 1;				\
    } while(0)

#define EMPTY_TO_MINSIZE(mp) do {					\
	memset((mp)->ma_smalltable, 0, sizeof((mp)->ma_smalltable));	\
	(mp)->ma_used = (mp)->ma_fill = 0;				\
	INIT_NONZERO_DICT_SLOTS(mp);					\
    } while(0)

PyObject *PyDict_New()
{
	dictobject *mp;
	if (dummy == NULL) 
	{ 
		dummy = PyString_FromString("<dummy key>");
		if (dummy == NULL)
		{
			return NULL;
		}
	}
	mp = PyObject_GC_New(dictobject, &PyDict_Type);
	if (mp == NULL)
		return NULL;
	EMPTY_TO_MINSIZE(mp);
	mp->ma_lookup = lookdict_string;
	_PyObject_GC_TRACK(mp);
	return (PyObject *)mp;
}

static dictentry *lookdict(dictobject *mp, PyObject *key, long hash)
{
	int i;
	unsigned int perturb;
	dictentry *freeslot;
	unsigned int mask = mp->ma_mask;
	dictentry *ep0 = mp->ma_table;
	dictentry *ep;
	int restore_error;
	int checked_error;
	int cmp;
	PyObject *err_type, *err_value, *err_tb;
	PyObject *startkey;

	i = hash & mask;
	ep = &ep0[i];
	if (ep->me_key == NULL || ep->me_key == key)
	{
		return ep;
	}

	restore_error = checked_error = 0;
	if (ep->me_key == dummy)
	{
		freeslot = ep;
	}
	else 
	{
		if (ep->me_hash == hash) 
		{
			checked_error = 1;
			if (PyErr_Occurred()) 
			{
				restore_error = 1;
				PyErr_Fetch(&err_type, &err_value, &err_tb);
			}
			startkey = ep->me_key;
			cmp = PyObject_RichCompareBool(startkey, key, Py_EQ);
			if (cmp < 0)
			{
				PyErr_Clear();
			}
			if (ep0 == mp->ma_table && ep->me_key == startkey) 
			{
				if (cmp > 0)
				{
					goto Done;
				}
			}
			else 
			{
				ep = lookdict(mp, key, hash);
 				goto Done;
 			}
		}
		freeslot = NULL;
	}

	for (perturb = hash; ; perturb >>= PERTURB_SHIFT) 
	{
		i = (i << 2) + i + perturb + 1;
		ep = &ep0[i & mask];
		if (ep->me_key == NULL) 
		{
			if (freeslot != NULL)
			{
				ep = freeslot;
			}
			break;
		}
		if (ep->me_key == key)
		{
			break;
		}
		if (ep->me_hash == hash && ep->me_key != dummy) 
		{
			if (!checked_error) 
			{
				checked_error = 1;
				if (PyErr_Occurred()) 
				{
					restore_error = 1;
					PyErr_Fetch(&err_type, &err_value,
						    &err_tb);
				}
			}
			startkey = ep->me_key;
			cmp = PyObject_RichCompareBool(startkey, key, Py_EQ);
			if (cmp < 0)
			{
				PyErr_Clear();
			}
			if (ep0 == mp->ma_table && ep->me_key == startkey) 
			{
				if (cmp > 0)
				{
					break;
				}
			}
			else 
			{
				ep = lookdict(mp, key, hash);
 				break;
 			}
		}
		else if (ep->me_key == dummy && freeslot == NULL)
		{
			freeslot = ep;
		}
	}

Done:
	if (restore_error)
	{
		PyErr_Restore(err_type, err_value, err_tb);
	}
	return ep;
}

static dictentry *lookdict_string(dictobject *mp, PyObject *key, long hash)
{
	int i;
	unsigned int perturb;
	dictentry *freeslot;
	unsigned int mask = mp->ma_mask;
	dictentry *ep0 = mp->ma_table;
	dictentry *ep;

	if (!PyString_CheckExact(key)) 
	{
		mp->ma_lookup = lookdict;
		return lookdict(mp, key, hash);
	}
	i = hash & mask;
	ep = &ep0[i];
	if (ep->me_key == NULL || ep->me_key == key)
	{
		return ep;
	}
	if (ep->me_key == dummy)
	{
		freeslot = ep;
	}
	else 
	{
		if (ep->me_hash == hash
		    && _PyString_Eq(ep->me_key, key)) 
		{
			return ep;
		}
		freeslot = NULL;
	}

	for (perturb = hash; ; perturb >>= PERTURB_SHIFT) 
	{
		i = (i << 2) + i + perturb + 1;
		ep = &ep0[i & mask];
		if (ep->me_key == NULL)
		{
			return freeslot == NULL ? ep : freeslot;
		}
		if (ep->me_key == key
		    || (ep->me_hash == hash
		        && ep->me_key != dummy
			&& _PyString_Eq(ep->me_key, key)))
		{
			return ep;
		}
		if (ep->me_key == dummy && freeslot == NULL)
		{
			freeslot = ep;
		}
	}
}

static void insertdict(dictobject *mp, PyObject *key, long hash, PyObject *value)
{
	PyObject *old_value;
	dictentry *ep;
	typedef PyDictEntry *(*lookupfunc)(PyDictObject *, PyObject *, long);

	assert(mp->ma_lookup != NULL);
	ep = mp->ma_lookup(mp, key, hash);
	if (ep->me_value != NULL) 
	{
		old_value = ep->me_value;
		ep->me_value = value;
		Py_DECREF(old_value);
		Py_DECREF(key);
	}
	else 
	{
		if (ep->me_key == NULL)
		{
			mp->ma_fill++;
		}
		else
		{
			Py_DECREF(ep->me_key);
		}
		ep->me_key = key;
		ep->me_hash = hash;
		ep->me_value = value;
		mp->ma_used++;
	}
}

static int dictresize(dictobject *mp, int minused)
{
	int newsize;
	dictentry *oldtable, *newtable, *ep;
	int i;
	int is_oldtable_malloced;
	dictentry small_copy[PyDict_MINSIZE];

	assert(minused >= 0);

	for (newsize = PyDict_MINSIZE;
	     newsize <= minused && newsize > 0;
	     newsize <<= 1)
	{
		;
	}
	if (newsize <= 0) 
	{
		PyErr_NoMemory();
		return -1;
	}

	oldtable = mp->ma_table;
	assert(oldtable != NULL);
	is_oldtable_malloced = oldtable != mp->ma_smalltable;

	if (newsize == PyDict_MINSIZE) 
	{
		newtable = mp->ma_smalltable;
		if (newtable == oldtable) 
		{
			if (mp->ma_fill == mp->ma_used) 
			{
				return 0;
			}
			assert(mp->ma_fill > mp->ma_used);
			memcpy(small_copy, oldtable, sizeof(small_copy));
			oldtable = small_copy;
		}
	}
	else 
	{
		newtable = PyMem_NEW(dictentry, newsize);
		if (newtable == NULL) 
		{
			PyErr_NoMemory();
			return -1;
		}
	}

	assert(newtable != oldtable);
	mp->ma_table = newtable;
	mp->ma_mask = newsize - 1;
	memset(newtable, 0, sizeof(dictentry) * newsize);
	mp->ma_used = 0;
	i = mp->ma_fill;
	mp->ma_fill = 0;

	for (ep = oldtable; i > 0; ep++) 
	{
		if (ep->me_value != NULL) 
		{	
			--i;
			insertdict(mp, ep->me_key, ep->me_hash, ep->me_value);
		}
		else if (ep->me_key != NULL) 
		{
			--i;
			assert(ep->me_key == dummy);
			Py_DECREF(ep->me_key);
		}
	}

	if (is_oldtable_malloced)
	{
		PyMem_DEL(oldtable);
	}
	return 0;
}

PyObject *PyDict_GetItem(PyObject *op, PyObject *key)
{
	long hash;
	dictobject *mp = (dictobject *)op;
	if (!PyDict_Check(op)) 
	{
		return NULL;
	}
	if (!PyString_CheckExact(key) ||
	    (hash = ((PyStringObject *) key)->ob_shash) == -1)
	{
		hash = PyObject_Hash(key);
		if (hash == -1) 
		{
			PyErr_Clear();
			return NULL;
		}
	}
	return (mp->ma_lookup)(mp, key, hash)->me_value;
}

int PyDict_SetItem(PyObject *op, PyObject *key, PyObject *value)
{
	dictobject *mp;
	long hash;
	int n_used;

	if (!PyDict_Check(op)) 
	{
		PyErr_BadInternalCall();
		return -1;
	}
	mp = (dictobject *)op;
	if (PyString_CheckExact(key)) 
	{
		if (((PyStringObject *)key)->ob_sinterned != NULL) 
		{
			key = ((PyStringObject *)key)->ob_sinterned;
			hash = ((PyStringObject *)key)->ob_shash;
		}
		else
		{
			hash = ((PyStringObject *)key)->ob_shash;
			if (hash == -1)
			{
				hash = PyObject_Hash(key);
			}
		}
	}
	else
	{
		hash = PyObject_Hash(key);
		if (hash == -1)
		{
			return -1;
		}
	}
	assert(mp->ma_fill <= mp->ma_mask); 
	n_used = mp->ma_used;
	Py_INCREF(value);
	Py_INCREF(key);
	insertdict(mp, key, hash, value);
	if (mp->ma_used > n_used && mp->ma_fill*3 >= (mp->ma_mask+1)*2) 
	{
		if (dictresize(mp, mp->ma_used*2) != 0)
		{
			return -1;
		}
	}
	return 0;
}

int PyDict_DelItem(PyObject *op, PyObject *key)
{
	dictobject *mp;
	long hash;
	dictentry *ep;
	PyObject *old_value, *old_key;

	if (!PyDict_Check(op)) 
	{
		PyErr_BadInternalCall();
		return -1;
	}
	if (!PyString_CheckExact(key) ||
	    (hash = ((PyStringObject *) key)->ob_shash) == -1)
	{
		hash = PyObject_Hash(key);
		if (hash == -1)
		{
			return -1;
		}
	}
	mp = (dictobject *)op;
	ep = (mp->ma_lookup)(mp, key, hash);
	if (ep->me_value == NULL) 
	{
		PyErr_SetObject(PyExc_KeyError, key);
		return -1;
	}
	old_key = ep->me_key;
	Py_INCREF(dummy);
	ep->me_key = dummy;
	old_value = ep->me_value;
	ep->me_value = NULL;
	mp->ma_used--;
	Py_DECREF(old_value);
	Py_DECREF(old_key);
	return 0;
}

void PyDict_Clear(PyObject *op)
{
	dictobject *mp;
	dictentry *ep, *table;
	int table_is_malloced;
	int fill;
	dictentry small_copy[PyDict_MINSIZE];
#ifdef _DEBUG
	int i, n;
#endif

	if (!PyDict_Check(op))
	{
		return;
	}
	mp = (dictobject *)op;
#ifdef _DEBUG
	n = mp->ma_mask + 1;
	i = 0;
#endif

	table = mp->ma_table;
	assert(table != NULL);
	table_is_malloced = table != mp->ma_smalltable;

	fill = mp->ma_fill;
	if (table_is_malloced)
	{
		EMPTY_TO_MINSIZE(mp);
	}

	else if (fill > 0) 
	{
		memcpy(small_copy, table, sizeof(small_copy));
		table = small_copy;
		EMPTY_TO_MINSIZE(mp);
	}

	for (ep = table; fill > 0; ++ep) 
	{
#ifdef _DEBUG
		assert(i < n);
		++i;
#endif
		if (ep->me_key) 
		{
			--fill;
			Py_DECREF(ep->me_key);
			Py_XDECREF(ep->me_value);
		}
#ifdef _DEBUG
		else
		{
			assert(ep->me_value == NULL);
		}
#endif
	}

	if (table_is_malloced)
	{
		PyMem_DEL(table);
	}
}

int PyDict_Next(PyObject *op, int *ppos, PyObject **pkey, PyObject **pvalue)
{
	int i;
	dictobject *mp;
	if (!PyDict_Check(op))
	{
		return 0;
	}
	mp = (dictobject *)op;
	i = *ppos;
	if (i < 0)
	{
		return 0;
	}
	while (i <= mp->ma_mask && mp->ma_table[i].me_value == NULL)
	{
		i++;
	}
	*ppos = i+1;
	if (i > mp->ma_mask)
	{
		return 0;
	}
	if (pkey)
	{
		*pkey = mp->ma_table[i].me_key;
	}
	if (pvalue)
	{
		*pvalue = mp->ma_table[i].me_value;
	}
	return 1;
}

static void dict_dealloc(dictobject *mp)
{
	dictentry *ep;
	int fill = mp->ma_fill;
 	PyObject_GC_UnTrack(mp);
	Py_TRASHCAN_SAFE_BEGIN(mp)
	for (ep = mp->ma_table; fill > 0; ep++) 
	{
		if (ep->me_key) 
		{
			--fill;
			Py_DECREF(ep->me_key);
			Py_XDECREF(ep->me_value);
		}
	}
	if (mp->ma_table != mp->ma_smalltable)
	{
		PyMem_DEL(mp->ma_table);
	}
	mp->ob_type->tp_free((PyObject *)mp);
	Py_TRASHCAN_SAFE_END(mp)
}

static int dict_print(dictobject *mp, FILE *fp, int flags)
{
	int i;
	int any;

	i = Py_ReprEnter((PyObject*)mp);
	if (i != 0) 
	{
		if (i < 0)
		{
			return i;
		}
		fprintf(fp, "{...}");
		return 0;
	}

	fprintf(fp, "{");
	any = 0;
	for (i = 0; i <= mp->ma_mask; i++) 
	{
		dictentry *ep = mp->ma_table + i;
		PyObject *pvalue = ep->me_value;
		if (pvalue != NULL) 
		{
			Py_INCREF(pvalue);
			if (any++ > 0)
			{
				fprintf(fp, ", ");
			}
			if (PyObject_Print((PyObject *)ep->me_key, fp, 0)!=0) 
			{
				Py_DECREF(pvalue);
				Py_ReprLeave((PyObject*)mp);
				return -1;
			}
			fprintf(fp, ": ");
			if (PyObject_Print(pvalue, fp, 0) != 0) 
			{
				Py_DECREF(pvalue);
				Py_ReprLeave((PyObject*)mp);
				return -1;
			}
			Py_DECREF(pvalue);
		}
	}
	fprintf(fp, "}");
	Py_ReprLeave((PyObject*)mp);
	return 0;
}

static PyObject *dict_repr(dictobject *mp)
{
	int i;
	PyObject *s, *temp, *colon = NULL;
	PyObject *pieces = NULL, *result = NULL;
	PyObject *key, *value;

	i = Py_ReprEnter((PyObject *)mp);
	if (i != 0) 
	{
		return i > 0 ? PyString_FromString("{...}") : NULL;
	}

	if (mp->ma_used == 0) 
	{
		result = PyString_FromString("{}");
		goto Done;
	}

	pieces = PyList_New(0);
	if (pieces == NULL)
	{
		goto Done;
	}

	colon = PyString_FromString(": ");
	if (colon == NULL)
	{
		goto Done;
	}

	i = 0;
	while (PyDict_Next((PyObject *)mp, &i, &key, &value)) 
	{
		int status;
		Py_INCREF(value);
		s = PyObject_Repr(key);
		PyString_Concat(&s, colon);
		PyString_ConcatAndDel(&s, PyObject_Repr(value));
		Py_DECREF(value);
		if (s == NULL)
		{
			goto Done;
		}
		status = PyList_Append(pieces, s);
		Py_DECREF(s); 
		if (status < 0)
		{
			goto Done;
		}
	}

	assert(PyList_GET_SIZE(pieces) > 0);
	s = PyString_FromString("{");
	if (s == NULL)
	{
		goto Done;
	}
	temp = PyList_GET_ITEM(pieces, 0);
	PyString_ConcatAndDel(&s, temp);
	PyList_SET_ITEM(pieces, 0, s);
	if (s == NULL)
	{
		goto Done;
	}

	s = PyString_FromString("}");
	if (s == NULL)
	{
		goto Done;
	}
	temp = PyList_GET_ITEM(pieces, PyList_GET_SIZE(pieces) - 1);
	PyString_ConcatAndDel(&temp, s);
	PyList_SET_ITEM(pieces, PyList_GET_SIZE(pieces) - 1, temp);
	if (temp == NULL)
	{
		goto Done;
	}

	s = PyString_FromString(", ");
	if (s == NULL)
	{
		goto Done;
	}
	result = _PyString_Join(s, pieces);
	Py_DECREF(s);

Done:
	Py_XDECREF(pieces);
	Py_XDECREF(colon);
	Py_ReprLeave((PyObject *)mp);
	return result;
}

static int dict_length(dictobject *mp)
{
	return mp->ma_used;
}

static PyObject *dict_subscript(dictobject *mp, PyObject *key)
{
	PyObject *v;
	long hash;
	assert(mp->ma_table != NULL);
	if (!PyString_CheckExact(key) ||
	    (hash = ((PyStringObject *) key)->ob_shash) == -1)
	{
		hash = PyObject_Hash(key);
		if (hash == -1)
		{
			return NULL;
		}
	}
	v = (mp->ma_lookup)(mp, key, hash) -> me_value;
	if (v == NULL)
	{
		PyErr_SetObject(PyExc_KeyError, key);
	}
	else
	{
		Py_INCREF(v);
	}
	return v;
}

static int dict_ass_sub(dictobject *mp, PyObject *v, PyObject *w)
{
	if (w == NULL)
	{
		return PyDict_DelItem((PyObject *)mp, v);
	}
	else
	{
		return PyDict_SetItem((PyObject *)mp, v, w);
	}
}

static PyMappingMethods dict_as_mapping = {
	(inquiry)dict_length, 
	(binaryfunc)dict_subscript,
	(objobjargproc)dict_ass_sub,
};

static PyObject *dict_keys(dictobject *mp)
{
	PyObject *v;
	int i, j, n;

again:
	n = mp->ma_used;
	v = PyList_New(n);
	if (v == NULL)
	{
		return NULL;
	}
	if (n != mp->ma_used) 
	{
		Py_DECREF(v);
		goto again;
	}
	for (i = 0, j = 0; i <= mp->ma_mask; i++) 
	{
		if (mp->ma_table[i].me_value != NULL) 
		{
			PyObject *key = mp->ma_table[i].me_key;
			Py_INCREF(key);
			PyList_SET_ITEM(v, j, key);
			j++;
		}
	}
	return v;
}

static PyObject *dict_values(dictobject *mp)
{
	PyObject *v;
	int i, j, n;

again:
	n = mp->ma_used;
	v = PyList_New(n);
	if (v == NULL)
	{
		return NULL;
	}
	if (n != mp->ma_used) 
	{
		Py_DECREF(v);
		goto again;
	}
	for (i = 0, j = 0; i <= mp->ma_mask; i++) 
	{
		if (mp->ma_table[i].me_value != NULL) 
		{
			PyObject *value = mp->ma_table[i].me_value;
			Py_INCREF(value);
			PyList_SET_ITEM(v, j, value);
			j++;
		}
	}
	return v;
}

static PyObject *dict_items(dictobject *mp)
{
	PyObject *v;
	int i, j, n;
	PyObject *item, *key, *value;

again:
	n = mp->ma_used;
	v = PyList_New(n);
	if (v == NULL)
	{
		return NULL;
	}
	for (i = 0; i < n; i++) 
	{
		item = PyTuple_New(2);
		if (item == NULL) 
		{
			Py_DECREF(v);
			return NULL;
		}
		PyList_SET_ITEM(v, i, item);
	}
	if (n != mp->ma_used) 
	{
		Py_DECREF(v);
		goto again;
	}
	for (i = 0, j = 0; i <= mp->ma_mask; i++) 
	{
		if (mp->ma_table[i].me_value != NULL) 
		{
			key = mp->ma_table[i].me_key;
			value = mp->ma_table[i].me_value;
			item = PyList_GET_ITEM(v, j);
			Py_INCREF(key);
			PyTuple_SET_ITEM(item, 0, key);
			Py_INCREF(value);
			PyTuple_SET_ITEM(item, 1, value);
			j++;
		}
	}
	assert(j == n);
	return v;
}

static PyObject *dict_update(PyObject *mp, PyObject *other)
{
	if (PyDict_Update(mp, other) < 0)
	{
		return NULL;
	}
	Py_INCREF(Py_None);
	return Py_None;
}

int PyDict_MergeFromSeq2(PyObject *d, PyObject *seq2, int override)
{
	PyObject *it;
	int i;		
	PyObject *item;
	PyObject *fast;

	assert(d != NULL);
	assert(PyDict_Check(d));
	assert(seq2 != NULL);

	it = PyObject_GetIter(seq2);
	if (it == NULL)
	{
		return -1;
	}

	for (i = 0; ; ++i) 
	{
		PyObject *key, *value;
		int n;

		fast = NULL;
		item = PyIter_Next(it);
		if (item == NULL) 
		{
			if (PyErr_Occurred())
			{
				goto Fail;
			}
			break;
		}

		fast = PySequence_Fast(item, "");
		if (fast == NULL) 
		{
			if (PyErr_ExceptionMatches(PyExc_TypeError))
			{
				PyErr_Format(PyExc_TypeError,
					"cannot convert dictionary update "
					"sequence element #%d to a sequence",
					i);
			}
			goto Fail;
		}
		n = PySequence_Fast_GET_SIZE(fast);
		if (n != 2) 
		{
			PyErr_Format(PyExc_ValueError,
				     "dictionary update sequence element #%d "
				     "has length %d; 2 is required",
				     i, n);
			goto Fail;
		}

		key = PySequence_Fast_GET_ITEM(fast, 0);
		value = PySequence_Fast_GET_ITEM(fast, 1);
		if (override || PyDict_GetItem(d, key) == NULL) 
		{
			int status = PyDict_SetItem(d, key, value);
			if (status < 0)
			{
				goto Fail;
			}
		}
		Py_DECREF(fast);
		Py_DECREF(item);
	}

	i = 0;
	goto Return;
Fail:
	Py_XDECREF(item);
	Py_XDECREF(fast);
	i = -1;
Return:
	Py_DECREF(it);
	return i;
}

int PyDict_Update(PyObject *a, PyObject *b)
{
	return PyDict_Merge(a, b, 1);
}

int PyDict_Merge(PyObject *a, PyObject *b, int override)
{
	PyDictObject *mp, *other;
	int i;
	dictentry *entry;

	if (a == NULL || !PyDict_Check(a) || b == NULL) 
	{
		PyErr_BadInternalCall();
		return -1;
	}
	mp = (dictobject*)a;
	if (PyDict_Check(b)) 
	{
		other = (dictobject*)b;
		if (other == mp || other->ma_used == 0)
		{
			return 0;
		}
		if ((mp->ma_fill + other->ma_used)*3 >= (mp->ma_mask+1)*2) 
		{
		   if (dictresize(mp, (mp->ma_used + other->ma_used)*3/2) != 0)
		   {
			   return -1;
		   }
		}
		for (i = 0; i <= other->ma_mask; i++) 
		{
			entry = &other->ma_table[i];
			if (entry->me_value != NULL &&
			    (override ||
			     PyDict_GetItem(a, entry->me_key) == NULL)) 
			{
				Py_INCREF(entry->me_key);
				Py_INCREF(entry->me_value);
				insertdict(mp, entry->me_key, entry->me_hash,
					   entry->me_value);
			}
		}
	}
	else 
	{
		PyObject *keys = PyMapping_Keys(b);
		PyObject *iter;
		PyObject *key, *value;
		int status;

		if (keys == NULL)
		{
			return -1;
		}

		iter = PyObject_GetIter(keys);
		Py_DECREF(keys);
		if (iter == NULL)
		{
			return -1;
		}

		for (key = PyIter_Next(iter); key; key = PyIter_Next(iter)) 
		{
			if (!override && PyDict_GetItem(a, key) != NULL) 
			{
				Py_DECREF(key);
				continue;
			}
			value = PyObject_GetItem(b, key);
			if (value == NULL) 
			{
				Py_DECREF(iter);
				Py_DECREF(key);
				return -1;
			}
			status = PyDict_SetItem(a, key, value);
			Py_DECREF(key);
			Py_DECREF(value);
			if (status < 0) 
			{
				Py_DECREF(iter);
				return -1;
			}
		}
		Py_DECREF(iter);
		if (PyErr_Occurred())
		{
			return -1;
		}
	}
	return 0;
}

static PyObject *dict_copy(dictobject *mp)
{
	return PyDict_Copy((PyObject*)mp);
}

PyObject *PyDict_Copy(PyObject *o)
{
	dictobject *mp;
	int i;
	dictobject *copy;
	dictentry *entry;

	if (o == NULL || !PyDict_Check(o)) 
	{
		PyErr_BadInternalCall();
		return NULL;
	}
	mp = (dictobject *)o;
	copy = (dictobject *)PyDict_New();
	if (copy == NULL)
	{
		return NULL;
	}
	if (mp->ma_used > 0) 
	{
		if (dictresize(copy, mp->ma_used*3/2) != 0)
		{
			return NULL;
		}
		for (i = 0; i <= mp->ma_mask; i++) 
		{
			entry = &mp->ma_table[i];
			if (entry->me_value != NULL) 
			{
				Py_INCREF(entry->me_key);
				Py_INCREF(entry->me_value);
				insertdict(copy, entry->me_key, entry->me_hash,
					   entry->me_value);
			}
		}
	}
	return (PyObject *)copy;
}

int PyDict_Size(PyObject *mp)
{
	if (mp == NULL || !PyDict_Check(mp)) 
	{
		PyErr_BadInternalCall();
		return 0;
	}
	return ((dictobject *)mp)->ma_used;
}

PyObject *PyDict_Keys(PyObject *mp)
{
	if (mp == NULL || !PyDict_Check(mp)) 
	{
		PyErr_BadInternalCall();
		return NULL;
	}
	return dict_keys((dictobject *)mp);
}

PyObject *PyDict_Values(PyObject *mp)
{
	if (mp == NULL || !PyDict_Check(mp)) 
	{
		PyErr_BadInternalCall();
		return NULL;
	}
	return dict_values((dictobject *)mp);
}

PyObject *PyDict_Items(PyObject *mp)
{
	if (mp == NULL || !PyDict_Check(mp)) 
	{
		PyErr_BadInternalCall();
		return NULL;
	}
	return dict_items((dictobject *)mp);
}

static PyObject *characterize(dictobject *a, dictobject *b, PyObject **pval)
{
	PyObject *akey = NULL; 
	PyObject *aval = NULL; 
	int i, cmp;

	for (i = 0; i <= a->ma_mask; i++) 
	{
		PyObject *thiskey, *thisaval, *thisbval;
		if (a->ma_table[i].me_value == NULL)
		{
			continue;
		}
		thiskey = a->ma_table[i].me_key;
		Py_INCREF(thiskey); 
		if (akey != NULL) 
		{
			cmp = PyObject_RichCompareBool(akey, thiskey, Py_LT);
			if (cmp < 0) 
			{
				Py_DECREF(thiskey);
				goto Fail;
			}
			if (cmp > 0 ||
			    i > a->ma_mask ||
			    a->ma_table[i].me_value == NULL)
			{
				Py_DECREF(thiskey);
				continue;
			}
		}

		thisaval = a->ma_table[i].me_value;
		assert(thisaval);
		Py_INCREF(thisaval); 
		thisbval = PyDict_GetItem((PyObject *)b, thiskey);
		if (thisbval == NULL)
		{
			cmp = 0;
		}
		else 
		{
			cmp = PyObject_RichCompareBool(
						thisaval, thisbval, Py_EQ);
			if (cmp < 0) 
			{
		    		Py_DECREF(thiskey);
		    		Py_DECREF(thisaval);
		    		goto Fail;
			}
		}
		if (cmp == 0) 
		{
			Py_XDECREF(akey);
			Py_XDECREF(aval);
			akey = thiskey;
			aval = thisaval;
		}
		else 
		{
			Py_DECREF(thiskey);
			Py_DECREF(thisaval);
		}
	}
	*pval = aval;
	return akey;

Fail:
	Py_XDECREF(akey);
	Py_XDECREF(aval);
	*pval = NULL;
	return NULL;
}

static int dict_compare(dictobject *a, dictobject *b)
{
	PyObject *adiff, *bdiff, *aval, *bval;
	int res;

	if (a->ma_used < b->ma_used)
	{
		return -1;	
	}
	else if (a->ma_used > b->ma_used)
	{
		return 1;	
	}

	bdiff = bval = NULL;
	adiff = characterize(a, b, &aval);
	if (adiff == NULL) 
	{
		assert(!aval);
		res = PyErr_Occurred() ? -1 : 0;
		goto Finished;
	}
	bdiff = characterize(b, a, &bval);
	if (bdiff == NULL && PyErr_Occurred()) 
	{
		assert(!bval);
		res = -1;
		goto Finished;
	}
	res = 0;
	if (bdiff) 
	{
		res = PyObject_Compare(adiff, bdiff);
	}
	if (res == 0 && bval != NULL)
	{
		res = PyObject_Compare(aval, bval);
	}

Finished:
	Py_XDECREF(adiff);
	Py_XDECREF(bdiff);
	Py_XDECREF(aval);
	Py_XDECREF(bval);
	return res;
}

static int dict_equal(dictobject *a, dictobject *b)
{
	int i;

	if (a->ma_used != b->ma_used)
	{
		return 0;
	}

	for (i = 0; i <= a->ma_mask; i++) 
	{
		PyObject *aval = a->ma_table[i].me_value;
		if (aval != NULL) 
		{
			int cmp;
			PyObject *bval;
			PyObject *key = a->ma_table[i].me_key;
			Py_INCREF(aval);
			bval = PyDict_GetItem((PyObject *)b, key);
			if (bval == NULL) 
			{
				Py_DECREF(aval);
				return 0;
			}
			cmp = PyObject_RichCompareBool(aval, bval, Py_EQ);
			Py_DECREF(aval);
			if (cmp <= 0) 
			{
				return cmp;
			}
 		}
	}
	return 1;
 }

static PyObject *dict_richcompare(PyObject *v, PyObject *w, int op)
{
	int cmp;
	PyObject *res;

	if (!PyDict_Check(v) || !PyDict_Check(w)) 
	{
		res = Py_NotImplemented;
	}
	else if (op == Py_EQ || op == Py_NE) 
	{
		cmp = dict_equal((dictobject *)v, (dictobject *)w);
		if (cmp < 0)
		{
			return NULL;
		}
		res = (cmp == (op == Py_EQ)) ? Py_True : Py_False;
	}
	else
	{
		res = Py_NotImplemented;
	}
	Py_INCREF(res);
	return res;
}

static PyObject *dict_has_key(dictobject *mp, PyObject *key)
{
	long hash;
	long ok;
	if (!PyString_CheckExact(key) ||
	    (hash = ((PyStringObject *) key)->ob_shash) == -1)
	{
		hash = PyObject_Hash(key);
		if (hash == -1)
			return NULL;
	}
	ok = (mp->ma_lookup)(mp, key, hash)->me_value != NULL;
	return PyInt_FromLong(ok);
}

static PyObject *dict_get(dictobject *mp, PyObject *args)
{
	PyObject *key;
	PyObject *failobj = Py_None;
	PyObject *val = NULL;
	long hash;

	if (!PyArg_ParseTuple(args, "O|O:get", &key, &failobj))
	{
		return NULL;
	}

	if (!PyString_CheckExact(key) ||
	    (hash = ((PyStringObject *) key)->ob_shash) == -1)
	{
		hash = PyObject_Hash(key);
		if (hash == -1)
		{
			return NULL;
		}
	}
	val = (mp->ma_lookup)(mp, key, hash)->me_value;

	if (val == NULL)
	{
		val = failobj;
	}
	Py_INCREF(val);
	return val;
}


static PyObject *dict_setdefault(dictobject *mp, PyObject *args)
{
	PyObject *key;
	PyObject *failobj = Py_None;
	PyObject *val = NULL;
	long hash;

	if (!PyArg_ParseTuple(args, "O|O:setdefault", &key, &failobj))
	{
		return NULL;
	}

	if (!PyString_CheckExact(key) ||
	    (hash = ((PyStringObject *) key)->ob_shash) == -1)
	{
		hash = PyObject_Hash(key);
		if (hash == -1)
		{
			return NULL;
		}
	}
	val = (mp->ma_lookup)(mp, key, hash)->me_value;
	if (val == NULL) 
	{
		val = failobj;
		if (PyDict_SetItem((PyObject*)mp, key, failobj))
		{
			val = NULL;
		}
	}
	Py_XINCREF(val);
	return val;
}


static PyObject *dict_clear(dictobject *mp)
{
	PyDict_Clear((PyObject *)mp);
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *dict_popitem(dictobject *mp)
{
	int i = 0;
	dictentry *ep;
	PyObject *res;

	res = PyTuple_New(2);
	if (res == NULL)
	{
		return NULL;
	}
	if (mp->ma_used == 0) 
	{
		Py_DECREF(res);
		PyErr_SetString(PyExc_KeyError,
				"popitem(): dictionary is empty");
		return NULL;
	}
	ep = &mp->ma_table[0];
	if (ep->me_value == NULL) 
	{
		i = (int)ep->me_hash;
		if (i > mp->ma_mask || i < 1)
		{
			i = 1;
		}
		while ((ep = &mp->ma_table[i])->me_value == NULL) 
		{
			i++;
			if (i > mp->ma_mask)
			{
				i = 1;
			}
		}
	}
	PyTuple_SET_ITEM(res, 0, ep->me_key);
	PyTuple_SET_ITEM(res, 1, ep->me_value);
	Py_INCREF(dummy);
	ep->me_key = dummy;
	ep->me_value = NULL;
	mp->ma_used--;
	assert(mp->ma_table[0].me_value == NULL);
	mp->ma_table[0].me_hash = i + 1; 
	return res;
}

static int dict_traverse(PyObject *op, visitproc visit, void *arg)
{
	int i = 0, err;
	PyObject *pk;
	PyObject *pv;

	while (PyDict_Next(op, &i, &pk, &pv)) 
	{
		err = visit(pk, arg);
		if (err)
		{
			return err;
		}
		err = visit(pv, arg);
		if (err)
		{
			return err;
		}
	}
	return 0;
}

static int dict_tp_clear(PyObject *op)
{
	PyDict_Clear(op);
	return 0;
}


staticforward PyObject *dictiter_new(dictobject *, binaryfunc);

static PyObject *select_key(PyObject *key, PyObject *value)
{
	Py_INCREF(key);
	return key;
}

static PyObject *select_value(PyObject *key, PyObject *value)
{
	Py_INCREF(value);
	return value;
}

static PyObject *select_item(PyObject *key, PyObject *value)
{
	PyObject *res = PyTuple_New(2);

	if (res != NULL) 
	{
		Py_INCREF(key);
		Py_INCREF(value);
		PyTuple_SET_ITEM(res, 0, key);
		PyTuple_SET_ITEM(res, 1, value);
	}
	return res;
}

static PyObject *dict_iterkeys(dictobject *dict)
{
	return dictiter_new(dict, select_key);
}

static PyObject *dict_itervalues(dictobject *dict)
{
	return dictiter_new(dict, select_value);
}

static PyObject *dict_iteritems(dictobject *dict)
{
	return dictiter_new(dict, select_item);
}


static char has_key__doc__[] =
	"D.has_key(k) -> 1 if D has a key k, else 0";

static char get__doc__[] =
	"D.get(k[,d]) -> D[k] if D.has_key(k), else d.  d defaults to None.";

static char setdefault_doc__[] =
	"D.setdefault(k[,d]) -> D.get(k,d), also set D[k]=d if not D.has_key(k)";

static char popitem__doc__[] =
	"D.popitem() -> (k, v), remove and return some (key, value) pair as a\n"
	"2-tuple; but raise KeyError if D is empty";

static char keys__doc__[] =
	"D.keys() -> list of D's keys";

static char items__doc__[] =
	"D.items() -> list of D's (key, value) pairs, as 2-tuples";

static char values__doc__[] =
	"D.values() -> list of D's values";

static char update__doc__[] =
	"D.update(E) -> None.  Update D from E: for k in E.keys(): D[k] = E[k]";

static char clear__doc__[] =
	"D.clear() -> None.  Remove all items from D.";

static char copy__doc__[] =
	"D.copy() -> a shallow copy of D";

static char iterkeys__doc__[] =
	"D.iterkeys() -> an iterator over the keys of D";

static char itervalues__doc__[] =
	"D.itervalues() -> an iterator over the values of D";

static char iteritems__doc__[] =
	"D.iteritems() -> an iterator over the (key, value) items of D";

static PyMethodDef mapp_methods[] = {
	{"has_key",	(PyCFunction)dict_has_key,      METH_O,
	 has_key__doc__},
	{"get",         (PyCFunction)dict_get,          METH_VARARGS,
	 get__doc__},
	{"setdefault",  (PyCFunction)dict_setdefault,   METH_VARARGS,
	 setdefault_doc__},
	{"popitem",	(PyCFunction)dict_popitem,	METH_NOARGS,
	 popitem__doc__},
	{"keys",	(PyCFunction)dict_keys,		METH_NOARGS,
	keys__doc__},
	{"items",	(PyCFunction)dict_items,	METH_NOARGS,
	 items__doc__},
	{"values",	(PyCFunction)dict_values,	METH_NOARGS,
	 values__doc__},
	{"update",	(PyCFunction)dict_update,	METH_O,
	 update__doc__},
	{"clear",	(PyCFunction)dict_clear,	METH_NOARGS,
	 clear__doc__},
	{"copy",	(PyCFunction)dict_copy,		METH_NOARGS,
	 copy__doc__},
	{"iterkeys",	(PyCFunction)dict_iterkeys,	METH_NOARGS,
	 iterkeys__doc__},
	{"itervalues",	(PyCFunction)dict_itervalues,	METH_NOARGS,
	 itervalues__doc__},
	{"iteritems",	(PyCFunction)dict_iteritems,	METH_NOARGS,
	 iteritems__doc__},
	{NULL,		NULL}
};

static int dict_contains(dictobject *mp, PyObject *key)
{
	long hash;

	if (!PyString_CheckExact(key) ||
	    (hash = ((PyStringObject *) key)->ob_shash) == -1)
	{
		hash = PyObject_Hash(key);
		if (hash == -1)
			return -1;
	}
	return (mp->ma_lookup)(mp, key, hash)->me_value != NULL;
}

static PySequenceMethods dict_as_sequence = {
	0,					
	0,					
	0,					
	0,					
	0,					
	0,					
	0,					
	(objobjproc)dict_contains,
	0,					
	0,					
};

static PyObject *dict_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	PyObject *self;

	assert(type != NULL && type->tp_alloc != NULL);
	self = type->tp_alloc(type, 0);
	if (self != NULL) 
	{
		PyDictObject *d = (PyDictObject *)self;
		assert(d->ma_table == NULL && d->ma_fill == 0 && d->ma_used == 0);
		INIT_NONZERO_DICT_SLOTS(d);
		d->ma_lookup = lookdict_string;
	}
	return self;
}

static int dict_init(PyObject *self, PyObject *args, PyObject *kwds)
{
	PyObject *arg = NULL;
	static char *kwlist[] = {"items", 0};
	int result = 0;

	if (!PyArg_ParseTupleAndKeywords(args, kwds, "|O:dict",
					 kwlist, &arg))
	{
		result = -1;
	}
	else if (arg != NULL) 
	{
		if (PyObject_HasAttrString(arg, "keys"))
		{
			result = PyDict_Merge(self, arg, 1);
		}
		else
		{
			result = PyDict_MergeFromSeq2(self, arg, 1);
		}
	}
	return result;
}

static long dict_nohash(PyObject *self)
{
	PyErr_SetString(PyExc_TypeError, "dict objects are unhashable");
	return -1;
}

static PyObject *dict_iter(dictobject *dict)
{
	return dictiter_new(dict, select_key);
}

static char dictionary_doc[] =
	"dict() -> new empty dictionary.\n"
	"dict(mapping) -> new dictionary initialized from a mapping object's\n"
	"    (key, value) pairs.\n"
	"dict(seq) -> new dictionary initialized as if via:\n"
	"    d = {}\n"
	"    for k, v in seq:\n"
	"        d[k] = v";

PyTypeObject PyDict_Type = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,
	"dict",
	sizeof(dictobject),
	0,
	(destructor)dict_dealloc,	
	(printfunc)dict_print,		
	0,					
	0,					
	(cmpfunc)dict_compare,	
	(reprfunc)dict_repr,	
	0,					
	&dict_as_sequence,		
	&dict_as_mapping,		
	dict_nohash,			
	0,					
	0,					
	PyObject_GenericGetAttr,	
	0,					
	0,					
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC |
		Py_TPFLAGS_BASETYPE,	
	dictionary_doc,				
	(traverseproc)dict_traverse,
	(inquiry)dict_tp_clear,		
	dict_richcompare,	
	0,					
	(getiterfunc)dict_iter,	
	0,					
	mapp_methods,		
	0,					
	0,					
	0,					
	0,					
	0,					
	0,					
	0,					
	(initproc)dict_init,	
	PyType_GenericAlloc,	
	dict_new,			
	_PyObject_GC_Del,	
};

PyObject *PyDict_GetItemString(PyObject *v, char *key)
{
	PyObject *kv, *rv;
	kv = PyString_FromString(key);
	if (kv == NULL)
	{
		return NULL;
	}
	rv = PyDict_GetItem(v, kv);
	Py_DECREF(kv);
	return rv;
}

int PyDict_SetItemString(PyObject *v, char *key, PyObject *item)
{
	PyObject *kv;
	int err;
	kv = PyString_FromString(key);
	if (kv == NULL)
	{
		return -1;
	}
	PyString_InternInPlace(&kv);
	err = PyDict_SetItem(v, kv, item);
	Py_DECREF(kv);
	return err;
}

int PyDict_DelItemString(PyObject *v, char *key)
{
	PyObject *kv;
	int err;
	kv = PyString_FromString(key);
	if (kv == NULL)
	{
		return -1;
	}
	err = PyDict_DelItem(v, kv);
	Py_DECREF(kv);
	return err;
}

extern PyTypeObject PyDictIter_Type;

typedef struct {
	PyObject_HEAD
	dictobject *di_dict;
	int di_used;
	int di_pos;
	binaryfunc di_select;
} dictiterobject;

static PyObject *dictiter_new(dictobject *dict, binaryfunc select)
{
	dictiterobject *di;
	di = PyObject_NEW(dictiterobject, &PyDictIter_Type);
	if (di == NULL)
	{
		return NULL;
	}
	Py_INCREF(dict);
	di->di_dict = dict;
	di->di_used = dict->ma_used;
	di->di_pos = 0;
	di->di_select = select;
	return (PyObject *)di;
}

static void dictiter_dealloc(dictiterobject *di)
{
	Py_DECREF(di->di_dict);
	PyObject_DEL(di);
}

static PyObject *dictiter_next(dictiterobject *di, PyObject *args)
{
	PyObject *key, *value;

	if (di->di_used != di->di_dict->ma_used) 
	{
		PyErr_SetString(PyExc_RuntimeError,
				"dictionary changed size during iteration");
		return NULL;
	}
	if (PyDict_Next((PyObject *)(di->di_dict), &di->di_pos, &key, &value)) 
	{
		return (*di->di_select)(key, value);
	}
	PyErr_SetObject(PyExc_StopIteration, Py_None);
	return NULL;
}

static PyObject *dictiter_getiter(PyObject *it)
{
	Py_INCREF(it);
	return it;
}

static PyMethodDef dictiter_methods[] = {
	{"next",	(PyCFunction)dictiter_next,	METH_VARARGS,
	 "it.next() -- get the next value, or raise StopIteration"},
	{NULL,		NULL}
};

static PyObject *dictiter_iternext(dictiterobject *di)
{
	PyObject *key, *value;

	if (di->di_used != di->di_dict->ma_used) 
	{
		PyErr_SetString(PyExc_RuntimeError,
				"dictionary changed size during iteration");
		return NULL;
	}
	if (PyDict_Next((PyObject *)(di->di_dict), &di->di_pos, &key, &value)) 
	{
		return (*di->di_select)(key, value);
	}
	return NULL;
}

PyTypeObject PyDictIter_Type = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,				
	"dictionary-iterator",
	sizeof(dictiterobject),	
	0,			
	(destructor)dictiter_dealloc, 	
	0,					
	0,					
	0,					
	0,					
	0,					
	0,					
	0,					
	0,					
	0,					
	0,					
	0,					
	PyObject_GenericGetAttr,
	0,					
	0,					
	Py_TPFLAGS_DEFAULT,	
 	0,					
 	0,					
 	0,					
	0,					
	0,					
	(getiterfunc)dictiter_getiter,	
	(iternextfunc)dictiter_iternext,
	dictiter_methods,			
	0,					
	0,					
	0,					
	0,					
	0,				
	0,				
};
