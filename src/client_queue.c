#include "client_queue.h"
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

client_queue_t* client_queue_init(int capacity) {
    client_queue_t* queue = malloc(sizeof(client_queue_t));
    if (!queue) {
        return NULL;
    }
    
    queue->sockets = malloc(sizeof(int) * capacity);
    if (!queue->sockets) {
        free(queue);
        return NULL;
    }
    
    queue->capacity = capacity;
    queue->count = 0;
    queue->front = 0;
    queue->rear = 0;
    
    if (pthread_mutex_init(&queue->mutex, NULL) != 0) {
        free(queue->sockets);
        free(queue);
        return NULL;
    }
    
    if (pthread_cond_init(&queue->cond, NULL) != 0) {
        pthread_mutex_destroy(&queue->mutex);
        free(queue->sockets);
        free(queue);
        return NULL;
    }
    
    return queue;
}

void client_queue_destroy(client_queue_t* queue) {
    if (!queue) return;
    
    pthread_mutex_destroy(&queue->mutex);
    pthread_cond_destroy(&queue->cond);
    free(queue->sockets);
    free(queue);
}

int client_queue_enqueue(client_queue_t* queue, int socket) {
    if (!queue) return -1;
    
    pthread_mutex_lock(&queue->mutex);
    
    if (queue->count >= queue->capacity) {
        pthread_mutex_unlock(&queue->mutex);
        return -1; // Queue full
    }
    
    queue->sockets[queue->rear] = socket;
    queue->rear = (queue->rear + 1) % queue->capacity;
    queue->count++;
    
    pthread_cond_signal(&queue->cond);
    pthread_mutex_unlock(&queue->mutex);
    
    return 0;
}

int client_queue_dequeue(client_queue_t* queue) {
    if (!queue) return -1;
    
    pthread_mutex_lock(&queue->mutex);
    
    // Wait for a client to be available (with timeout)
    struct timespec timeout;
    clock_gettime(CLOCK_REALTIME, &timeout);
    timeout.tv_sec += 1; // 1 second timeout
    
    while (queue->count == 0) {
        int result = pthread_cond_timedwait(&queue->cond, &queue->mutex, &timeout);
        if (result == ETIMEDOUT) {
            pthread_mutex_unlock(&queue->mutex);
            return -1; // Timeout
        }
        if (result != 0) {
            pthread_mutex_unlock(&queue->mutex);
            return -1; // Error
        }
    }
    
    int socket = queue->sockets[queue->front];
    queue->front = (queue->front + 1) % queue->capacity;
    queue->count--;
    
    pthread_mutex_unlock(&queue->mutex);
    
    return socket;
}

int client_queue_is_empty(client_queue_t* queue) {
    if (!queue) return 1;
    
    pthread_mutex_lock(&queue->mutex);
    int empty = (queue->count == 0);
    pthread_mutex_unlock(&queue->mutex);
    
    return empty;
}

int client_queue_is_full(client_queue_t* queue) {
    if (!queue) return 1;
    
    pthread_mutex_lock(&queue->mutex);
    int full = (queue->count >= queue->capacity);
    pthread_mutex_unlock(&queue->mutex);
    
    return full;
}
