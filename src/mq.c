//
// Created by Mehmet Eren Balasar on 18.11.2023.
//

#include "mq.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int receive_request(Request* req, mqd_t receive_mq) {
    if (req == NULL) {
        fprintf(stderr, "Request pointer is NULL\n");
        return -1;
    }

    size_t total_size = sizeof(Request) - sizeof(req->value) + vsize;

    char *buffer = malloc(total_size);
    if (buffer == NULL) {
        perror("Unable to allocate memory for buffer");
        return -1;
    }

    ssize_t bytes_read = mq_receive(receive_mq, buffer, total_size, 0);
    if (bytes_read == -1) {
        perror("Error receiving message");
        free(buffer);
        return -1;
    }

    memcpy(req, buffer, sizeof(Request) - sizeof(req->value));

    req->value = malloc(vsize);
    if (req->value == NULL) {
        perror("Unable to allocate memory for request value");
        free(buffer);
        return -1;
    }
    memcpy(req->value, buffer + sizeof(Request) - sizeof(req->value), vsize);

    free(buffer);
    return 0;
}



int send_response(Response* res, mqd_t send_mq) {
    if (res == NULL) {
        fprintf(stderr, "Response pointer is NULL\n");
        return -1;
    }

    size_t total_size = sizeof(Response) - sizeof(res->value) + vsize;
    char *buffer = malloc(total_size);

    if (buffer == NULL) {
        perror("Unable to allocate memory for buffer");
        return -1;
    }

    memcpy(buffer, res, sizeof(Response) - sizeof(res->value));

    memcpy(buffer + sizeof(Response) - sizeof(res->value), res->value, vsize);

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

int create_mqs() {
    mqd_t request_mq; mqd_t response_mq;

    request_mq_name = malloc(strlen(mqname) + 2);
    response_mq_name = malloc(strlen(mqname) + 2);

    sprintf(request_mq_name, "/%s1", mqname);
    sprintf(response_mq_name, "/%s2", mqname);

    mq_unlink(request_mq_name);
    mq_unlink(response_mq_name);

    struct mq_attr attr;
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = sizeof(Request) - sizeof(char*) + vsize;

    request_mq = mq_open(request_mq_name, O_CREAT | O_RDWR, 0666, &attr);
    if (request_mq == (mqd_t)-1) {
        perror("Error creating message queue 1");
        return -1;
    }

    attr.mq_msgsize = sizeof(Response) - sizeof(char*) + vsize;
    response_mq = mq_open(response_mq_name, O_CREAT | O_RDWR, 0666, &attr);
    if (response_mq == (mqd_t)-1) {
        perror("Error creating message queue 2");
        mq_close(request_mq);
        return -1;
    }

    mq_getattr(request_mq, &attr);
    printf("Request queue attributes:\n");
    printf("Maximum # of messages on queue:   %ld\n", attr.mq_maxmsg);
    printf("Maximum message size:             %ld\n", attr.mq_msgsize);

    mq_getattr(response_mq, &attr);
    printf("Response queue attributes:\n");
    printf("Maximum # of messages on queue:   %ld\n", attr.mq_maxmsg);
    printf("Maximum message size:             %ld\n", attr.mq_msgsize);

    mq_close(request_mq);
    mq_close(response_mq);

    return 0;
}
