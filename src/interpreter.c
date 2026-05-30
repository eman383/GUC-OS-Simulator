#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "interpreter.h"
#include "syscalls.h"
#include <ctype.h> // Add this at the top with your other includes!
#include "mutex.h"


// Tell the compiler about the memory function we need to fetch instructions
extern char* readFromMemory(int pid, char* key);

// Helper 1: Checks if a string is purely numbers
int isNumber(char* str) {
    for (int i = 0; str[i] != '\0'; i++) {
        // Allow negative signs at the beginning
        if (!isdigit(str[i]) && str[i] != '-') {
            return 0; // It's a variable (e.g., "x")
        }
    }
    return 1; // It's a number (e.g., "10")
}
// NEW: Helper to pause the system and wait for the GUI button
void step_pause() {
    printf("--- [STEP] Waiting for GUI --- ");
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}
// Helper 2: The Resolver
char* resolve_value(char* word, PCB* p) {
    if (isNumber(word)) {
        // It's already a number, just return it
        return word; 
    } else {
        // It's a variable! Let's fetch it from your memory system
        char memKey[40];
        sprintf(memKey, "var_%d_%s", p->processID, word);
        char* value = sys_read_mem(memKey, p);
        
        if (value == NULL || strlen(value) == 0) {
            // Safety check in case they read an empty variable
            return "0"; 
        }
        return value;
    }
}


// NEW: Make sure interpreter.c knows about the updated print function
extern void printMemory(int current_clock);

// NEW: Added 'int current_clock' to the parameters
int execute_instruction(PCB* p, int current_clock) {
    // ---------------------------------------------------------
    // 1. FETCH: Get the instruction string from Memory
    // ---------------------------------------------------------
    char instrKey[40];
    sprintf(instrKey, "instr_%d_%d", p->processID, p->pc); 
    char* instruction = readFromMemory(p->processID, instrKey);
    
    if (instruction == NULL || strlen(instruction) == 0) {
        printf("Error: Could not fetch instruction at PC %d or process finished.\n", p->pc);
        return -1; // -1 indicates the process is completely done
    }

    printf("\n>>> Interpreter executing: '%s' for Process %d\n", instruction, p->processID);

    // ---------------------------------------------------------
    // 2. PARSE: Split the instruction by spaces
    // ---------------------------------------------------------
    char instr_copy[60];
    strcpy(instr_copy, instruction);

    char* words[5]; // Up to 5 tokens for long commands like "assign x readFile b"
    for(int j = 0; j < 5; j++) words[j] = NULL; 

    int i = 0;
    words[i] = strtok(instr_copy, " ");
    while (words[i] != NULL && i < 4) {
        i++;
        words[i] = strtok(NULL, " ");
    }

  
  // ---------------------------------------------------------
    // 3. EXECUTE: Call system calls or semaphores
    // ---------------------------------------------------------
    char* command = words[0];

    // --- CASE 1: MUTUAL EXCLUSION (Locks) ---
    if (strcmp(command, "semWait") == 0) {
        Semaphore* target;
        if (strcmp(words[1], "userInput") == 0) target = &userInput;
        else if (strcmp(words[1], "userOutput") == 0) target = &userOutput;
        else target = &fileSystem; // default to fileSystem if "file"

        if (semWait(target, p) == 0) {
            printf("    -> Action: Process %d BLOCKED waiting for %s\n", p->processID, words[1]);
            
            // NEW: Print memory even if blocked, because a clock cycle still passed
            printMemory(current_clock); 
            step_pause(); // NEW: Pause after a block
            return 0; // STOP! Return to Scheduler. Do not increment PC!
        }
        printf("    -> Action: Process %d successfully locked %s\n", p->processID, words[1]);
    } 
    else if (strcmp(command, "semSignal") == 0) {
        Semaphore* target;
        if (strcmp(words[1], "userInput") == 0) target = &userInput;
        else if (strcmp(words[1], "userOutput") == 0) target = &userOutput;
        else target = &fileSystem;

        semSignal(target, p);
        printf("    -> Action: Process %d released lock for %s\n", p->processID, words[1]);
    }
    
    // --- CASE 2: ASSIGNMENT AND I/O ---
    else if (strcmp(command, "assign") == 0) {
        char* variableName = words[1];  
        char* action = words[2];        
        
        char memKey[40];
        sprintf(memKey, "var_%d_%s", p->processID, variableName);

        if (strcmp(action, "input") == 0) {
            char buffer[256];
            sys_take_input(buffer); 
            int flush; while ((flush = getchar()) != '\n' && flush != EOF);
            sys_write_mem(memKey, buffer, p);
            printf("    -> Action: Took input '%s' and assigned to '%s'\n", buffer, memKey);
        } 
        else if (strcmp(action, "readFile") == 0) {
            char* fileVarOrName = words[3]; 
            char* actualFileName = resolve_value(fileVarOrName, p); 
            
            char* fileContent = sys_read_file(actualFileName);
            if (fileContent != NULL) {
                sys_write_mem(memKey, fileContent, p);
                printf("    -> Action: Read file '%s' and assigned content to '%s'\n", actualFileName, memKey);
                free(fileContent); // Prevent memory leak
            } else {
                sys_write_mem(memKey, "", p); 
            }
        } 
        else {
            // Normal number or variable assignment
            char* finalValue = resolve_value(action, p);
            sys_write_mem(memKey, finalValue, p);
            printf("    -> Action: Assigned %s to '%s'\n", finalValue, memKey);
        }
    } 
    
    // --- CASE 3: PRINT ---
    else if (strcmp(command, "print") == 0) {
        char* rawValue = words[1]; 
        char* finalValue = resolve_value(rawValue, p);
        sys_print(finalValue);
    }

    // --- CASE 4: PRINT FROM TO (For Program 1) ---
    else if (strcmp(command, "printFromTo") == 0) {
        char* startVar = words[1]; 
        char* endVar = words[2]; 
        
        // Resolve both values using your helper
        char* startValStr = resolve_value(startVar, p);
        char* endValStr = resolve_value(endVar, p);
        
        // Convert strings to integers so we can loop
        int start = atoi(startValStr);
        int end = atoi(endValStr);
        
        // Print the sequence
        printf("    -> Action: Printing from %d to %d: ", start, end);
        for (int k = start; k <= end; k++) {
            printf("%d ", k);
        }
        printf("\n");
    }
    
    // --- CASE 5: WRITE FILE ---
    else if (strcmp(command, "writeFile") == 0) {
        char* fileVarName = words[1]; 
        char* dataVarName = words[2]; 
        
        char* actualFileName = resolve_value(fileVarName, p);
        char* actualData = resolve_value(dataVarName, p);
        
        sys_write_file(actualFileName, actualData);
        printf("    -> Action: Wrote data '%s' to file '%s'\n", actualData, actualFileName);
    }
    
    // --- CATCH-ALL ---
    else {
        printf("    -> Action: Unknown command '%s'\n", command);
    }

    // ---------------------------------------------------------
    // 4. INCREMENT & SYNC PC (Only reached if NOT blocked)
    // ---------------------------------------------------------
    increment_pc(p);
    
    char pcStr[10];
    sprintf(pcStr, "%d", p->pc);
    char pcKey[40];
    sprintf(pcKey, "PCB_%d_PC", p->processID);
    sys_write_mem(pcKey, pcStr, p);
    
    // NEW: Print memory upon successful execution of the instruction
    printMemory(current_clock);
    step_pause(); // NEW: Pause after a successful instruction
    return 1; // Return 1 to tell Scheduler the execution was successful
}