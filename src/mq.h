//
// Created by Mehmet Eren Balasar on 18.11.2023.
//

#ifndef SRC_MQ_H
#define SRC_MQ_H

#include "globals.h"
#include <mqueue.h>

int create_mqs();
int receive_request(Request* req, mqd_t receive_mq); // retrieves request from message queue 1
int send_response(Response*, mqd_t send_mq); // sends response to message queue 2

#endif //SRC_MQ_H
