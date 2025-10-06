#ifndef SERVER_H
#define SERVER_H

#include <sys/socket.h>
#include <netinet/in.h>

// Server configuration
#define DEFAULT_PORT 8080
#define MAX_CLIENTS 100
#define THREAD_POOL_SIZE 10
#define BUFFER_SIZE 1024

// Function prototypes
int create_server_socket(int port);
void handle_client(int client_socket);

#endif // SERVER_H
