#include "server.h"
#include "auth.h"
#include "command_parser.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

int create_server_socket(int port) {
    int sockfd;
    struct sockaddr_in server_addr;
    int opt = 1;
    
    // Create socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        perror("socket");
        return -1;
    }
    
    // Set socket options
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        perror("setsockopt");
        close(sockfd);
        return -1;
    }
    
    // Bind socket
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);
    
    if (bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind");
        close(sockfd);
        return -1;
    }
    
    // Listen for connections
    if (listen(sockfd, MAX_CLIENTS) == -1) {
        perror("listen");
        close(sockfd);
        return -1;
    }
    
    return sockfd;
}

void handle_client(int client_socket) {
    char buffer[BUFFER_SIZE];
    user_session_t session = {0};
    session.authenticated = 0;
    
    // Send welcome message
    send_response(client_socket, "Welcome to OS Mini Server! Type HELP for commands.\n");
    
    while (1) {
        memset(buffer, 0, sizeof(buffer));
        
        // Read command from client
        ssize_t bytes_read = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
        
        if (bytes_read <= 0) {
            if (bytes_read == 0) {
                printf("Client disconnected\n");
            } else {
                perror("recv");
            }
            break;
        }
        
        // Remove newline characters
        buffer[strcspn(buffer, "\r\n")] = 0;
        
        printf("Received command: %s\n", buffer);
        
        // Parse and handle command
        command_type_t cmd_type = parse_command(buffer);
        
        switch (cmd_type) {
            case CMD_LOGIN: {
                // Parse login command: LOGIN username password
                char username[64], password[64];
                if (sscanf(buffer, "LOGIN %63s %63s", username, password) == 2) {
                    if (auth_login(username, password, &session) == 0) {
                        send_response(client_socket, "LOGIN_SUCCESS: Authentication successful\n");
                    } else {
                        send_response(client_socket, "LOGIN_ERROR: Invalid credentials\n");
                    }
                } else {
                    send_response(client_socket, "LOGIN_ERROR: Usage: LOGIN username password\n");
                }
                break;
            }
            
            case CMD_SIGNUP: {
                // Parse signup command: SIGNUP username password
                char username[64], password[64];
                if (sscanf(buffer, "SIGNUP %63s %63s", username, password) == 2) {
                    if (auth_signup(username, password) == 0) {
                        send_response(client_socket, "SIGNUP_SUCCESS: Account created successfully\n");
                    } else {
                        send_response(client_socket, "SIGNUP_ERROR: Failed to create account (username may exist)\n");
                    }
                } else {
                    send_response(client_socket, "SIGNUP_ERROR: Usage: SIGNUP username password\n");
                }
                break;
            }
            
            case CMD_LOGOUT: {
                if (session.authenticated) {
                    auth_logout(&session);
                    send_response(client_socket, "LOGOUT_SUCCESS: Logged out successfully\n");
                } else {
                    send_response(client_socket, "LOGOUT_ERROR: Not logged in\n");
                }
                break;
            }
            
            case CMD_QUIT: {
                send_response(client_socket, "GOODBYE: Connection closed\n");
                return; // Exit the function to close connection
            }
            
            case CMD_UPLOAD:
            case CMD_DOWNLOAD:
            case CMD_DELETE:
            case CMD_LIST: {
                if (!session.authenticated) {
                    send_response(client_socket, "AUTH_ERROR: Please login first\n");
                    break;
                }
                
                // Handle authenticated commands
                if (handle_authenticated_command(buffer, &session, client_socket) != 0) {
                    send_response(client_socket, "COMMAND_ERROR: Failed to process command\n");
                }
                break;
            }
            
            default: {
                if (strcasecmp(buffer, "HELP") == 0) {
                    send_response(client_socket, 
                        "Available commands:\n"
                        "  SIGNUP username password - Create new account\n"
                        "  LOGIN username password - Login to account\n"
                        "  LOGOUT - Logout from account\n"
                        "  UPLOAD filename - Upload file (requires login)\n"
                        "  UPLOAD_FILE /path/to/file - Upload file from server disk\n"  // NEW
                        "  DOWNLOAD filename - Download file (requires login)\n"
                        "  DELETE filename - Delete file (requires login)\n"
                        "  LIST - List files (requires login)\n"
                        "  QUIT - Disconnect\n"
                        "  HELP - Show this help\n");
                } else {
                    send_response(client_socket, "ERROR: Unknown command. Type HELP for available commands.\n");
                }
                break;
            }
        }
    }
}
