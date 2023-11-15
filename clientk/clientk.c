//
// Created by hasatsrnkn on 13.11.2023.
//
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include "global_clientk.h"
#include "util_clientk.h"

#define MAX_VSIZE 1024


struct ClientThreadData {
    int thread_id;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    // Additional data as needed
};

struct ClientThreadData* clientThreadsData;  // Array of client thread data

struct Request {
    int client_ip;
    char method[10];
    long int key;
    char value[MAX_VSIZE];
};

struct Response {
    int client_ip;
    char value[1024];
    int status_code;
    char info_message[MAX_VSIZE];
};

struct Request parseRequest(char *line, int threadId) {
    struct Request req;
    memset(&req, 0, sizeof(struct Request));

    req.client_ip = threadId;

    char *token = strtok(line, " ");
    if (token != NULL) {
        strncpy(req.method, token, sizeof(req.method) - 1);
        req.method[sizeof(req.method) - 1] = '\0';

        token = strtok(NULL, " ");
        if (token != NULL) {
            req.key = atol(token);

            if (strcmp(req.method, "PUT") == 0) {
                token = strtok(NULL, " ");
                if (token != NULL) {
                    strncpy(req.value, token, sizeof(req.value) - 1);
                    req.value[sizeof(req.value) - 1] = '\0'; // Ensure null-termination
                }
            }
        }
    }

    return req;
}

void sendMessage(struct Request request) {
    //Send the request to the server via message queue
}

void* clientThreadFunction(void* arg) {
    struct ClientThreadData* data = (struct ClientThreadData*)arg;
    int threadId = data->thread_id;
    printf("Client thread started with thread id: %d\n", threadId);
    char filename[100];
    sprintf(filename, "%s%d", fname,threadId);

    FILE* inputFile = fopen(filename, "r");

    if (inputFile == NULL) {
        perror("Error opening input file");
        pthread_exit(NULL);
    }

    const int BUFFER_SIZE = 256;
    char line[BUFFER_SIZE];

    // Read each line with fgets
    while (fgets(line, BUFFER_SIZE, inputFile) != NULL) {
        // Process the line here
        printf("%s", line); // Example: Just printing the line
        struct Request request = parseRequest(line, threadId);
        sendMessage(request);

        // Lock mutex and wait for response
        printf("Client thread waiting with thread id: %d\n", threadId);

        pthread_mutex_lock(&data->mutex);
        pthread_cond_wait(&data->cond, &data->mutex);

        // Process the response here
        // You can add logic to handle the received response

        pthread_mutex_unlock(&data->mutex);
    }

    fclose(inputFile);
    return NULL;
}

void* frontEndThreadFunction(void* arg) {
    while (1) {  // Replace with an appropriate condition for termination
        struct Response response;

        // Placeholder: Receive a response from the server
        // This should be replaced with actual message queue receive logic

        // Assuming response.client_ip indicates the client thread to be notified
        int clientThreadId = response.client_ip;

        // Notify the corresponding client thread
        pthread_mutex_lock(&clientThreadsData[clientThreadId - 1].mutex);

        // Process response or store it in a shared area accessible by the client thread
        // ...

        pthread_cond_signal(&clientThreadsData[clientThreadId - 1].cond);
        pthread_mutex_unlock(&clientThreadsData[clientThreadId - 1].mutex);
    }

    return NULL;
}


int main(int argc, char* argv[]) {
    cli(argc, argv);

    pthread_t* clientThreads = malloc(clicount * sizeof(pthread_t));
    clientThreadsData = malloc(clicount * sizeof(struct ClientThreadData));

    for (int i = 0; i < clicount; i++) {

       clientThreadsData[i].thread_id  = i + 1;
       pthread_mutex_init(&clientThreadsData[i].mutex, NULL);
       pthread_cond_init(&clientThreadsData[i].cond, NULL);
       pthread_create(&clientThreads[i], NULL, clientThreadFunction, &clientThreadsData[i]);
    }

    //pthread_t feThread;
    //pthread_create(&feThread, NULL, frontEndThreadFunction, NULL);


    // Wait for all threads to complete
    for (int i = 0; i < clicount; i++) {
        pthread_join(clientThreads[i], NULL);
    }

    // Wait for the front-end thread to complete (if it ever does)
    //pthread_join(feThread, NULL);

    printf("program finished\n" );
}