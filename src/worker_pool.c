#include "worker_pool.h"
#include "locking.h"  
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <dirent.h>   
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
        
        printf("Worker processing task: type=%d, user=%s, file=%s\n", 
               task->type, task->username, task->filename);
        
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

// Task processing function
void process_task(task_t* task) {
    printf("Worker processing task for user '%s': type=%d, file=%s\n", 
           task->username, task->type, task->filename);
    
    // ACQUIRE USER LOCK
    lock_user(task->username);
    
    // Operate within user's files directory
    char files_dir[1024] = {0};
    snprintf(files_dir, sizeof(files_dir), "%s/files", task->user_dir);
    
    switch (task->type) {
        case CMD_UPLOAD: {
            // Create a placeholder file with simple content
            char path[1400];
            snprintf(path, sizeof(path), "%s/%s", files_dir, task->filename);
            FILE *fp = fopen(path, "w");
            if (!fp) {
                snprintf(task->result, sizeof(task->result), 
                        "UPLOAD_ERROR: Could not create file\n");
                break;
            }
            fprintf(fp, "Uploaded by %s\nProcessed by worker thread\n", task->username);
            fclose(fp);
            snprintf(task->result, sizeof(task->result), 
                    "UPLOAD_SUCCESS: File '%s' uploaded successfully\n", task->filename);
            break;
        }
        case CMD_DOWNLOAD: {
            char path[1400];
            snprintf(path, sizeof(path), "%s/%s", files_dir, task->filename);
            FILE *fp = fopen(path, "r");
            if (!fp) {
                snprintf(task->result, sizeof(task->result), 
                        "DOWNLOAD_ERROR: File '%s' not found\n", task->filename);
                break;
            }
            fclose(fp);
            snprintf(task->result, sizeof(task->result), 
                    "DOWNLOAD_SUCCESS: File '%s' ready for download\n", task->filename);
            break;
        }
        case CMD_DELETE: {
            char path[1400];
            snprintf(path, sizeof(path), "%s/%s", files_dir, task->filename);
            if (unlink(path) == 0) {
                snprintf(task->result, sizeof(task->result), 
                        "DELETE_SUCCESS: File '%s' deleted successfully\n", task->filename);
            } else {
                snprintf(task->result, sizeof(task->result), 
                        "DELETE_ERROR: File '%s' not found or cannot delete\n", task->filename);
            }
            break;
        }
        case CMD_LIST: {
            DIR *d = opendir(files_dir);
            if (!d) {
                snprintf(task->result, sizeof(task->result), 
                        "LIST_ERROR: Cannot open user files directory\n");
                break;
            }
            struct dirent *de;
            char buffer[4096];
            size_t offset = 0;
            const char *header = "LIST_SUCCESS:";
            offset = snprintf(buffer, sizeof(buffer), "%s", header);
            int count = 0;
            while ((de = readdir(d)) != NULL) {
                if (strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0) continue;
                size_t need = strlen(de->d_name) + 2;
                if (offset + need + 1 >= sizeof(buffer)) {
                    break;
                }
                offset += snprintf(buffer + offset, sizeof(buffer) - offset, " %s", de->d_name);
                count++;
            }
            closedir(d);
            if (count == 0) {
                snprintf(task->result, sizeof(task->result), "LIST_SUCCESS: No files found\n");
            } else {
                if (offset + 1 < sizeof(buffer)) {
                    buffer[offset++] = '\n';
                    buffer[offset] = '\0';
                }
                strncpy(task->result, buffer, sizeof(task->result) - 1);
            }
            break;
        }
        default:
            snprintf(task->result, sizeof(task->result), "ERROR: Unknown command type %d\n", task->type);
            break;
    }
    
    // RELEASE USER LOCK
    unlock_user(task->username);
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