//
// Created by hasatsrnkn on 13.11.2023.
//
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include "global_clientk.h"
#include "util_clientk.h"
#include <mqueue.h>

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
    char* value;
};

struct Response {
    int client_ip;
    int status_code;
    char info_message[100];
    char* value;

};

struct Response* responses;

struct Request parseRequest(char *line, int threadId) {
    struct Request req;
    memset(&req, 0, sizeof(struct Request));

    req.client_ip = threadId;
    req.value = malloc(vsize);
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
                    strncpy(req.value, token, vsize);
                    req.value[sizeof(req.value) - 1] = '\0'; // Ensure null-termination
                }
            }
        }
    }

    return req;
}

int sendRequest(struct Request* request, mqd_t send_mq) {
    //Send the request to the server via message queue

    if (request == NULL) {
        fprintf(stderr, "Request pointer is NULL\n");
        return -1;
    }

    size_t total_size = sizeof(struct Request) - sizeof(request->value) + vsize;
    char *buffer = malloc(total_size);

    if (buffer == NULL) {
        perror("Unable to allocate memory for buffer");
        return -1;
    }
    if (memcpy(buffer, request, sizeof(struct Request) - sizeof(request->value)) < 0){
        perror("Error copying request's first part");
        free(buffer);
        return -1;
    }

    if (memcpy(buffer + sizeof(struct Request) - sizeof(request->value), request->value, vsize) < 0){
        perror("Error copying request's second part");
        free(buffer);
        return -1;
    }

    // Send the buffer
    int status = mq_send(send_mq, buffer, total_size, 0);
    if (status == -1) {
        perror("Error sending message");
        free(buffer);
        return -1;
    }

    free(buffer);
    return 0;
}

int receiveResponse(struct Response* response, mqd_t receive_mq) {
    //Send the request to the server via message queue

    if (response == NULL) {
        fprintf(stderr, "Response pointer is NULL\n");
        return -1;
    }

    size_t total_size = sizeof(struct Response) - sizeof(response->value) + vsize;

    char *buffer = malloc(total_size);
    if (buffer == NULL) {
        perror("Unable to allocate memory for buffer");
        return -1;
    }

    ssize_t bytes_read = mq_receive(receive_mq, buffer, total_size, NULL);
    if (bytes_read == -1) {
        perror("Error receiving message");
        free(buffer);
        return -1;
    }

    memcpy(response, buffer, sizeof(struct Response) - sizeof(response->value));

    response->value = malloc(vsize);
    if (response->value == NULL) {
        perror("Unable to allocate memory for request value");
        free(buffer);
        return -1;
    }
    memcpy(response->value, buffer + sizeof(struct Response) - sizeof(response->value), vsize);

    free(buffer);
    return 0;
}

void* clientThreadFunction(void* arg) {
    mqd_t request_mq;
    char* request_mq_name = malloc(strlen(mqname) + sizeof(char));
    sprintf(request_mq_name, "/%s1", mqname);
    request_mq = mq_open(request_mq_name, O_WRONLY);
    free(request_mq_name);

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

    const int BUFFER_SIZE = 1050;
    char line[BUFFER_SIZE];

    // Read each line with fgets
    while (fgets(line, BUFFER_SIZE, inputFile) != NULL) {
        // Process the line here
        struct Request request = parseRequest(line, threadId);
        sendRequest(&request, request_mq);
        if (dlevel == 1) {
            printf("Request of thread%d: %s %ld %s\n", threadId, request.method, request.key, request.value);
        }
        // Lock mutex and wait for response
        pthread_mutex_lock(&data->mutex);
        pthread_cond_wait(&data->cond, &data->mutex);

        // Process the response here
        // 404 not found get delete
        // 200 success get delete put
        // 500 server error get delete put
        struct Response response = responses[threadId - 1];
        if (dlevel == 1) {
            printf("Response of thread%d for %s %ld %s\n",threadId,request.method, request.key, request.value);
            printf("%s\n", response.info_message);
            if ((strcmp("GET", request.method) == 0) && response.status_code != 404)
                printf("Value is: %s\n", response.value);
        }
        // You can add logic to handle the received response
        pthread_mutex_unlock(&data->mutex);
    }

    fclose(inputFile);

    return NULL;
}

void* frontEndThreadFunction(void* arg) {
    mqd_t response_mqt;
    char* response_mq_name = malloc(strlen(mqname) + sizeof(char));
    sprintf(response_mq_name, "/%s2", mqname);
    response_mqt = mq_open(response_mq_name, O_RDONLY);
    free(response_mq_name);

    struct Response response;

    while (!isFinished) {  // Replace with an appropriate condition for termination

        // Placeholder: Receive a response from the server
        // This should be replaced with actual message queue receive logic
        if (receiveResponse(&response, response_mqt) < 0) {
            perror("Error receiving response\n");
            break;
        }

        // Assuming response.client_ip indicates the client thread to be notified
        int clientThreadId = response.client_ip;

        // Notify the corresponding client thread
        pthread_mutex_lock(&clientThreadsData[clientThreadId - 1].mutex);

        // Process response or store it in a shared area accessible by the client thread
        responses[clientThreadId- 1 ] = response;

        pthread_cond_signal(&clientThreadsData[clientThreadId - 1].cond);
        pthread_mutex_unlock(&clientThreadsData[clientThreadId - 1].mutex);
    }

    return NULL;
}



int main(int argc, char* argv[]) {
    cli(argc, argv);

    //input file mode
    if ( clicount != 0 ) {
        pthread_t* clientThreads = malloc(clicount * sizeof(pthread_t));
        clientThreadsData = malloc(clicount * sizeof(struct ClientThreadData));

        //response array
        responses = malloc(clicount * sizeof(struct Response));
        for (int i = 0; i < clicount; i++) {

            clientThreadsData[i].thread_id  = i + 1;
            pthread_mutex_init(&clientThreadsData[i].mutex, NULL);
            pthread_cond_init(&clientThreadsData[i].cond, NULL);
            pthread_create(&clientThreads[i], NULL, clientThreadFunction, &clientThreadsData[i]);
        }

        pthread_t feThread;
        pthread_create(&feThread, NULL, frontEndThreadFunction, NULL);


        // Wait for all threads to complete
        for (int i = 0; i < clicount; i++) {
            pthread_join(clientThreads[i], NULL);
        }

        // Wait for the front-end thread to complete (if it ever does)
        isFinished = 1;

        mqd_t request_mq;
        char* request_mq_name = malloc(strlen(mqname) + sizeof(char));
        sprintf(request_mq_name, "/%s1", mqname);
        request_mq = mq_open(request_mq_name, O_WRONLY);
        free(request_mq_name);

        struct Request request;
        strncpy(request.method, "DUMP", sizeof(request.method) - 1);
        request.method[sizeof(request.method) - 1] = '\0';
        request.client_ip = 1;
        request.value = malloc(vsize);
        strncpy(request.value, "datastoredump.txt", vsize);
        sendRequest(&request, request_mq );


        struct Request request2;
        strncpy(request.method, "QUITSERVER", sizeof(request.method) - 1);
        request.method[sizeof(request.method) - 1] = '\0';
        request.client_ip = 1;
        request.value = malloc(vsize);
        sendRequest(&request2, request_mq);


        pthread_join(feThread, NULL);

    }

    // interactive mode
    else {
        printf("Interactive mode\n");
        char input[1024];
        struct Request request;

        mqd_t request_mq;
        char* request_mq_name = malloc(strlen(mqname) + sizeof(char));
        sprintf(request_mq_name, "/%s1", mqname);
        request_mq = mq_open(request_mq_name, O_WRONLY);
        free(request_mq_name);

        while (isFinished == 0) {
            printf("Enter request (PUT, GET, DEL, DUMP, QUIT, QUITSERVER): ");
            if (fgets(input, sizeof(input), stdin) == NULL) {
                printf("Error reading input.\n");
                continue;
            }

            char *token = strtok(input, " \n");
            if (token == NULL) continue;

            strncpy(request.method, token, sizeof(request.method) - 1);
            request.method[sizeof(request.method) - 1] = '\0';
            request.client_ip = 1;
            request.value = malloc(vsize);

            if (strcmp(request.method, "QUIT") == 0) {
                isFinished = 1;
                continue;
            } else if (strcmp(request.method, "QUITSERVER") == 0) {
                // Send QUITSERVER request to server
                sendRequest(&request, request_mq);
                isFinished = 1;
                continue;
            } else if (strcmp(request.method, "DUMP") == 0) {
                token = strtok(NULL, " \n");
                printf("%s\n", token);
                if (token != NULL) {
                    // Handle DUMP request logic here
                    strncpy(request.value, token, vsize);
                    // The token will have the output file name
                    sendRequest(&request, request_mq);
                }
            } else {
                // For PUT, GET, DEL
                token = strtok(NULL, " \n");
                printf("%s\n", token);

                if (token != NULL) {
                    request.key = atol(token);

                    if (strcmp(request.method, "PUT") == 0) {
                        token = strtok(NULL, " \n");
                        printf("%s\n", token);

                        if (token != NULL) {
                            strncpy(request.value, token, vsize);
                            request.value[sizeof(request.value) - 1] = '\0';
                        }
                    }
                    // Send the request to the server
                    sendRequest(&request, request_mq);
                }
            }
        }
    }

    printf("program finished\n" );
}