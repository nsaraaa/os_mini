#include "worker_pool.h"
#include "locking.h"
#include "module3_integration.h"
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>

// Forward declaration
void process_task(task_t* task);

void* worker_thread_func(void* arg) {
    worker_pool_t* pool = (worker_pool_t*)arg;
    
    printf("Worker thread started\n");
    
    while (!pool->shutdown) {
        task_t* task = task_queue_dequeue(pool->task_queue);
        
        if (!task) {
            break; // Shutdown signal
        }
        
        printf("Worker processing task: type=%d, user=%s, file=%s, data_size=%zu\n", 
               task->type, task->username, task->filename, task->file_size);
        
        // Process the task
        process_task(task);
        
        // Signal the client thread that result is ready
        pthread_mutex_lock(&task->result_mutex);
        task->result_complete = 1;
        pthread_cond_signal(&task->result_ready);
        pthread_mutex_unlock(&task->result_mutex);
    }
    
    printf("Worker thread exiting\n");
    return NULL;
}

// Task processing function - NOW USES MODULE 3
void process_task(task_t* task) {
    printf("Worker processing task for user '%s': type=%d, file=%s\n", 
           task->username, task->type, task->filename);
    
    // ACQUIRE USER LOCK (from Module 2)
    lock_user(task->username);
    
    // Process using Module 3 functions
    int result = 0;
    
    switch (task->type) {
        case CMD_UPLOAD:
            result = module3_process_upload(task);
            break;
            
        case CMD_DOWNLOAD:
            result = module3_process_download(task);
            break;
            
        case CMD_DELETE:
            result = module3_process_delete(task);
            break;
            
        case CMD_LIST:
            result = module3_process_list(task);
            break;
            
        default:
            snprintf(task->result, sizeof(task->result), 
                    "ERROR: Unknown command type %d\n", task->type);
            result = -1;
            break;
    }
    
    // RELEASE USER LOCK (from Module 2)
    unlock_user(task->username);
    
    if (result != 0) {
        printf("Task processing failed for user '%s'\n", task->username);
    }
}

worker_pool_t* worker_pool_init(int num_threads, task_queue_t *task_queue) {
    worker_pool_t* pool = malloc(sizeof(worker_pool_t));
    if (!pool) return NULL;
    
    pool->threads = malloc(sizeof(pthread_t) * num_threads);
    if (!pool->threads) {
        free(pool);
        return NULL;
    }
    
    pool->num_threads = num_threads;
    pool->task_queue = task_queue;
    pool->shutdown = 0;
    
    for (int i = 0; i < num_threads; i++) {
        if (pthread_create(&pool->threads[i], NULL, worker_thread_func, pool) != 0) {
            // Cleanup on failure
            for (int j = 0; j < i; j++) {
                pthread_cancel(pool->threads[j]);
            }
            free(pool->threads);
            free(pool);
            return NULL;
        }
    }
    
    printf("Worker pool created with %d threads\n", num_threads);
    return pool;
}

void worker_pool_shutdown(worker_pool_t* pool) {
    if (!pool) return;
    
    pool->shutdown = 1;
    task_queue_shutdown(pool->task_queue);
    
    for (int i = 0; i < pool->num_threads; i++) {
        pthread_join(pool->threads[i], NULL);
    }
}

void worker_pool_destroy(worker_pool_t* pool) {
    if (!pool) return;
    
    free(pool->threads);
    free(pool);
}