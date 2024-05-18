//20180123
#include "python.h"
#include <ctype.h>

int PyOS_snprintf(char *str, size_t size, const  char  *format, ...)
{
	int rc;
	va_list va;

	va_start(va, format);
	rc = PyOS_vsnprintf(str, size, format, va);
	va_end(va);
	return rc;
}

int PyOS_vsnprintf(char *str, size_t size, const char  *format, va_list va)
{
	int len;
	assert(str != NULL);
	assert(size > 0);
	assert(format != NULL);

	len = vsnprintf(str, size, format, va);
	str[size-1] = '\0';
	return len;
}
