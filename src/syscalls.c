#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "pcb.h"
// ---------------------------------------------------------
// PROMISES TO THE COMPILER (Linking to his memory.c code)
// ---------------------------------------------------------
extern void writeToMemory(int pid, char* key, char* value);
extern char* readFromMemory(int pid, char* key); 


// ---------------------------------------------------------
// 1. MEMORY SYSTEM CALLS (Delegating to his logic)
// ---------------------------------------------------------
void sys_write_mem(char* key, char* value, PCB* p) {
    // We don't need to check boundaries here anymore!
    // His writeToMemory function handles it using p->memLow and p->memHigh.
    writeToMemory(p->processID, key, value);
}

char* sys_read_mem(char* key, PCB* p) {
    // Fetch the value using his function
    char* result = readFromMemory(p->processID, key);
    
     
    if (result == NULL) {
        printf("CRITICAL: Process %d could not find key '%s'\n", p->processID, key);
        return NULL; // Return NULL instead of a string literal to be safe
    }
    return result;
}


// ---------------------------------------------------------
// 2. DISK / FILE SYSTEM CALLS
// ---------------------------------------------------------
char* sys_read_file(char* filename) {
    // FIXED: Removed the "disk/" folder assumption. It reads from the current directory.
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        printf("Error: File '%s' not found\n", filename);
        return NULL; // FIXED: Return NULL so the interpreter knows it failed and doesn't try to free() a string literal
    }

    // Allocate memory for the file content
    char* content = malloc(256);
    if (content == NULL) {
        fclose(file);
        return NULL; // Safety check in case malloc fails
    }

    if (fgets(content, 256, file) != NULL) {
        content[strcspn(content, "\n")] = 0; // Strip the trailing newline character
    } else {
        strcpy(content, ""); // Handle empty files safely
    }
    
    fclose(file);
    return content;
}

void sys_write_file(char* filename, char* data) {
    // FIXED: Removed the "disk/" folder assumption. 
    FILE* file = fopen(filename, "w"); 
    if (file != NULL) {
        fprintf(file, "%s", data);
        fclose(file);
    } else {
        printf("Error: Could not create or open file '%s'\n", filename);
    }
}


// ---------------------------------------------------------
// 3. I/O SYSTEM CALLS
// ---------------------------------------------------------
void sys_print(char* data) {
    printf("OS_OUTPUT: %s\n", data);
}

void sys_take_input(char* buffer) {
    printf("Please enter a value: "); 
    fgets(buffer, 256, stdin);
    buffer[strcspn(buffer, "\n")] = 0; // Clean the input by removing the newline
}