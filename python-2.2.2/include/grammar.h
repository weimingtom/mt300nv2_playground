//20180318
#pragma once

#include "bitset.h"

typedef struct {
    int		 lb_type;
    char	*lb_str;
} label;

#define EMPTY 0	

typedef struct {
    int		 ll_nlabels;
    label	*ll_label;
} labellist;

typedef struct {
    short	a_lbl;
    short	a_arrow;
} arc;

typedef struct {
    int		 s_narcs;
    arc		*s_arc;
    int		 s_lower;
    int		 s_upper;
    int		*s_accel;
    int		 s_accept;
} state;

typedef struct {
    int		 d_type;
    char	*d_name;
    int		 d_initial;
    int		 d_nstates;
    state	*d_state;
    bitset	 d_first;
} dfa;

typedef struct {
    int		 g_ndfas;
    dfa		*g_dfa;
    labellist	 g_ll;
    int		 g_start;
    int		 g_accel;
} grammar;

grammar *newgrammar(int start);
dfa *adddfa(grammar *g, int type, char *name);
int addstate(dfa *d);
void addarc(dfa *d, int from, int to, int lbl);
dfa *PyGrammar_FindDFA(grammar *g, int type);
int addlabel(labellist *ll, int type, char *str);
int findlabel(labellist *ll, int type, char *str);
char *PyGrammar_LabelRepr(label *lb);
void translatelabels(grammar *g);
void addfirstsets(grammar *g);
void PyGrammar_AddAccelerators(grammar *g);
void PyGrammar_RemoveAccelerators(grammar *);
void printgrammar(grammar *g, FILE *fp);
void printnonterminals(grammar *g, FILE *fp);
