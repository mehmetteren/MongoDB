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
int* thread_ids;

int buffer_size = 50;
int count = 0;

int start_server(){

    generate_datafiles(dcount, fname);
    generate_hash_tables(dcount);

    file_locks = (pthread_mutex_t *)malloc(dcount * sizeof(pthread_mutex_t) + sizeof(pthread_mutex_t));
    for (int i = 1; i <= dcount; i++) {
        pthread_mutex_init(&file_locks[i], NULL);
    }

    create_mqs();

    create_workers();

    request_buffer = (Request*)malloc(buffer_size * sizeof(Request));

    pthread_t fe_thread;
    pthread_create(&fe_thread, NULL, fe_thread_func, NULL);

    for (int i = 0; i < tcount; i++) {
        pthread_join(workers[i], NULL);
    }

    pthread_join(fe_thread, NULL);

    return 0;
}

void* worker_thread_func(void* id){
    int tid = *(int*)id;
    logg(INFO, "Worker thread %d started\n", tid);

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
        printf("Worker thread %d received request from buffer, request method: %s\n", tid, req.method);

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
        else if (strcmp(req.method, "DEL") == 0){
            logg(INFO, "DEL request received\n");
            res = delete(&req);
        }
        else if (strcmp(req.method, "DUMP") == 0){
            logg(INFO, "DUMP request received\n");
            res = dump(&req);
        }
        else{
            perror("Invalid request type");
            continue;
        }

        // send response to message queue 2
        if (send_response(&res, mqd) < 0) {
            perror("Error sending response from worker");
            break;
        }
        logg(INFO, "Response sent from worker #%d, client_ip -> %d\n status_code -> %d\n info_message -> %s\n value -> %s",
             tid, res.client_ip, res.status_code, res.info_message, res.value);
    }

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

        if (strcmp(req.method, "QUITSERVER") == 0){
            mqd_t mqd2 = mq_open(response_mq_name, O_RDWR);

            logg(INFO, "QUITSERVER request received\n");

            Response res;
            res = quitserver(&req);

            // send response to message queue 2
            if (send_response(&res, mqd2) < 0) {
                perror("Error sending response");
                break;
            }

            free(workers);
            mq_unlink(request_mq_name);
            mq_unlink(response_mq_name);

            mq_close(mqd);
            mq_close(mqd2);
            logg(INFO, "Server is shutting down\n");
            exit(0);
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

    return NULL;
}

int create_workers(){
    workers = (pthread_t *)malloc(tcount * sizeof(pthread_t));
    thread_ids = (int *)malloc(sizeof(int) * tcount);

    for (int i = 0; i < tcount; i++) {
        thread_ids[i] = i;
        if (pthread_create(&workers[i], NULL, worker_thread_func, &thread_ids[i]) != 0) {
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

    Entry* entry;
    if (get_entry(key, &entry) < 0){
        response.status_code = 404;
        strcpy(response.value, "");
        strcpy(response.info_message, "KEY NOT FOUND");
    }
    else{
        response.status_code = 200;
        printf("value: %s\n", entry->value);
        strcpy(response.value, entry->value);
        strcpy(response.info_message, "OK");
        freeEntry(entry);
    }

    return response;
}

Response put(Request* request) {
    long int key = request->key;
    char* value = request->value;

    Response response;
    response.client_ip = request->client_ip;

    Entry* entry = createEntry(key, value, vsize, 0);

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

    if(handle_dump_request(request->value) >= 0){
        response.status_code = 200;
        sprintf(response.info_message, "DUMPED DATAFILES INTO %s", request->value);
    }
    else{
        response.status_code = 500;
        strcpy(response.info_message, "INTERNAL SERVER ERROR");
    }
    response.value = malloc(vsize);
    strcpy(response.value, "");

    return response;
}

Response quitserver(Request* request) {
    Response response;

    for (int i = 1; i <= dcount; i++){
        //free_hash_table(tables[i]);
    }
    //free(file_locks);
    //free(thread_ids);
    //free(request_buffer);
    
    response.value = malloc(vsize);
    response.client_ip = request->client_ip;
    response.status_code = 200;
    strcpy(response.info_message, "QUITTED SERVER");
    strcpy(response.value, "");

    return response;
}
