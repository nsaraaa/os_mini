#include "module3_integration.h"
#include "file_ops.h"
#include "metadata.h"
#include "persistence.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int module3_init(void) {
    printf("Initializing Module 3...\n");
    
    // Initialize metadata system
    if (init_metadata() != 0) {
        fprintf(stderr, "Failed to initialize metadata system\n");
        return -1;
    }
    
    // Initialize file storage
    if (init_file_storage() != 0) {
        fprintf(stderr, "Failed to initialize file storage\n");
        cleanup_metadata();
        return -1;
    }
    
    // Load existing metadata
    load_all_metadata();
    
    printf("Module 3 initialized successfully\n");
    return 0;
}

void module3_cleanup(void) {
    printf("Cleaning up Module 3...\n");
    save_all_metadata();
    cleanup_metadata();
    printf("Module 3 cleanup complete\n");
}

int module3_handle_signup(const char* username, const char* password_hash) {
    if (!username || !password_hash) {
        return -1;
    }
    
    // Create user metadata
    user_metadata_t* user = create_user(username, password_hash);
    if (!user) {
        return -1;
    }
    
    // Create user directory
    if (create_user_directory(username) != 0) {
        delete_user(username);
        return -1;
    }
    
    // Save metadata
    save_user_metadata(user);
    
    printf("Module 3: User '%s' signed up successfully\n", username);
    return 0;
}

int module3_process_upload(task_t* task) {
    if (!task || !task->authenticated) {
        return FILE_OP_INVALID_PARAM;
    }
    
    printf("Processing upload: user=%s, file=%s, size=%zu\n", 
           task->username, task->filename, task->file_size);
    
    // Use ACTUAL file data instead of sample data
    int result;
    if (task->file_data && task->file_size > 0) {
        // Use real file data received from client
        result = upload_file(task->username, task->filename, 
                            task->file_data, task->file_size);
    } else {
        // Fallback to sample data (for backward compatibility)
        char sample_data[1024];
        snprintf(sample_data, sizeof(sample_data), 
                 "File: %s\nUploaded by: %s\nTimestamp: %ld\n",
                 task->filename, task->username, time(NULL));
        result = upload_file(task->username, task->filename, 
                            sample_data, strlen(sample_data));
    }
    
    switch (result) {
        case FILE_OP_SUCCESS:
            snprintf(task->result, sizeof(task->result),
                    "UPLOAD_SUCCESS: File '%s' uploaded successfully (%zu bytes)\n", 
                    task->filename, task->file_size);
            return 0;
            
        case FILE_OP_QUOTA_EXCEEDED:
            snprintf(task->result, sizeof(task->result),
                    "UPLOAD_ERROR: Quota exceeded for user '%s'\n", 
                    task->username);
            return -1;
            
        default:
            snprintf(task->result, sizeof(task->result),
                    "UPLOAD_ERROR: Failed to upload file '%s' (error %d)\n", 
                    task->filename, result);
            return -1;
    }
}

int module3_process_download(task_t* task) {
    if (!task || !task->authenticated) {
        return FILE_OP_INVALID_PARAM;
    }
    
    void* data = NULL;
    size_t data_size = 0;
    
    int result = download_file(task->username, task->filename, 
                               &data, &data_size);
    
    if (result == FILE_OP_SUCCESS) {
        snprintf(task->result, sizeof(task->result),
                "DOWNLOAD_SUCCESS: File '%s' (%zu bytes)\n", 
                task->filename, data_size);
        free(data);
        return 0;
    } else if (result == FILE_OP_NOT_FOUND) {
        snprintf(task->result, sizeof(task->result),
                "DOWNLOAD_ERROR: File '%s' not found\n", 
                task->filename);
        return -1;
    } else {
        snprintf(task->result, sizeof(task->result),
                "DOWNLOAD_ERROR: Failed to download file '%s'\n", 
                task->filename);
        return -1;
    }
}

int module3_process_delete(task_t* task) {
    if (!task || !task->authenticated) {
        return FILE_OP_INVALID_PARAM;
    }
    
    int result = delete_file(task->username, task->filename);
    
    if (result == FILE_OP_SUCCESS) {
        snprintf(task->result, sizeof(task->result),
                "DELETE_SUCCESS: File '%s' deleted successfully\n", 
                task->filename);
        return 0;
    } else if (result == FILE_OP_NOT_FOUND) {
        snprintf(task->result, sizeof(task->result),
                "DELETE_ERROR: File '%s' not found\n", 
                task->filename);
        return -1;
    } else {
        snprintf(task->result, sizeof(task->result),
                "DELETE_ERROR: Failed to delete file '%s'\n", 
                task->filename);
        return -1;
    }
}

int module3_process_list(task_t* task) {
    if (!task || !task->authenticated) {
        return FILE_OP_INVALID_PARAM;
    }
    
    char file_list[4096];
    int result = list_files(task->username, file_list, sizeof(file_list));
    
    if (result == FILE_OP_SUCCESS) {
        snprintf(task->result, sizeof(task->result),
                "LIST_SUCCESS:\n%s", file_list);
        return 0;
    } else {
        snprintf(task->result, sizeof(task->result),
                "LIST_ERROR: Failed to list files\n");
        return -1;
    }
}