#ifndef QUEUE_H
#define QUEUE_H

#include <stdbool.h>

typedef struct {
    int pids[40];    // Array to store Process IDs
    int head;        // Index of the front element
    int tail;        // Index of the next available slot
    int size;        // Current number of elements
} Queue;

// Function Prototypes
void initQueue(Queue* q);
bool enqueue(Queue* q, int pid);
int dequeue(Queue* q);
bool isEmpty(Queue* q);
void printQueue(Queue *q);

#endif