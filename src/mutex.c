#include "mutex.h"
#include <stdio.h>

extern void set_process_state(PCB *pcb, ProcessState newState);
extern bool enqueue(Queue* q, int pid);
extern int dequeue(Queue* q);
extern bool isEmpty(Queue* q);
extern Queue readyQueue;
extern PCB table[40];
extern int processCount;

Semaphore userInput;
Semaphore userOutput;
Semaphore fileSystem;

void initSemaphores() {
    userInput.value = 1;
    userInput.blockedQ = &inputBlocked;
    userInput.ownerID = -1;

    userOutput.value = 1;
    userOutput.blockedQ = &outputBlocked;
    userOutput.ownerID = -1;

    fileSystem.value = 1;
    fileSystem.blockedQ = &diskBlocked;
    fileSystem.ownerID = -1;
}

int semWait(Semaphore* s, PCB* p) {
    // If the lock is free OR this process was pre-granted ownership
    // via a direct transfer from semSignal, allow it through
    if (s->value == 1 || s->ownerID == p->processID) {
        s->value = 0;
        s->ownerID = p->processID;
        return 1;
    } else {
        // Resource is truly held by someone else — block
        set_process_state(p, BLOCKED);
        enqueue(s->blockedQ, p->processID);
        return 0;
    }
}

void semSignal(Semaphore* s, PCB* p) {
    // Only the owner can release the lock
    if (s->ownerID != p->processID) return;

    if (!isEmpty(s->blockedQ)) {
        // Transfer ownership directly to the next waiting process
        int nextPID = dequeue(s->blockedQ);
        s->ownerID = nextPID;
        // s->value stays 0 — lock remains held by new owner

        for (int i = 0; i < processCount; i++) {
            if (table[i].processID == nextPID) {
                set_process_state(&table[i], READY);
                enqueue(&readyQueue, nextPID);
                break;
            }
        }
    } else {
        // Nobody waiting — fully release
        s->value = 1;
        s->ownerID = -1;
    }
}