//
// Created by Mehmet Eren Balasar on 14.11.2023.
//
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#include "globals.h"

struct timespec program_start;
int dcount = 2;
char* fname = "datafile";
int tcount = 5;
int vsize = 64;

char* mqname = "mq";
char* request_mq_name;
char* response_mq_name;

int dlevel = DEEP_DEBUG;

HashTable** tables;
pthread_mutex_t* file_locks;
pthread_t* workers;


Entry* createEntry(long int key, const char *value, int valueSize) {
    Entry* entry = malloc(sizeof(Entry));
    entry->is_deleted = false;
    entry->key = key;
    entry->value = malloc(valueSize);

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

Response* createResponse(int client_ip, char *value, int status_code, char* info_message, int valueSize) {
    Response* res = malloc(sizeof(Response));
    res->client_ip = client_ip;
    res->status_code = status_code;
    res->value = malloc(vsize);

    if (res->value == NULL) {
        perror("Unable to allocate memory for new node");
        exit(EXIT_FAILURE);
    }
    strcpy(res->info_message, info_message);

    strncpy(res->value, value, valueSize);

    res->value[valueSize - 1] = '\0';
    return res;
}

void freeResponse(Response *res) {
    free(res->value);
    res->value = NULL;
    free(res);
}

void logg(int level, const char *format, ...) {
    if (level > dlevel){
        return;
    }
    struct timespec end;
    clock_gettime(CLOCK_REALTIME, &end);

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

