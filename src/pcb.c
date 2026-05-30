#include <stdio.h>
#include <string.h>
#include "pcb.h" 
#include "queue.h"

// DEFINE the actual variables here
PCB table[40];
int processCount = 0;
int all_process_sizes[40] = {0};

// NEW: The four waiting rooms
Queue readyQueue;
Queue inputBlocked;
Queue outputBlocked;
Queue diskBlocked;

// Call this once at the very start of main()
void initAllQueues() {
    initQueue(&readyQueue);
    initQueue(&inputBlocked);
    initQueue(&outputBlocked);
    initQueue(&diskBlocked);
}

void init_pcb(PCB *pcb, int id, int start, int end) {
    pcb->processID = id;
    pcb->state = READY;
    pcb->pc = 0;  
    pcb->memLow = start;
    pcb->memHigh = end;
        
    // Store the size in our internal OS array instead of the PCB struct
    if (id >= 0 && id < 40) {
        all_process_sizes[id] = (end - start) + 1;
    }
    printf("PCB Created: ID %d | Bounds [%d-%d]\n", pcb->processID, pcb->memLow, pcb->memHigh);
}
/**
 * Returns the size from our internal OS tracking array.
 * Used by the scheduler to ensure enough room is freed before swapping in.
 */
 int get_process_size(int pid) {
    if (pid >= 0 && pid < 40) {
        return all_process_sizes[pid];
    }
    return 15; // Fallback: 5 PCB + 3 Vars + 7 Instructions
}
/**
 * Updates the state and prints it for the evaluation.
 */
void set_process_state(PCB *pcb, ProcessState newState) {
    pcb->state = newState;
    const char* stateNames[] = {"READY", "RUNNING", "BLOCKED", "FINISHED"};
    printf("Process %d state changed to %s\n", pcb->processID, stateNames[newState]);
}

/**
 * Moves the Program Counter to the next instruction.
 * We must let it increment naturally so the scheduler knows when it finishes!
 */
 void increment_pc(PCB *pcb) {
    pcb->pc++;
}
