//
// Created by Mehmet Eren Balasar on 14.11.2023.
//

#ifndef SRC_GLOBALS_H
#define SRC_GLOBALS_H

// delete_ht the files if you change this
#define MAX_DELETED_OFFSET_COUNT 2

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
#include <pthread.h>
#include "hash_table.h"

extern struct timespec program_start;
extern int dcount;
extern char* fname;
extern int tcount;
extern int vsize;
extern char* mqname;
extern char* request_mq_name;
extern char* response_mq_name;
extern int dlevel;

// starts from 1, datafile1 -> 1, datafile2 -> 2, ...
extern HashTable** tables;
extern pthread_mutex_t* file_locks;
extern pthread_t* workers;

// 4 + 4 + 100 + vsize = 108 + vsize
typedef struct {
    int client_ip;
    int status_code;
    char info_message[100];
    char* value;
} Response;

// 4 + 10 + 8 + vsize = 22 + vsize
typedef struct {
    int client_ip;
    char method[11];
    long int key;
    char* value;
} Request;

typedef struct {
    long int key;
    bool is_deleted;
    char* value;
} Entry;

Entry* createEntry(long int key, const char *value, int valueSize);
void freeEntry(Entry* entry);
Response* createResponse(int client_ip, char* value, int status_code, char* info_message, int valueSize);
void freeResponse(Response* res);
void logg(int level, const char *format, ...);

#endif //SRC_GLOBALS_H
