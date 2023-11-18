//
// Created by Mehmet Eren Balasar on 14.11.2023.
//

#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <mqueue.h>

#include "server.h"
#include "globals.h"
#include "hash_table.h"
#include "disk.h"
#include "mq.h"

pthread_mutex_t buffer_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t buffer_cond = PTHREAD_COND_INITIALIZER;
Request* request_buffer;

int buffer_size = 50;
int count = 0;

int start_server(){

    generate_datafiles(dcount, fname);
    generate_hash_tables(dcount);

    file_locks = (pthread_mutex_t *)malloc(dcount * sizeof(pthread_mutex_t));
    for (int i = 0; i < dcount; i++) {
        pthread_mutex_init(&file_locks[i], NULL);
    }

    create_mqs();

    create_workers();

    pthread_t fe_thread;
    pthread_create(&fe_thread, NULL, fe_thread_func, NULL);



    for (int i = 0; i < tcount; i++) {
        pthread_join(workers[i], NULL);
    }
    pthread_join(fe_thread, NULL);

    return 0;
}

void* worker_thread_func(){
    mqd_t mqd = mq_open(response_mq_name, O_WRONLY);
    if (mqd == -1) {
        perror("Error opening message queue");
        exit(EXIT_FAILURE);
    }
    Request req;
    Response res;

    while (1) {
        pthread_mutex_lock(&buffer_mutex);

        // check if buffer is empty
        while (count == 0) {
            pthread_cond_wait(&buffer_cond, &buffer_mutex); // wait for a signal from the front-end thread
        }

        memcpy(&req, &request_buffer[count - 1], sizeof(Request));
        count--;

        pthread_cond_signal(&buffer_cond);
        pthread_mutex_unlock(&buffer_mutex);

        // process request
        if (strcmp(req.method, "GET") == 0){
            logg(INFO, "GET request received\n");
            res = get(&req);
        }
        else if (strcmp(req.method, "PUT") == 0){
            logg(INFO, "PUT request received\n");
            res = put(&req);
        }
        else if (strcmp(req.method, "DELETE") == 0){
            logg(INFO, "DELETE request received\n");
            res = delete(&req);
        }
        else if (strcmp(req.method, "DUMP") == 0){
            logg(INFO, "DUMP request received\n");
            res = dump(&req);
        }
        else if (strcmp(req.method, "QUITSERVER") == 0){
            logg(INFO, "QUITSERVER request received\n");
            res = quitserver(&req);
        }
        else{
            perror("Invalid request type");
        }

        // Send response to message queue 2
        if (send_response(&res, mqd) < 0) {
            perror("Error sending response");
            break; // Exit loop on error
        }
        logg(INFO, "Response sent, client_ip -> %d\n status_code -> %d\n info_message -> %s\n value -> %s",
             res.client_ip, res.status_code, res.info_message, res.value);
    }

    mq_close(mqd);
    return NULL;
}

void* fe_thread_func(){
    mqd_t mqd = mq_open(request_mq_name, O_RDONLY);

    Request req;

    while (1) {

        if (receive_request(&req, mqd) < 0) {
            perror("Error receiving request");
            break;
        }

        // lock mutex to add request to buffer
        pthread_mutex_lock(&buffer_mutex);

        // check if buffer is full
        while (count == buffer_size) {
            pthread_cond_wait(&buffer_cond, &buffer_mutex); // wait for a signal from a worker thread
        }

        memcpy(&request_buffer[count], &req, sizeof(Request));
        count++;

        pthread_cond_signal(&buffer_cond);
        pthread_mutex_unlock(&buffer_mutex);
    }

    mq_close(mqd);
    return NULL;
}

int create_workers(){
    workers = (pthread_t *)malloc(tcount * sizeof(pthread_t));

    for (int i = 0; i < tcount; i++) {
        if (pthread_create(&workers[i], NULL, worker_thread_func, NULL) != 0) {
            perror("Failed to create worker thread");
            return 1;
        }
    }

    return 0;
}

Response get(Request* request) {
    long int key = request->key;
    Response response;
    response.client_ip = request->client_ip;

    Entry entry;
    if (get_entry(key, &entry) < 0){
        response.status_code = 404;
        strcpy(response.value, "");
        strcpy(response.info_message, "KEY NOT FOUND");
    }
    else{
        response.status_code = 200;
        strcpy(response.value, entry.value);
        strcpy(response.info_message, "OK");
    }

    return response;
}

Response put(Request* request) {
    long int key = request->key;
    char* value = request->value;

    Response response;
    response.client_ip = request->client_ip;

    Entry* entry = createEntry(key, value, vsize);

    int insert_result = insert_entry(entry);

    if (insert_result < 0){
        response.status_code = 500;
        strcpy(response.value, "");
        strcpy(response.info_message, "INTERNAL SERVER ERROR");
    }
    else{
        response.status_code = 200;
        if (insert_result == 0){
            strcpy(response.info_message, "INSERTED");
        }
        else if (insert_result == 1){
            strcpy(response.info_message, "UPDATED");
        }
        strcpy(response.value, entry->value);
    }

    freeEntry(entry);
    return response;
}

Response delete(Request* request) {
    long int key = request->key;

    Response response;
    response.client_ip = request->client_ip;

    int delete_result = delete_entry(key);

    if (delete_result < 0){
        response.status_code = 404;
        strcpy(response.value, "");
        strcpy(response.info_message, "KEY NOT FOUND");
    }
    else{
        response.status_code = 200;
        strcpy(response.info_message, "OK");
        strcpy(response.value, "");
    }

    return response;
}

Response dump(Request* request) {
    Response response;
    response.client_ip = request->client_ip;
    response.status_code = 200;
    strcpy(response.info_message, "I HAVEN'T IMPLEMENTED YET");
    response.value = malloc(vsize);
    strcpy(response.value, "");

    return response;
}

Response quitserver(Request* request) {
    Response response;
    response.value = malloc(vsize);
    response.client_ip = request->client_ip;
    response.status_code = 200;
    strcpy(response.info_message, "I HAVEN'T IMPLEMENTED YET");
    strcpy(response.value, "");

    // TODO: free memory of hash tables and write them to disk
    // TODO: free memory of mutexes

    return response;
}
