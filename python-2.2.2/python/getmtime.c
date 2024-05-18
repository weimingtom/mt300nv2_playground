//20180118
#include "python.h"
#include "pyconfig.h"

time_t PyOS_GetLastModificationTime(char *path, FILE *fp)
{
	struct stat st;
	if (fstat(fileno(fp), &st) != 0)
	{
		return -1;
	}
	else
	{
		return st.st_mtime;
	}
}
