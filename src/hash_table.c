//
// Created by Mehmet Eren Balasar on 14.11.2023.
//

#include "hash_table.h"
#include "globals.h"
#include "disk.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/fcntl.h>

void generate_hash_tables(int dcount_) {

    //TODO: implement recovery from file

    tables = (HashTable**)malloc(sizeof(HashTable*) * (dcount_ + 1));
    for (int i = 1; i <= dcount_; i++) {
        HashTable* table = (HashTable*)malloc(sizeof(HashTable));
        initHashTable(table);
        tables[i] = table;
    }
    logg(INFO, "Hash tables generated\n");

    int fd;
    long int offset;
    Entry entry;
    long int entry_size = sizeof(long int) + sizeof(bool) + vsize;

    for (int i = 1; i <= dcount_; i++) {
        char file_name[100];
        sprintf(file_name, "%s%d", fname, i);  // construct the data file name

        // Open the data file
        fd = open(file_name, O_RDONLY);
        if (fd < 0) {
            perror("Error opening data file");
            exit(-1);
        }

        offset = 0;
        while (read_entry_from_file(&entry, fd, offset) >= 0) {
            if (!entry.is_deleted) {
                insert(tables[i], entry.key, offset);
            }
            offset += entry_size;
        }
        close(fd);
    }

}

// Hash function
unsigned int hash(long int key) {
    return key % HASH_TABLE_SIZE;
}

void initHashTable(HashTable *table) {
    for (int i = 0; i < HASH_TABLE_SIZE; i++) {
        table->buckets[i] = NULL;
    }
}

void insert(HashTable *table, long int key, long int fileOffset) {
    unsigned int index = hash(key);
    Node *newNode = (Node *)malloc(sizeof(Node));
    if (newNode == NULL) {
        perror("Unable to allocate memory for new node");
        exit(EXIT_FAILURE);
    }
    newNode->key = key;
    newNode->fileOffset = fileOffset;
    newNode->next = table->buckets[index];
    table->buckets[index] = newNode;
    logg(DEEP_DEBUG, "Inserted key %ld to hash table\n", key);
}

long int search(HashTable *table, long int key) {
    unsigned int index = hash(key);
    Node *current = table->buckets[index];
    while (current) {
        if (current->key == key) {
            logg(DEEP_DEBUG, "Found key %ld in hash table\n", key);
            return current->fileOffset; // Key found
        }
        current = current->next;
    }
    logg(DEEP_DEBUG, "Key %ld not found in hash table\n", key);
    return -1; // Key not found
}

void delete_ht(HashTable *table, long int key) {
    unsigned int index = hash(key);
    Node *current = table->buckets[index];
    Node *previous = NULL;
    while (current) {
        if (current->key == key) {
            if (previous == NULL) {
                table->buckets[index] = current->next;
            } else {
                previous->next = current->next;
            }
            free(current);
            logg(DEEP_DEBUG, "Deleted key %ld from hash table\n", key);
            return;
        }
        previous = current;
        current = current->next;
    }
}

void free_hash_table(HashTable *table) {
    for (int i = 0; i < HASH_TABLE_SIZE; i++) {
        Node *current = table->buckets[i];
        while (current) {
            Node *temp = current;
            current = current->next;
            free(temp);
        }
    }
    free(table);
}
