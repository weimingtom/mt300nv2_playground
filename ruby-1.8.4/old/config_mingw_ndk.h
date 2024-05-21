
#define PACKAGE_NAME ""
#define PACKAGE_TARNAME ""
#define PACKAGE_VERSION ""
#define PACKAGE_STRING ""
#define PACKAGE_BUGREPORT ""
#define USE_BUILTIN_FRAME_ADDRESS 1
#define STDC_HEADERS 1
#define HAVE_SYS_TYPES_H 1
#define HAVE_SYS_STAT_H 1
#define HAVE_STDLIB_H 1
#define HAVE_STRING_H 1
#define HAVE_MEMORY_H 1
#define HAVE_STRINGS_H 1
#define HAVE_INTTYPES_H 1
#define HAVE_STDINT_H 1
#define HAVE_UNISTD_H 1
#define _FILE_OFFSET_BITS 64
#define HAVE_LONG_LONG 1
#define HAVE_OFF_T 1
#define SIZEOF_INT 4
#define SIZEOF_SHORT 2
#define SIZEOF_LONG 4
#define SIZEOF_LONG_LONG 8
#ifdef __MINGW32__
#define SIZEOF___INT64 8
#define SIZEOF_OFF_T 4
#else
#define SIZEOF___INT64 0
#define SIZEOF_OFF_T 8
#endif
#define SIZEOF_VOIDP 4
#define SIZEOF_FLOAT 4
#define SIZEOF_DOUBLE 8
#define SIZEOF_TIME_T 4
#define rb_pid_t pid_t
#ifdef __MINGW32__
#define rb_gid_t int
#define rb_uid_t int
#else
#define rb_gid_t gid_t
#define rb_uid_t uid_t
#endif
#define HAVE_PROTOTYPES 1
#define TOKEN_PASTE(x,y) x##y
#define HAVE_STDARG_PROTOTYPES 1
#define NORETURN(x) x __attribute__ ((noreturn))
#define HAVE_DECL_SYS_NERR 1
//#define HAVE_LIBCRYPT 1
#ifndef __MINGW32__
#define HAVE_LIBDL 1
#endif
#define HAVE_DIRENT_H 1
#define STDC_HEADERS 1
#ifndef __MINGW32__
#define HAVE_SYS_WAIT_H 1
#endif
#define HAVE_STDLIB_H 1
#define HAVE_STRING_H 1
#define HAVE_UNISTD_H 1
#define HAVE_LIMITS_H 1
#define HAVE_SYS_FILE_H 1
#ifndef __MINGW32__
#define HAVE_SYS_IOCTL_H 1
#endif
//#define HAVE_SYS_SYSCALL_H 1
#define HAVE_FCNTL_H 1
#define HAVE_SYS_FCNTL_H 1
#ifndef __MINGW32__
#define HAVE_SYS_SELECT_H 1
#endif
#define HAVE_SYS_TIME_H 1
#ifndef __MINGW32__
#define HAVE_SYS_TIMES_H 1
#define HAVE_SYS_PARAM_H 1
//#define HAVE_SYSCALL_H 1
#define HAVE_PWD_H 1
#define HAVE_GRP_H 1
#define HAVE_A_OUT_H 1
#define HAVE_UTIME_H 1
#endif
#define HAVE_MEMORY_H 1
#ifndef __MINGW32__
#define HAVE_SYS_RESOURCE_H 1
#define HAVE_NETINET_IN_SYSTM_H 1
#endif
#ifdef __MINGW32__
#define HAVE_SYS_UTIME_H 1
#endif
#define HAVE_FLOAT_H 1
#ifndef __MINGW32__
#define HAVE_PTHREAD_H 1
#define HAVE_UCONTEXT_H 1
#define HAVE_STRUCT_STAT_ST_BLKSIZE 1
#define HAVE_ST_BLKSIZE 1
#define HAVE_STRUCT_STAT_ST_BLOCKS 1
#define HAVE_ST_BLOCKS 1
#endif
#define HAVE_STRUCT_STAT_ST_RDEV 1
#define HAVE_ST_RDEV 1
#ifdef __MINGW32__
#define uid_t int
#define gid_t int
#endif
#ifdef __MINGW32__
#define GETGROUPS_T int
#else
#define GETGROUPS_T gid_t
#endif
#define RETSIGTYPE void
#ifndef __MINGW32__
#define HAVE_ALLOCA_H 1
#endif
#define HAVE_ALLOCA 1
#ifndef __MINGW32__
#define HAVE_FSEEKO 1
#define HAVE_FTELLO 1
#endif
#define HAVE_DUP2 1
#define HAVE_MEMMOVE 1
#define HAVE_STRCASECMP 1
#define HAVE_STRNCASECMP 1
#define HAVE_STRERROR 1
#define HAVE_STRFTIME 1
#define HAVE_STRCHR 1
#define HAVE_STRSTR 1
#define HAVE_STRTOUL 1
#ifndef __MINGW32__
//#define HAVE_CRYPT 1
#define HAVE_FLOCK 1
#endif
#define HAVE_VSNPRINTF 1
#define HAVE_ISNAN 1
#define HAVE_FINITE 1
#define HAVE_ISINF 1
#define HAVE_HYPOT 1
#define HAVE_ACOSH 1
#define HAVE_ERF 1
#define HAVE_FMOD 1
#ifndef __MINGW32__
#define HAVE_KILLPG 1
#define HAVE_WAIT4 1
#endif
#define HAVE_WAITPID 1
#ifndef __MINGW32__
#define HAVE_SYSCALL 1
#define HAVE_CHROOT 1
#endif
#define HAVE_FSYNC 1
#define HAVE_GETCWD 1
#ifndef __MINGW32__
#define HAVE_TRUNCATE 1
#endif
#ifdef __MINGW32__
#define HAVE_CHSIZE 1
#endif
#define HAVE_TIMES 1
#ifndef __MINGW32__
#define HAVE_UTIMES 1
#define HAVE_FCNTL 1
#define HAVE_LOCKF 1
#define HAVE_LSTAT 1
#define HAVE_SYMLINK 1
#define HAVE_LINK 1
#define HAVE_READLINK 1
#define HAVE_SETITIMER 1
#define HAVE_SETEUID 1
#define HAVE_SETREUID 1
#define HAVE_SETRESUID 1
#define HAVE_SETEGID 1
#define HAVE_SETREGID 1
#define HAVE_SETRESGID 1
#define HAVE_PAUSE 1
#define HAVE_LCHOWN 1
#define HAVE_GETPGRP 1
#define HAVE_SETPGRP 1
#define HAVE_GETPGID 1
#define HAVE_SETPGID 1
#define HAVE_INITGROUPS 1
#define HAVE_GETGROUPS 1
#define HAVE_SETGROUPS 1
#define HAVE_GETPRIORITY 1
#define HAVE_GETRLIMIT 1
#define HAVE_DLOPEN 1
#define HAVE_SIGPROCMASK 1
#define HAVE_SIGACTION 1
#endif
#define HAVE__SETJMP 1
#ifndef __MINGW32__
#define HAVE_SETSID 1
#endif
#ifdef __MINGW32__
#define HAVE_TELLDIR 1
#define HAVE_SEEKDIR 1
#endif
//#define HAVE_TELLDIR 1
//#define HAVE_SEEKDIR 1
#ifndef __MINGW32__
#define HAVE_FCHMOD 1
#endif
#define HAVE_MKTIME 1
//#define HAVE_TIMEGM 1
#define HAVE_COSH 1
#define HAVE_SINH 1
#define HAVE_TANH 1
#ifndef __MINGW32__
#define HAVE_SETUID 1
#define HAVE_SETGID 1
#define HAVE_STRUCT_TM_TM_ZONE 1
#define HAVE_TM_ZONE 1
#define HAVE_STRUCT_TM_TM_GMTOFF 1
#endif
#define HAVE_DAYLIGHT 1
#ifndef __MINGW32__
#define NEGATIVE_TIME_T 1
#define POSIX_SIGNAL 1
#define GETPGRP_VOID 1
#endif
#define SETPGRP_VOID 1
#define RSHIFT(x,y) ((x)>>(int)y)
#ifdef __MINGW32__
#define FILE_COUNT _cnt
#define FILE_READPTR _ptr
#endif
//#define FILE_READPTR _IO_read_ptr
//#define FILE_READEND _IO_read_end
#define HUGE_ST_INO 1
#define STACK_GROW_DIRECTION -1
#define DEFAULT_KCODE KCODE_NONE
#define USE_ELF 1
#define DLEXT ".so"
#define RUBY_LIB "/usr/local/lib/ruby/1.8"
#define RUBY_SITE_LIB "/usr/local/lib/ruby/site_ruby"
#define RUBY_SITE_LIB2 "/usr/local/lib/ruby/site_ruby/1.8"
#define RUBY_PLATFORM "i686-linux"
#define RUBY_ARCHLIB "/usr/local/lib/ruby/1.8/i686-linux"
#define RUBY_SITE_ARCHLIB "/usr/local/lib/ruby/site_ruby/1.8/i686-linux"
