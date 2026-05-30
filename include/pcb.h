// include/pcb.h
#ifndef PCB_H
#define PCB_H
#include "queue.h" // Add this at the top of pcb.h if it isn't there

// Export the queues so main.c and memory.c can see them
extern Queue readyQueue;
extern Queue inputBlocked;
extern Queue outputBlocked;
extern Queue diskBlocked;

extern void initAllQueues();
// Shared Enum for the 4 states required by the project
typedef enum {
    READY, 
    RUNNING, 
    BLOCKED, 
    FINISHED
} ProcessState;

/**
 * PCB Structure: Strictly tracking the 4 required values.
 */
typedef struct {
    int processID;          // 1. ProcessID
    ProcessState state;     // 2. Process State
    int pc;                 // 3. Program Counter
    int memLow;             // 4. Memory Boundaries (Start)
    int memHigh;            // 4. Memory Boundaries (End)
} PCB;

// Global Process Table and Count
extern PCB table[40]; 
extern int processCount; 
// Function Prototypes
void init_pcb(PCB *pcb, int id, int start, int end);
void set_process_state(PCB *pcb, ProcessState newState);
void increment_pc(PCB *pcb);

// Added so the scheduler can look up original sizes when bringing processes back from disk
int get_process_size(int pid); 

#endif