//
// Created by Mehmet Eren Balasar on 14.11.2023.
//

#include <stdlib.h>
#include <unistd.h>
#include <sys/file.h>
#include "server.h"
#include "globals.h"
#include "hash_table.h"
#include "disk.h"

int start_server(){

    generate_datafiles(dcount, fname);
    generate_hash_tables(dcount);

}

int quit_server(){
    // TODO: free memory of hash tables and write them to disk
}

// returns 0 for insertion, 1 for modification, -1 for failure
int insert_entry(Entry* entry){
    // key value isdeleted
    long int key = entry->key;

    int section = (key % dcount + 1);

    long int offset = search(tables[section], key);

    char filename[100];
    sprintf(filename, "%s%d", fname, section);
    int fd = open(filename, O_RDWR);

    if (offset >= 0){
        // key exists, modify
        write_entry_to_file(entry, fd, offset);
        return 1;
    }
    else {
        // key does not exist, insert
        Metadata file_metadata = read_metadata(fd);
        offset = find_available_offset(&file_metadata);
        if (offset >= 0){
            // there is an available offset
            write_entry_to_file(entry, fd, offset);
            insert(tables[section], key, offset);

            return 0;
        }
        else {
            // there is no available offset so traverse the file
            Entry temp_entry;
            long int temp_offset = 0;
            while (read_entry_from_file(&temp_entry, fd, temp_offset) > 0){
                if (temp_entry.isDeleted){
                    // found an available offset
                    write_entry_to_file(entry, fd, temp_offset);
                    insert(tables[section], key, temp_offset);
                    return 0;
                }
                temp_offset += sizeof(Entry);
            }

            return 0;
        }
    }


}

int get(Request* request) {
    long int key = request->key;


}


