#include "queue.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>

typedef struct queue {
    void **buffer;
    int size;
    int push;
    int pop;
    pthread_mutex_t mutex;
    pthread_cond_t not_empty;
    pthread_cond_t not_full;
} queue_t;

queue_t *queue_new(int size) {
    queue_t *q = (queue_t *) malloc(sizeof(queue_t));
    if (q == NULL) {
        return NULL;
    }

    q->buffer = (void **) malloc(size * sizeof(void *));
    if (q->buffer == NULL) {
        free(q);
        return NULL;
    }

    q->size = size;
    q->push = 0;
    q->pop = 0;
    pthread_mutex_init(&q->mutex, NULL);
    pthread_cond_init(&q->not_empty, NULL);
    pthread_cond_init(&q->not_full, NULL);
    return q;
}

void queue_delete(queue_t **q) {
    if (q == NULL || *q == NULL) {
        return;
    }
    queue_t *queue = *q;
    pthread_mutex_destroy(&queue->mutex);
    pthread_cond_destroy(&queue->not_empty);
    pthread_cond_destroy(&queue->not_full);

    free(queue->buffer);
    queue->buffer = NULL;
    free(queue);
    *q = NULL;
}

bool queue_push(queue_t *q, void *elem) {
    if (q == NULL) {
        return false;
    }

    pthread_mutex_lock(&q->mutex);

    while ((q->push + 1) % q->size == q->pop) {
        pthread_cond_wait(&q->not_full, &q->mutex);
    }

    q->buffer[q->push] = elem;
    q->push = (q->push + 1) % q->size;

    pthread_cond_signal(&q->not_empty);
    pthread_mutex_unlock(&q->mutex);

    return true;
}

bool queue_pop(queue_t *q, void **elem) {
    if (q == NULL) {
        return false;
    }

    pthread_mutex_lock(&q->mutex);

    while (q->push == q->pop) {
        pthread_cond_wait(&q->not_empty, &q->mutex);
    }

    *elem = q->buffer[q->pop];
    q->pop = (q->pop + 1) % q->size;

    pthread_cond_signal(&q->not_full);
    pthread_mutex_unlock(&q->mutex);

    return true;
}
