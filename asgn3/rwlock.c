#include "rwlock.h"
#include <pthread.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

typedef struct rwlock {
    pthread_mutex_t lock;
    pthread_cond_t read;
    pthread_cond_t write;
    PRIORITY prio;
    int n_way;
    int waiting_readers;
    int waiting_writers;
    int active_readers;
    int active_writers;
    int read_count;
} rwlock;

rwlock_t *rwlock_new(PRIORITY p, uint32_t n) {
    // allocate memory for struct
    rwlock_t *rw = malloc(sizeof(rwlock_t));
    // put values into struct
    rw->prio = p;
    rw->n_way = n;
    rw->waiting_readers = 0;
    rw->waiting_writers = 0;
    rw->active_readers = 0;
    rw->active_writers = 0;
    rw->read_count = 0;
    // initilize mutex and CVs
    pthread_mutex_init(&(rw->lock), NULL);
    pthread_cond_init(&(rw->read), NULL);
    pthread_cond_init(&(rw->write), NULL);
    return rw;
}

void rwlock_delete(rwlock_t **rw) {
    // destroy mutex and CVs
    pthread_mutex_destroy(&((*rw)->lock));
    pthread_cond_destroy(&((*rw)->read));
    pthread_cond_destroy(&((*rw)->write));
    // free struct
    free(*rw);
    // set pointer to NULL
    *rw = NULL;
}

void reader_lock(rwlock_t *rw) {
    pthread_mutex_lock(&(rw->lock));
    // increment waiting reads
    rw->waiting_readers++;
    if (rw->prio == READERS) {
        // if prio is readers, should only wait if there are active writers
        while (rw->active_writers > 0) {
            pthread_cond_wait(&(rw->read), &(rw->lock));
        }
    } else if (rw->prio == WRITERS) {
        // if prio is writers, should only wait if there are waiting writers
        while (rw->waiting_writers > 0) {
            pthread_cond_wait(&(rw->read), &(rw->lock));
        }
    } else {
        // if prio is n-way, should wait if n-way reads have happened and there are waiting writers
        // or if there are active_writers
        while ((rw->read_count >= rw->n_way && rw->waiting_writers > 0) || rw->active_writers > 0) {
            pthread_cond_wait(&(rw->read), &(rw->lock));
        }
    }
    // decrement waiting reads and increment active reads
    // also increment read count which keeps track of how many reads have happened since last write (for n-way)
    rw->active_readers++;
    rw->read_count++;
    rw->waiting_readers--;
    pthread_mutex_unlock(&(rw->lock));
}

void reader_unlock(rwlock_t *rw) {
    pthread_mutex_lock(&(rw->lock));
    // decrement active readers
    rw->active_readers--;
    if (rw->active_readers == 0) {
        // if no active readers and there are waiting writers, broadcast to waiting writes
        if (rw->waiting_writers > 0) {
            pthread_cond_broadcast(&(rw->write));
            // if no active readers and no waiting writers, just broadcast to the reads
        } else {
            pthread_cond_broadcast(&(rw->read));
        }
    }
    pthread_mutex_unlock(&(rw->lock));
}

void writer_lock(rwlock_t *rw) {
    pthread_mutex_lock(&(rw->lock));
    // increment waiting writers
    rw->waiting_writers++;
    if (rw->prio == READERS) {
        // wait if there are waiting readers or if theres active writers
        while (rw->waiting_readers > 0 || rw->active_writers > 0) {
            pthread_cond_wait(&(rw->write), &(rw->lock));
        }
    } else if (rw->prio == WRITERS) {
        // wait if there are active writers
        while (rw->active_writers > 0) {
            pthread_cond_wait(&(rw->write), &(rw->lock));
        }
    } else {
        // wait if there are active writers or if read count is less than n-way and there are waiting reader
        while (rw->active_readers > 0 || (rw->read_count < rw->n_way && rw->waiting_readers > 0)
               || rw->active_writers > 0) {
            pthread_cond_wait(&(rw->write), &(rw->lock));
        }
    }
    // increment active writers and decrement waiting writers
    // also reset read count to zero after write has been called
    rw->active_writers++;
    rw->waiting_writers--;
    rw->read_count = 0;
    pthread_mutex_unlock(&(rw->lock));
}

void writer_unlock(rwlock_t *rw) {
    pthread_mutex_lock(&(rw->lock));
    // decrement active writers
    rw->active_writers--;
    assert(rw->active_writers == 0);
    // should broadcast to writers if there is a writers prio or if there are no waiting readers and there are waiting writers
    if ((rw->prio == WRITERS && rw->waiting_writers > 0)
        || (rw->waiting_readers == 0 && rw->waiting_writers > 0)) {
        pthread_cond_broadcast(&(rw->write));
        // if neither of those conditions, just broadcast to read, cuz threads shouldnt wait for something that aint there
    } else {
        pthread_cond_broadcast(&(rw->read));
    }
    pthread_mutex_unlock(&(rw->lock));
}
