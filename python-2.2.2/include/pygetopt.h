//20180324
#pragma once

extern int _PyOS_opterr;
extern int _PyOS_optind;
extern char * _PyOS_optarg;
int _PyOS_GetOpt(int argc, char **argv, char *optstring);
