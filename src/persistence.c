#include "persistence.h"
#include "file_ops.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>

void get_metadata_path(const char* username, char* path, size_t path_size) {
    snprintf(path, path_size, "%s/%s.meta", METADATA_DIR, username);
}

int save_user_metadata(user_metadata_t* user) {
    if (!user) return -1;
    
    char path[512];
    char temp_path[512];
    get_metadata_path(user->username, path, sizeof(path));
    snprintf(temp_path, sizeof(temp_path), "%s.tmp", path);
    
    // Write to temporary file first (atomic operation)
    FILE* fp = fopen(temp_path, "w");
    if (!fp) {
        perror("fopen metadata temp file");
        return -1;
    }
    
    // Write user metadata in simple text format
    fprintf(fp, "username=%s\n", user->username);
    fprintf(fp, "password_hash=%s\n", user->password_hash);
    fprintf(fp, "storage_used=%zu\n", user->storage_used);
    fprintf(fp, "storage_quota=%zu\n", user->storage_quota);
    fprintf(fp, "file_count=%d\n", user->file_count);
    fprintf(fp, "created_at=%ld\n", user->created_at);
    fprintf(fp, "last_login=%ld\n", user->last_login);
    
    // Write file list
    fprintf(fp, "FILES_BEGIN\n");
    file_metadata_t* file = user->files;
    while (file) {
        fprintf(fp, "file=%s,%zu,%ld\n", file->filename, file->size, file->uploaded_at);
        file = file->next;
    }
    fprintf(fp, "FILES_END\n");
    
    fclose(fp);
    
    // Atomic rename
    if (rename(temp_path, path) == -1) {
        perror("rename metadata file");
        unlink(temp_path);
        return -1;
    }
    
    printf("Saved metadata for user '%s'\n", user->username);
    return 0;
}

user_metadata_t* load_user_metadata(const char* username) {
    if (!username) return NULL;
    
    char path[512];
    get_metadata_path(username, path, sizeof(path));
    
    FILE* fp = fopen(path, "r");
    if (!fp) {
        return NULL; // File doesn't exist
    }
    
    user_metadata_t* user = malloc(sizeof(user_metadata_t));
    if (!user) {
        fclose(fp);
        return NULL;
    }
    
    memset(user, 0, sizeof(user_metadata_t));
    user->next = NULL;
    user->files = NULL;
    
    char line[1024];
    bool in_files_section = false;
    
    while (fgets(line, sizeof(line), fp)) {
        // Remove newline
        line[strcspn(line, "\r\n")] = 0;
        
        if (strcmp(line, "FILES_BEGIN") == 0) {
            in_files_section = true;
            continue;
        }
        
        if (strcmp(line, "FILES_END") == 0) {
            in_files_section = false;
            continue;
        }
        
        if (in_files_section) {
            // Parse file entry: filename,size,timestamp
            if (strncmp(line, "file=", 5) == 0) {
                char filename[MAX_FILENAME];
                size_t size;
                time_t uploaded_at;
                
                if (sscanf(line + 5, "%255[^,],%zu,%ld", filename, &size, &uploaded_at) == 3) {
                    file_metadata_t* file = malloc(sizeof(file_metadata_t));
                    if (file) {
                        strncpy(file->filename, filename, MAX_FILENAME - 1);
                        file->filename[MAX_FILENAME - 1] = '\0';
                        file->size = size;
                        file->uploaded_at = uploaded_at;
                        file->next = user->files;
                        user->files = file;
                        user->file_count++;
                    }
                }
            }
        } else {
            // Parse user metadata
            if (strncmp(line, "username=", 9) == 0) {
                strncpy(user->username, line + 9, MAX_USERNAME - 1);
            } else if (strncmp(line, "password_hash=", 14) == 0) {
                strncpy(user->password_hash, line + 14, sizeof(user->password_hash) - 1);
            } else if (strncmp(line, "storage_used=", 13) == 0) {
                sscanf(line + 13, "%zu", &user->storage_used);
            } else if (strncmp(line, "storage_quota=", 14) == 0) {
                sscanf(line + 14, "%zu", &user->storage_quota);
            } else if (strncmp(line, "file_count=", 11) == 0) {
                // Will be recalculated from file list
            } else if (strncmp(line, "created_at=", 11) == 0) {
                sscanf(line + 11, "%ld", &user->created_at);
            } else if (strncmp(line, "last_login=", 11) == 0) {
                sscanf(line + 11, "%ld", &user->last_login);
            }
        }
    }
    
    fclose(fp);
    printf("Loaded metadata for user '%s' (%d files)\n", user->username, user->file_count);
    return user;
}

int save_all_metadata(void) {
    // This function would iterate through all users and save their metadata
    // For now, metadata is saved per-operation in the file_ops functions
    printf("Saving all metadata (not implemented - using per-operation saves)\n");
    return 0;
}

int load_all_metadata(void) {
    DIR* dir = opendir(METADATA_DIR);
    if (!dir) {
        perror("opendir metadata directory");
        return -1;
    }
    
    struct dirent* entry;
    int loaded = 0;
    
    while ((entry = readdir(dir)) != NULL) {
        // Look for .meta files
        size_t len = strlen(entry->d_name);
        if (len > 5 && strcmp(entry->d_name + len - 5, ".meta") == 0) {
            // Extract username
            char username[MAX_USERNAME];
            strncpy(username, entry->d_name, len - 5);
            username[len - 5] = '\0';
            
            // Load metadata
            user_metadata_t* user = load_user_metadata(username);
            if (user) {
                // Add to metadata manager (this would need to be integrated)
                loaded++;
                // Note: In real implementation, you'd add user to the global list here
                free(user); // For now we just free it
            }
        }
    }
    
    closedir(dir);
    printf("Loaded %d user metadata files\n", loaded);
    return 0;
}