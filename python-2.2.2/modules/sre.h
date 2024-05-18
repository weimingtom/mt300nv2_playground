//20180406
#pragma once

#include "sre_constants.h"

#define SRE_CODE unsigned short

typedef struct {
    PyObject_VAR_HEAD
    int groups;
    PyObject* groupindex;
    PyObject* indexgroup;
	PyObject* pattern;
    int flags;
    int codesize;
    SRE_CODE code[1];
} PatternObject;

#define PatternObject_GetCode(o) (((PatternObject*)(o))->code)

typedef struct {
    PyObject_VAR_HEAD
    PyObject* string;
    PyObject* regs; 
    PatternObject* pattern; 
    int pos, endpos;
    int lastindex; 
    int groups;
    int mark[1];
} MatchObject;

typedef unsigned int (*SRE_TOLOWER_HOOK)(unsigned int ch);

#define SRE_MARK_SIZE 200

typedef struct SRE_REPEAT_T {
    int count;
    SRE_CODE* pattern;
    struct SRE_REPEAT_T *prev;
} SRE_REPEAT;

typedef struct {
    void* ptr;
    void* beginning;
    void* start;
    void* end; 
    PyObject* string;
    int pos, endpos;
    int charsize;
    int lastindex;
    int lastmark;
    void* mark[SRE_MARK_SIZE];
    void** mark_stack;
    int mark_stack_size;
    int mark_stack_base;
    SRE_REPEAT *repeat;
    SRE_TOLOWER_HOOK lower;
} SRE_STATE;

typedef struct {
    PyObject_HEAD
    PyObject* pattern;
    SRE_STATE state;
} ScannerObject;
