//
// Created by Mehmet Eren Balasar on 14.11.2023.
//
#ifndef SRC_SERVER_H
#define SRC_SERVER_H
#include "globals.h"


int start_server();
int start_fe_thread();
int create_workers();
Request* receive_request(); // retrieves request from message queue 1
int send_response(Response*); // sends response to message queue 2
int find_key();
int write_to_file();


#endif //SRC_SERVER_H
