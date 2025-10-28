#include "quota.h"
#include <stdio.h>

bool check_quota(user_metadata_t* user, size_t file_size) {
    if (!user) return false;
    
    size_t required = user->storage_used + file_size;
    bool allowed = (required <= user->storage_quota);
    
    if (!allowed) {
        printf("Quota check failed for user '%s': required=%zu, quota=%zu, used=%zu\n",
               user->username, required, user->storage_quota, user->storage_used);
    }
    
    return allowed;
}

int update_quota_on_upload(user_metadata_t* user, size_t file_size) {
    if (!user) return QUOTA_ERROR;
    
    // Check quota first
    if (!check_quota(user, file_size)) {
        return QUOTA_EXCEEDED;
    }
    
    user->storage_used += file_size;
    
    printf("Updated quota for user '%s' after upload: used=%zu/%zu (%.1f%%)\n",
           user->username, user->storage_used, user->storage_quota,
           get_quota_usage_percent(user));
    
    return QUOTA_OK;
}

int update_quota_on_delete(user_metadata_t* user, size_t file_size) {
    if (!user) return QUOTA_ERROR;
    
    if (file_size > user->storage_used) {
        user->storage_used = 0;
    } else {
        user->storage_used -= file_size;
    }
    
    printf("Updated quota for user '%s' after delete: used=%zu/%zu (%.1f%%)\n",
           user->username, user->storage_used, user->storage_quota,
           get_quota_usage_percent(user));
    
    return QUOTA_OK;
}

size_t get_remaining_quota(user_metadata_t* user) {
    if (!user) return 0;
    
    if (user->storage_used >= user->storage_quota) {
        return 0;
    }
    
    return user->storage_quota - user->storage_used;
}

double get_quota_usage_percent(user_metadata_t* user) {
    if (!user || user->storage_quota == 0) return 0.0;
    
    return (double)user->storage_used / (double)user->storage_quota * 100.0;
}