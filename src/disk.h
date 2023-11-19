//
// Created by Mehmet Eren Balasar on 16.11.2023.
//

#ifndef SRC_DISK_H
#define SRC_DISK_H
#include "globals.h"
#include <stdio.h>

typedef struct {
    int count;
    long int offsets[MAX_DELETED_OFFSET_COUNT];
} Metadata;

int insert_entry(Entry* entry);
int delete_entry(long int key);
int get_entry(long int key, Entry* entry);

void initialize_metadata(Metadata *metadata);
void update_metadata_on_delete(Metadata *metadata, long int deletedOffset);
long int find_available_offset(Metadata *metadata);
Metadata read_metadata(int fd);
int write_metadata(int fd, Metadata *metadata);

int generate_datafiles(int count, char* name);
int write_entry_to_file(Entry* entry, int fd, long int file_offset);
int read_entry_from_file(Entry* entry, int fd, long int file_offset);
int set_is_deleted(int fd, long int file_offset, bool is_deleted);

int handle_dump_request(char* dump_file_name);

#endif //SRC_DISK_H
