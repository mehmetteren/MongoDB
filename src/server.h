//
// Created by Mehmet Eren Balasar on 14.11.2023.
//
#ifndef SRC_SERVER_H
#define SRC_SERVER_H
#include "globals.h"


int start_server();
void* fe_thread_func();
void* worker_thread_func();

Response get(Request* req);
Response put(Request* req);
Response delete(Request* req);
Response dump(Request* req);
Response quitserver(Request* req);

int create_workers();

#endif //SRC_SERVER_H
