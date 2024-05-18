//20180324
#pragma once

typedef struct {
    int size;
    int (*getname)(Py_UCS4 code, char* buffer, int buflen);
    int (*getcode)(const char* name, int namelen, Py_UCS4* code);
} _PyUnicode_Name_CAPI;
