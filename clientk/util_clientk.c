//
// Created by hasatsrnkn on 13.11.2023.
//

#include "util_clientk.h"
#include "global_clientk.h"
#include <stdio.h>
#include <time.h>
#include <stdarg.h>
#include <mqueue.h>
#include <math.h>
#include <stdlib.h>

int cli( int argc, char *argv[]) {

    if (argc < 2 || argc > 11) {
        printf("Usage: %s -n clicount -f fname -s vsize -m mqname -d dlevel\n", argv[0]);
        return 1;
    }

    for (int i = 1; i < argc; i += 2) {
        if (argv[i][0] == '-' && i + 1 < argc) {
            switch (argv[i][1]) {
                case 'n':
                    clicount = atoi(argv[i + 1]);
                    break;
                case 'f':
                    fname =argv[i + 1];
                    break;
                case 's':
                    vsize = atoi(argv[i + 1]);
                    break;
                case 'm':
                    mqname = argv[i + 1];
                    break;
                case 'd':
                    dlevel = atoi(argv[i + 1]);
                    break;
                default:
                    printf("Invalid option: %s\n", argv[i]);
                    exit_handler(1);
            }
        } else {
            printf("Invalid arguments\n");
            exit_handler(1);
        }
    }

    if ( clicount < 0 || clicount > 10) {
        printf("N must be between 0 and 10\n");
        exit_handler(1);
    }

    //verification
    isFinished = 0;
    return 0;
}

void logger(const char *format, ...) {
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

void exit_handler(int sig){

    if (sig == 0){
        printf("Program exited successfully\n");
        exit(0);
    } else {
        printf("Program exited with signal %d\n", sig);
        exit(sig);
    }
}
