#include "command_parser.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <sys/socket.h>
#include <dirent.h>

// Global task queue (placeholder - will be replaced by Module 2 interface)
static task_t task_queue[1000];
static int task_queue_count = 0;
static pthread_mutex_t task_queue_mutex = PTHREAD_MUTEX_INITIALIZER;

// Forward declaration
void simulate_task_processing(task_t* task);

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
    
    // Create task for Module 2
    task_t* task = create_task(cmd_type, filename, session, client_socket);
    if (!task) {
        send_response(client_socket, "ERROR: Failed to create task\n");
        return -1;
    }
    
    // Push task to queue (placeholder implementation)
    if (push_task_to_queue(task) != 0) {
        send_response(client_socket, "ERROR: Failed to submit task\n");
        free(task);
        return -1;
    }
    
    // In the placeholder queue, we process immediately, so no extra ack to avoid duplicates
    return 0;
}

int push_task_to_queue(task_t* task) {
    // Placeholder implementation - will be replaced by Module 2 interface
    if (!task) return -1;
    
    pthread_mutex_lock(&task_queue_mutex);
    if (task_queue_count >= 1000) {
        pthread_mutex_unlock(&task_queue_mutex);
        printf("Task queue full!\n");
        return -1;
    }
    // Copy task to queue
    memcpy(&task_queue[task_queue_count], task, sizeof(task_t));
    task_queue_count++;
    pthread_mutex_unlock(&task_queue_mutex);
    
    printf("Task queued: type=%d, user=%s, file=%s\n", 
           task->type, task->username, task->filename);
    
    // TODO: This is where we'll interface with Module 2's task queue
    // For now, just simulate processing
    simulate_task_processing(task);
    
    return 0;
}

// Placeholder function to simulate task processing
void simulate_task_processing(task_t* task) {
    if (!task) return;

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
                send_response(task->client_socket, "UPLOAD_ERROR: Could not create file\n");
                break;
            }
            fprintf(fp, "Uploaded by %s\n", task->username);
            fclose(fp);
            send_response(task->client_socket, "UPLOAD_SUCCESS: File uploaded successfully\n");
            break;
        }
        case CMD_DOWNLOAD: {
            char path[1400];
            snprintf(path, sizeof(path), "%s/%s", files_dir, task->filename);
            FILE *fp = fopen(path, "r");
            if (!fp) {
                send_response(task->client_socket, "DOWNLOAD_ERROR: File not found\n");
                break;
            }
            // For now, just report success (not streaming content)
            fclose(fp);
            send_response(task->client_socket, "DOWNLOAD_SUCCESS: File download completed\n");
            break;
        }
        case CMD_DELETE: {
            char path[1400];
            snprintf(path, sizeof(path), "%s/%s", files_dir, task->filename);
            if (unlink(path) == 0) {
                send_response(task->client_socket, "DELETE_SUCCESS: File deleted successfully\n");
            } else {
                send_response(task->client_socket, "DELETE_ERROR: File not found or cannot delete\n");
            }
            break;
        }
        case CMD_LIST: {
            DIR *d = opendir(files_dir);
            if (!d) {
                send_response(task->client_socket, "LIST_ERROR: Cannot open user files directory\n");
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
                // Append with space separation
                size_t need = strlen(de->d_name) + 2; // space + name
                if (offset + need + 1 >= sizeof(buffer)) { // +1 for newline
                    break;
                }
                offset += snprintf(buffer + offset, sizeof(buffer) - offset, " %s", de->d_name);
                count++;
            }
            closedir(d);
            if (count == 0) {
                send_response(task->client_socket, "LIST_SUCCESS: No files found\n");
            } else {
                if (offset + 1 < sizeof(buffer)) {
                    buffer[offset++] = '\n';
                    buffer[offset] = '\0';
                }
                send_response(task->client_socket, buffer);
            }
            break;
        }
        default:
            break;
    }
    // Since create_task used malloc, free the task here after processing in placeholder flow
    free(task);
}
