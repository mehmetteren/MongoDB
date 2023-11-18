//
// Created by Mehmet Eren Balasar on 14.11.2023.
//

#include "hash_table.h"
#include "globals.h"

#include <stdio.h>
#include <stdlib.h>

void generate_hash_tables(int dcount_) {

    //TODO: implement recovery from file

    tables = (HashTable**)malloc(sizeof(HashTable*) * (dcount_ + 1));
    for (int i = 1; i <= dcount_; i++) {
        HashTable* table = (HashTable*)malloc(sizeof(HashTable));
        initHashTable(table);
        tables[i] = table;
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
}

long int search(HashTable *table, long int key) {
    unsigned int index = hash(key);
    Node *current = table->buckets[index];
    while (current) {
        if (current->key == key) {
            return current->fileOffset; // Key found
        }
        current = current->next;
    }
    return -1; // Key not found
}

void delete(HashTable *table, long int key) {
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
            return;
        }
        previous = current;
        current = current->next;
    }
}
