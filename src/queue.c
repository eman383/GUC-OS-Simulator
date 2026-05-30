#include "queue.h"
#include <stdio.h>

void initQueue(Queue* q) {
    q->head = 0;
    q->tail = 0;
    q->size = 0;
}

bool enqueue(Queue* q, int pid) {
    if (q->size == 40) {
        printf("Error: Queue is full!\n");
        return false;
    }
    q->pids[q->tail] = pid;
    q->tail = (q->tail + 1) % 40; // Wrap around logic
    q->size++;
    return true;
}

int dequeue(Queue* q) {
    if (isEmpty(q)) return -1;

    int pid = q->pids[q->head];
    q->head = (q->head + 1) % 40; // Wrap around logic
    q->size--;
    return pid;
}

bool isEmpty(Queue* q) {
    return q->size == 0;
}

void printQueue(Queue *q) {
    if (isEmpty(q)) {
        printf("[]\n");
        return;
    }

    Queue temp;
    initQueue(&temp);
    
    printf("[");
    int first = 1;
    
    // Dequeue everything to print, and save to temp queue
    while (!isEmpty(q)) {
        int pid = dequeue(q);
        if (!first) {
            printf(", ");
        }
        printf("P%d", pid);
        first = 0;
        
        enqueue(&temp, pid);
    }
    printf("]\n");

    // Restore the original queue in the exact same order
    while (!isEmpty(&temp)) {
        enqueue(q, dequeue(&temp));
    }
}