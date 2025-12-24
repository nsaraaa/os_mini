#ifndef WORKER_POOL_H
#define WORKER_POOL_H

#include "task_queue.h"

typedef struct {
    pthread_t *threads;
    int num_threads;
    task_queue_t *task_queue;
    int shutdown;
} worker_pool_t;

worker_pool_t* worker_pool_init(int num_threads, task_queue_t *task_queue);
void worker_pool_shutdown(worker_pool_t* pool);
void worker_pool_destroy(worker_pool_t* pool);

#endif