#ifndef INTERPRETER_H
#define INTERPRETER_H

#include "pcb.h"

// The main function the Scheduler will call to execute one line of code
int execute_instruction(PCB* p, int current_clock);
#endif