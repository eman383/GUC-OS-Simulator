#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "loader.h"

// Tell the compiler about our allocation function in memory.c
extern int allocate(int pid, char** instructions, int instructionCount);

int load_program(char* filename, int pid) {
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        printf("ERROR: Could not find program file '%s'\n", filename);
        return 0;
    }

    char line[256];
    int count = 0;

    // --- PASS 1: Count how many instructions are in the file ---
    while (fgets(line, sizeof(line), file)) {
        // Skip empty lines
        if (strlen(line) > 1) count++; 
    }

    // --- PASS 2: Load the instructions into a temporary array ---
    rewind(file); // Go back to the start of the file
    char** instructions = malloc(count * sizeof(char*));
    
    int i = 0;
    while (fgets(line, sizeof(line), file)) {
        if (strlen(line) > 1) {
            // Remove the newline character (\n or \r)
            line[strcspn(line, "\r\n")] = 0;
            
            // Allocate space for this specific instruction string
            instructions[i] = malloc(strlen(line) + 1);
            strcpy(instructions[i], line);
            i++;
        }
    }
    fclose(file);

    printf("[LOADER] Loaded %d instructions from %s\n", count, filename);

    // --- PASS 3: Send it to the Memory System ---
    allocate(pid, instructions, count);

    // --- CLEANUP: Free the temporary array (Memory system has its own copy now) ---
    for (int j = 0; j < count; j++) {
        free(instructions[j]);
    }
    free(instructions);

    return 1;
}