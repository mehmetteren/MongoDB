//
// Created by Mehmet Eren Balasar on 14.11.2023.
//
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#include "globals.h"

int dcount = 2;
char* fname = "datafile";
int tcount = 5;
int vsize = 64;
char* mqname = "mq";
int dlevel = DEEP_DEBUG;

HashTable** tables;

Entry* createEntry(long int key, const char *value, int valueSize) {
    Entry* entry = malloc(sizeof(Entry));
    entry->is_deleted = false;
    entry->key = key;
    entry->value = malloc(valueSize * sizeof(char));

    if (entry->value == NULL) {
        perror("Unable to allocate memory for new node");
        exit(EXIT_FAILURE);
    }

    strncpy(entry->value, value, valueSize);
    entry->value[valueSize - 1] = '\0';
    return entry;
}

void freeEntry(Entry *entry) {
    free(entry->value);
    entry->value = NULL;
    free(entry);
}

void logg(int level, const char *format, ...) {
    if (level > dlevel){
        return;
    }
    struct timespec end;
    clock_gettime(CLOCK_MONOTONIC, &end);

    long seconds = end.tv_sec - program_start.tv_sec;
    long nanoseconds = end.tv_nsec - program_start.tv_nsec;

    if (nanoseconds < 0) {
        nanoseconds += 1e9;
        seconds -= 1;  // Adjust seconds accordingly
    }

    double elapsed = seconds * 1000.0 + nanoseconds / 1e6;

    printf("%f --- ", elapsed);

    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
}