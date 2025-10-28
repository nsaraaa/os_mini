#include "metadata.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

static metadata_manager_t metadata_mgr = {NULL, 0};

int init_metadata(void) {
    metadata_mgr.users = NULL;
    metadata_mgr.user_count = 0;
    printf("Metadata system initialized\n");
    return 0;
}

void cleanup_metadata(void) {
    user_metadata_t *current = metadata_mgr.users;
    
    while (current) {
        user_metadata_t *next_user = current->next;
        
        // Free file list
        file_metadata_t *file = current->files;
        while (file) {
            file_metadata_t *next_file = file->next;
            free(file);
            file = next_file;
        }
        
        free(current);
        current = next_user;
    }
    
    metadata_mgr.users = NULL;
    metadata_mgr.user_count = 0;
    printf("Metadata system cleaned up\n");
}

user_metadata_t* create_user(const char* username, const char* password_hash) {
    if (!username || !password_hash) return NULL;
    
    // Check if user already exists
    if (get_user(username) != NULL) {
        return NULL;
    }
    
    user_metadata_t *user = malloc(sizeof(user_metadata_t));
    if (!user) return NULL;
    
    strncpy(user->username, username, MAX_USERNAME - 1);
    user->username[MAX_USERNAME - 1] = '\0';
    
    strncpy(user->password_hash, password_hash, sizeof(user->password_hash) - 1);
    user->password_hash[sizeof(user->password_hash) - 1] = '\0';
    
    user->storage_used = 0;
    user->storage_quota = DEFAULT_QUOTA_BYTES;
    user->file_count = 0;
    user->files = NULL;
    user->created_at = time(NULL);
    user->last_login = time(NULL);
    
    // Add to linked list
    user->next = metadata_mgr.users;
    metadata_mgr.users = user;
    metadata_mgr.user_count++;
    
    printf("Created metadata for user '%s'\n", username);
    return user;
}

user_metadata_t* get_user(const char* username) {
    if (!username) return NULL;
    
    user_metadata_t *current = metadata_mgr.users;
    while (current) {
        if (strcmp(current->username, username) == 0) {
            return current;
        }
        current = current->next;
    }
    
    return NULL;
}

int delete_user(const char* username) {
    if (!username) return -1;
    
    user_metadata_t *current = metadata_mgr.users;
    user_metadata_t *prev = NULL;
    
    while (current) {
        if (strcmp(current->username, username) == 0) {
            // Free file list
            file_metadata_t *file = current->files;
            while (file) {
                file_metadata_t *next_file = file->next;
                free(file);
                file = next_file;
            }
            
            // Remove from linked list
            if (prev) {
                prev->next = current->next;
            } else {
                metadata_mgr.users = current->next;
            }
            
            free(current);
            metadata_mgr.user_count--;
            printf("Deleted metadata for user '%s'\n", username);
            return 0;
        }
        prev = current;
        current = current->next;
    }
    
    return -1;
}

int add_file_to_user(user_metadata_t* user, const char* filename, size_t size) {
    if (!user || !filename) return -1;
    
    // Check if file already exists (update case)
    file_metadata_t *existing = get_file_metadata(user, filename);
    if (existing) {
        // Update existing file size
        user->storage_used -= existing->size;
        existing->size = size;
        user->storage_used += size;
        existing->uploaded_at = time(NULL);
        printf("Updated file '%s' for user '%s' (size: %zu bytes)\n", 
               filename, user->username, size);
        return 0;
    }
    
    // Check file count limit
    if (user->file_count >= MAX_FILES_PER_USER) {
        return -1;
    }
    
    // Create new file metadata
    file_metadata_t *file = malloc(sizeof(file_metadata_t));
    if (!file) return -1;
    
    strncpy(file->filename, filename, MAX_FILENAME - 1);
    file->filename[MAX_FILENAME - 1] = '\0';
    file->size = size;
    file->uploaded_at = time(NULL);
    
    // Add to linked list
    file->next = user->files;
    user->files = file;
    user->file_count++;
    user->storage_used += size;
    
    printf("Added file '%s' to user '%s' (size: %zu bytes)\n", 
           filename, user->username, size);
    return 0;
}

int remove_file_from_user(user_metadata_t* user, const char* filename) {
    if (!user || !filename) return -1;
    
    file_metadata_t *current = user->files;
    file_metadata_t *prev = NULL;
    
    while (current) {
        if (strcmp(current->filename, filename) == 0) {
            // Remove from linked list
            if (prev) {
                prev->next = current->next;
            } else {
                user->files = current->next;
            }
            
            user->storage_used -= current->size;
            user->file_count--;
            
            printf("Removed file '%s' from user '%s' (freed %zu bytes)\n", 
                   filename, user->username, current->size);
            
            free(current);
            return 0;
        }
        prev = current;
        current = current->next;
    }
    
    return -1; // File not found
}

file_metadata_t* get_file_metadata(user_metadata_t* user, const char* filename) {
    if (!user || !filename) return NULL;
    
    file_metadata_t *current = user->files;
    while (current) {
        if (strcmp(current->filename, filename) == 0) {
            return current;
        }
        current = current->next;
    }
    
    return NULL;
}

file_metadata_t* get_file_list(user_metadata_t* user) {
    return user ? user->files : NULL;
}

int update_user_storage(user_metadata_t* user, ssize_t delta) {
    if (!user) return -1;
    
    if (delta < 0 && (size_t)(-delta) > user->storage_used) {
        user->storage_used = 0;
    } else {
        user->storage_used += delta;
    }
    
    return 0;
}

size_t get_user_storage_used(user_metadata_t* user) {
    return user ? user->storage_used : 0;
}

size_t get_user_storage_quota(user_metadata_t* user) {
    return user ? user->storage_quota : 0;
}
void debug_print_users(void) {
    
    user_metadata_t *current = metadata_mgr.users;
    int count = 0;
    while (current && count < 100) {  // Safety limit
        printf("  User[%d]: %s (files: %d, storage: %zu/%zu)\n",
               count, current->username, current->file_count,
               current->storage_used, current->storage_quota);
        current = current->next;
        count++;
    }
    
    if (count >= 100) {
        printf("  WARNING: Possible infinite loop in user list!\n");
    }
  
}