#include "httpfuncs.h"
#include "rwlock.h"
#include "asgn2_helper_funcs.h"
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <regex.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>

// this function will take in a buffer containing a request and will parse it. Returns 1 for success, 0 otherwise
int parse_request(char *buff, Request *rq) {
    rq->request_id = 0;
    // regex for request line
    static const char *const re1
        = "^([a-zA-Z]{1,8}) (/[a-zA-Z0-9.-]{1,63}) (HTTP/[0-9].[0-9])\r\n.*$";
    // regex for header files
    static const char *const re2 = "^([a-zA-Z0-9-]{1,128}): ([ -~]{,128})\r\n.*$";
    regex_t regex;
    regmatch_t pmatch[4];

    // compiling regex for request line
    if (regcomp(&regex, re1, REG_NEWLINE | REG_EXTENDED)) {
        fprintf(stderr, "you fucked upedd\n");
        exit(EXIT_FAILURE);
    }

    // checking for match
    if (regexec(&regex, buff, 4, pmatch, 0) == 0) {
        rq->method = buff;
        rq->URI = buff + pmatch[2].rm_so;
        rq->version = buff + pmatch[3].rm_so;
        // adding null terminators to end of every match. Doesn't effect the values because of separation
        buff[pmatch[1].rm_eo] = '\0'; // method
        buff[pmatch[2].rm_eo] = '\0'; // uri
        buff[pmatch[3].rm_eo] = '\0'; // version
    }
    // if there is no match, return 0 and print response to client. also free regex
    else {
        write_n_bytes(rq->connection,
            "HTTP/1.1 400 Bad Request\r\nContent-Length: 12\r\n\r\nBad Request\n", 60);
        regfree(&regex);
        return 1;
    }

    // incrementing buffer to header-field
    buff += pmatch[3].rm_eo + 2;

    // find bytes of message_body
    rq->bytes_left = rq->bytes_read - pmatch[3].rm_eo - 2;

    // compiling regex for header-field
    if (regcomp(&regex, re2, REG_NEWLINE | REG_EXTENDED)) {
        fprintf(stderr, "you fucked upedd\n");
        exit(EXIT_FAILURE);
    }

    // using temp pointer so cuz ima be moving around the buffer
    char *temp = buff;
    int cmp = regexec(&regex, temp, 3, pmatch, 0);
    while (cmp == 0) {
        temp[pmatch[1].rm_eo] = '\0';
        temp[pmatch[2].rm_eo] = '\0';
        // look for Content-length and if ya, place content_length in struct
        if (strncmp(temp + pmatch[1].rm_so, "Content-Length", 15) == 0) {
            rq->content_length = atoi(temp + pmatch[2].rm_so);
            if (rq->content_length == 0) {
                write_n_bytes(rq->connection,
                    "HTTP/1.1 400 Bad Request\r\nContent-Length: 12\r\n\r\nBad Request\n", 60);
                regfree(&regex);
                return 1;
            }
        }
        // look for Request-Id and if ya, place Request-Id in struct
        if (strncmp(temp + pmatch[1].rm_so, "Request-Id", 11) == 0) {
            rq->request_id = atoi(temp + pmatch[2].rm_so);
        }

        // use a temp ptr to increment to the next header-field
        temp += pmatch[2].rm_eo + 2;
        rq->bytes_left -= (pmatch[2].rm_eo + 2);

        // try finding next match
        cmp = regexec(&regex, temp, 3, pmatch, 0);
    }
    // check for \r\n
    if (temp[0] != '\r' || temp[1] != '\n') {
        write_n_bytes(rq->connection,
            "HTTP/1.1 400 Bad Request\r\nContent-Length: 12\r\n\r\nBad Request\n", 60);
        regfree(&regex);
        return 1;
    } else {
        rq->message_body = temp + 2;
        rq->bytes_left -= 2;
    }

    regfree(&regex);
    return 0;
}

// this helper function for run_request will perform a get request with rq shit
int get(Request *rq) {
    // check if shits a directory
    if (open(rq->URI + 1, O_DIRECTORY) != -1) {
        write_n_bytes(
            rq->connection, "HTTP/1.1 403 Forbidden\r\nContent-Length: 10\r\n\r\nForbidden\n", 56);
        fprintf(stderr, "%s,%s,%d,%d\n", rq->method, rq->URI, 403, rq->request_id);
        return 1;
    }
    // attempt to open file
    int fd = open(rq->URI + 1, O_RDONLY, 0);
    // if unable
    if (fd == -1) {
        // checks if error was due to no file found
        if (errno == ENOENT) {
            write_n_bytes(rq->connection,
                "HTTP/1.1 404 Not Found\r\nContent-Length: 12\r\n\r\nNot Found\n", 56);
            fprintf(stderr, "%s,%s,%d,%d\n", rq->method, rq->URI, 404, rq->request_id);
            return 1;
        }
        // cheacks if error was due to being unable to access
        else if (errno == EACCES) {
            write_n_bytes(rq->connection,
                "HTTP/1.1 403 Forbidden\r\nContent-Length: 10\r\n\r\nForbidden\n", 56);
            fprintf(stderr, "%s,%s,%d,%d\n", rq->method, rq->URI, 403, rq->request_id);
            return 1;
        }
        // shit just fucked up for some reason
        else {
            write_n_bytes(rq->connection,
                "HTTP/1.1 500 Internal Server Error\r\nContent-Length: 22\r\n\r\nInternal Server "
                "Error\n",
                80);
            fprintf(stderr, "%s,%s,%d,%d\n", rq->method, rq->URI, 500, rq->request_id);
            return 1;
        }
    }

    // get size of file
    int size = lseek(fd, 0, SEEK_END);
    lseek(fd, 0, SEEK_SET);

    // create response
    char response[60];
    sprintf(response, "HTTP/1.1 200 OK\r\nContent-Length: %d\r\n\r\n", size);
    fprintf(stderr, "%s,%s,%d,%d\n", rq->method, rq->URI, 200, rq->request_id);

    // printf out response
    write_n_bytes(rq->connection, response, strlen(response));
    // move shit from file to connection
    pass_n_bytes(fd, rq->connection, size);
    close(fd);
    return 0;
}

// this helper function for run_request will perform put request with rq shit
int put(Request *rq) {
    // check if shits a directory
    if (open(rq->URI + 1, O_DIRECTORY) != -1) {
        write_n_bytes(
            rq->connection, "HTTP/1.1 403 Forbidden\r\nContent-Length: 10\r\n\r\nForbidden\n", 56);
        fprintf(stderr, "%s,%s,%d,%d\n", rq->method, rq->URI, 403, rq->request_id);
        return 1;
    }
    // attempt to open file
    // if no file but is valid, create one
    int fd = open(rq->URI + 1, O_WRONLY | O_CREAT | O_TRUNC | O_EXCL, 0666);
    // if unable
    if (fd == -1 && errno != EEXIST) {
        // checks if error was due to being unable to access
        if (errno == EACCES) {
            write_n_bytes(rq->connection,
                "HTTP/1.1 403 Forbidden\r\nContent-Length: 10\r\n\r\nForbidden\n", 56);
            fprintf(stderr, "%s,%s,%d,%d\n", rq->method, rq->URI, 403, rq->request_id);
            return 1;
        }
        // shit just fucked up for some reason
        else {
            write_n_bytes(rq->connection,
                "HTTP/1.1 500 Internal Server Error\r\nContent-Length: 22\r\n\r\nInternal Server "
                "Error\n",
                80);
            fprintf(stderr, "%s,%s,%d,%d\n", rq->method, rq->URI, 500, rq->request_id);
            return 1;
        }
    }
    // if file exists
    else if (errno == EEXIST) {
        fd = open(rq->URI + 1, O_WRONLY | O_TRUNC, 0666);
        // write response to client
        write_n_bytes(rq->connection, "HTTP/1.1 200 OK\r\nContent-Length: 3\r\n\r\nOK\n", 41);
        fprintf(stderr, "%s,%s,%d,%d\n", rq->method, rq->URI, 200, rq->request_id);
    }
    // if file was created
    else {
        write_n_bytes(
            rq->connection, "HTTP/1.1 201 Created\r\nContent-Length: 8\r\n\r\nCreated\n", 51);
        fprintf(stderr, "%s,%s,%d,%d\n", rq->method, rq->URI, 201, rq->request_id);
    }
    // print out message body to file
    write_n_bytes(fd, rq->message_body, rq->bytes_left);
    // prints out remaining message_body that wasn't previously read yet
    pass_n_bytes(rq->connection, fd, rq->content_length - rq->bytes_left);
    // close file
    close(fd);
    return 0;
}

// this function will perform the request. Returns 1 for success, 0 otherwise
int run_request(Request *rq) {
    // check version
    if (strcmp(rq->version, "HTTP/1.1")) {
        write_n_bytes(rq->connection,
            "HTTP/1.1 505 Version Not Supported\r\nContent-Length: 22\r\n\r\nVersion Not "
            "Supported\n",
            80);
        fprintf(stderr, "%s,%s,%d,%d\n", rq->method, rq->URI, 505, rq->request_id);
        return 1;
    }
    // deterine get or put
    if (strcmp(rq->method, "GET") == 0) {
        get(rq);
    } else if (strcmp(rq->method, "PUT") == 0) {
        put(rq);
    } else {
        write_n_bytes(rq->connection,
            "HTTP/1.1 501 Not Implemented\r\nContent-Length: 16\r\n\r\nNot Implemented\n", 68);
        fprintf(stderr, "%s,%s,%d,%d\n", rq->method, rq->URI, 501, rq->request_id);
        return 1;
    }
    return 0;
}

// returns the lock for the specific uri
// if there is no lock for that uri, will replace a uri no being used
locknode *obtainlock(locknode **la, Request *rq, int num_threads) {
    for (int i = 0; i < num_threads; i++) {
        if (strcmp(la[i]->file_name, rq->URI) == 0) {
            return la[i];
        }
    }
    for (int i = 0; i < num_threads; i++) {
        if (la[i]->num_users == 0) {
            la[i]->file_name = rq->URI;
            return la[i];
        }
    }
    return NULL;
}
