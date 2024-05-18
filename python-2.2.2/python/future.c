//20180118
#include "python.h"
#include "node.h"
#include "token.h"
#include "graminit.h"
#include "compile.h"
#include "symtable.h"

#define UNDEFINED_FUTURE_FEATURE "future feature %.100s is not defined"
#define FUTURE_IMPORT_STAR "future statement does not support import *"

#define FUTURE_POSSIBLE(FF) ((FF)->ff_last_lineno == -1)

static int future_check_features(PyFutureFeatures *ff, node *n, char *filename)
{
	int i;
	char *feature;
	node *ch;

	REQ(n, import_stmt);

	for (i = 3; i < NCH(n); i += 2) 
	{
		ch = CHILD(n, i);
		if (TYPE(ch) == STAR) 
		{
			PyErr_SetString(PyExc_SyntaxError,
					FUTURE_IMPORT_STAR);
			PyErr_SyntaxLocation(filename, ch->n_lineno);
			return -1;
		}
		REQ(ch, import_as_name);
		feature = STR(CHILD(ch, 0));
		if (strcmp(feature, FUTURE_NESTED_SCOPES) == 0) 
		{
			continue;
		} 
		else if (strcmp(feature, FUTURE_GENERATORS) == 0) 
		{
			ff->ff_features |= CO_GENERATOR_ALLOWED;
		} 
		else if (strcmp(feature, FUTURE_DIVISION) == 0) 
		{
			ff->ff_features |= CO_FUTURE_DIVISION;
		} 
		else if (strcmp(feature, "braces") == 0) 
		{
			PyErr_SetString(PyExc_SyntaxError,
					"not a chance");
			PyErr_SyntaxLocation(filename, CHILD(ch, 0)->n_lineno);
			return -1;
		} 
		else 
		{
			PyErr_Format(PyExc_SyntaxError,
				     UNDEFINED_FUTURE_FEATURE, feature);
			PyErr_SyntaxLocation(filename, CHILD(ch, 0)->n_lineno);
			return -1;
		}
	}
	return 0;
}

static void future_error(node *n, char *filename)
{
	PyErr_SetString(PyExc_SyntaxError,
			"from __future__ imports must occur at the "
			"beginning of the file");
	PyErr_SyntaxLocation(filename, n->n_lineno);
}

static int future_parse(PyFutureFeatures *ff, node *n, char *filename)
{
	int i, r;
loop:

	switch (TYPE(n)) 
	{
	case single_input:
		if (TYPE(CHILD(n, 0)) == simple_stmt) 
		{
			n = CHILD(n, 0);
			goto loop;
		}
		return 0;

	case file_input:
		for (i = 0; i < NCH(n); i++) 
		{
			node *ch = CHILD(n, i);
			if (TYPE(ch) == stmt) {
				r = future_parse(ff, ch, filename);
				if (r < 1 && !FUTURE_POSSIBLE(ff))
				{
					return r;
				}
			}
		}
		return 0;

	case simple_stmt:
		if (NCH(n) == 2) 
		{
			REQ(CHILD(n, 0), small_stmt);
			n = CHILD(n, 0);
			goto loop;
		} 
		else 
		{
			int found = 0, end_of_future = 0;

			for (i = 0; i < NCH(n); i += 2) 
			{
				if (TYPE(CHILD(n, i)) == small_stmt) 
				{
					r = future_parse(ff, CHILD(n, i), 
							 filename);
					if (r < 1)
					{
						end_of_future = 1;
					}
					else 
					{
						found = 1;
						if (end_of_future) 
						{
							future_error(n, 
								     filename);
							return -1;
						}
					}
				}
			}

			if (found)
			{
				ff->ff_last_lineno = n->n_lineno + 1;
			}
			else
			{
				ff->ff_last_lineno = n->n_lineno;
			}

			if (end_of_future && found)
			{
				return 1;
			}
			else 
			{
				return 0;
			}
		}
	
	case stmt:
		if (TYPE(CHILD(n, 0)) == simple_stmt) 
		{
			n = CHILD(n, 0);
			goto loop;
		} 
		else if (TYPE(CHILD(n, 0)) == expr_stmt) 
		{
			n = CHILD(n, 0);
			goto loop;
		} 
		else 
		{
			REQ(CHILD(n, 0), compound_stmt);
			ff->ff_last_lineno = n->n_lineno;
			return 0;
		}

	case small_stmt:
		n = CHILD(n, 0);
		goto loop;

	case import_stmt: 
		{
			node *name;

			if (STR(CHILD(n, 0))[0] != 'f') 
			{ 
				ff->ff_last_lineno = n->n_lineno;
				return 0;
			}
			name = CHILD(n, 1);
			if (strcmp(STR(CHILD(name, 0)), "__future__") != 0)
			{
				return 0;
			}
			if (future_check_features(ff, n, filename) < 0)
			{
				return -1;
			}
			ff->ff_last_lineno = n->n_lineno + 1;
			return 1;
		}

	case expr_stmt:
	case testlist:
	case test:
	case and_test:
	case not_test:
	case comparison:
	case expr:
	case xor_expr:
	case and_expr:
	case shift_expr:
	case arith_expr:
	case term:
	case factor:
	case power:
		if (NCH(n) == 1) 
		{
			n = CHILD(n, 0);
			goto loop;
		}
		break;

	case atom:
		if (TYPE(CHILD(n, 0)) == STRING 
		    && ff->ff_found_docstring == 0) 
		{
			ff->ff_found_docstring = 1;
			return 0;
		}
		ff->ff_last_lineno = n->n_lineno;
		return 0;

	default:
		ff->ff_last_lineno = n->n_lineno;
		return 0;
	}
	return 0;
}

PyFutureFeatures *PyNode_Future(node *n, char *filename)
{
	PyFutureFeatures *ff;

	ff = (PyFutureFeatures *)PyMem_Malloc(sizeof(PyFutureFeatures));
	if (ff == NULL)
	{
		return NULL;
	}
	ff->ff_found_docstring = 0;
	ff->ff_last_lineno = -1;
	ff->ff_features = 0;

	if (future_parse(ff, n, filename) < 0) 
	{
		PyMem_Free((void *)ff);
		return NULL;
	}
	return ff;
}

