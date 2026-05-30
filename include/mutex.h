#ifndef MUTEX_H
#define MUTEX_H

#include "pcb.h"
#include "queue.h"

typedef struct {
    int value;         // 1 for free, 0 for locked
    Queue* blockedQ;   // Pointer to the specific queue (e.g., &diskBlocked)
    int ownerID;       // Which PID currently has the lock (-1 if none)
} Semaphore;

// Global resource locks
extern Semaphore userInput;
extern Semaphore userOutput;
extern Semaphore fileSystem;

void initSemaphores();
int semWait(Semaphore* s, PCB* p);
void semSignal(Semaphore* s, PCB* p);

#endif