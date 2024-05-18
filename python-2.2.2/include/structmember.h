//20180324
#pragma once

#include <stddef.h>

struct memberlist {
	char *name;
	int type;
	int offset;
	int flags;
};

typedef struct PyMemberDef {
	char *name;
	int type;
	int offset;
	int flags;
	char *doc;
} PyMemberDef;

#define T_SHORT 0
#define T_INT 1
#define T_LONG 2
#define T_FLOAT 3
#define T_DOUBLE 4
#define T_STRING 5
#define T_OBJECT 6
#define T_CHAR 7
#define T_BYTE 8
#define T_UBYTE 9
#define T_USHORT 10
#define T_UINT 11
#define T_ULONG 12
#define T_STRING_INPLACE 13
#define T_OBJECT_EX	16	
#define READONLY 1
#define RO READONLY
#define READ_RESTRICTED	2
#define WRITE_RESTRICTED 4
#define RESTRICTED (READ_RESTRICTED | WRITE_RESTRICTED)

PyObject * PyMember_Get(char *, struct memberlist *, char *);
int PyMember_Set(char *, struct memberlist *, char *, PyObject *);
PyObject * PyMember_GetOne(char *, struct PyMemberDef *);
int PyMember_SetOne(char *, struct PyMemberDef *, PyObject *);
