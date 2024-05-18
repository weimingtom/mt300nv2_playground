//20180324
#pragma once

struct _symtable_entry;

struct symtable {
	int st_pass;             
	char *st_filename;       
	struct _symtable_entry *st_cur; 
	PyObject *st_symbols;    
    PyObject *st_stack;  
	PyObject *st_global;     
	int st_nscopes;          
	int st_errors;           
	char *st_private;        
	int st_tmpname;          
	PyFutureFeatures *st_future;
};

typedef struct _symtable_entry {
	PyObject_HEAD
	PyObject *ste_id;        
	PyObject *ste_symbols;   
	PyObject *ste_name;      
	PyObject *ste_varnames;  
	PyObject *ste_children;  
	int ste_type;           
	int ste_lineno;         
	int ste_optimized;      
	int ste_nested;         
	int ste_child_free;     
	int ste_generator;      
	int ste_opt_lineno;     
	struct symtable *ste_table;
} PySymtableEntryObject;

extern PyTypeObject PySymtableEntry_Type;
#define PySymtableEntry_Check(op) ((op)->ob_type == &PySymtableEntry_Type)
extern PyObject * PySymtableEntry_New(struct symtable *, char *, int, int);
struct symtable * PyNode_CompileSymtable(struct _node *, char *);
void PySymtable_Free(struct symtable *);

#define TOP "global"

#define DEF_GLOBAL 1        
#define DEF_LOCAL 2         
#define DEF_PARAM 2<<1      
#define USE 2<<2            
#define DEF_STAR 2<<3       
#define DEF_DOUBLESTAR 2<<4 
#define DEF_INTUPLE 2<<5    
#define DEF_FREE 2<<6       
#define DEF_FREE_GLOBAL 2<<7
#define DEF_FREE_CLASS 2<<8 
#define DEF_IMPORT 2<<9

#define DEF_BOUND (DEF_LOCAL | DEF_PARAM | DEF_IMPORT)

#define TYPE_FUNCTION 1
#define TYPE_CLASS 2
#define TYPE_MODULE 3

#define LOCAL 1
#define GLOBAL_EXPLICIT 2
#define GLOBAL_IMPLICIT 3
#define FREE 4
#define CELL 5

#define OPT_IMPORT_STAR 1
#define OPT_EXEC 2
#define OPT_BARE_EXEC 4
