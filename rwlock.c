#include "rwlock.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <semaphore.h>

typedef struct rwlock {
    pthread_mutex_t lock;
    pthread_cond_t readers_done;
    pthread_cond_t writer_done;
    int readers;
    int writers;
    int readers_waiting;
    int writers_waiting;
    int done_readers;
    int n;
    PRIORITY priority;
} rwlock_t;

rwlock_t *rwlock_new(PRIORITY p, uint32_t n) {
    rwlock_t *rwlock = (rwlock_t *) malloc(sizeof(rwlock_t));
    if (rwlock == NULL) {
        exit(1);
    }
    if (p == N_WAY && n <= 0) {
        return NULL;
    }

    pthread_mutex_init(&rwlock->lock, NULL);
    pthread_cond_init(&rwlock->readers_done, NULL);
    pthread_cond_init(&rwlock->writer_done, NULL);
    rwlock->readers = 0;
    rwlock->writers = 0;
    rwlock->readers_waiting = 0;
    rwlock->writers_waiting = 0;
    rwlock->done_readers = 0;
    rwlock->n = n;
    rwlock->priority = p;

    return rwlock;
}

void rwlock_delete(rwlock_t **l) {
    if (l == NULL || *l == NULL) {
        return;
    }

    rwlock_t *rwlock = *l;
    pthread_mutex_lock(&rwlock->lock);
    pthread_mutex_unlock(&rwlock->lock);
    pthread_mutex_destroy(&rwlock->lock);
    pthread_cond_destroy(&rwlock->readers_done);
    pthread_cond_destroy(&rwlock->writer_done);
    free(rwlock);
    *l = NULL;
}

void reader_lock(rwlock_t *rw) {
    pthread_mutex_lock(&rw->lock);
    rw->readers_waiting++;
    // Check for Writer Non-Starvation
    while (rw->writers > 0 || (rw->priority == WRITERS && rw->writers_waiting > 0)
           || (rw->priority == N_WAY && (rw->writers_waiting > 0 && rw->done_readers >= rw->n))
           || (rw->priority == N_WAY && rw->writers > 0 && rw->n == 1)) {
        pthread_cond_wait(&rw->readers_done, &rw->lock);
    }

    rw->readers_waiting--;
    rw->readers++;
    rw->done_readers++;
    pthread_mutex_unlock(&rw->lock);
}

void reader_unlock(rwlock_t *rw) {
    pthread_mutex_lock(&rw->lock);
    rw->readers--;

    if (rw->priority == N_WAY) {
        if (rw->writers_waiting > 0) {
            if (rw->done_readers >= rw->n && rw->readers == 0) {
                pthread_cond_signal(&rw->writer_done);
            } else if (rw->readers_waiting > 0) {
                pthread_cond_signal(&rw->readers_done);
            } else if (rw->readers == 0) {
                pthread_cond_signal(&rw->writer_done);
            }
        } else {
            pthread_cond_broadcast(&rw->readers_done);
        }
    } else if (rw->priority == WRITERS && rw->writers_waiting == 0) {
        pthread_cond_broadcast(&rw->readers_done);
    } else if (rw->priority == WRITERS && rw->readers == 0) {
        pthread_cond_signal(&rw->writer_done);
    } else if (rw->priority == READERS && rw->readers_waiting > 0) {
        pthread_cond_broadcast(&rw->readers_done);
    } else if (rw->priority == READERS && rw->readers == 0) {
        pthread_cond_signal(&rw->writer_done);
    }

    pthread_mutex_unlock(&rw->lock);
}

void writer_lock(rwlock_t *rw) {
    pthread_mutex_lock(&rw->lock);
    rw->writers_waiting++;

    while (rw->readers > 0 || rw->writers > 0
           || (rw->priority == READERS && rw->readers_waiting > 0)
           || (rw->priority == N_WAY && (rw->readers_waiting > 0 && rw->done_readers < rw->n))) {
        pthread_cond_wait(&rw->writer_done, &rw->lock);
    }

    rw->writers_waiting--;
    rw->writers = 1;
    rw->done_readers = 0;

    pthread_mutex_unlock(&rw->lock);
}

void writer_unlock(rwlock_t *rw) {
    pthread_mutex_lock(&rw->lock);
    rw->writers = 0;

    if (rw->priority == N_WAY) {
        if (rw->writers_waiting > 0) {
            if (rw->done_readers >= rw->n && rw->readers == 0) {
                pthread_cond_signal(&rw->writer_done);
            } else if (rw->readers_waiting > 0) {
                pthread_cond_signal(&rw->readers_done);
            } else if (rw->readers == 0) {
                pthread_cond_signal(&rw->writer_done);
            }
        } else {
            pthread_cond_broadcast(&rw->readers_done);
        }
    } else if (rw->priority == WRITERS && rw->writers_waiting == 0) {
        pthread_cond_broadcast(&rw->readers_done);
    } else if (rw->priority == WRITERS && rw->readers == 0) {
        pthread_cond_signal(&rw->writer_done);
    } else if (rw->priority == READERS && rw->readers_waiting > 0) {
        pthread_cond_broadcast(&rw->readers_done);
    } else if (rw->priority == READERS && rw->readers == 0) {
        pthread_cond_signal(&rw->writer_done);
    }

    pthread_mutex_unlock(&rw->lock);
}
