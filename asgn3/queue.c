#include "queue.h"
#include <pthread.h>
#include <stdlib.h>

typedef struct queue {
    void **buffer;
    int size;
    int in;
    int out;
    int count;
    pthread_mutex_t lock;
    pthread_cond_t empty;
    pthread_cond_t full;
} queue;

queue_t *queue_new(int size) {
    // allocating memory for struct
    queue_t *q = malloc(sizeof(queue_t));
    // allocating memory for buffer
    q->buffer = malloc(sizeof(void *) * size);
    // default values for struct
    q->size = size;
    q->in = 0;
    q->out = 0;
    q->count = 0;
    pthread_mutex_init(&(q->lock), NULL);
    pthread_cond_init(&(q->empty), NULL);
    pthread_cond_init(&(q->full), NULL);

    return q;
}

void queue_delete(queue_t **q) {
    //free buffer
    free((*q)->buffer);

    // destroy mutex and CVs
    pthread_mutex_destroy(&((*q)->lock));
    pthread_cond_destroy(&((*q)->empty));
    pthread_cond_destroy(&((*q)->full));

    // free queue
    free(*q);
    *q = NULL;
}

bool queue_push(queue_t *q, void *elem) {
    pthread_mutex_lock(&(q->lock));
    while (q->count == q->size) {
        pthread_cond_wait(&(q->full), &(q->lock));
    }
    q->buffer[q->in] = elem;
    q->in = (q->in + 1) % q->size;

    q->count += 1;
    pthread_cond_signal(&(q->empty));
    pthread_mutex_unlock(&(q->lock));

    return (1);
}

bool queue_pop(queue_t *q, void **elem) {
    pthread_mutex_lock(&(q->lock));
    while (q->count == 0) {
        pthread_cond_wait(&(q->empty), &(q->lock));
    }
    *elem = q->buffer[q->out];
    q->out = (q->out + 1) % q->size;

    q->count -= 1;
    pthread_cond_signal(&(q->full));
    pthread_mutex_unlock(&(q->lock));

    return (1);
}
