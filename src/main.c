
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <time.h>
#include <stdbool.h>
#include <fcntl.h>

#include "globals.h"
#include "disk.h"
#include "hash_table.h"
#include "server.h"

int main() {
    //cli
    unsigned long entry_size = sizeof(long int) + sizeof(bool) + vsize;
    clock_gettime(CLOCK_MONOTONIC, &program_start);
    start_server();

    Entry temp_entry;
    Entry* entry = (createEntry(12937, "test", vsize));
    unsigned long temp_offset = 0;
    int fd = open("datafile2", O_RDWR);

    while(read_entry_from_file(&temp_entry, fd, temp_offset) > 0){
        if (temp_entry.is_deleted){
            // found an available offset
            write_entry_to_file(entry, fd, temp_offset);
            insert(tables[2], entry->key, temp_offset);
            return 0;
        }
        temp_offset += entry_size;
    }
}

int mock_data(){
    unsigned long entry_size = sizeof(long int) + sizeof(bool) + vsize;

    FILE* ifps[dcount];
    for (int x = 0; x < dcount; x++){
        // Open a file in binary write mode
        char filename[100];
        strcpy(filename, fname);
        sprintf(filename + strlen(filename), "%d", x+1);

        ifps[x] = fopen(filename, "wb");
        if (ifps[x] == NULL) {
            perror("Error opening file");
            return EXIT_FAILURE;
        }
    }

    for (int i = 0; i < 100; i++){

        Entry* kv = createEntry(i, "ExampleValuelflaksdlasndaklsdanld", vsize);

        // Write the key-value pair to the file
        FILE* file_pt = ifps[(kv->key % dcount)];

        writeEntry(file_pt, kv, i * entry_size);

        freeEntry(kv);

    }

    for(int y = 0; y < dcount; y++){
        fclose(ifps[y]);
    }

    FILE* reader = fopen("datafile2", "rb");

    for (int k = 0; k < 10; k++){
        Entry kvx;
        readEntry(reader, &kvx, 0);

        printf("Key: %ld, Value: %s\n", kvx.key, kvx.value);
    }



    printf("Successfully written to file\n");
    return EXIT_SUCCESS;
}
