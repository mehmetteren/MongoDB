//
// Created by Mehmet Eren Balasar on 14.11.2023.
//

#ifndef SRC_GLOBALS_H
#define SRC_GLOBALS_H

#define MAX_DELETED_OFFSET_COUNT 20

#define MAX_VSIZE 1024
#define MIN_VSIZE 32

#define MAX_TCOUNT 5
#define MIN_TCOUNT 1

#define MAX_DCOUNT 5
#define MIN_DCOUNT 1

#define DEEP_DEBUG 2
#define DEBUG 1
#define INFO 0

#include <stdbool.h>
#include <time.h>
#include "hash_table.h"


struct timespec program_start;
extern int dcount;
extern char* fname;
extern int tcount;
extern int vsize;
extern char* mqname;
extern int dlevel;

// starts from 1, datafile1 -> 1, datafile2 -> 2, ...
extern HashTable** tables;

typedef struct {
    int client_ip;
    char value[MAX_VSIZE];
    int status_code;
    char info_message[100];
} Response;

typedef struct {
    int client_ip;
    char method[10];
    long int key;
    char value[MAX_VSIZE];
} Request;

typedef struct {
    long int key;
    char* value;
    bool is_deleted;
} Entry;

Entry* createEntry(long int key, const char *value, int valueSize);
void freeEntry(Entry* entry);
void logg(int level, const char *format, ...);

#endif //SRC_GLOBALS_H
