//20170403
#include "pgenheaders.h"
#include "bitset.h"

bitset newbitset(int nbits)
{
	int nbytes = NBYTES(nbits);
	bitset ss = PyMem_NEW(char, nbytes);
	
	if (ss == NULL)
	{
		Py_FatalError("no mem for bitset");
	}
	ss += nbytes;
	while (--nbytes >= 0)
	{
		*--ss = 0;
	}
	return ss;
}

void delbitset(bitset ss)
{
	PyMem_DEL(ss);
}

int addbit(bitset ss, int ibit)
{
	int ibyte = BIT2BYTE(ibit);
	char mask = BIT2MASK(ibit);
	
	if (ss[ibyte] & mask)
	{
		return 0;
	}
	ss[ibyte] |= mask;
	return 1;
}

int samebitset(bitset ss1, bitset ss2, int nbits)
{
	int i;
	
	for (i = NBYTES(nbits); --i >= 0; )
	{
		if (*ss1++ != *ss2++)
		{
			return 0;
		}
	}
	return 1;
}

void mergebitset(bitset ss1, bitset ss2, int nbits)
{
	int i;
	
	for (i = NBYTES(nbits); --i >= 0; )
	{
		*ss1++ |= *ss2++;
	}
}
