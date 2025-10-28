#include "auth.h"
#include "module3_integration.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pwd.h>
#include <pthread.h>
#include <time.h>
#include <errno.h>
#ifdef __APPLE__
// crypt() is available in unistd.h on macOS
#else
#include <crypt.h>
#endif

#define USER_DATA_DIR "./user_data"
#define MAX_PASSWORD_LEN 64

// Global authentication state
static pthread_mutex_t auth_mutex = PTHREAD_MUTEX_INITIALIZER;

int auth_init(void) {
    // Create user data directory if it doesn't exist
    struct stat st = {0};
    if (stat(USER_DATA_DIR, &st) == -1) {
        if (mkdir(USER_DATA_DIR, 0755) == -1) {
            perror("mkdir user data directory");
            return -1;
        }
        printf("Created user data directory: %s\n", USER_DATA_DIR);
    }
    
    return 0;
}

void auth_cleanup(void) {
    // Cleanup any global resources
    pthread_mutex_destroy(&auth_mutex);
}

int auth_signup(const char* username, const char* password) {
    if (!username || !password) {
        return -1;
    }
    
    pthread_mutex_lock(&auth_mutex);
    
    // Create user directory path FIRST
    char user_dir[1024];
    snprintf(user_dir, sizeof(user_dir), "%s/%s", USER_DATA_DIR, username);
    
    // Create user directory
    if (auth_create_user_directory(username) != 0) {
        pthread_mutex_unlock(&auth_mutex);
        return -1;
    }
    
    // Create user in Module 3 metadata BEFORE auth files
    char *hashed = crypt(password, "os");
    if (!hashed) {
        rmdir(user_dir);  // Cleanup directory
        pthread_mutex_unlock(&auth_mutex);
        return -1;
    }
    
    if (module3_handle_signup(username, hashed) != 0) {
        printf("ERROR: Failed to create user in Module 3 metadata\n");
        rmdir(user_dir);  // Cleanup directory
        pthread_mutex_unlock(&auth_mutex);
        return -1;
    }
    
    // Now create auth files
    char password_file[1024];
    snprintf(password_file, sizeof(password_file), "%s/.password", user_dir);
    
    FILE *fp = fopen(password_file, "w");
    if (!fp) {
        perror("fopen password file");
        // Cleanup Module 3 on failure - use metadata function
        // delete_user() is in metadata.h, so use module3 function or direct call
        // For now, just log the error
        printf("WARNING: Failed to create password file but user exists in metadata\n");
        pthread_mutex_unlock(&auth_mutex);
        return -1;
    }
    
    fprintf(fp, "%s\n", hashed);
    fclose(fp);
    
    // Create user info file
    char info_file[1024];
    snprintf(info_file, sizeof(info_file), "%s/.info", user_dir);
    
    fp = fopen(info_file, "w");
    if (fp) {
        time_t now = time(NULL);
        fprintf(fp, "created=%ld\n", now);
        fclose(fp);
    }
    
    printf("User '%s' created successfully in both modules\n", username);
    pthread_mutex_unlock(&auth_mutex);
    return 0;
}

int auth_login(const char* username, const char* password, user_session_t* session) {
    if (!username || !password || !session) {
        return -1;
    }

    // Build paths
    char user_dir[512];
    char password_file[512];
    
    snprintf(user_dir, sizeof(user_dir), "%s/%s", USER_DATA_DIR, username);
    snprintf(password_file, sizeof(password_file), "%s/.password", user_dir);
    
    // Check if user directory exists
    struct stat st;
    if (stat(user_dir, &st) == -1) {
        return -1;
    }
    
    // Read stored password hash
    FILE *fp = fopen(password_file, "r");
    if (!fp) {
        return -1;
    }
    
    char stored_hash[128];
    if (fgets(stored_hash, sizeof(stored_hash), fp) == NULL) {
        fclose(fp);
        return -1;
    }
    fclose(fp);
    
    // Remove newline
    stored_hash[strcspn(stored_hash, "\r\n")] = 0;
    
    // Verify password
    char *hashed = crypt(password, "os");
    if (!hashed || strcmp(hashed, stored_hash) != 0) {
        return -1;
    }
    
    // Set up session
    strncpy(session->username, username, sizeof(session->username) - 1);
    strncpy(session->user_dir, user_dir, sizeof(session->user_dir) - 1);
    session->authenticated = 1;
    session->last_activity = time(NULL);
    
    printf("User '%s' logged in successfully\n", username);
    return 0;
}

int auth_logout(user_session_t* session) {
    if (!session) {
        return -1;
    }
    
    printf("User '%s' logged out\n", session->username);
    
    memset(session, 0, sizeof(user_session_t));
    return 0;
}

int auth_validate_session(user_session_t* session) {
    if (!session || !session->authenticated) {
        return -1;
    }
    
    // Check if user directory still exists
    struct stat st;
    if (stat(session->user_dir, &st) == -1) {
        session->authenticated = 0;
        return -1;
    }
    
    // Update last activity
    session->last_activity = time(NULL);
    return 0;
}

int auth_create_user_directory(const char* username) {
    if (!username) {
        return -1;
    }
    
    char user_dir[512];
    snprintf(user_dir, sizeof(user_dir), "%s/%s", USER_DATA_DIR, username);
    
    // Check if directory already exists
    struct stat st;
    if (stat(user_dir, &st) == 0) {
        return -1; // User already exists
    }
    
    // Create directory
    if (mkdir(user_dir, 0755) == -1) {
        perror("mkdir user directory");
        return -1;
    }
    
    printf("Created user directory: %s\n", user_dir);
    return 0;
}