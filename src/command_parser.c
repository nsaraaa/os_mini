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
#include <errno.h>

// Global task system (Phase 2)
static task_queue_t *global_task_queue = NULL;
static worker_pool_t *global_worker_pool = NULL;

// Initialize task system (called from main)
void init_task_system(void) {
    global_task_queue = task_queue_init(1000);
    global_worker_pool = worker_pool_init(4, global_task_queue);
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
    
    // Check LONGER commands FIRST to avoid prefix matching issues
    if (strncasecmp(input, "UPLOAD_FILE", 11) == 0) return CMD_UPLOAD_FILE;
    if (strncasecmp(input, "UPLOAD", 6) == 0) return CMD_UPLOAD;
    if (strncasecmp(input, "DOWNLOAD", 8) == 0) return CMD_DOWNLOAD;
    if (strncasecmp(input, "SIGNUP", 6) == 0) return CMD_SIGNUP;
    if (strncasecmp(input, "LOGIN", 5) == 0) return CMD_LOGIN;
    if (strncasecmp(input, "LOGOUT", 6) == 0) return CMD_LOGOUT;
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
    task->file_data = NULL;
    task->file_size = 0;
    
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

task_t* create_upload_task(const char* filename, void* data, size_t data_size, user_session_t* session, int client_socket) {
    task_t* task = create_task(CMD_UPLOAD, filename, session, client_socket);
    if (!task) {
        return NULL;
    }
    
    // Copy file data
    if (data && data_size > 0) {
        task->file_data = malloc(data_size);
        if (task->file_data) {
            memcpy(task->file_data, data, data_size);
            task->file_size = data_size;
        }
    }
    
    return task;
}

int handle_upload_file_command(const char* command, user_session_t* session, int client_socket) {
    char filepath[512] = {0};
    
    // Parse file path: UPLOAD_FILE /path/to/file.txt
    if (sscanf(command, "UPLOAD_FILE %511s", filepath) != 1) {
        send_response(client_socket, "UPLOAD_FILE_ERROR: Usage: UPLOAD_FILE /path/to/file\n");
        return -1;
    }
    
    // Extract filename from path
    char filename[256];
    const char* last_slash = strrchr(filepath, '/');
    if (last_slash) {
        strncpy(filename, last_slash + 1, sizeof(filename) - 1);
    } else {
        strncpy(filename, filepath, sizeof(filename) - 1);
    }
    filename[sizeof(filename) - 1] = '\0';
    
    // Read file from local disk
    FILE* fp = fopen(filepath, "rb");
    if (!fp) {
        send_response(client_socket, "UPLOAD_FILE_ERROR: Cannot open local file\n");
        return -1;
    }
    
    // Get file size
    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    
    if (file_size <= 0) {
        fclose(fp);
        send_response(client_socket, "UPLOAD_FILE_ERROR: Empty file or size error\n");
        return -1;
    }
    
    // Read file data
    void* file_data = malloc(file_size);
    if (!file_data) {
        fclose(fp);
        send_response(client_socket, "UPLOAD_FILE_ERROR: Memory allocation failed\n");
        return -1;
    }
    
    size_t read_size = fread(file_data, 1, file_size, fp);
    fclose(fp);
    
    if (read_size != (size_t)file_size) {
        free(file_data);
        send_response(client_socket, "UPLOAD_FILE_ERROR: Failed to read file\n");
        return -1;
    }
    
    // Create upload task with the file data
    task_t* task = create_task(CMD_UPLOAD_FILE, filename, session, client_socket);
    if (task && file_data && file_size > 0) {
        task->file_data = file_data;
        task->file_size = file_size;
    }
    if (!task) {
        free(file_data);
        send_response(client_socket, "ERROR: Failed to create task\n");
        return -1;
    }
    
    // Initialize task synchronization primitives
    pthread_mutex_init(&task->result_mutex, NULL);
    pthread_cond_init(&task->result_ready, NULL);
    task->result_complete = 0;
    task->task_id = rand();
    
    if (push_task_to_queue(task) != 0) {
        send_response(client_socket, "ERROR: Failed to submit task\n");
        pthread_mutex_destroy(&task->result_mutex);
        pthread_cond_destroy(&task->result_ready);
        free(file_data);
        free(task);
        return -1;
    }
    
    send_response(client_socket, "TASK_QUEUED: Local file read, processing upload...\n");
    
    // Wait for worker completion
    pthread_mutex_lock(&task->result_mutex);
    while (!task->result_complete) {
        pthread_cond_wait(&task->result_ready, &task->result_mutex);
    }
    pthread_mutex_unlock(&task->result_mutex);
    
    // Send result
    send_response(client_socket, task->result);
    
    // Cleanup
    if (task->file_data) {
        free(task->file_data);
    }
    pthread_mutex_destroy(&task->result_mutex);
    pthread_cond_destroy(&task->result_ready);
    free(task);
    
    return 0;
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
    char filename[256] = {0};
    void* file_data = NULL;
    size_t total_received = 0;
    
    // Parse filename
    if (sscanf(command, "UPLOAD %255s", filename) != 1) {
        send_response(client_socket, "UPLOAD_ERROR: Usage: UPLOAD filename\n");
        return -1;
    }
    
    // Send READY signal to client
    if (send_response(client_socket, "READY: Send file data followed by END\n") != 0) {
        return -1;
    }
    
    // Receive file data
    char buffer[4096];
    file_data = malloc(10 * 1024 * 1024); // 10MB max
    if (!file_data) {
        send_response(client_socket, "UPLOAD_ERROR: Memory allocation failed\n");
        return -1;
    }
    
    int reception_timeout = 0;
    int reception_success = 0;
    
    while (reception_timeout < 10 && !reception_success) {
        ssize_t bytes_received = recv(client_socket, buffer, sizeof(buffer), 0);
        
        if (bytes_received <= 0) {
            if (bytes_received == 0) {
                printf("Client disconnected during file reception\n");
            } else {
                perror("recv");
            }
            free(file_data);
            return -1;
        }
        
        // Check for END marker
        int end_found = 0;
        for (int i = 0; i <= bytes_received - 3; i++) {
            if (memcmp(buffer + i, "END", 3) == 0) {
                // Copy data before END marker
                if (i > 0) {
                    memcpy((char*)file_data + total_received, buffer, i);
                    total_received += i;
                }
                end_found = 1;
                reception_success = 1;
                break;
            }
        }
        
        if (!end_found) {
            // No END marker found, copy all data
            memcpy((char*)file_data + total_received, buffer, bytes_received);
            total_received += bytes_received;
        }
        
        // Safety check
        if (total_received > 10 * 1024 * 1024) {
            free(file_data);
            send_response(client_socket, "UPLOAD_ERROR: File too large\n");
            return -1;
        }
        
        reception_timeout++;
    }
    
    if (!reception_success) {
        free(file_data);
        send_response(client_socket, "UPLOAD_ERROR: Timeout - no END marker received\n");
        return -1;
    }
    
    // Create upload task with actual file data
    task_t* task = create_upload_task(filename, file_data, total_received, session, client_socket);
    if (!task) {
        free(file_data);  // FREE HERE if task creation fails
        send_response(client_socket, "ERROR: Failed to create task\n");
        return -1;
    }
    
    // Initialize task synchronization primitives
    pthread_mutex_init(&task->result_mutex, NULL);
    pthread_cond_init(&task->result_ready, NULL);
    task->result_complete = 0;
    task->task_id = rand();
    
    // Push task to worker queue
    if (push_task_to_queue(task) != 0) {
        send_response(client_socket, "ERROR: Failed to submit task\n");
        pthread_mutex_destroy(&task->result_mutex);
        pthread_cond_destroy(&task->result_ready);
        free(file_data);  // FREE HERE if queue push fails
        free(task);
        return -1;
    }
    
    send_response(client_socket, "TASK_QUEUED: File data received, processing...\n");
    
    // Wait for worker to complete
    pthread_mutex_lock(&task->result_mutex);
    while (!task->result_complete) {
        pthread_cond_wait(&task->result_ready, &task->result_mutex);
    }
    pthread_mutex_unlock(&task->result_mutex);
    
    // Send result back to client
    send_response(client_socket, task->result);
    
    // Cleanup - task owns file_data, so we don't free it here
    // The task cleanup will handle file_data in handle_authenticated_command
    pthread_mutex_destroy(&task->result_mutex);
    pthread_cond_destroy(&task->result_ready);
    free(task);
    
    return 0;
}
int handle_authenticated_command(const char* command, user_session_t* session, int client_socket) {
    if (!command || !session || !session->authenticated) {
        return -1;
    }
    
    command_type_t cmd_type = parse_command(command);
    char filename[256] = {0}; 
    
    // Handle different upload types
    if (cmd_type == CMD_UPLOAD) {
        return handle_upload_command(command, session, client_socket);
    }
    
    if (cmd_type == CMD_UPLOAD_FILE) {
        return handle_upload_file_command(command, session, client_socket);
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
    
    return task_queue_enqueue(global_task_queue, task);
}