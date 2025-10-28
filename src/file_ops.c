#include "file_ops.h"
#include "quota.h"
#include "locking.h"
#include "persistence.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>

int init_file_storage(void) {
    // Create base directory
    struct stat st = {0};
    
    if (stat(STORAGE_BASE_DIR, &st) == -1) {
        if (mkdir(STORAGE_BASE_DIR, 0755) == -1) {
            perror("mkdir storage base");
            return -1;
        }
        printf("Created storage directory: %s\n", STORAGE_BASE_DIR);
    }
    
    // Create users directory
    if (stat(USERS_DIR, &st) == -1) {
        if (mkdir(USERS_DIR, 0755) == -1) {
            perror("mkdir users directory");
            return -1;
        }
        printf("Created users directory: %s\n", USERS_DIR);
    }
    
    // Create metadata directory
    if (stat(METADATA_DIR, &st) == -1) {
        if (mkdir(METADATA_DIR, 0755) == -1) {
            perror("mkdir metadata directory");
            return -1;
        }
        printf("Created metadata directory: %s\n", METADATA_DIR);
    }
    
    return 0;
}

void get_user_file_path(const char* username, const char* filename, 
                        char* path, size_t path_size) {
    snprintf(path, path_size, "%s/%s/%s", USERS_DIR, username, filename);
}

int create_user_directory(const char* username) {
    if (!username) return -1;
    
    char user_dir[512];
    snprintf(user_dir, sizeof(user_dir), "%s/%s", USERS_DIR, username);
    
    struct stat st = {0};
    if (stat(user_dir, &st) == 0) {
        return 0; // Already exists
    }
    
    if (mkdir(user_dir, 0755) == -1) {
        perror("mkdir user directory");
        return -1;
    }
    
    printf("Created user directory: %s\n", user_dir);
    return 0;
}

bool file_exists(const char* username, const char* filename) {
    char path[1024];
    get_user_file_path(username, filename, path, sizeof(path));
    
    struct stat st;
    return (stat(path, &st) == 0);
}

ssize_t get_file_size(const char* username, const char* filename) {
    char path[1024];
    get_user_file_path(username, filename, path, sizeof(path));
    
    struct stat st;
    if (stat(path, &st) == -1) {
        return -1;
    }
    
    return st.st_size;
}

int write_file_to_disk(const char* username, const char* filename, 
                       const void* data, size_t size) {
    if (!username || !filename || !data) return FILE_OP_INVALID_PARAM;
    
    char path[1024];
    char temp_path[1024];
    get_user_file_path(username, filename, path, sizeof(path));
    snprintf(temp_path, sizeof(temp_path), "%s.tmp", path);
    
    // Write to temporary file first
    FILE* fp = fopen(temp_path, "wb");
    if (!fp) {
        perror("fopen temp file");
        return FILE_OP_ERROR;
    }
    
    size_t written = fwrite(data, 1, size, fp);
    fclose(fp);
    
    if (written != size) {
        unlink(temp_path);
        return FILE_OP_ERROR;
    }
    
    // Atomic rename
    if (rename(temp_path, path) == -1) {
        perror("rename file");
        unlink(temp_path);
        return FILE_OP_ERROR;
    }
    
    printf("Wrote file to disk: %s (%zu bytes)\n", path, size);
    return FILE_OP_SUCCESS;
}

int read_file_from_disk(const char* username, const char* filename, 
                        void** data, size_t* size) {
    if (!username || !filename || !data || !size) return FILE_OP_INVALID_PARAM;
    
    char path[1024];
    get_user_file_path(username, filename, path, sizeof(path));
    
    FILE* fp = fopen(path, "rb");
    if (!fp) {
        return FILE_OP_NOT_FOUND;
    }
    
    // Get file size
    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    
    if (file_size < 0) {
        fclose(fp);
        return FILE_OP_ERROR;
    }
    
    // Allocate buffer
    *data = malloc(file_size);
    if (!*data) {
        fclose(fp);
        return FILE_OP_ERROR;
    }
    
    // Read file
    size_t read_size = fread(*data, 1, file_size, fp);
    fclose(fp);
    
    if (read_size != (size_t)file_size) {
        free(*data);
        *data = NULL;
        return FILE_OP_ERROR;
    }
    
    *size = file_size;
    printf("Read file from disk: %s (%zu bytes)\n", path, *size);
    return FILE_OP_SUCCESS;
}

int upload_file(const char* username, const char* filename, 
                const void* data, size_t data_size) {
    if (!username || !filename || !data) return FILE_OP_INVALID_PARAM;
    
    printf("Upload request: user='%s', file='%s', size=%zu\n", 
           username, filename, data_size);
    
    // NO LOCK - caller (worker) already holds it!
    
    // Get user metadata
    user_metadata_t* user = get_user(username);
    if (!user) {
        printf("ERROR: User '%s' not found in metadata\n", username);
        return FILE_OP_ERROR;
    }
    
    printf("DEBUG: User found, checking file existence\n");
    
    // Check if file exists (for overwrite case)
    ssize_t old_size = 0;
    if (file_exists(username, filename)) {
        printf("DEBUG: File exists, getting old size\n");
        old_size = get_file_size(username, filename);
    }
    
    printf("DEBUG: Checking quota (old_size=%ld, new_size=%zu)\n", old_size, data_size);
    
    // Check quota (accounting for potential overwrite)
    size_t net_increase = data_size;
    if (old_size > 0) {
        net_increase = (data_size > (size_t)old_size) ? 
                       (data_size - old_size) : 0;
    }
    
    if (!check_quota(user, net_increase)) {
        printf("ERROR: Quota check failed\n");
        return FILE_OP_QUOTA_EXCEEDED;
    }
    
    printf("DEBUG: Writing file to disk\n");
    
    // Write file to disk
    int result = write_file_to_disk(username, filename, data, data_size);
    if (result != FILE_OP_SUCCESS) {
        printf("ERROR: Failed to write file to disk\n");
        return result;
    }
    
    printf("DEBUG: Updating metadata\n");
    
    // Update metadata
    if (old_size > 0) {
        remove_file_from_user(user, filename);
    }
    add_file_to_user(user, filename, data_size);
    
    printf("DEBUG: Saving metadata\n");
    
    // Save metadata
    save_user_metadata(user);
    
    printf("Upload completed successfully for user '%s'\n", username);
    return FILE_OP_SUCCESS;
}

int download_file(const char* username, const char* filename, 
                  void** data, size_t* data_size) {
    if (!username || !filename || !data || !data_size) 
        return FILE_OP_INVALID_PARAM;
    
    printf("Download request: user='%s', file='%s'\n", username, filename);
    
    // NO LOCK - caller already holds it!
    
    // Get user metadata
    user_metadata_t* user = get_user(username);
    if (!user) {
        return FILE_OP_ERROR;
    }
    
    // Check if file exists in metadata
    file_metadata_t* file_meta = get_file_metadata(user, filename);
    if (!file_meta) {
        return FILE_OP_NOT_FOUND;
    }
    
    // Read file from disk
    int result = read_file_from_disk(username, filename, data, data_size);
    
    if (result == FILE_OP_SUCCESS) {
        printf("Download completed successfully for user '%s'\n", username);
    }
    
    return result;
}

int delete_file(const char* username, const char* filename) {
    if (!username || !filename) return FILE_OP_INVALID_PARAM;
    
    printf("Delete request: user='%s', file='%s'\n", username, filename);
    
    // NO LOCK - caller already holds it!
    
    // Get user metadata
    user_metadata_t* user = get_user(username);
    if (!user) {
        return FILE_OP_ERROR;
    }
    
    // Check if file exists
    file_metadata_t* file_meta = get_file_metadata(user, filename);
    if (!file_meta) {
        return FILE_OP_NOT_FOUND;
    }
    
    size_t file_size = file_meta->size;
    
    // Delete from disk
    char path[1024];
    get_user_file_path(username, filename, path, sizeof(path));
    
    if (unlink(path) == -1) {
        perror("unlink file");
        return FILE_OP_ERROR;
    }
    
    // Update metadata
    remove_file_from_user(user, filename);
    
    // Save metadata
    save_user_metadata(user);
    
    printf("Delete completed successfully for user '%s'\n", username);
    return FILE_OP_SUCCESS;
}

int list_files(const char* username, char* buffer, size_t buffer_size) {
    if (!username || !buffer) return FILE_OP_INVALID_PARAM;
    
    printf("DEBUG: list_files called for user '%s'\n", username);
    
    // NO LOCK - caller already holds it!
    
    // Get user metadata
    user_metadata_t* user = get_user(username);
    if (!user) {
        printf("ERROR: User '%s' not found\n", username);
        return FILE_OP_ERROR;
    }
    
    printf("DEBUG: User found, file_count=%d\n", user->file_count);
    
    // Build file list
    size_t offset = 0;
    file_metadata_t* file = user->files;
    
    if (!file) {
        snprintf(buffer, buffer_size, "No files found");
        printf("DEBUG: No files for user\n");
        return FILE_OP_SUCCESS;
    }
    
    printf("DEBUG: Building file list\n");
    
    while (file && offset < buffer_size - 1) {
        int written = snprintf(buffer + offset, buffer_size - offset,
                              "%s (%zu bytes)\n", file->filename, file->size);
        if (written < 0 || offset + written >= buffer_size) {
            break;
        }
        offset += written;
        file = file->next;
    }
    
    printf("DEBUG: File list built successfully\n");
    return FILE_OP_SUCCESS;
}