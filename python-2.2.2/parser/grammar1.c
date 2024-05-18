//20170403
#include "python.h"
#include "pgenheaders.h"
#include "grammar.h"
#include "token.h"

dfa *PyGrammar_FindDFA(grammar *g, int type)
{
	dfa *d;
	d = &g->g_dfa[type - NT_OFFSET];
	assert(d->d_type == type);
	return d;
}

char *PyGrammar_LabelRepr(label *lb)
{
	static char buf[100];
	
	if (lb->lb_type == ENDMARKER)
	{
		return "EMPTY";
	}
	else if (ISNONTERMINAL(lb->lb_type)) 
	{
		if (lb->lb_str == NULL) 
		{
			PyOS_snprintf(buf, sizeof(buf), "NT%d", lb->lb_type);
			return buf;
		}
		else
		{
			return lb->lb_str;
		}
	}
	else 
	{
		if (lb->lb_str == NULL)
		{
			return _PyParser_TokenNames[lb->lb_type];
		}
		else 
		{
			PyOS_snprintf(buf, sizeof(buf), "%.32s(%.32s)",
				_PyParser_TokenNames[lb->lb_type], lb->lb_str);
			return buf;
		}
	}
}
