//20170403
#pragma once

#define MAXSTACK 500

typedef struct {
	int s_state;
	dfa	*s_dfa;
	struct _node *s_parent;
} stackentry;

typedef struct {
	stackentry *s_top;
	stackentry s_base[MAXSTACK];
} stack;

typedef struct {
	stack p_stack;	
	grammar *p_grammar;	
	node *p_tree;	
	int	p_generators;
} parser_state;

parser_state *PyParser_New(grammar *g, int start);
void PyParser_Delete(parser_state *ps);
int PyParser_AddToken(parser_state *ps, int type, char *str, int lineno, int *expected_ret);
void PyGrammar_AddAccelerators(grammar *g);
