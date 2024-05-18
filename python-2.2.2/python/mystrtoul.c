//20180123
#include "python.h"

#define Py_CHARMASK(c)		((c) & 0xff)

#include <ctype.h>
#include <errno.h>

unsigned long PyOS_strtoul(char *str, char **ptr, int base)
{
    unsigned long result;
    int	c;
    unsigned long temp;
    int	ovf;	

    result = 0;
    ovf = 0;

    if (base != 0 && (base < 2 || base > 36))
    {
		if (ptr)
		{
			*ptr = str;
		}
		return 0;
    }

    while (*str && isspace(Py_CHARMASK(*str)))
	{
		str++;
	}

    switch (base)
    {
	case 0:
		if (*str == '0')
		{
			str++;
			if (*str == 'x' || *str == 'X')
			{
				str++;
				base = 16;
			}
			else
			{
				base = 8;
			}
		}
		else
		{
			base = 10;
		}
		break;

	case 16:
		if (*str == '0' && (*(str+1) == 'x' || *(str+1) == 'X'))
		{
			str += 2;
		}
		break;
    }


    while ((c = Py_CHARMASK(*str)) != '\0')
    {
		if (isdigit(c) && c - '0' < base)
		{
			c -= '0';
		}
		else
		{
			if (isupper(c))
			{
				c = tolower(c);
			}
			if (c >= 'a' && c <= 'z')
			{
				c -= 'a' - 10;
			}
			else
			{
				break;
			}
			if (c >= base)
			{
				break;
			}
		}
		temp = result;
		result = result * base + c;
		if (base == 10) 
		{
			if(((long)(result - c) / base != (long)temp))
			{
				ovf = 1;
			}
		}
		else 
		{
			if ((result - c) / base != temp)
			{
				ovf = 1;
			}
		}
		str++;
    }

    if (ptr)
	{
		*ptr = str;
    }
	if (ovf)
    {
		result = (unsigned long) ~0L;
		errno = ERANGE;
    }
    return result;
}

long PyOS_strtol(char *str, char **ptr, int base)
{
	long result;
	char sign;
	
	while (*str && isspace(Py_CHARMASK(*str)))
	{
		str++;
	}

	sign = *str;
	if (sign == '+' || sign == '-')
	{
		str++;
	}

	result = (long) PyOS_strtoul(str, ptr, base);
	
	if (result < 0 && !(sign == '-' && result == -result)) 
	{
		errno = ERANGE;
		result = 0x7fffffff;
	}
	
	if (sign == '-')
	{
		result = -result;
	}
	return result;
}
