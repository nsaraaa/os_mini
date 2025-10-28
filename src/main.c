#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <signal.h>
#include <errno.h>

#include "server.h"
#include "client_queue.h"
#include "auth.h"
#include "command_parser.h"
#include "worker_pool.h"
#include "locking.h"
#include "module3_integration.h"  // ADD MODULE 3

#define DEFAULT_PORT 8080
#define MAX_CLIENTS 100
#define THREAD_POOL_SIZE 10

// Global server state
static int server_running = 1;
static int server_socket = -1;
static pthread_t client_threads[THREAD_POOL_SIZE];
static client_queue_t *client_queue;

// Signal handler for graceful shutdown
void signal_handler(int sig) {
    printf("\nReceived signal %d, shutting down server...\n", sig);
    server_running = 0;
    
    if (server_socket != -1) {
        close(server_socket);
    }
}

// Client thread function
void* client_thread_func(void* arg) {
    int thread_id = *(int*)arg;
    printf("Client thread %d started\n", thread_id);
    
    while (server_running) {
        int client_socket = client_queue_dequeue(client_queue);
        
        if (client_socket == -1) {
            // Timeout or error, continue loop
            continue;
        }
        
        printf("Thread %d handling client socket %d\n", thread_id, client_socket);
        
        // Handle client communication
        handle_client(client_socket);
        
        close(client_socket);
        printf("Thread %d closed client socket %d\n", thread_id, client_socket);
    }
    
    printf("Client thread %d exiting\n", thread_id);
    return NULL;
}

int main(int argc, char* argv[]) {
    int port = DEFAULT_PORT;
    
    // Parse command line arguments
    if (argc > 1) {
        port = atoi(argv[1]);
        if (port <= 0 || port > 65535) {
            fprintf(stderr, "Invalid port number: %s\n", argv[1]);
            exit(1);
        }
    }
    
    printf("Starting OS Mini Server on port %d\n", port);
    printf("===========================================\n");
    
    // Set up signal handler
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // Initialize client queue
    client_queue = client_queue_init(MAX_CLIENTS);
    if (!client_queue) {
        fprintf(stderr, "Failed to initialize client queue\n");
        exit(1);
    }
    
    // Initialize authentication system (Module 1)
    if (auth_init() != 0) {
        fprintf(stderr, "Failed to initialize authentication system\n");
        exit(1);
    }
    
    // Initialize locking system (Module 2)
    if (lock_manager_init() != 0) {
        fprintf(stderr, "Failed to initialize locking system\n");
        exit(1);
    }

    // Initialize task system (Module 2)
    init_task_system();
    
    // INITIALIZE MODULE 3 - File Storage System
    if (module3_init() != 0) {
        fprintf(stderr, "Failed to initialize Module 3\n");
        shutdown_task_system();
        exit(1);
    }
    
    // Create server socket
    server_socket = create_server_socket(port);
    if (server_socket == -1) {
        fprintf(stderr, "Failed to create server socket\n");
        module3_cleanup();
        shutdown_task_system();
        exit(1);
    }
    
    printf("Server socket created successfully\n");
    
    // Create client thread pool
    for (int i = 0; i < THREAD_POOL_SIZE; i++) {
        int *thread_id = malloc(sizeof(int));
        *thread_id = i;
        
        if (pthread_create(&client_threads[i], NULL, client_thread_func, thread_id) != 0) {
            fprintf(stderr, "Failed to create client thread %d\n", i);
            // Cleanup on failure
            for (int j = 0; j < i; j++) {
                pthread_cancel(client_threads[j]);
            }
            module3_cleanup();
            shutdown_task_system();
            exit(1);
        }
    }
    
    printf("Thread pool created with %d threads\n", THREAD_POOL_SIZE);
    printf("===========================================\n");
    printf("Server listening for connections...\n");
    printf("===========================================\n");
    
    // Main accept loop
    while (server_running) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        
        int client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_len);
        
        if (client_socket == -1) {
            if (errno == EINTR) {
                // Interrupted by signal, continue loop
                continue;
            }
            perror("accept");
            continue;
        }
        
        printf("Accepted connection from %s:%d\n", 
               inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
        
        // Add client to queue
        if (client_queue_enqueue(client_queue, client_socket) != 0) {
            fprintf(stderr, "Failed to enqueue client socket %d\n", client_socket);
            close(client_socket);
        }
    }
    
    printf("\n===========================================\n");
    printf("Shutting down server...\n");
    printf("===========================================\n");
    
    // Wait for all client threads to finish
    for (int i = 0; i < THREAD_POOL_SIZE; i++) {
        pthread_join(client_threads[i], NULL);
    }
    
    // Cleanup in reverse order of initialization
    module3_cleanup();          // NEW - Module 3 cleanup
    shutdown_task_system();     // Module 2 task system
    lock_manager_cleanup();     // Module 2 locking
    client_queue_destroy(client_queue);
    auth_cleanup();             // Module 1 auth
    
    if (server_socket != -1) {
        close(server_socket);
    }
    
    printf("Server shutdown complete\n");
    return 0;
}