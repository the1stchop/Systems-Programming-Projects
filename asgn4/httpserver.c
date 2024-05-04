#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>

#include "queue.h"
#include "rwlock.h"
#include "httpfuncs.h"
#include "asgn2_helper_funcs.h"

#define OPTIONS "t:"

// initialize queue
queue_t *queue;
// lock array
locknode **locks;
// number of threads
int num_threads;

// mutext lock for array
pthread_mutex_t arrlock;

void *worker_thread();

int main(int argc, char **argv) {
    // default settings
    num_threads = 4;
    int port = 0;
    // checking for thread input
    if (getopt(argc, argv, OPTIONS) != -1) {
        num_threads = atoi(optarg);
    }
    // checking for port input
    if ((port = atoi(argv[optind])) == -1) {
        fprintf(stderr, "shits wrong try again.\n");
    }

    queue = queue_new(num_threads);

    // making list of locknodes
    locks = (locknode **) malloc(sizeof(locknode *) * num_threads);
    for (int i = 0; i < num_threads; i++) {
        locks[i] = (locknode *) malloc(sizeof(locknode));
        locks[i]->lock = rwlock_new(N_WAY, 1);
        locks[i]->file_name = "";
        locks[i]->num_users = 0;
    }

    // initializing lock array
    pthread_mutex_init(&arrlock, NULL);

    // creating threads
    pthread_t thread[num_threads];
    for (int i = 0; i < num_threads; i++) {
        pthread_create(&thread[i], NULL, worker_thread, NULL);
    }

    // initialize sock
    Listener_Socket sock;

    // initialize port
    listener_init(&sock, port);

    while (1) {
        // shit youre gonne be reading from
        uintptr_t connection_fd = listener_accept(&sock);

        // push connection
        queue_push(queue, (void *) connection_fd);
    }
}

void *worker_thread() {
    char cmd_buff[2049] = "";
    while (1) {
        // grabbing connection from queue
        uintptr_t connection;
        queue_pop(queue, (void **) &connection);

        // create Request struct
        Request rq;
        rq.connection = connection;

        // read request to buffer and remembers how many bytes read
        rq.bytes_read = read_until(connection, cmd_buff, 2048, "\r\n\r\n");

        // call parse_request
        if (parse_request(cmd_buff, &rq)) {
            fprintf(stderr, "something wrong with parse_request.\n");
            close(connection);
        } else {
            // check lock array for file
            pthread_mutex_lock(&(arrlock));
            locknode *ln = obtainlock(locks, &rq, num_threads);
            pthread_mutex_unlock(&(arrlock));
            if (strcmp(rq.method, "GET")) {
                reader_lock(ln->lock);
                run_request(&rq);
                reader_unlock(ln->lock);
            } else if (strcmp(rq.method, "PUT")) {
                writer_lock(ln->lock);
                run_request(&rq);
                writer_unlock(ln->lock);
            }
        }
        //close connection
        close(connection);
    }
}
