#include <stdio.h>
#include <stdlib.h>
#include "pcb.h"
#include "mutex.h"
#include "loader.h"
#include "scheduler.h"

extern PCB table[40];
extern int processCount;

// UPDATED: Added the int parameter to match memory.c
extern void printMemory(int current_clock); 

int main() {
    setvbuf(stdout, NULL, _IONBF, 0);
    printf("========== BOOTING GUC-OS ==========\n");

    initAllQueues();   
    initSemaphores();  

    // ONLY LOAD PROCESS 1 AT BOOT (Arrival Time = 0)
    table[0].processID = 1;
    processCount++;
    load_program("../programs/Program_1.txt", 1); 

    printf("\n[BOOT] Initialization complete. Initial Memory State:\n");
    
    // UPDATED: Pass 0 as the system clock at boot
    printMemory(0); 
    
    // FIXED 8: Interactive Menu for Algorithm Selection
    int choice = 0;
    printf("\n========== SCHEDULER SELECTION ==========\n");
    printf("1. Round Robin (RR)\n");
    printf("2. Highest Response Ratio Next (HRRN)\n");
    printf("3. Multi-Level Feedback Queue (MLFQ)\n");
    printf("Enter your choice (1-3): ");
    
   // Read the evaluator's choice
   if (scanf("%d", &choice) != 1) {
    printf("Invalid input. Defaulting to Round Robin.\n");
    choice = 1;
}

// NEW: Clear the leftover newline from the buffer!
int flush;
while ((flush = getchar()) != '\n' && flush != EOF);

printf("\n========== STARTING EXECUTION ==========\n");

    // Hand control to the selected Scheduler
    switch (choice) {
        case 1:
            schedule_rr(&readyQueue);
            break;
        case 2:
            schedule_hrrn(&readyQueue);
            break;
        case 3:
            schedule_mlfq(&readyQueue);
            break;
        default:
            printf("Invalid choice. Defaulting to Round Robin.\n");
            schedule_rr(&readyQueue);
            break;
    }

    printf("\n========== ALL PROCESSES FINISHED. SYSTEM SHUTDOWN ==========\n");
    
    // UPDATED: Pass a placeholder (-1) since the final clock is likely inside the scheduler
    // Alternatively, if you declare `extern int system_clock;` at the top, you can pass that here.
    printMemory(-1); 

    return 0;
}