//
// Created by Mehmet Eren Balasar on 14.11.2023.
//

#ifndef SRC_HASH_TABLE_H
#define SRC_HASH_TABLE_H
#define HASH_TABLE_SIZE 1024


// Node structure for the linked list

typedef struct Node {
    long int key;
    long int fileOffset;
    struct Node *next;
} Node;

// Hash table structure
typedef struct {
    Node* buckets[HASH_TABLE_SIZE];
} HashTable;


void generate_hash_tables(int dcount);
unsigned int hash(long int key);
void initHashTable(HashTable *table);
void insert(HashTable *table, long int key, long int fileOffset);
long int search(HashTable *table, long int key);
void delete_ht(HashTable *table, long int key);
void free_hash_table(HashTable *table);


#endif //SRC_HASH_TABLE_H
