#ifndef TASK_QUEUE_H
#define TASK_QUEUE_H

#include "command_parser.h"
#include <pthread.h>

// Task queue structure
typedef struct {
    task_t **tasks;
    int capacity;
    int count;
    int front;
    int rear;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    int shutdown;
} task_queue_t;

// Function prototypes
task_queue_t* task_queue_init(int capacity);
void task_queue_destroy(task_queue_t* queue);
int task_queue_enqueue(task_queue_t* queue, task_t* task);
task_t* task_queue_dequeue(task_queue_t* queue);
int task_queue_is_empty(task_queue_t* queue);
int task_queue_is_full(task_queue_t* queue);
void task_queue_shutdown(task_queue_t* queue);

#endif