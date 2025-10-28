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
#include <errno.h>  // Add this line

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
    task->file_data = NULL;  // Initialize
    task->file_size = 0;     // Initialize
    
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

// NEW: Create upload task with actual file data
task_t* create_upload_task(const char* filename, void* data, size_t data_size, user_session_t* session, int client_socket) {
    printf("=== DEBUG: create_upload_task START ===\n");
    printf("DEBUG: Creating task for file: %s, data_size: %zu\n", filename, data_size);
    
    task_t* task = create_task(CMD_UPLOAD, filename, session, client_socket);
    if (!task) {
        printf("DEBUG: create_task failed\n");
        return NULL;
    }
    
    // Copy file data
    if (data && data_size > 0) {
        printf("DEBUG: Allocating %zu bytes for file data\n", data_size);
        task->file_data = malloc(data_size);
        if (task->file_data) {
            printf("DEBUG: Copying file data\n");
            memcpy(task->file_data, data, data_size);
            task->file_size = data_size;
            printf("DEBUG: File data copied successfully\n");
        } else {
            printf("DEBUG: Failed to allocate memory for file data\n");
        }
    } else {
        printf("DEBUG: No file data provided\n");
    }
    
    printf("=== DEBUG: create_upload_task COMPLETED ===\n");
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

int handle_upload_command(const char* command, user_session_t* session, int client_socket) {
    printf("=== DEBUG: handle_upload_command START ===\n");
    printf("DEBUG: Command received: %s\n", command);
    printf("DEBUG: Session user: %s\n", session->username);
    printf("DEBUG: Client socket: %d\n", client_socket);
    
    char filename[256] = {0};
    
    // Parse filename
    if (sscanf(command, "UPLOAD %255s", filename) != 1) {
        printf("DEBUG: Failed to parse filename from command\n");
        send_response(client_socket, "UPLOAD_ERROR: Usage: UPLOAD filename\n");
        return -1;
    }
    
    printf("DEBUG: Filename parsed: %s\n", filename);
    
    // Send READY signal to client
    printf("DEBUG: Sending READY to client\n");
    if (send_response(client_socket, "READY: Send file data followed by END\n") != 0) {
        printf("DEBUG: Failed to send READY response\n");
        return -1;
    }
    printf("DEBUG: READY sent successfully\n");
    
    // Receive file data
    printf("DEBUG: Starting file data reception\n");
    char buffer[4096];
    size_t total_received = 0;
    void* file_data = malloc(10 * 1024 * 1024); // 10MB max
    if (!file_data) {
        printf("DEBUG: Memory allocation failed for file data\n");
        send_response(client_socket, "UPLOAD_ERROR: Memory allocation failed\n");
        return -1;
    }
    printf("DEBUG: File data buffer allocated\n");
    
    int reception_timeout = 0;
    while (reception_timeout < 10) { // Timeout after 10 iterations
        printf("DEBUG: [Loop %d] Waiting to receive file data...\n", reception_timeout);
        ssize_t bytes_received = recv(client_socket, buffer, sizeof(buffer), 0);
        printf("DEBUG: Received %zd bytes\n", bytes_received);
        
        if (bytes_received <= 0) {
            if (bytes_received == 0) {
                printf("DEBUG: Client disconnected during file reception\n");
            } else {
                printf("DEBUG: recv() error: %s\n", strerror(errno));
            }
            free(file_data);
            return -1;
        }
        
        // Print first few bytes for debugging
        printf("DEBUG: First 10 bytes: ");
        for (int i = 0; i < (bytes_received < 10 ? bytes_received : 10); i++) {
            printf("%02x ", (unsigned char)buffer[i]);
        }
        printf("\n");
        
        // Check for END marker
        int end_found = 0;
        for (int i = 0; i <= bytes_received - 3; i++) {
            if (memcmp(buffer + i, "END", 3) == 0) {
                printf("DEBUG: Found END marker at position %d\n", i);
                // Copy data before END marker
                if (i > 0) {
                    memcpy((char*)file_data + total_received, buffer, i);
                    total_received += i;
                }
                end_found = 1;
                break;
            }
        }
        
        if (end_found) {
            printf("DEBUG: END marker found, finishing upload\n");
            break;
        }
        
        // No END marker found, copy all data
        memcpy((char*)file_data + total_received, buffer, bytes_received);
        total_received += bytes_received;
        
        printf("DEBUG: Total received so far: %zu bytes\n", total_received);
        
        // Safety check
        if (total_received > 10 * 1024 * 1024) {
            printf("DEBUG: File too large (%zu bytes), aborting\n", total_received);
            free(file_data);
            send_response(client_socket, "UPLOAD_ERROR: File too large\n");
            return -1;
        }
        
        reception_timeout++;
        
        // Small delay to prevent tight loop
        usleep(100000); // 100ms
    }
    
    if (reception_timeout >= 10) {
        printf("DEBUG: File reception timeout - no END marker received\n");
        free(file_data);
        send_response(client_socket, "UPLOAD_ERROR: Timeout - no END marker received\n");
        return -1;
    }
    
    printf("DEBUG: File data received successfully, total size: %zu bytes\n", total_received);
    
    // Create upload task with actual file data
    printf("DEBUG: Creating upload task\n");
    task_t* task = create_upload_task(filename, file_data, total_received, session, client_socket);
    if (!task) {
        printf("DEBUG: Failed to create upload task\n");
        free(file_data);
        send_response(client_socket, "ERROR: Failed to create task\n");
        return -1;
    }
    printf("DEBUG: Upload task created successfully\n");
    
    // Initialize task synchronization primitives
    printf("DEBUG: Initializing task synchronization\n");
    pthread_mutex_init(&task->result_mutex, NULL);
    pthread_cond_init(&task->result_ready, NULL);
    task->result_complete = 0;
    task->task_id = rand();
    printf("DEBUG: Task synchronization initialized\n");
    
    // Push task to worker queue
    printf("DEBUG: Pushing task to worker queue\n");
    if (push_task_to_queue(task) != 0) {
        printf("DEBUG: Failed to push task to queue\n");
        send_response(client_socket, "ERROR: Failed to submit task\n");
        pthread_mutex_destroy(&task->result_mutex);
        pthread_cond_destroy(&task->result_ready);
        free(file_data);
        free(task);
        return -1;
    }
    printf("DEBUG: Task pushed to queue successfully\n");
    
    send_response(client_socket, "TASK_QUEUED: File data received, processing...\n");
    printf("DEBUG: TASK_QUEUED sent to client\n");
    
    // Wait for worker to complete
    printf("DEBUG: Waiting for worker to complete task...\n");
    pthread_mutex_lock(&task->result_mutex);
    while (!task->result_complete) {
        printf("DEBUG: Waiting on condition variable...\n");
        pthread_cond_wait(&task->result_ready, &task->result_mutex);
        printf("DEBUG: Condition variable signaled\n");
    }
    pthread_mutex_unlock(&task->result_mutex);
    printf("DEBUG: Worker completed task\n");
    
    // Send result back to client
    printf("DEBUG: Sending result to client: %s", task->result);
    send_response(client_socket, task->result);
    printf("DEBUG: Result sent to client\n");
    
    // Cleanup
    printf("DEBUG: Cleaning up task resources\n");
    if (task->file_data) {
        free(task->file_data);
    }
    pthread_mutex_destroy(&task->result_mutex);
    pthread_cond_destroy(&task->result_ready);
    free(task);
    
    printf("=== DEBUG: handle_upload_command COMPLETED SUCCESSFULLY ===\n");
    return 0;
}

int handle_authenticated_command(const char* command, user_session_t* session, int client_socket) {
    if (!command || !session || !session->authenticated) {
        return -1;
    }
    
    command_type_t cmd_type = parse_command(command);
    char filename[256] = {0};
    
    // Handle UPLOAD separately with file data
    if (cmd_type == CMD_UPLOAD) {
        return handle_upload_command(command, session, client_socket);
    }
    
    // Parse filename for other file operations
    switch (cmd_type) {
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
    
    // Create task for worker (for non-upload commands)
    task_t* task = create_task(cmd_type, filename, session, client_socket);
    if (!task) {
        send_response(client_socket, "ERROR: Failed to create task\n");
        return -1;
    }
    
    // Initialize task synchronization primitives
    pthread_mutex_init(&task->result_mutex, NULL);
    pthread_cond_init(&task->result_ready, NULL);
    task->result_complete = 0;
    task->task_id = rand();
    
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
    
    // Wait for worker to complete
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
    
    printf("Pushing task to queue: type=%d, user=%s, file=%s, data_size=%zu\n", 
           task->type, task->username, task->filename, task->file_size);
    
    return task_queue_enqueue(global_task_queue, task);
}