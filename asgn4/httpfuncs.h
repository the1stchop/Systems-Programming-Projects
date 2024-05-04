#ifndef HTTPFUNCS_H_INCLUDE_
#define HTTPFUNCS_H_INCLUDE_

#include <stdio.h>

#include "rwlock.h"

// this struct with hold all the shit from the request
typedef struct Request {
    char *method; // hold method string
    char *URI; // holds uri string
    char *version; // holds virsion string
    int content_length; // holds content-length of message-body
    int request_id; // holds request-id
    int connection; // holds connection socket
    char *message_body; // holds part of message body initial read_until read
    int bytes_read; // holds bytes read by read_until
    int bytes_left; // holds number of bytes of message body initially read by read_until

} Request;

// this struct holds all the shit for a single element in the lock array
typedef struct locknode {
    rwlock_t *lock;
    char *file_name;
    int num_users;
} locknode;

// this function will take in a buffer containing a request and will parse it. Returns 0 for success, 1 otherwise
int parse_request(char *buff, Request *rq);

// this function will take in a Request struct and attempt to execute it. Returns 0 for success, 1 otherwise
int run_request(Request *request);

// returns the lock for the specific uri
// if there is no lock for that uri, will replace a uri no being used
locknode *obtainlock(locknode **la, Request *rq, int num_threads);

#endif
