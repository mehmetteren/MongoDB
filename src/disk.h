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

void initialize_metadata(Metadata *metadata);
void update_metadata_on_delete(Metadata *metadata, long int deletedOffset);
long int find_available_offset(Metadata *metadata);
Metadata read_metadata(int fd);

int generate_datafiles(int count, char* name);
int write_entry_to_file(Entry* entry, int fd, unsigned long file_offset);
int read_entry_from_file(Entry* entry, int fd, unsigned long file_offset);

#endif //SRC_DISK_H
