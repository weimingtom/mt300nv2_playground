/**********************************************************************

  main.c -

  $Author: eban $
  $Date: 2004/10/31 16:06:57 $
  created at: Fri Aug 19 13:19:58 JST 1994

  Copyright (C) 1993-2003 Yukihiro Matsumoto

**********************************************************************/

#include "ruby.h"

#ifdef __human68k__
int _stacksize = 262144;
#endif

#if defined __MINGW32__
int _CRT_glob = 0;
#endif

#if defined(__MACOS__) && defined(__MWERKS__)
#include <console.h>
#endif

/* to link startup code with ObjC support */
#if (defined(__APPLE__) || defined(__NeXT__)) && defined(__MACH__)
static void objcdummyfunction( void ) { objc_msgSend(); }
#endif


#ifdef _MSC_VER
#include <windows.h>
#include <crtdbg.h>
#endif

static int state;
static int run_times;

int
main(argc, argv, envp)
    int argc;
    char **argv, **envp;
{
#if defined(_MSC_VER) && defined(_DEBUG) && USE_VC6_MEMORY_LEAK
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#ifdef _CONSOLE
	_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE);
	_CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDOUT);
	_CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE);
	_CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDOUT);
	_CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
	_CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDOUT);
#else
	_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG);
	_CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_DEBUG);
	_CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_DEBUG);
#endif
#endif

#if 0
#ifdef _WIN32
    NtInitialize(&argc, &argv);
#endif
#if defined(__MACOS__) && defined(__MWERKS__)
    argc = ccommand(&argv);
#endif

    ruby_init();
    ruby_options(argc, argv);
    ruby_run();


#elif 0
	char *ARGV[] = {"ruby.exe", "test_fib.rb", NULL};
#ifdef _WIN32
	NtInitialize(&argc, &argv);
#endif
	argc = 2;
	argv = ARGV;
	ruby_init();
	ruby_options(argc, argv);
	ruby_run();


#elif 0
	ruby_init();
	ruby_init_loadpath();
	rb_load_file("test_fib.rb");
	ruby_stop(ruby_exec());

#else
	
	//ruby reentrant test
	for (run_times = 0; run_times < 2; run_times++) {
		ruby_debug = Qtrue;
		//warning: method redefined; discarding old
		ruby_verbose = Qtrue;

		//see http://my.opera.com/subjam/blog/embedding-ruby-in-c-programs
		//see http://aeditor.rubyforge.org/ruby_cplusplus/index.html
		ruby_init();
		//rb_eval_string("puts 'hello world'");
		//rb_eval_string("printf 'hello'");
		//ruby_finalize();

		//ruby_init();
		//rb_eval_string("puts 'hello world'");
		//rb_eval_string("printf 'hello'");
		//ruby_finalize();
		ruby_init_loadpath();
		rb_load_file("test_fib.rb");
		//rb_exec_end_proc();
		//state = ruby_exec();
		//ruby_cleanup(state);
		ruby_cleanup(ruby_exec());
	}

	//ruby_debug = Qtrue;
	//ruby_verbose = Qtrue;
	//ruby_init();
	//ruby_init_loadpath();
	//rb_load_file("test_fib.rb");
	//state = ruby_exec();
	//ruby_cleanup(state);

#endif

#if defined(_MSC_VER) && defined(_DEBUG) && USE_VC6_MEMORY_LEAK
	_CrtDumpMemoryLeaks();
#endif

    return 0;
}
