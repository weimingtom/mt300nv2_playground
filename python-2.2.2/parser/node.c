//20170403
#include "python.h"
#include "node.h"
#include "errcode.h"

node *PyNode_New(int type)
{
	node *n = PyMem_NEW(node, 1);
	if (n == NULL)
	{
		return NULL;
	}
	n->n_type = type;
	n->n_str = NULL;
	n->n_lineno = 0;
	n->n_nchildren = 0;
	n->n_child = NULL;
	return n;
}

static int fancy_roundup(int n)
{
	int result = 256;
	assert(n > 128);
	while (result < n) 
	{
		result <<= 1;
		if (result <= 0)
		{
			return -1;
		}
	}
	return result;
}

#define XXXROUNDUP(n) ((n) == 1 ? 1 : \
	(n) <= 128 ? (((n) + 3) & ~3) : \
	fancy_roundup(n))

int PyNode_AddChild(node *n1, int type, char *str, int lineno)
{
	const int nch = n1->n_nchildren;
	int current_capacity;
	int required_capacity;
	node *n;

	if (nch == INT_MAX || nch < 0)
	{
		return E_OVERFLOW;
	}
	current_capacity = XXXROUNDUP(nch);
	required_capacity = XXXROUNDUP(nch + 1);
	if (current_capacity < 0 || required_capacity < 0)
	{
		return E_OVERFLOW;
	}
	if (current_capacity < required_capacity) 
	{
		n = n1->n_child;
		PyMem_RESIZE(n, node, required_capacity);
		if (n == NULL)
		{
			return E_NOMEM;
		}
		n1->n_child = n;
	}
	n = &n1->n_child[n1->n_nchildren++];
	n->n_type = type;
	n->n_str = str;
	n->n_lineno = lineno;
	n->n_nchildren = 0;
	n->n_child = NULL;
	return 0;
}

static void freechildren(node *);

void PyNode_Free(node *n)
{
	if (n != NULL) 
	{
		freechildren(n);
		PyMem_DEL(n);
	}
}

static void freechildren(node *n)
{
	int i;
	for (i = NCH(n); --i >= 0; )
	{
		freechildren(CHILD(n, i));
	}
	if (n->n_child != NULL)
	{
		PyMem_DEL(n->n_child);
	}
	if (STR(n) != NULL)
	{
		PyMem_DEL(STR(n));
	}
}
