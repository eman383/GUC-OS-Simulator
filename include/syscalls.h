// include/syscalls.h
#ifndef SYSCALLS_H
#define SYSCALLS_H

#include "pcb.h"

// I/O System Calls
void sys_print(char* message);
void sys_take_input(char* buffer);

// File System Calls
void sys_write_file(char* filename, char* data);
char* sys_read_file(char* filename);

// Memory System Calls (Updated for Key-Value)
void sys_write_mem(char* key, char* value, PCB* p);
char* sys_read_mem(char* key, PCB* p);

#endif