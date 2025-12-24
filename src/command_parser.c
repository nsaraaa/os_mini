#include "command_parser.h"
#include "task_queue.h"
#include "worker_pool.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <sys/socket.h>
#include <dirent.h>

// Global task system (Phase 2)
static task_queue_t *global_task_queue = NULL;
static worker_pool_t *global_worker_pool = NULL;

// Initialize task system (called from main)
void init_task_system(void) {
    global_task_queue = task_queue_init(1000);
    global_worker_pool = worker_pool_init(4, global_task_queue); // 4 worker threads
    printf("Task system initialized\n");
}

// Shutdown task system (called from main)
void shutdown_task_system(void) {
    if (global_worker_pool) {
        worker_pool_shutdown(global_worker_pool);
        worker_pool_destroy(global_worker_pool);
        global_worker_pool = NULL;
    }
    if (global_task_queue) {
        task_queue_destroy(global_task_queue);
        global_task_queue = NULL;
    }
    printf("Task system shutdown\n");
}


command_type_t parse_command(const char* input) {
    if (!input) return CMD_UNKNOWN;
    
    // Skip leading whitespace
    while (*input == ' ' || *input == '\t') input++;
    
    if (strncasecmp(input, "LOGIN", 5) == 0) return CMD_LOGIN;
    if (strncasecmp(input, "SIGNUP", 6) == 0) return CMD_SIGNUP;
    if (strncasecmp(input, "LOGOUT", 6) == 0) return CMD_LOGOUT;
    if (strncasecmp(input, "UPLOAD", 6) == 0) return CMD_UPLOAD;
    if (strncasecmp(input, "DOWNLOAD", 8) == 0) return CMD_DOWNLOAD;
    if (strncasecmp(input, "DELETE", 6) == 0) return CMD_DELETE;
    if (strncasecmp(input, "LIST", 4) == 0) return CMD_LIST;
    if (strncasecmp(input, "QUIT", 4) == 0) return CMD_QUIT;
    
    return CMD_UNKNOWN;
}

task_t* create_task(command_type_t type, const char* filename, user_session_t* session, int client_socket) {
    task_t* task = malloc(sizeof(task_t));
    if (!task) {
        return NULL;
    }
    
    memset(task, 0, sizeof(task_t));
    task->type = type;
    task->client_socket = client_socket;
    
    if (filename) {
        strncpy(task->filename, filename, sizeof(task->filename) - 1);
    }
    
    if (session && session->authenticated) {
        strncpy(task->username, session->username, sizeof(task->username) - 1);
        strncpy(task->user_dir, session->user_dir, sizeof(task->user_dir) - 1);
        task->authenticated = 1;
    }
    
    return task;
}

int send_response(int client_socket, const char* response) {
    if (!response) return -1;
    
    size_t len = strlen(response);
    ssize_t sent = send(client_socket, response, len, 0);
    
    if (sent != (ssize_t)len) {
        perror("send");
        return -1;
    }
    
    return 0;
}

int handle_authenticated_command(const char* command, user_session_t* session, int client_socket) {
    if (!command || !session || !session->authenticated) {
        return -1;
    }
    
    command_type_t cmd_type = parse_command(command);
    char filename[256] = {0};
    
    // Parse filename for file operations
    switch (cmd_type) {
        case CMD_UPLOAD:
            if (sscanf(command, "UPLOAD %255s", filename) != 1) {
                send_response(client_socket, "UPLOAD_ERROR: Usage: UPLOAD filename\n");
                return -1;
            }
            break;
            
        case CMD_DOWNLOAD:
            if (sscanf(command, "DOWNLOAD %255s", filename) != 1) {
                send_response(client_socket, "DOWNLOAD_ERROR: Usage: DOWNLOAD filename\n");
                return -1;
            }
            break;
            
        case CMD_DELETE:
            if (sscanf(command, "DELETE %255s", filename) != 1) {
                send_response(client_socket, "DELETE_ERROR: Usage: DELETE filename\n");
                return -1;
            }
            break;
            
        case CMD_LIST:
            // No filename needed for LIST
            break;
            
        default:
            return -1;
    }
    
    // Create task for worker
    task_t* task = create_task(cmd_type, filename, session, client_socket);
    if (!task) {
        send_response(client_socket, "ERROR: Failed to create task\n");
        return -1;
    }
    
    // Initialize task synchronization primitives
    pthread_mutex_init(&task->result_mutex, NULL);
    pthread_cond_init(&task->result_ready, NULL);
    task->result_complete = 0;
    task->task_id = rand(); // Simple ID generation
    
    // Send initial response
    send_response(client_socket, "TASK_QUEUED: Task submitted to worker pool\n");
    
    // Push task to worker queue
    if (push_task_to_queue(task) != 0) {
        send_response(client_socket, "ERROR: Failed to submit task\n");
        pthread_mutex_destroy(&task->result_mutex);
        pthread_cond_destroy(&task->result_ready);
        free(task);
        return -1;
    }
    
    // Wait for worker to complete (Phase 2 communication)
    pthread_mutex_lock(&task->result_mutex);
    while (!task->result_complete) {
        pthread_cond_wait(&task->result_ready, &task->result_mutex);
    }
    pthread_mutex_unlock(&task->result_mutex);
    
    // Send result back to client
    send_response(client_socket, task->result);
    
    // Cleanup task
    pthread_mutex_destroy(&task->result_mutex);
    pthread_cond_destroy(&task->result_ready);
    free(task);
    
    return 0;
}

int push_task_to_queue(task_t* task) {
    if (!global_task_queue || !task) return -1;
    
    printf("Pushing task to queue: type=%d, user=%s, file=%s\n", 
           task->type, task->username, task->filename);
    
    return task_queue_enqueue(global_task_queue, task);
}