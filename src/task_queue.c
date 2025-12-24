#include "task_queue.h"
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

task_queue_t* task_queue_init(int capacity) {
    task_queue_t* queue = malloc(sizeof(task_queue_t));
    if (!queue) return NULL;
    
    queue->tasks = malloc(sizeof(task_t*) * capacity);
    if (!queue->tasks) {
        free(queue);
        return NULL;
    }
    
    queue->capacity = capacity;
    queue->count = 0;
    queue->front = 0;
    queue->rear = 0;
    queue->shutdown = 0;
    
    if (pthread_mutex_init(&queue->mutex, NULL) != 0) {
        free(queue->tasks);
        free(queue);
        return NULL;
    }
    
    if (pthread_cond_init(&queue->cond, NULL) != 0) {
        pthread_mutex_destroy(&queue->mutex);
        free(queue->tasks);
        free(queue);
        return NULL;
    }
    
    return queue;
}

void task_queue_destroy(task_queue_t* queue) {
    if (!queue) return;
    
    pthread_mutex_destroy(&queue->mutex);
    pthread_cond_destroy(&queue->cond);
    free(queue->tasks);
    free(queue);
}

int task_queue_enqueue(task_queue_t* queue, task_t* task) {
    if (!queue || !task) return -1;
    
    pthread_mutex_lock(&queue->mutex);
    
    if (queue->shutdown) {
        pthread_mutex_unlock(&queue->mutex);
        return -1;
    }
    
    if (queue->count >= queue->capacity) {
        pthread_mutex_unlock(&queue->mutex);
        return -1; // Queue full
    }
    
    queue->tasks[queue->rear] = task;
    queue->rear = (queue->rear + 1) % queue->capacity;
    queue->count++;
    
    pthread_cond_signal(&queue->cond);
    pthread_mutex_unlock(&queue->mutex);
    
    printf("Task enqueued: type=%d, user=%s, file=%s\n", 
           task->type, task->username, task->filename);
    return 0;
}

task_t* task_queue_dequeue(task_queue_t* queue) {
    if (!queue) return NULL;
    
    pthread_mutex_lock(&queue->mutex);
    
    while (queue->count == 0 && !queue->shutdown) {
        pthread_cond_wait(&queue->cond, &queue->mutex);
    }
    
    if (queue->shutdown && queue->count == 0) {
        pthread_mutex_unlock(&queue->mutex);
        return NULL;
    }
    
    task_t* task = queue->tasks[queue->front];
    queue->front = (queue->front + 1) % queue->capacity;
    queue->count--;
    
    pthread_mutex_unlock(&queue->mutex);
    
    return task;
}

void task_queue_shutdown(task_queue_t* queue) {
    if (!queue) return;
    
    pthread_mutex_lock(&queue->mutex);
    queue->shutdown = 1;
    pthread_cond_broadcast(&queue->cond);
    pthread_mutex_unlock(&queue->mutex);
}