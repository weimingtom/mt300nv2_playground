//20180324
#pragma once

#define NO_EXIT_PROG
typedef void *PyThread_type_lock;
typedef void *PyThread_type_sema;
void PyThread_init_thread();
long PyThread_start_new_thread(void (*)(void *), void *);
void PyThread_exit_thread();
void PyThread__PyThread_exit_thread();
long PyThread_get_thread_ident();
PyThread_type_lock PyThread_allocate_lock();
void PyThread_free_lock(PyThread_type_lock);
int PyThread_acquire_lock(PyThread_type_lock, int);
#define WAIT_LOCK 1
#define NOWAIT_LOCK	0
void PyThread_release_lock(PyThread_type_lock);
PyThread_type_sema PyThread_allocate_sema(int);
void PyThread_free_sema(PyThread_type_sema);
int PyThread_down_sema(PyThread_type_sema, int);
#define WAIT_SEMA 1
#define NOWAIT_SEMA	0
void PyThread_up_sema(PyThread_type_sema);
int PyThread_create_key();
void PyThread_delete_key(int);
int PyThread_set_key_value(int, void *);
void * PyThread_get_key_value(int);
