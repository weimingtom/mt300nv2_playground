//20180406
#include "python.h"
#include <stdio.h>

extern int Py_Main(int, char **);

int main(int argc, char **argv)
{
	return Py_Main(argc, argv);
}
