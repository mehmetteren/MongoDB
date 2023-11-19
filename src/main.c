
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <time.h>
#include <stdbool.h>
#include <fcntl.h>
#include <unistd.h>

#include "globals.h"
#include "disk.h"
#include "hash_table.h"
#include "server.h"
int insert_test();
//gcc -Wall -o server disk.c globals.c hash_table.c server.c mq.c main.c -lrt
int main(int argc, char* argv[]){
    //insert_test();
    cli(argc, argv);
    clock_gettime(CLOCK_MONOTONIC, &program_start);

    start_server();
    //printf("%ld", sizeof(Request));
}

int insert_test() {
    //cli
    clock_gettime(CLOCK_MONOTONIC, &program_start);
    start_server();

    for (int a = 12937; a < 12957; a+=2){
        char* val = "WHATTHEFUCKISTHISVALUE";
        char value[vsize];

        sprintf(value, "%s--%d", val, a);
        Entry* entry = (createEntry(a, value, vsize, 0));
        int res = insert_entry(entry);
        printf("result = %d\n", res);

        printf("========\n");
    }
    printf("***========***\n");

    int res4 = delete_entry(12937);
    printf("delete_result: %d\n", res4);

    int res5 = delete_entry(12945);
    printf("delete_result: %d\n", res5);

    int res6 = delete_entry(12947);
    printf("delete_result: %d\n", res6);

    printf("***========***\n");

    for (int a = 12937; a < 12957; a+=2){
        char* val = "NEWVALUE";
        char value[vsize];

        sprintf(value, "%s--%d", val, a);
        Entry* entry = (createEntry(a, value, vsize, 0));
        int res = insert_entry(entry);
        printf("result = %d\n", res);

        printf("========\n");
    }


    return 0;
}

