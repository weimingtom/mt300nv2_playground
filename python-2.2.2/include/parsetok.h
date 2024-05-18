//20180324
#pragma once

typedef struct {
    int error;
    char *filename;
    int lineno;
    int offset;
    char *text;
    int token;
    int expected;
} perrdetail;

#define PyPARSE_YIELD_IS_KEYWORD	0x0001
extern node * PyParser_ParseString(char *, grammar *, int, perrdetail *);
extern node * PyParser_ParseFile (FILE *, char *, grammar *, int, char *, char *, perrdetail *);
extern node * PyParser_ParseStringFlags(char *, grammar *, int, perrdetail *, int);
extern node * PyParser_ParseFileFlags(FILE *, char *, grammar *, int, char *, char *, perrdetail *, int);

