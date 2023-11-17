//
// Created by Mehmet Eren Balasar on 16.11.2023.
//

#include <stdio.h>
#include <stdlib.h>
#include <sys/file.h>
#include <unistd.h>
#include "disk.h"

void initialize_metadata(Metadata *metadata) {
    metadata->count = 0;
    for (int i = 0; i < MAX_DELETED_OFFSET_COUNT; i++) {
        metadata->offsets[i] = -1;
    }
}

void update_metadata_on_delete(Metadata *metadata, long int deletedOffset) {
    if (metadata->count < MAX_DELETED_OFFSET_COUNT) {
        metadata->offsets[metadata->count++] = deletedOffset;
    } else {
        for (int i = 1; i < MAX_DELETED_OFFSET_COUNT; i++) {
            metadata->offsets[i - 1] = metadata->offsets[i];
        }
        metadata->offsets[MAX_DELETED_OFFSET_COUNT - 1] = deletedOffset;
    }
}

long int find_available_offset(Metadata *metadata) {
    if (metadata->count > 0) {
        long int offset = metadata->offsets[0];
        for (int i = 1; i < metadata->count; i++) {
            metadata->offsets[i - 1] = metadata->offsets[i];
        }
        metadata->count--;
        return offset;
    }
    return -1; // No available offset
}

Metadata read_metadata(int fd) {
    if (fd < 0) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    Metadata metadata;
    read(fd, &metadata, sizeof(Metadata));

    return metadata;
}



int generate_datafiles(int count, char* name){

    for (int i = 1; i <= dcount; i++) {
        char filename[100];
        snprintf(filename, sizeof(filename), "%s%d", fname, i);
        FILE *file = fopen(filename, "wb");
        if (file == NULL) {
            perror("Error opening file");
            return EXIT_FAILURE;
        }

        Metadata metadata;
        initialize_metadata(&metadata);
        fwrite(&metadata, sizeof(Metadata), 1, file);

        fclose(file);
    }
    return EXIT_SUCCESS;
}

int write_entry_to_file(Entry* entry, int fd, long int file_offset){

    file_offset += sizeof(Metadata);
    lseek(fd, file_offset, SEEK_SET);

    ssize_t written_key = write(fd, &(entry->key), sizeof(long int));
    if (written_key != sizeof(long int)) {
        logg(DEEP_DEBUG, "Error writing the key <%ld> to file\n", entry->key);
        close(fd);
        return EXIT_FAILURE;
    }
    logg(DEEP_DEBUG, "fd: %ld\n", lseek(fd, 0, SEEK_CUR));

    ssize_t written_flag = write(fd, &(entry->is_deleted), sizeof(bool));
    if (written_flag != sizeof(bool)) {
        logg(DEEP_DEBUG, "Error writing the flag to file for key <%ld>\n", entry->key);
        close(fd);
        return EXIT_FAILURE;
    }
    logg(DEEP_DEBUG, "fd: %ld\n", lseek(fd, 0, SEEK_CUR));

    ssize_t written_value = write(fd, entry->value, vsize);
    if (written_value != vsize) {
        logg(DEEP_DEBUG, "Error writing the value to file for key <%ld>\n", entry->key);
        close(fd);
        return EXIT_FAILURE;
    }
    logg(DEEP_DEBUG, "fd: %ld\n", lseek(fd, 0, SEEK_CUR));

    logg(DEEP_DEBUG, "Entry successfully written. File offset after write: %ld\n", lseek(fd, 0, SEEK_CUR));
    return EXIT_SUCCESS;

}

int read_entry_from_file(Entry* entry, int fd, long int file_offset){

    file_offset += sizeof(Metadata);
    lseek(fd, file_offset, SEEK_SET);

    long int key;
    bool is_deleted;
    char value[vsize];

    ssize_t read_key = read(fd, &key, sizeof(long int));
    if (read_key != sizeof(long int)) {
        logg(DEEP_DEBUG, "Error reading the key \n");
        close(fd);
        return EXIT_FAILURE;
    }

    ssize_t read_flag = read(fd, &is_deleted, sizeof(bool));
    if (read_flag != sizeof(bool)) {
        logg(DEEP_DEBUG, "Error reading the flag from file for key <%ld>\n", key);
        close(fd);
        return EXIT_FAILURE;
    }

    ssize_t read_value = read(fd, value, vsize);
    if (read_value != vsize) {
        logg(DEEP_DEBUG, "Error reading the value from file for key <%ld>\n", key);
        close(fd);
        return EXIT_FAILURE;
    }

    *(entry) = *(createEntry(key, value, vsize));

    logg(DEEP_DEBUG, "File offset after read: %ld\n", lseek(fd, 0, SEEK_CUR));
    return EXIT_SUCCESS;

}
