#include <stdio.h>
#include "scheduler.h"
#include "pcb.h"
#include "interpreter.h"
#include "queue.h"

// Access the global table and memory functions
extern PCB table[40];
extern int processCount;
extern int currentCount; 

// Access the global queues defined in pcb.c
extern Queue readyQueue;
extern Queue inputBlocked;
extern Queue outputBlocked;
extern Queue diskBlocked;

// External functions from memory.c & loader
extern int chooseVictim(int runningPid);
extern void swapOut(int pid);
extern void swapIn(int pid);
extern void compactMemory();
extern void freeMemory(int pid); 
extern void printMemory(); 
extern void writeToMemory(int pid, char* key, char* value); 
extern void load_program(const char* filename, int pid);

#define MAX_SIZE 40 
#define TIME_SLICE 2 
extern int get_process_size(int pid); 

// Grader-configurable arrival times: index 0 = P1, 1 = P2, 2 = P3, ...
static const int arrival_times_config[] = {0, 1, 4};
#define ARRIVAL_CONFIG_COUNT (sizeof(arrival_times_config) / sizeof(arrival_times_config[0]))

static const char* arrival_programs[] = {
    "../programs/Program_1.txt",
    "../programs/Program_2.txt",
    "../programs/Program_3.txt",
};

static void apply_arrival_config(void) {
    for (int i = 0; i < ARRIVAL_CONFIG_COUNT; i++) {
        arrival_times[i + 1] = arrival_times_config[i];
    }
}

// =========================================================
// AUXILIARY SHADOW TABLES & TIMING
// =========================================================
int arrival_times[100] = {0}; 
int burst_times[100] = {0};
int system_clock = 0;

// Helper to find a PCB pointer using a Process ID
PCB* get_pcb_from_pid(int pid) {
    for (int i = 0; i < processCount; i++) {
        if (table[i].processID == pid) {
            return &table[i];
        }
    }
    return NULL;
}

void init_shadow_tables() {
    apply_arrival_config();
    for(int i = 0; i < processCount; i++) {
        int pid = table[i].processID;
        if (table[i].memLow != -1) {
            burst_times[pid] = (table[i].memHigh - table[i].memLow) - 7;
        } else {
            burst_times[pid] = 10; 
        }
    }
}

// =========================================================
// DYNAMIC PROCESS LOADING 
// =========================================================
void check_arrivals() {
    for (int pid = 2; pid <= (int)ARRIVAL_CONFIG_COUNT; pid++) {
        int configIndex = pid - 1;
        if (system_clock == arrival_times_config[configIndex] && processCount == pid - 1) {
            printf("\n[CLOCK %d] Process %d Arrival Time Reached. Loading...\n", system_clock, pid);
            table[pid - 1].processID = pid;
            processCount++;
            load_program((char*)arrival_programs[configIndex], pid);
        }
    }
}

// =========================================================
// STATE SYNC HELPER
// =========================================================
void update_state(PCB *p, ProcessState new_state) {
    // 1. Update the struct and print the transition
    set_process_state(p, new_state); 
    
    // 2. Sync the state to the simulated RAM if it is currently loaded
    if (p->memLow != -1) {
        char stateKey[40];
        sprintf(stateKey, "PCB_%d_state", p->processID);
        const char* stateNames[] = {"READY", "RUNNING", "BLOCKED", "FINISHED"};
        writeToMemory(p->processID, stateKey, (char*)stateNames[new_state]);
    }
}

// =========================================================
// QUEUE & MEMORY SWAP HELPERS
// =========================================================
void print_system_queues() {
    printf("\n--- Current System Queues ---\n");
    printf("Ready Queue:         ");
    printQueue(&readyQueue);
    
    // UPDATED: Renamed labels to match required resource names
    printf("userInput Blocked:   "); 
    printQueue(&inputBlocked);
    
    printf("userOutput Blocked:  ");
    printQueue(&outputBlocked);
    
    printf("file Blocked:        ");
    printQueue(&diskBlocked);
    printf("-----------------------------\n");
}

void ensure_in_memory(PCB* p) {
    if (p->memLow == -1) {
        printf("\n[PAGE FAULT] Process %d is not in RAM. Swapping in...\n", p->processID);
        int requiredSpace = get_process_size(p->processID);
        
        while ((MAX_SIZE - currentCount) < requiredSpace) { 
            int victim = chooseVictim(p->processID);
            if (victim != -1) {
                swapOut(victim);
                compactMemory();
            } else {
                printf("CRITICAL: Memory is full but no valid victim found to swap out!\n");
                break; 
            }
        }
        swapIn(p->processID);
    }
}

// =========================================================
// 1. ROUND ROBIN
// =========================================================
// =========================================================
// 1. ROUND ROBIN (Updated with Clock Sync)
// =========================================================
void schedule_rr(Queue *rq) {
    printf("\n[SCHEDULER] Starting Round Robin (Quantum: %d)...\n", TIME_SLICE);

    int all_done = 0;
    while (!all_done) {
        all_done = 1;
        for (int i = 0; i < processCount; i++) {
            if (table[i].state != FINISHED) {
                all_done = 0;
                break;
            }
        }
        if (all_done) break;

        if (isEmpty(rq)) continue;

        int currentPID = dequeue(rq);
        PCB *p = get_pcb_from_pid(currentPID);
        if (p == NULL) continue;

        printf("\n[EVENT] Process %d chosen for execution.\n", currentPID);
        print_system_queues();

        ensure_in_memory(p);
        update_state(p, RUNNING);
        printf("\n>>> RR: Executing Process %d (PC: %d)\n", p->processID, p->pc);

        for (int i = 0; i < TIME_SLICE; i++) {
            check_arrivals();

            // UPDATED: Pass system_clock so the interpreter can print memory correctly
            int status = execute_instruction(p, system_clock);
            system_clock++;

            if (status == 0) {
                update_state(p, BLOCKED);
                printf("\n[EVENT] Process %d BLOCKED during execution.\n", p->processID);
                print_system_queues();
                break;
            }

            int max_instructions = (p->memHigh - p->memLow) - 7;
            if (status == -1 || p->pc >= max_instructions) {
                update_state(p, FINISHED);
                freeMemory(p->processID);
                printf("\n[EVENT] Process %d FINISHED execution.\n", p->processID);
                print_system_queues();
                break;
            }
        }

        // NOTE: You can remove this specific printMemory call if your 
        // execute_instruction function is already printing it every cycle!
        // If you keep it, update it like this:

        if (p->state == RUNNING) {
            update_state(p, READY);
            enqueue(rq, p->processID);
        }
    }
}

// =========================================================
// 2. HRRN (Highest Response Ratio Next)
// =========================================================
int extract_best_hrrn(Queue *rq) {
    int best_pid = -1;
    double max_ratio = -1.0;
    
    Queue tempQueue;
    initQueue(&tempQueue);

    while (!isEmpty(rq)) {
        int pid = dequeue(rq);
        int wait_time = system_clock - arrival_times[pid];
        int burst = (burst_times[pid] > 0) ? burst_times[pid] : 1;
        
        double ratio = (double)(wait_time + burst) / (double)burst;
        
        if (ratio > max_ratio) {
            max_ratio = ratio;
            best_pid = pid;
        }
        enqueue(&tempQueue, pid); 
    }

    while (!isEmpty(&tempQueue)) {
        int pid = dequeue(&tempQueue);
        if (pid != best_pid) {
            enqueue(rq, pid);
        }
    }
    return best_pid;
}

void schedule_hrrn(Queue *rq) {
    printf("\n[SCHEDULER] Starting HRRN...\n");
    init_shadow_tables();
    while (!isEmpty(rq)) {
        int pid = extract_best_hrrn(rq);
        if (pid == -1) break;
        
        PCB *p = get_pcb_from_pid(pid);
        
        printf("\n[EVENT] Process %d chosen for execution.\n", pid);
        print_system_queues();
        
        ensure_in_memory(p);

        update_state(p, RUNNING);
        printf("\n>>> HRRN: Executing Process %d (PC: %d)\n", p->processID, p->pc);

        while (p->state == RUNNING) {
            check_arrivals();

            // FIXED: Added system_clock as the second argument
            int status = execute_instruction(p, system_clock); 
            system_clock++;
           
            
            if (status == 0) {
                update_state(p, BLOCKED);
                printf("\n[EVENT] Process %d BLOCKED during execution.\n", p->processID);
                print_system_queues();
                break; 
            }
            
            // CLEAN TERMINATION HOOK
            int max_instructions = (p->memHigh - p->memLow) - 7;
            if (status == -1 || p->pc >= max_instructions) {
                update_state(p, FINISHED);
                freeMemory(p->processID); 
                
                printf("\n[EVENT] Process %d FINISHED execution.\n", p->processID);
                print_system_queues();
                break;
            }
        }

        if (p->state == RUNNING) { 
            update_state(p, READY);
            enqueue(rq, pid);
        }
    }
}
// =========================================================
// 3. MLFQ (Multi-Level Feedback Queue)
// =========================================================
void schedule_mlfq(Queue *rq) {
    printf("\n[SCHEDULER] Starting MLFQ...\n");
    
    Queue levels[4];
    for (int i = 0; i < 4; i++) {
        initQueue(&levels[i]);
    }

    // Track which level each process belongs to
    int process_level[100] = {0};

    int active = 1;
    while (active || !isEmpty(rq)) { 
        
        // Drain newly arrived or unblocked processes into their correct level
        while (!isEmpty(rq)) {
            int pid = dequeue(rq);
            enqueue(&levels[process_level[pid]], pid);
        }

        active = 0; 
        for (int i = 0; i < 4; i++) {
            if (!isEmpty(&levels[i])) {
                active = 1;
                int pid = dequeue(&levels[i]);
                PCB *p = get_pcb_from_pid(pid);
                
                printf("\n[EVENT] Process %d chosen for execution from Level %d.\n", pid, i);
                print_system_queues();
                
                ensure_in_memory(p);

                update_state(p, RUNNING);
                int quantum = 1 << i; 
                printf("\n>>> MLFQ (Level %d | Quantum %d): Executing Process %d\n", i, quantum, pid);
                
                int count = 0;
                int blocked_or_finished = 0;

                while (count < quantum) {
                    check_arrivals();

                    // FIXED: Added system_clock as the second argument
                    int status = execute_instruction(p, system_clock);
                    system_clock++;
                    count++;
                    
                    if (status == 0) { 
                        update_state(p, BLOCKED);
                        printf("\n[EVENT] Process %d BLOCKED during execution.\n", p->processID);
                        print_system_queues();
                        blocked_or_finished = 1;
                        break;
                    }
                    
                    // CLEAN TERMINATION HOOK
                    int max_instructions = (p->memHigh - p->memLow) - 7;
                    if (status == -1 || p->pc >= max_instructions) { 
                        update_state(p, FINISHED);
                        freeMemory(p->processID); 
                        
                        printf("\n[EVENT] Process %d FINISHED execution.\n", p->processID);
                        print_system_queues();
                        blocked_or_finished = 1;
                        break;
                    }
                }
              

                if (!blocked_or_finished) {
                    // Process used its full quantum — demote it to the next level
                    int nextLevel = (i < 3) ? i + 1 : 3;
                    process_level[pid] = nextLevel;
                    update_state(p, READY);
                    enqueue(&levels[nextLevel], pid);
                } else if (p->state == BLOCKED) {
                    // Process blocked — remember its current level for when it wakes up
                    process_level[pid] = i;
                }
                
                break; 
            }
        }
    }
}