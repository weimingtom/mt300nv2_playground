//20170403
#pragma once

#include "token.h"

#define MAXINDENT 100

struct tok_state {
	char *buf;
	char *cur;
	char *inp;
	char *end;
	char *start;
	int done;
	FILE *fp;
	int tabsize;
	int indent;
	int indstack[MAXINDENT];
	int atbol;
	int pendin;
	char *prompt, *nextprompt;
	int lineno;
	int level;
	char *filename;
	int altwarning;
	int alterror;
	int alttabsize;
	int altindstack[MAXINDENT];
};

extern struct tok_state *PyTokenizer_FromString(char *);
extern struct tok_state *PyTokenizer_FromFile(FILE *, char *, char *);
extern void PyTokenizer_Free(struct tok_state *);
extern int PyTokenizer_Get(struct tok_state *, char **, char **);
