//
// Created by Mehmet Eren Balasar on 16.11.2023.
//

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/fcntl.h>
#include "disk.h"

int delete_entry(long int key){
    int section = (key % dcount + 1);
    pthread_mutex_lock(&file_locks[section]);

    long int offset = search(tables[section], key);

    char filename[100];
    sprintf(filename, "%s%d", fname, section);
    int fd = open(filename, O_RDWR);

    if (offset >= 0){
        // key exists, mark as deleted
        set_is_deleted(fd, offset, true);

        delete_ht(tables[section], key);

        Metadata metadata = read_metadata(fd);
        update_metadata_on_delete(&metadata, offset);
        write_metadata(fd, &metadata);

        close(fd);
        logg(DEEP_DEBUG, "Key <%ld> deleted\n", key);

        pthread_mutex_unlock(&file_locks[section]);
        return 0;
    }
    else {
        // key does not exist
        logg(DEEP_DEBUG, "Key <%ld> does not exist\n", key);
        close(fd);

        pthread_mutex_unlock(&file_locks[section]);
        return -1;
    }
}

int get_entry(long int key, Entry* entry){
    int section = (key % dcount + 1);

    long int offset = search(tables[section], key);

    char filename[100];
    sprintf(filename, "%s%d", fname, section);
    int fd = open(filename, O_RDONLY);

    if (offset >= 0){
        // key exists, read entry
        read_entry_from_file(entry, fd, offset);

        logg(DEEP_DEBUG, "Key <%ld> found\n", key);
        close(fd);
        return 0;
    }
    else {
        // key does not exist
        logg(DEEP_DEBUG, "get_entry: Key <%ld> does not exist\n", key);
        close(fd);
        return -1;
    }
}


// returns 0 for insertion, 1 for modification, -1 for failure
int insert_entry(Entry* entry){
    //
    long int entry_size = sizeof(long int) + sizeof(bool) + vsize;

    long int key = entry->key;
    int section = (key % dcount + 1);

    pthread_mutex_lock(&file_locks[section]);

    long int offset = search(tables[section], key);

    char filename[100];
    sprintf(filename, "%s%d", fname, section);
    int fd = open(filename, O_RDWR);

    if (offset >= 0){
        // key exists, modify
        write_entry_to_file(entry, fd, offset);
        logg(DEEP_DEBUG, "Key <%ld> modified, new value-> %s\n", key, entry->value);
        close(fd);

        pthread_mutex_unlock(&file_locks[section]);
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

            write_metadata(fd, &file_metadata);

            logg(DEEP_DEBUG, "Key <%ld> inserted to the offset <%ld> found in metadata, value-> %s\n", key, offset, entry->value);
            close(fd);

            pthread_mutex_unlock(&file_locks[section]);
            return 0;
        }
        else {
            // there is no available offset so traverse the file
            Entry temp_entry;
            long int temp_offset = 0;
            while(read_entry_from_file(&temp_entry, fd, temp_offset) >= 0){
                if (temp_entry.is_deleted || temp_entry.key == entry->key){
                    // found an available offset or the key (maybe hash table not updated yet)
                    break;
                }
                temp_offset += entry_size;
            }

            write_entry_to_file(entry, fd, temp_offset);
            insert(tables[section], entry->key, temp_offset);

            logg(DEEP_DEBUG, "Key <%ld> inserted to the offset <%ld> found by traversing the file, value-> %s\n", key, temp_offset, entry->value);

            pthread_mutex_unlock(&file_locks[section]);
            close(fd);
            return 0;
        }
    }
}



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
    lseek(fd, 0, SEEK_SET);
    if (fd < 0) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    Metadata metadata;
    read(fd, &metadata, sizeof(Metadata));

    return metadata;
}

int write_metadata(int fd, Metadata *metadata) {
    if (fd < 0) {
        perror("Error opening file");
        return -1;
    }

    lseek(fd, 0, SEEK_SET);
    write(fd, metadata, sizeof(Metadata));

    return 0;
}

int generate_datafiles(int count, char* name){

    for (int i = 1; i <= dcount; i++) {
        char filename[100];
        snprintf(filename, sizeof(filename), "%s%d", fname, i);
        int fd = open(filename, O_CREAT | O_RDWR, 0666);
        if (fd < 0) {
            logg(DEEP_DEBUG,"Error opening or creating file\n");
            return EXIT_FAILURE;
        }

        Metadata metadata;
        initialize_metadata(&metadata);
        write(fd, &metadata, sizeof(Metadata));

        close(fd);
    }
    return EXIT_SUCCESS;
}

int write_entry_to_file(Entry* entry, int fd, long int file_offset){

    if (file_offset < 0){
        logg(DEEP_DEBUG, "Error writing the entry to file. Offset is negative\n");
        //close(fd);
        return -1;
    }

    file_offset += sizeof(Metadata);
    lseek(fd, file_offset, SEEK_SET);

    ssize_t written_key = write(fd, &(entry->key), sizeof(long int));
    if (written_key != sizeof(long int)) {
        logg(DEEP_DEBUG, "Error writing the key <%ld> to file\n", entry->key);
        //close(fd);
        return -1;
    }

    ssize_t written_flag = write(fd, &(entry->is_deleted), sizeof(bool));
    if (written_flag != sizeof(bool)) {
        logg(DEEP_DEBUG, "Error writing the flag to file for key <%ld>\n", entry->key);
        //close(fd);
        return -1;
    }

    ssize_t written_value = write(fd, entry->value, vsize);
    if (written_value != vsize) {
        logg(DEEP_DEBUG, "Error writing the value to file for key <%ld>\n", entry->key);
        //close(fd);
        return -1;
    }

    logg(DEEP_DEBUG, "Entry successfully written. File offset after write: %ld\n", lseek(fd, 0, SEEK_CUR));
    return EXIT_SUCCESS;

}

int read_entry_from_file(Entry* entry, int fd, long int file_offset){

    if (file_offset < 0){
        logg(DEEP_DEBUG, "Error reading the entry from file. Offset is negative\n");
        //close(fd);
        return -1;
    }

    file_offset += sizeof(Metadata);
    lseek(fd, file_offset, SEEK_SET);

    long int key;
    bool is_deleted;
    char value[vsize];

    ssize_t read_key = read(fd, &key, sizeof(long int));
    if (read_key != sizeof(long int)) {
        logg(DEEP_DEBUG, "Error reading the key or EOF\n");
        //close(fd);
        return -1;
    }

    ssize_t read_flag = read(fd, &is_deleted, sizeof(bool));
    if (read_flag != sizeof(bool)) {
        logg(DEEP_DEBUG, "Error reading the flag from file for key <%ld>\n", key);
        //close(fd);
        return -1;
    }

    ssize_t read_value = read(fd, value, vsize);
    if (read_value != vsize) {
        logg(DEEP_DEBUG, "Error reading the value from file for key <%ld>\n", key);
        //close(fd);
        return -1;
    }

    *(entry) = *(createEntry(key, value, vsize));

    logg(DEEP_DEBUG, "Read entry: key->%ld, is_deleted->%d, value->%s\n", key, is_deleted, value);
    return EXIT_SUCCESS;

}

int set_is_deleted(int fd, long int file_offset, bool is_deleted){

    file_offset += sizeof(Metadata) + sizeof(long int);

    lseek(fd, file_offset, SEEK_SET);
    ssize_t written_flag = write(fd, &is_deleted, sizeof(bool));
    if (written_flag != sizeof(bool)) {
        logg(DEEP_DEBUG, "Error writing the flag to file for offset <%ld>\n", file_offset);
        //close(fd);
        return -1;
    }
    logg(DEEP_DEBUG, "deleted, fd: %ld\n", lseek(fd, 0, SEEK_CUR));

    return EXIT_SUCCESS;
}

int handle_dump_request(char *dump_file_name) {
    int fd, dump_fd;
    long int offset;
    Entry entry;
    long int entry_size = sizeof(long int) + sizeof(bool) + vsize;

    dump_fd = open(dump_file_name, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (dump_fd < 0) {
        perror("Error opening dump file");
        return -1;
    }

    printf("Dumping data to file %s\n", dump_file_name);
    for (int i = 0; i < dcount; i++) {
        char file_name[100];
        sprintf(file_name, "%s%d", fname, i + 1);  // construct the data file name

        // Open the data file
        fd = open(file_name, O_RDONLY);
        if (fd < 0) {
            perror("Error opening data file");
            return -1;
        }

        offset = 0;
        while (read_entry_from_file(&entry, fd, offset) >= 0) {
            if (!entry.is_deleted) {
                dprintf(dump_fd, "%ld %s", entry.key, entry.value);
            }
            offset += entry_size;
        }
        dprintf(dump_fd, "\n");
        close(fd);

    }

    close(dump_fd);
    return 0;
}


