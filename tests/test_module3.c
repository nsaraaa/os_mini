// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// #include <assert.h>
// #include <sys/stat.h>
// #include <unistd.h>

// #include "metadata.h"
// #include "quota.h"
// #include "file_ops.h"
// #include "persistence.h"
// #include "locking.h"

// #define TEST_USER "testuser"
// #define TEST_FILE1 "test1.txt"
// #define TEST_FILE2 "test2.txt"
// #define TEST_FILE3 "test3.txt"

// void print_test_header(const char* test_name) {
//     printf("\n===========================================\n");
//     printf("TEST: %s\n", test_name);
//     printf("===========================================\n");
// }

// void print_test_result(const char* test_name, int passed) {
//     if (passed) {
//         printf("✓ %s PASSED\n", test_name);
//     } else {
//         printf("✗ %s FAILED\n", test_name);
//     }
// }

// void test_metadata_init() {
//     print_test_header("Metadata Initialization");
    
//     int result = init_metadata();
//     assert(result == 0);
    
//     print_test_result("Metadata Init", 1);
// }

// void test_create_user() {
//     print_test_header("Create User");
    
//     user_metadata_t* user = create_user(TEST_USER, "hashed_password");
//     assert(user != NULL);
//     assert(strcmp(user->username, TEST_USER) == 0);
//     assert(user->storage_used == 0);
//     assert(user->storage_quota == DEFAULT_QUOTA_BYTES);
//     assert(user->file_count == 0);
    
//     print_test_result("Create User", 1);
// }

// void test_get_user() {
//     print_test_header("Get User");
    
//     user_metadata_t* user = get_user(TEST_USER);
//     assert(user != NULL);
//     assert(strcmp(user->username, TEST_USER) == 0);
    
//     print_test_result("Get User", 1);
// }

// void test_quota_system() {
//     print_test_header("Quota System");
    
//     user_metadata_t* user = get_user(TEST_USER);
//     assert(user != NULL);
    
//     // Test quota check
//     size_t test_size = 1000;
//     assert(check_quota(user, test_size) == true);
    
//     // Test remaining quota
//     size_t remaining = get_remaining_quota(user);
//     assert(remaining == DEFAULT_QUOTA_BYTES);
    
//     // Test quota update
//     int result = update_quota_on_upload(user, test_size);
//     assert(result == QUOTA_OK);
//     assert(user->storage_used == test_size);
    
//     remaining = get_remaining_quota(user);
//     assert(remaining == DEFAULT_QUOTA_BYTES - test_size);
    
//     // Test quota exceeded
//     size_t huge_size = DEFAULT_QUOTA_BYTES;
//     assert(check_quota(user, huge_size) == false);
    
//     // Test quota reduction
//     result = update_quota_on_delete(user, test_size);
//     assert(result == QUOTA_OK);
//     assert(user->storage_used == 0);
    
//     print_test_result("Quota System", 1);
// }

// void test_file_operations() {
//     print_test_header("File Operations");
    
//     // Initialize file storage
//     int result = init_file_storage();
//     assert(result == 0);
    
//     // Create user directory
//     result = create_user_directory(TEST_USER);
//     assert(result == 0);
    
//     // Test file upload
//     const char* test_data = "Hello, this is test data for Module 3!";
//     size_t data_size = strlen(test_data);
    
//     result = upload_file(TEST_USER, TEST_FILE1, test_data, data_size);
//     assert(result == FILE_OP_SUCCESS);
    
//     // Verify file exists
//     assert(file_exists(TEST_USER, TEST_FILE1) == true);
    
//     // Test file download
//     void* downloaded_data = NULL;
//     size_t downloaded_size = 0;
//     result = download_file(TEST_USER, TEST_FILE1, &downloaded_data, &downloaded_size);
//     assert(result == FILE_OP_SUCCESS);
//     assert(downloaded_size == data_size);
//     assert(memcmp(downloaded_data, test_data, data_size) == 0);
//     free(downloaded_data);
    
//     // Test file list
//     char list_buffer[4096];
//     result = list_files(TEST_USER, list_buffer, sizeof(list_buffer));
//     assert(result == FILE_OP_SUCCESS);
//     assert(strstr(list_buffer, TEST_FILE1) != NULL);
    
//     // Test file delete
//     result = delete_file(TEST_USER, TEST_FILE1);
//     assert(result == FILE_OP_SUCCESS);
//     assert(file_exists(TEST_USER, TEST_FILE1) == false);
    
//     print_test_result("File Operations", 1);
// }

// void test_metadata_persistence() {
//     print_test_header("Metadata Persistence");
    
//     user_metadata_t* user = get_user(TEST_USER);
//     assert(user != NULL);
    
//     // Add some files to metadata
//     add_file_to_user(user, TEST_FILE1, 500);
//     add_file_to_user(user, TEST_FILE2, 1000);
//     add_file_to_user(user, TEST_FILE3, 1500);
    
//     assert(user->file_count == 3);
//     assert(user->storage_used == 3000);
    
//     // Save metadata
//     int result = save_user_metadata(user);
//     assert(result == 0);
    
//     // Delete user from memory
//     delete_user(TEST_USER);
//     assert(get_user(TEST_USER) == NULL);
    
//     // Load metadata back
//     user_metadata_t* loaded_user = load_user_metadata(TEST_USER);
//     assert(loaded_user != NULL);
//     assert(strcmp(loaded_user->username, TEST_USER) == 0);
//     assert(loaded_user->file_count == 3);
//     assert(loaded_user->storage_used == 3000);
    
//     // Verify files
//     file_metadata_t* file = get_file_metadata(loaded_user, TEST_FILE1);
//     assert(file != NULL);
//     assert(file->size == 500);
    
//     // Cleanup loaded user
//     file_metadata_t* current_file = loaded_user->files;
//     while (current_file) {
//         file_metadata_t* next = current_file->next;
//         free(current_file);
//         current_file = next;
//     }
//     free(loaded_user);
    
//     print_test_result("Metadata Persistence", 1);
// }

// void test_concurrent_operations() {
//     print_test_header("Concurrent Operations (Lock Testing)");
    
//     // Initialize locking system
//     int result = lock_manager_init();
//     assert(result == 0);
    
//     // Test lock/unlock
//     lock_user(TEST_USER);
//     printf("Lock acquired for %s\n", TEST_USER);
    
//     // Simulate work
//     sleep(1);
    
//     unlock_user(TEST_USER);
//     printf("Lock released for %s\n", TEST_USER);
    
//     // Test try_lock
//     bool locked = try_lock_user(TEST_USER);
//     assert(locked == true);
//     unlock_user(TEST_USER);
    
//     lock_manager_cleanup();
    
//     print_test_result("Concurrent Operations", 1);
// }

// void test_quota_enforcement() {
//     print_test_header("Quota Enforcement");
    
//     // Recreate user with default quota
//     delete_user(TEST_USER);
//     user_metadata_t* user = create_user(TEST_USER, "hashed_password");
//     assert(user != NULL);
    
//     create_user_directory(TEST_USER);
    
//     // Try to upload a file that exceeds quota
//     size_t large_size = DEFAULT_QUOTA_BYTES + 1000;
//     char* large_data = malloc(large_size);
//     memset(large_data, 'A', large_size);
    
//     int result = upload_file(TEST_USER, "large_file.txt", large_data, large_size);
//     assert(result == FILE_OP_QUOTA_EXCEEDED);
    
//     free(large_data);
    
//     // Upload a file within quota
//     const char* small_data = "Small file";
//     result = upload_file(TEST_USER, TEST_FILE1, small_data, strlen(small_data));
//     assert(result == FILE_OP_SUCCESS);
    
//     // Verify quota updated
//     user = get_user(TEST_USER);
//     assert(user->storage_used == strlen(small_data));
    
//     print_test_result("Quota Enforcement", 1);
// }

// void test_file_overwrite() {
//     print_test_header("File Overwrite");
    
//     user_metadata_t* user = get_user(TEST_USER);
//     size_t initial_usage = user->storage_used;
    
//     // Upload first version
//     const char* data1 = "First version";
//     int result = upload_file(TEST_USER, TEST_FILE2, data1, strlen(data1));
//     assert(result == FILE_OP_SUCCESS);
    
//     user = get_user(TEST_USER);
//     size_t after_first = user->storage_used;
//     assert(after_first == initial_usage + strlen(data1));
    
//     // Overwrite with larger file
//     const char* data2 = "Second version with more data";
//     result = upload_file(TEST_USER, TEST_FILE2, data2, strlen(data2));
//     assert(result == FILE_OP_SUCCESS);
    
//     user = get_user(TEST_USER);
//     size_t after_second = user->storage_used;
//     assert(after_second == initial_usage + strlen(data2));
    
//     // Verify file content
//     void* downloaded = NULL;
//     size_t downloaded_size = 0;
//     result = download_file(TEST_USER, TEST_FILE2, &downloaded, &downloaded_size);
//     assert(result == FILE_OP_SUCCESS);
//     assert(downloaded_size == strlen(data2));
//     assert(memcmp(downloaded, data2, strlen(data2)) == 0);
//     free(downloaded);
    
//     print_test_result("File Overwrite", 1);
// }

// void test_error_handling() {
//     print_test_header("Error Handling");
    
//     // Test download non-existent file
//     void* data = NULL;
//     size_t size = 0;
//     int result = download_file(TEST_USER, "nonexistent.txt", &data, &size);
//     assert(result == FILE_OP_NOT_FOUND);
    
//     // Test delete non-existent file
//     result = delete_file(TEST_USER, "nonexistent.txt");
//     assert(result == FILE_OP_NOT_FOUND);
    
//     // Test operations with NULL parameters
//     result = upload_file(NULL, TEST_FILE1, "data", 4);
//     assert(result == FILE_OP_INVALID_PARAM);
    
//     result = download_file(TEST_USER, NULL, &data, &size);
//     assert(result == FILE_OP_INVALID_PARAM);
    
//     print_test_result("Error Handling", 1);
// }

// void cleanup_test_environment() {
//     print_test_header("Cleanup");
    
//     // Remove test files
//     char cmd[512];
//     snprintf(cmd, sizeof(cmd), "rm -rf server_storage");
//     system(cmd);
    
//     printf("Test environment cleaned up\n");
// }

// int main() {
//     printf("\n");
//     printf("╔═══════════════════════════════════════════╗\n");
//     printf("║   MODULE 3 COMPREHENSIVE TEST SUITE      ║\n");
//     printf("╚═══════════════════════════════════════════╝\n");
    
//     // Run all tests
//     test_metadata_init();
//     test_create_user();
//     test_get_user();
//     test_quota_system();
//     test_file_operations();
//     test_metadata_persistence();
//     test_concurrent_operations();
//     test_quota_enforcement();
//     test_file_overwrite();
//     test_error_handling();
    
//     // Cleanup
//     cleanup_metadata();
//     cleanup_test_environment();
    
//     printf("\n");
//     printf("╔═══════════════════════════════════════════╗\n");
//     printf("║   ALL TESTS PASSED SUCCESSFULLY! ✓       ║\n");
//     printf("╚═══════════════════════════════════════════╝\n");
//     printf("\n");
    
//     return 0;
// }