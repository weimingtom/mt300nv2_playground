//20180318
#pragma once

extern int PyOS_InterruptOccurred();
extern void PyOS_InitInterrupts();
void PyOS_AfterFork();
