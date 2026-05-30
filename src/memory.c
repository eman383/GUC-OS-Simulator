#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "pcb.h"
#include "queue.h"

#define MAX_SIZE 40

typedef struct {
    char key[40];
    char val[60];
} MemoryWord;

MemoryWord memory[MAX_SIZE];
int currentCount = 0;   

// Safely locates a PCB by its actual ID instead of assuming array index
static PCB* get_pcb(int pid) {
    for (int i = 0; i < processCount; i++) {
        if (table[i].processID == pid) {
            return &table[i];
        }
    }
    return NULL;
}

static void clearWord(int index) {
    strcpy(memory[index].key, "");
    strcpy(memory[index].val, "");
}

static void setWord(int index, const char* key, const char* val) {
    strncpy(memory[index].key, key, sizeof(memory[index].key) - 1);
    strncpy(memory[index].val, val, sizeof(memory[index].val) - 1);
    memory[index].key[sizeof(memory[index].key) - 1] = '\0';
    memory[index].val[sizeof(memory[index].val) - 1] = '\0';
}

char* readFromMemory(int pid, char* key) {
    PCB* p = get_pcb(pid);
    if (p == NULL || p->memLow == -1) {
        printf("Error: Process %d is either not found or currently swapped out.\n", pid);
        return NULL;
    }

    for (int i = p->memLow; i <= p->memHigh; i++) {
        if (strcmp(memory[i].key, key) == 0)
            return memory[i].val;
    }

    printf("Error the Process %d: key %s is not found\n", pid, key);
    return NULL;
}
void writeToMemory(int pid, char* key, char* value) {
    PCB* p = get_pcb(pid);
    if (p == NULL || p->memLow == -1) {
        printf("Error: Process %d is not found or swapped out.\n", pid);
        return;
    }

    // 1. FIRST PASS: Try to update an existing key
    for (int i = p->memLow; i <= p->memHigh; i++) {
        if (strcmp(memory[i].key, key) == 0) {
            strncpy(memory[i].val, value, sizeof(memory[i].val) - 1);
            return;
        }
    }

    // 2. SECOND PASS: Dynamic Variable Binding
    if (strncmp(key, "var_", 4) == 0) {
        for (int i = p->memLow; i <= p->memHigh; i++) {
            if (strncmp(memory[i].key, "var_", 4) == 0) {

                // Find the position of the last underscore
                char* lastUnderscore = strrchr(memory[i].key, '_');

                // A slot is only FREE if the segment after the last '_' is a single digit
                // e.g., "var_1_0", "var_1_1", "var_1_2" are free/unbound slots
                // e.g., "var_1_x", "var_1_y" are already claimed — do NOT reuse them
                if (lastUnderscore != NULL &&
                    isdigit((unsigned char)lastUnderscore[1]) &&
                    lastUnderscore[2] == '\0') {

                    // Rename the generic key to the actual variable name
                    strncpy(memory[i].key, key, sizeof(memory[i].key) - 1);
                    memory[i].key[sizeof(memory[i].key) - 1] = '\0';

                    // Save the value
                    strncpy(memory[i].val, value, sizeof(memory[i].val) - 1);
                    memory[i].val[sizeof(memory[i].val) - 1] = '\0';
                    return;
                }
            }
        }
        printf("Error: Process %d has used all 3 variable slots!\n", pid);
        return;
    }

    printf("Error the Process %d tried to write key %s but it is out of bounds\n", pid, key);
}

void swapOut(int pid) {
    PCB* p = get_pcb(pid);
    if (p == NULL || p->memLow == -1) return;

    char filename[50];
    sprintf(filename, "process_%d.txt", pid);

    FILE* fptr = fopen(filename, "w");
    if (fptr == NULL) {
        printf("Error creating swap file for process %d\n", pid);
        return;                                 
    }

    // FIXED 6: Print the disk format header for the evaluator
    printf("\n--- Disk Memory Format (File: %s) ---\n", filename);

    for (int i = p->memLow; i <= p->memHigh; i++) { 
        // Write the data to the actual text file
        fprintf(fptr, "%s=%s\n", memory[i].key, memory[i].val); 
        
        // Output the exact format to the console so the grader sees it
        printf("%s=%s\n", memory[i].key, memory[i].val);
        
        // Safely clear the word from main memory
        clearWord(i);                           
    }
    fclose(fptr);

    printf("-----------------------------------------\n");
    printf("[SWAP OUT] Process %d written to \"%s\" (words %d–%d freed)\n", pid, filename, p->memLow, p->memHigh);

    // Use bounds = -1 to indicate the process is out of main memory
    p->memLow = -1;
    p->memHigh = -1;
}

void compactMemory() {
    int writeIdx = 0;

    // Shift everything up to remove empty spaces
    for (int i = 0; i < MAX_SIZE; i++) {
        if (strlen(memory[i].key) > 0) {         
            if (i != writeIdx){
                memory[writeIdx] = memory[i];
            }
            writeIdx++;
        }
    }

    // Clear the remaining blocks at the end
    for (int i = writeIdx; i < MAX_SIZE; i++){
        clearWord(i);
    }
        
    currentCount = writeIdx;

    // Update PCB boundaries for processes still in memory
    for (int i = 0; i < processCount; i++) {
        PCB* p = &table[i];

        if (p->memLow == -1 || p->state == FINISHED) continue;

        char idKey[40];
        sprintf(idKey, "PCB_%d_id", p->processID);

        for (int j = 0; j < currentCount; j++) {
            if (strcmp(memory[j].key, idKey) == 0) {
                int oldSize = p->memHigh - p->memLow;
                p->memLow  = j;
                p->memHigh = j + oldSize;
                
                // Sync the updated bounds directly to the simulated memory array
                char buf[20], temp[40];
                sprintf(temp, "PCB_%d_memLow", p->processID);
                sprintf(buf, "%d", p->memLow);
                writeToMemory(p->processID, temp, buf);

                sprintf(temp, "PCB_%d_memHigh", p->processID);
                sprintf(buf, "%d", p->memHigh);
                writeToMemory(p->processID, temp, buf);
                break;
            }
        }
    }

    printf("Memory compacted. %d words in use.\n", currentCount);
}

void swapIn(int pid) {
    PCB* p = get_pcb(pid);
    if (p == NULL) return;

    char filename[50];
    sprintf(filename, "process_%d.txt", pid);

    FILE* fptr = fopen(filename, "r");
    if (fptr == NULL) {
        printf("Error: swap file %s not found\n", filename);
        return;
    }

    int base    = currentCount;
    int wordIdx = base;
    char line[130];

    while (fgets(line, sizeof(line), fptr) && wordIdx < MAX_SIZE) {
        line[strcspn(line, "\n")] = '\0';          

        char* eq = strchr(line, '=');             
        if (!eq) continue;

        *eq = '\0';                               
        setWord(wordIdx, line, eq + 1);
        wordIdx++;
    }
    fclose(fptr);

    // Update in process table
    p->memLow    = base;
    p->memHigh   = wordIdx - 1;
    currentCount = wordIdx;
    
    // Sync new bounds into the memory array
    char buf[20];
    char temp[40];
    sprintf(buf, "%d", p->memLow);
    sprintf(temp, "PCB_%d_memLow", pid);
    writeToMemory(pid, temp, buf);

    sprintf(buf, "%d", p->memHigh);
    sprintf(temp, "PCB_%d_memHigh", pid);
    writeToMemory(pid, temp, buf);

    printf("Process %d loaded from external file (words %d–%d)\n", pid, p->memLow, p->memHigh);
}

int chooseVictim(int runningPid) {
    for (int i = 0; i < processCount; i++) {
        PCB* p = &table[i];
        
        // FIXED: Added p->state != RUNNING. 
        // We must never swap out the process that is actively executing on the CPU!
        if (p->memLow != -1 && p->state != FINISHED && p->state != RUNNING && p->processID != runningPid) {
            return p->processID;
        }
    }
    return -1; 
}

int allocate(int pid, char** instructions, int instructionCount) {
    int needed = 5 + 3 + instructionCount; // 5 PCB keys + 3 variables + instructions

    // Loop to ensure enough space is freed if one victim isn't enough
    while (currentCount + needed > MAX_SIZE) {
        int victim = chooseVictim(pid);          
        if (victim == -1) {
            printf("No swap candidate — memory exhausted\n");
            return -1;
        }
        swapOut(victim);
        compactMemory();                            
    }

    int base = currentCount;
    char key[40], val[60];

    sprintf(key, "PCB_%d_id", pid);
    sprintf(val, "%d", pid);
    setWord(base, key, val);                      

    sprintf(key, "PCB_%d_state", pid);
    setWord(base + 1, key, "READY");

    sprintf(key, "PCB_%d_PC", pid);
    setWord(base + 2, key, "0");

    sprintf(key, "PCB_%d_memLow", pid);
    sprintf(val, "%d", base);
    setWord(base + 3, key, val);

    sprintf(key, "PCB_%d_memHigh", pid);
    sprintf(val, "%d", base + needed - 1);
    setWord(base + 4, key, val);

    for (int v = 0; v < 3; v++) {
        sprintf(key, "var_%d_%d", pid, v);
        setWord(base + 5 + v, key, "");
    }

    for (int i = 0; i < instructionCount; i++) {        
        sprintf(key, "instr_%d_%d", pid, i);
        setWord(base + 8 + i, key, instructions[i]);
    }

    currentCount += needed;

    PCB* p = get_pcb(pid);
    if (p != NULL) {
        // 1. Initialize data (The 4 fields)
        init_pcb(p, pid, base, base + needed - 1);
        
        // 2. Add to Scheduler (The Queue)
        enqueue(&readyQueue, pid); 
    }

    printf("Process %d was allocated words %d–%d\n", pid, base, base + needed - 1);
    return base; 
}

void freeMemory(int pid) {
    PCB* p = get_pcb(pid);
    if (p == NULL) return;

    for (int i = p->memLow; i <= p->memHigh; i++){
        clearWord(i);
    }
    set_process_state(p, FINISHED);
    
    p->memLow  = -1;
    p->memHigh = -1;

    compactMemory();

    printf("Process %d memory released and compacted\n", pid);
}

void printMemory(int current_clock) {
    // NEW: Added the system clock to the header print statement
    printf("\n--- Memory State After Clock Cycle (System Clock: %d) ---\n\n", current_clock);
    printf("╔══════════════════════════════════════════════════╗\n");
    printf("║               MEMORY STATE                       ║\n");
    printf("╠══════╦═══════════════════════════╦═══════════════╣\n");
    printf("║ Word ║ Key                       ║ Value         ║\n");
    printf("╠══════╬═══════════════════════════╬═══════════════╣\n");

    for (int i = 0; i < MAX_SIZE; i++) {
        if (strlen(memory[i].key) > 0) {
            printf("║ %-4d ║ %-25s ║ %-13s ║\n",
                   i, memory[i].key, memory[i].val);
        } else {
            printf("║ %-4d ║ [free]                    ║               ║\n", i);
        }
    }

    printf("╚══════╩═══════════════════════════╩═══════════════╝\n");
    printf("  Words in use: %d / %d\n\n", currentCount, MAX_SIZE);
}