#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "queue.h"

// The standard scheduler
void schedule_rr(Queue *rq);

// Advanced schedulers
void schedule_hrrn(Queue *rq);
void schedule_mlfq(Queue *rq);

#endif