#ifndef CLIENT_QUEUE_H
#define CLIENT_QUEUE_H

#include <pthread.h>

// Client queue structure
typedef struct {
    int *sockets;           // Array of socket descriptors
    int capacity;           // Maximum number of clients
    int count;              // Current number of clients in queue
    int front;              // Front index for dequeuing
    int rear;               // Rear index for enqueuing
    pthread_mutex_t mutex;  // Mutex for thread safety
    pthread_cond_t cond;    // Condition variable for waiting
} client_queue_t;

// Function prototypes
client_queue_t* client_queue_init(int capacity);
void client_queue_destroy(client_queue_t* queue);
int client_queue_enqueue(client_queue_t* queue, int socket);
int client_queue_dequeue(client_queue_t* queue);
int client_queue_is_empty(client_queue_t* queue);
int client_queue_is_full(client_queue_t* queue);

#endif // CLIENT_QUEUE_H
