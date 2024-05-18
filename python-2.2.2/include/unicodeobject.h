//20180324
#pragma once

#include <ctype.h>

#  define HAVE_WCHAR_H

#  include <wchar.h>

typedef unsigned int Py_UCS4; 

typedef PY_UNICODE_TYPE Py_UNICODE;



# define PyUnicode_AsASCIIString PyUnicodeUCS2_AsASCIIString
# define PyUnicode_AsCharmapString PyUnicodeUCS2_AsCharmapString
# define PyUnicode_AsEncodedString PyUnicodeUCS2_AsEncodedString
# define PyUnicode_AsLatin1String PyUnicodeUCS2_AsLatin1String
# define PyUnicode_AsRawUnicodeEscapeString PyUnicodeUCS2_AsRawUnicodeEscapeString
# define PyUnicode_AsUTF16String PyUnicodeUCS2_AsUTF16String
# define PyUnicode_AsUTF8String PyUnicodeUCS2_AsUTF8String
# define PyUnicode_AsUnicode PyUnicodeUCS2_AsUnicode
# define PyUnicode_AsUnicodeEscapeString PyUnicodeUCS2_AsUnicodeEscapeString
# define PyUnicode_AsWideChar PyUnicodeUCS2_AsWideChar
# define PyUnicode_Compare PyUnicodeUCS2_Compare
# define PyUnicode_Concat PyUnicodeUCS2_Concat
# define PyUnicode_Contains PyUnicodeUCS2_Contains
# define PyUnicode_Count PyUnicodeUCS2_Count
# define PyUnicode_Decode PyUnicodeUCS2_Decode
# define PyUnicode_DecodeASCII PyUnicodeUCS2_DecodeASCII
# define PyUnicode_DecodeCharmap PyUnicodeUCS2_DecodeCharmap
# define PyUnicode_DecodeLatin1 PyUnicodeUCS2_DecodeLatin1
# define PyUnicode_DecodeRawUnicodeEscape PyUnicodeUCS2_DecodeRawUnicodeEscape
# define PyUnicode_DecodeUTF16 PyUnicodeUCS2_DecodeUTF16
# define PyUnicode_DecodeUTF8 PyUnicodeUCS2_DecodeUTF8
# define PyUnicode_DecodeUnicodeEscape PyUnicodeUCS2_DecodeUnicodeEscape
# define PyUnicode_Encode PyUnicodeUCS2_Encode
# define PyUnicode_EncodeASCII PyUnicodeUCS2_EncodeASCII
# define PyUnicode_EncodeCharmap PyUnicodeUCS2_EncodeCharmap
# define PyUnicode_EncodeDecimal PyUnicodeUCS2_EncodeDecimal
# define PyUnicode_EncodeLatin1 PyUnicodeUCS2_EncodeLatin1
# define PyUnicode_EncodeRawUnicodeEscape PyUnicodeUCS2_EncodeRawUnicodeEscape
# define PyUnicode_EncodeUTF16 PyUnicodeUCS2_EncodeUTF16
# define PyUnicode_EncodeUTF8 PyUnicodeUCS2_EncodeUTF8
# define PyUnicode_EncodeUnicodeEscape PyUnicodeUCS2_EncodeUnicodeEscape
# define PyUnicode_Find PyUnicodeUCS2_Find
# define PyUnicode_Format PyUnicodeUCS2_Format
# define PyUnicode_FromEncodedObject PyUnicodeUCS2_FromEncodedObject
# define PyUnicode_FromObject PyUnicodeUCS2_FromObject
# define PyUnicode_FromUnicode PyUnicodeUCS2_FromUnicode
# define PyUnicode_FromWideChar PyUnicodeUCS2_FromWideChar
# define PyUnicode_GetDefaultEncoding PyUnicodeUCS2_GetDefaultEncoding
# define PyUnicode_GetMax PyUnicodeUCS2_GetMax
# define PyUnicode_GetSize PyUnicodeUCS2_GetSize
# define PyUnicode_Join PyUnicodeUCS2_Join
# define PyUnicode_Replace PyUnicodeUCS2_Replace
# define PyUnicode_Resize PyUnicodeUCS2_Resize
# define PyUnicode_SetDefaultEncoding PyUnicodeUCS2_SetDefaultEncoding
# define PyUnicode_Split PyUnicodeUCS2_Split
# define PyUnicode_Splitlines PyUnicodeUCS2_Splitlines
# define PyUnicode_Tailmatch PyUnicodeUCS2_Tailmatch
# define PyUnicode_Translate PyUnicodeUCS2_Translate
# define PyUnicode_TranslateCharmap PyUnicodeUCS2_TranslateCharmap
# define _PyUnicode_AsDefaultEncodedString _PyUnicodeUCS2_AsDefaultEncodedString
# define _PyUnicode_Fini _PyUnicodeUCS2_Fini
# define _PyUnicode_Init _PyUnicodeUCS2_Init
# define _PyUnicode_IsAlpha _PyUnicodeUCS2_IsAlpha
# define _PyUnicode_IsDecimalDigit _PyUnicodeUCS2_IsDecimalDigit
# define _PyUnicode_IsDigit _PyUnicodeUCS2_IsDigit
# define _PyUnicode_IsLinebreak _PyUnicodeUCS2_IsLinebreak
# define _PyUnicode_IsLowercase _PyUnicodeUCS2_IsLowercase
# define _PyUnicode_IsNumeric _PyUnicodeUCS2_IsNumeric
# define _PyUnicode_IsTitlecase _PyUnicodeUCS2_IsTitlecase
# define _PyUnicode_IsUppercase _PyUnicodeUCS2_IsUppercase
# define _PyUnicode_IsWhitespace _PyUnicodeUCS2_IsWhitespace
# define _PyUnicode_ToDecimalDigit _PyUnicodeUCS2_ToDecimalDigit
# define _PyUnicode_ToDigit _PyUnicodeUCS2_ToDigit
# define _PyUnicode_ToLowercase _PyUnicodeUCS2_ToLowercase
# define _PyUnicode_ToNumeric _PyUnicodeUCS2_ToNumeric
# define _PyUnicode_ToTitlecase _PyUnicodeUCS2_ToTitlecase
# define _PyUnicode_ToUppercase _PyUnicodeUCS2_ToUppercase


#define Py_UNICODE_ISSPACE(ch) _PyUnicode_IsWhitespace(ch)

#define Py_UNICODE_ISLOWER(ch) _PyUnicode_IsLowercase(ch)
#define Py_UNICODE_ISUPPER(ch) _PyUnicode_IsUppercase(ch)
#define Py_UNICODE_ISTITLE(ch) _PyUnicode_IsTitlecase(ch)
#define Py_UNICODE_ISLINEBREAK(ch) _PyUnicode_IsLinebreak(ch)

#define Py_UNICODE_TOLOWER(ch) _PyUnicode_ToLowercase(ch)
#define Py_UNICODE_TOUPPER(ch) _PyUnicode_ToUppercase(ch)
#define Py_UNICODE_TOTITLE(ch) _PyUnicode_ToTitlecase(ch)

#define Py_UNICODE_ISDECIMAL(ch) _PyUnicode_IsDecimalDigit(ch)
#define Py_UNICODE_ISDIGIT(ch) _PyUnicode_IsDigit(ch)
#define Py_UNICODE_ISNUMERIC(ch) _PyUnicode_IsNumeric(ch)

#define Py_UNICODE_TODECIMAL(ch) _PyUnicode_ToDecimalDigit(ch)
#define Py_UNICODE_TODIGIT(ch) _PyUnicode_ToDigit(ch)
#define Py_UNICODE_TONUMERIC(ch) _PyUnicode_ToNumeric(ch)

#define Py_UNICODE_ISALPHA(ch) _PyUnicode_IsAlpha(ch)

#define Py_UNICODE_ISALNUM(ch) \
       (Py_UNICODE_ISALPHA(ch) || \
        Py_UNICODE_ISDECIMAL(ch) || \
        Py_UNICODE_ISDIGIT(ch) || \
        Py_UNICODE_ISNUMERIC(ch))

#define Py_UNICODE_COPY(target, source, length)\
    (memcpy((target), (source), (length)*sizeof(Py_UNICODE)))

#define Py_UNICODE_FILL(target, value, length) do\
    {int i; for (i = 0; i < (length); i++) (target)[i] = (value);}\
    while (0)

#define Py_UNICODE_MATCH(string, offset, substring)\
    ((*((string)->str + (offset)) == *((substring)->str)) &&\
     !memcmp((string)->str + (offset), (substring)->str,\
             (substring)->length*sizeof(Py_UNICODE)))

typedef struct {
    PyObject_HEAD
    int length;	
    Py_UNICODE *str;
    long hash;		
    PyObject *defenc;
} PyUnicodeObject;

extern PyTypeObject PyUnicode_Type;

#define PyUnicode_Check(op) PyObject_TypeCheck(op, &PyUnicode_Type)
#define PyUnicode_CheckExact(op) ((op)->ob_type == &PyUnicode_Type)

#define PyUnicode_GET_SIZE(op) \
        (((PyUnicodeObject *)(op))->length)
#define PyUnicode_GET_DATA_SIZE(op) \
        (((PyUnicodeObject *)(op))->length * sizeof(Py_UNICODE))
#define PyUnicode_AS_UNICODE(op) \
        (((PyUnicodeObject *)(op))->str)
#define PyUnicode_AS_DATA(op) \
        ((const char *)((PyUnicodeObject *)(op))->str)


#define Py_UNICODE_REPLACEMENT_CHARACTER ((Py_UNICODE) 0xFFFD)

extern PyObject* PyUnicode_FromUnicode(
    const Py_UNICODE *u,        
    int size                    
    );

extern Py_UNICODE * PyUnicode_AsUnicode(
    PyObject *unicode	 	
    );

extern int PyUnicode_GetSize(
    PyObject *unicode	 	
    );

extern Py_UNICODE PyUnicode_GetMax();

extern int PyUnicode_Resize(
    PyObject **unicode,		
    int length			
    );

extern PyObject* PyUnicode_FromEncodedObject(
    PyObject *obj, 	
    const char *encoding,       
    const char *errors          
    );

extern PyObject* PyUnicode_FromObject(
    PyObject *obj 
    );

extern PyObject* PyUnicode_FromWideChar(
    const wchar_t *w,  
    int size                    
    );

extern int PyUnicode_AsWideChar(
    PyUnicodeObject *unicode,  
    wchar_t *w,       
    int size                   
    );

extern PyObject* PyUnicode_FromOrdinal(int ordinal);

extern PyObject * _PyUnicode_AsDefaultEncodedString(
    PyObject *, const char *);

extern const char* PyUnicode_GetDefaultEncoding();

extern int PyUnicode_SetDefaultEncoding(
    const char *encoding
    );

extern PyObject* PyUnicode_Decode(
    const char *s,              
    int size,                   
    const char *encoding,       
    const char *errors          
    );

extern PyObject* PyUnicode_Encode(
    const Py_UNICODE *s,       
    int size,                  
    const char *encoding,      
    const char *errors         
    );

extern PyObject* PyUnicode_AsEncodedString(
    PyObject *unicode,	 
    const char *encoding,
    const char *errors	
    );

extern PyObject* PyUnicode_DecodeUTF7(
    const char *string, 
    int length,	 
    const char *errors
    );

extern PyObject* PyUnicode_EncodeUTF7(
    const Py_UNICODE *data, 
    int length,	 	
    int encodeSetO,      
    int encodeWhiteSpace,     
    const char *errors	
    );

extern PyObject* PyUnicode_DecodeUTF8(
    const char *string, 
    int length,	 	
    const char *errors	
    );

extern PyObject* PyUnicode_AsUTF8String(
    PyObject *unicode	 
    );

extern PyObject* PyUnicode_EncodeUTF8(
    const Py_UNICODE *data, 
    int length,	 	
    const char *errors	
    );

extern PyObject* PyUnicode_DecodeUTF16(
    const char *string, 
    int length,	 		
    const char *errors,	
    int *byteorder
    );

extern PyObject* PyUnicode_AsUTF16String(
    PyObject *unicode	
    );

extern PyObject* PyUnicode_EncodeUTF16(
    const Py_UNICODE *data, 
    int length,	 	
    const char *errors,	
    int byteorder	
    );

extern PyObject* PyUnicode_DecodeUnicodeEscape(
    const char *string, 
    int length,	 
    const char *errors	
    );

extern PyObject* PyUnicode_AsUnicodeEscapeString(
    PyObject *unicode	 
    );

extern PyObject* PyUnicode_EncodeUnicodeEscape(
    const Py_UNICODE *data, 
    int length	 	
    );

extern PyObject* PyUnicode_DecodeRawUnicodeEscape(
    const char *string, 
    int length,	 	
    const char *errors
    );

extern PyObject* PyUnicode_AsRawUnicodeEscapeString(
    PyObject *unicode	
    );

extern PyObject* PyUnicode_EncodeRawUnicodeEscape(
    const Py_UNICODE *data, 
    int length	 	
    );

extern PyObject* PyUnicode_DecodeLatin1(
    const char *string, 
    int length,	 	
    const char *errors	
    );

extern PyObject* PyUnicode_AsLatin1String(
    PyObject *unicode	
    );

extern PyObject* PyUnicode_EncodeLatin1(
    const Py_UNICODE *data, 
    int length,	 
    const char *errors	
    );

extern PyObject* PyUnicode_DecodeASCII(
    const char *string, 
    int length,	 	
    const char *errors	
    );

extern PyObject* PyUnicode_AsASCIIString(
    PyObject *unicode	
    );

extern PyObject* PyUnicode_EncodeASCII(
    const Py_UNICODE *data, 
    int length,	 
    const char *errors	
    );

extern PyObject* PyUnicode_DecodeCharmap(
    const char *string, 
    int length,	 
    PyObject *mapping,	
    const char *errors	
    );

extern PyObject* PyUnicode_AsCharmapString(
    PyObject *unicode,	 
    PyObject *mapping	
    );

extern PyObject* PyUnicode_EncodeCharmap(
    const Py_UNICODE *data, 
    int length,	 	
    PyObject *mapping,	
    const char *errors
    );

extern PyObject * PyUnicode_TranslateCharmap(
    const Py_UNICODE *data, 
    int length,	 
    PyObject *table,	
    const char *errors	
    );

extern PyObject* PyUnicode_AsMBCSString(
    PyObject *unicode     
    );

extern int PyUnicode_EncodeDecimal(
    Py_UNICODE *s,	
    int length,		
    char *output,	
    const char *errors
    );

extern PyObject* PyUnicode_Concat(
    PyObject *left,	 
    PyObject *right	
    );

extern PyObject* PyUnicode_Split(
    PyObject *s,	
    PyObject *sep,	
    int maxsplit	
    );		

extern PyObject* PyUnicode_Splitlines(
    PyObject *s,		
    int keepends		
    );		



extern PyObject * PyUnicode_Translate(
    PyObject *str,	
    PyObject *table,	
    const char *errors	
    );
    
extern PyObject* PyUnicode_Join(
    PyObject *separator, 
    PyObject *seq
    );

extern int PyUnicode_Tailmatch(
    PyObject *str,	
    PyObject *substr,	
    int start,		
    int end,	
    int direction	
    );

extern int PyUnicode_Find(
    PyObject *str,	 
    PyObject *substr,
    int start,		
    int end,		
    int direction
    );

extern int PyUnicode_Count(
    PyObject *str,	
    PyObject *substr,	
    int start,		
    int end		
    );

extern PyObject * PyUnicode_Replace(
    PyObject *str,	 
    PyObject *substr,
    PyObject *replstr,
    int maxcount	
    );

extern int PyUnicode_Compare(
    PyObject *left,	
    PyObject *right	
    );

extern PyObject * PyUnicode_Format(
    PyObject *format,	
    PyObject *args	
    );

extern int PyUnicode_Contains(
    PyObject *container,
    PyObject *element	
    );

extern PyObject * _PyUnicode_XStrip(
    PyUnicodeObject *self,
    int striptype,
    PyObject *sepobj
    );

extern int _PyUnicode_IsLowercase(
    Py_UNICODE ch 
    );

extern int _PyUnicode_IsUppercase(
    Py_UNICODE ch 
    );

extern int _PyUnicode_IsTitlecase(
    Py_UNICODE ch 
    );

extern int _PyUnicode_IsWhitespace(
    Py_UNICODE ch 
    );

extern int _PyUnicode_IsLinebreak(
    Py_UNICODE ch 
    );

extern Py_UNICODE _PyUnicode_ToLowercase(
    Py_UNICODE ch 
    );

extern Py_UNICODE _PyUnicode_ToUppercase(
    Py_UNICODE ch 
    );

extern Py_UNICODE _PyUnicode_ToTitlecase(
    Py_UNICODE ch 
    );

extern int _PyUnicode_ToDecimalDigit(
    Py_UNICODE ch 
    );

extern int _PyUnicode_ToDigit(
    Py_UNICODE ch 
    );

extern double _PyUnicode_ToNumeric(
    Py_UNICODE ch 
    );

extern int _PyUnicode_IsDecimalDigit(
    Py_UNICODE ch 
    );

extern int _PyUnicode_IsDigit(
    Py_UNICODE ch 
    );

extern int _PyUnicode_IsNumeric(
    Py_UNICODE ch 
    );

extern int _PyUnicode_IsAlpha(
    Py_UNICODE ch 
    );

