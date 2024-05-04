#include "asgn2_helper_funcs.h"
#include "httpfuncs.h"
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <regex.h>
#include <errno.h>

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Invalid Port\n");
        exit(1);
    }
    int port = atoi(argv[1]);

    // initialize sock
    Listener_Socket sock;

    // initialize port
    listener_init(&sock, port);

    // command buffer
    char cmd_buff[2049] = "";

    while (1) {
        // shit youre gonne be reading from
        int connection_fd = listener_accept(&sock);
        if (connection_fd > 0) {
            // create Request struct
            Request rq;
            rq.connection = connection_fd;

            // read request to buffer and remembers how many bytes read
            rq.bytes_read = read_until(connection_fd, cmd_buff, 2048, "\r\n\r\n");

            // call parse_request
            if (parse_request(cmd_buff, &rq)) {
                fprintf(stderr, "something wrong with parse_request.\n");
                close(connection_fd);
            } else {
                if (run_request(&rq)) {
                    fprintf(stderr, "something wrong with run_request.\n");
                    close(connection_fd);
                }
            }
            //close connection
            close(connection_fd);
        }
    }
}
