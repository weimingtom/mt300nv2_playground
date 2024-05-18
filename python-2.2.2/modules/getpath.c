//20180406
#include "python.h"
#include "osdefs.h"

#include <sys/types.h>
#include <string.h>

#if defined(_MSC_VER)
#include <direct.h>
#define getcwd _getcwd
#endif

#define VERSION "2.1"

#define VPATH "."

#define LANDMARK "os.py"

static char prefix[MAXPATHLEN+1];
static char exec_prefix[MAXPATHLEN+1];
static char progpath[MAXPATHLEN+1];
static char *module_search_path = NULL;
static char lib_python[] = "lib/python" VERSION;

static void reduce(char *dir)
{
    size_t i = strlen(dir);
    while (i > 0 && dir[i] != SEP)
    {
		--i;
    }
	dir[i] = '\0';
}


static int isfile(char *filename)
{
    struct stat buf;
    if (stat(filename, &buf) != 0)
	{
        return 0;
    }
	if (!S_ISREG_(buf.st_mode))
    {
		return 0;
    }
	return 1;
}


static int ismodule(char *filename) 
{
    if (isfile(filename))
	{
        return 1;
	}

    if (strlen(filename) < MAXPATHLEN) 
	{
        strcat(filename, Py_OptimizeFlag ? "o" : "c");
        if (isfile(filename))
        {
			return 1;
		}
    }
    return 0;
}


static int isxfile(char *filename)
{
    struct stat buf;
    if (stat(filename, &buf) != 0)
	{
        return 0;
    }
	if (!S_ISREG_(buf.st_mode))
    {
		return 0;
    }
	if ((buf.st_mode & 0111) == 0)
    {
		return 0;
    }
	return 1;
}

static int isdir(char *filename)
{
    struct stat buf;
    if (stat(filename, &buf) != 0)
	{
        return 0;
    }
	if (!S_ISDIR_(buf.st_mode))
    {
		return 0;
    }
	return 1;
}

static void joinpath(char *buffer, char *stuff)
{
    size_t n, k;
    if (stuff[0] == SEP)
	{
        n = 0;
    }
	else 
	{
        n = strlen(buffer);
        if (n > 0 && buffer[n-1] != SEP && n < MAXPATHLEN)
        {
			buffer[n++] = SEP;
		}
    }
    k = strlen(stuff);
    if (n + k > MAXPATHLEN)
    {
		k = MAXPATHLEN - n;
    }
	strncpy(buffer+n, stuff, k);
    buffer[n+k] = '\0';
}

static void copy_absolute(char *path, char *p)
{
    if (p[0] == SEP)
	{
        strcpy(path, p);
    }
	else 
	{
        getcwd(path, MAXPATHLEN);
        if (p[0] == '.' && p[1] == SEP)
		{
            p += 2;
        }
		joinpath(path, p);
    }
}

static void absolutize(char *path)
{
    char buffer[MAXPATHLEN + 1];

    if (path[0] == SEP)
	{
        return;
    }
	copy_absolute(buffer, path);
    strcpy(path, buffer);
}

static int search_for_prefix(char *argv0_path, char *home)
{
    size_t n;
    char *vpath;

    if (home) 
	{
        char *delim;
        strncpy(prefix, home, MAXPATHLEN);
        delim = strchr(prefix, DELIM);
        if (delim)
		{
			*delim = '\0';
        }
		joinpath(prefix, lib_python);
        joinpath(prefix, LANDMARK);
        return 1;
    }

    strcpy(prefix, argv0_path);
    joinpath(prefix, "Modules/Setup");
    if (isfile(prefix)) 
	{
        vpath = VPATH;
        strcpy(prefix, argv0_path);
        joinpath(prefix, vpath);
        joinpath(prefix, "Lib");
        joinpath(prefix, LANDMARK);
        if (ismodule(prefix))
		{
            return -1;
		}
    }

    copy_absolute(prefix, argv0_path);
    do 
	{
        n = strlen(prefix);
        joinpath(prefix, lib_python);
        joinpath(prefix, LANDMARK);
        if (ismodule(prefix))
		{
            return 1;
        }
		prefix[n] = '\0';
        reduce(prefix);
    } while (prefix[0]);

    strncpy(prefix, PREFIX, MAXPATHLEN);
    joinpath(prefix, lib_python);
    joinpath(prefix, LANDMARK);
    if (ismodule(prefix))
	{
        return 1;
	}

    return 0;
}


static int search_for_exec_prefix(char *argv0_path, char *home)
{
    size_t n;

    if (home) 
	{
        char *delim;
        delim = strchr(home, DELIM);
        if (delim)
		{
			strncpy(exec_prefix, delim+1, MAXPATHLEN);
        }
		else
        {
			strncpy(exec_prefix, home, MAXPATHLEN);
        }
		joinpath(exec_prefix, lib_python);
        joinpath(exec_prefix, "lib-dynload");
        return 1;
    }

    strcpy(exec_prefix, argv0_path);
    joinpath(exec_prefix, "Modules/Setup");
    if (isfile(exec_prefix)) 
	{
        reduce(exec_prefix);
        return -1;
    }

    copy_absolute(exec_prefix, argv0_path);
    do 
	{
        n = strlen(exec_prefix);
        joinpath(exec_prefix, lib_python);
        joinpath(exec_prefix, "lib-dynload");
        if (isdir(exec_prefix))
        {
			return 1;
        }
		exec_prefix[n] = '\0';
        reduce(exec_prefix);
    } while (exec_prefix[0]);

    strncpy(exec_prefix, EXEC_PREFIX, MAXPATHLEN);
    joinpath(exec_prefix, lib_python);
    joinpath(exec_prefix, "lib-dynload");
    if (isdir(exec_prefix))
    {
		return 1;
	}

    return 0;
}


static void calculate_path()
{
    extern char *Py_GetProgramName();

    static char delimiter[2] = {DELIM, '\0'};
    static char separator[2] = {SEP, '\0'};
    char *pythonpath = PYTHONPATH;
    char *rtpypath = Py_GETENV("PYTHONPATH");
    char *home = Py_GetPythonHome();
    char *path = getenv("PATH");
    char *prog = Py_GetProgramName();
    char argv0_path[MAXPATHLEN + 1];
    int pfound, efound;
    char *buf;
    size_t bufsz;
    size_t prefixsz;
    char *defpath = pythonpath;

	if (strchr(prog, SEP))
	{
		strncpy(progpath, prog, MAXPATHLEN);
	}
	else if (path) 
	{
		while (1) 
		{
			char *delim = strchr(path, DELIM);

			if (delim) 
			{
				size_t len = delim - path;
				if (len > MAXPATHLEN)
				{
					len = MAXPATHLEN;
				}
				strncpy(progpath, path, len);
				*(progpath + len) = '\0';
			}
			else
			{
				strncpy(progpath, path, MAXPATHLEN);
			}

			joinpath(progpath, prog);
			if (isxfile(progpath))
			{
				break;
			}

			if (!delim) 
			{
				progpath[0] = '\0';
				break;
			}
			path = delim + 1;
		}
	}
	else
	{
		progpath[0] = '\0';
	}
	if (progpath[0] != SEP)
	{
		absolutize(progpath);
	}
	strncpy(argv0_path, progpath, MAXPATHLEN);
	argv0_path[MAXPATHLEN] = '\0';

    reduce(argv0_path);
    
    if (!(pfound = search_for_prefix(argv0_path, home))) 
	{
        if (!Py_FrozenFlag) 
		{
            fprintf(stderr,
                "Could not find platform independent libraries <prefix>\n");
		}
		strncpy(prefix, PREFIX, MAXPATHLEN);
        joinpath(prefix, lib_python);
    }
    else
	{
		reduce(prefix);
	}

    if (!(efound = search_for_exec_prefix(argv0_path, home))) 
	{
        if (!Py_FrozenFlag) 
		{
            fprintf(stderr,
                "Could not find platform dependent libraries <exec_prefix>\n");
        }
		strncpy(exec_prefix, EXEC_PREFIX, MAXPATHLEN);
        joinpath(exec_prefix, "lib/lib-dynload");
    }

    if ((!pfound || !efound) && !Py_FrozenFlag) 
	{
		fprintf(stderr,
                "Consider setting $PYTHONHOME to <prefix>[:<exec_prefix>]\n");
	}

    bufsz = 0;

    if (rtpypath)
	{
        bufsz += strlen(rtpypath) + 1;
	}

    prefixsz = strlen(prefix) + 1;

    while (1) 
	{
        char *delim = strchr(defpath, DELIM);

        if (defpath[0] != SEP)
        {
			bufsz += prefixsz;
		}

        if (delim)
        {
			bufsz += delim - defpath + 1;
        }
		else 
		{
            bufsz += strlen(defpath) + 1;
            break;
        }
        defpath = delim + 1;
    }

    bufsz += strlen(exec_prefix) + 1;

    buf = PyMem_Malloc(bufsz);

    if (buf == NULL) 
	{
        fprintf(stderr, "Not enough memory for dynamic PYTHONPATH.\n");
        fprintf(stderr, "Using default static PYTHONPATH.\n");
        module_search_path = PYTHONPATH;
    }
    else 
	{
        if (rtpypath) 
		{
            strcpy(buf, rtpypath);
            strcat(buf, delimiter);
        }
        else
        {
			buf[0] = '\0';
		}

        defpath = pythonpath;
        while (1) 
		{
            char *delim = strchr(defpath, DELIM);

            if (defpath[0] != SEP) 
			{
                strcat(buf, prefix);
                strcat(buf, separator);
            }

            if (delim) 
			{
                size_t len = delim - defpath + 1;
                size_t end = strlen(buf) + len;
                strncat(buf, defpath, len);
                *(buf + end) = '\0';
            }
            else 
			{
                strcat(buf, defpath);
                break;
            }
            defpath = delim + 1;
        }
        strcat(buf, delimiter);

        strcat(buf, exec_prefix);

        module_search_path = buf;
    }

    if (pfound > 0) 
	{
        reduce(prefix);
        reduce(prefix);
    }
    else
    {
		strncpy(prefix, PREFIX, MAXPATHLEN);
	}

    if (efound > 0) 
	{
        reduce(exec_prefix);
        reduce(exec_prefix);
        reduce(exec_prefix);
    }
    else
    {
		strncpy(exec_prefix, EXEC_PREFIX, MAXPATHLEN);
	}
}

char *Py_GetPath()
{
    if (!module_search_path)
    {
		calculate_path();
    }
	return module_search_path;
}

char *Py_GetPrefix()
{
    if (!module_search_path)
	{
		calculate_path();
    }
	return prefix;
}

char *Py_GetExecPrefix()
{
    if (!module_search_path)
	{
        calculate_path();
    }
	return exec_prefix;
}

char *Py_GetProgramFullPath()
{
    if (!module_search_path)
	{
        calculate_path();
    }
	return progpath;
}
