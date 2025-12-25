// library.c
#include "common.h"

// --- Data Structures ---
#define MAX_USERS 100
#define MAX_BOOKS 100

typedef struct {
    char name[50];
    int id;
} User;

typedef struct {
    char title[50];
    int available; // 1 = Yes, 0 = No
} Book;

// Global Tables
User users[MAX_USERS];
int user_count = 0;
Book books[MAX_BOOKS];
int book_count = 0;

// READ-WRITE LOCKS
pthread_rwlock_t user_lock;
pthread_rwlock_t book_lock;

// --- Helper: Find Book Index ---
int find_book_index(char *title) {
    for (int i = 0; i < book_count; i++) {
        if (strcmp(books[i].title, title) == 0) return i;
    }
    return -1;
}

// --- Helper: Find User by Name ---
int find_user_by_name(char *name) {
    for (int i = 0; i < user_count; i++) {
        if (strcmp(users[i].name, name) == 0) return i;
    }
    return -1;
}

// --- Handler Thread ---
void *client_handler(void *socket_desc) {
    int sock = *(int*)socket_desc;
    free(socket_desc);
    char buffer[BUFFER_SIZE] = {0};
    char response[BUFFER_SIZE] = {0};

    // Read Request
    ssize_t bytes_read = read(sock, buffer, BUFFER_SIZE - 1);
    if (bytes_read < 0) {
        perror("Read error");
        close(sock);
        return NULL;
    }
    buffer[bytes_read] = '\0';
    
    char command[50], arg1[50], arg2[50];
    memset(arg1, 0, sizeof(arg1));
    memset(arg2, 0, sizeof(arg2));
    sscanf(buffer, "%s %s %s", command, arg1, arg2);

    char log_msg[BUFFER_SIZE];
    snprintf(log_msg, sizeof(log_msg), "Request received: %s", buffer);
    write_log("LIBRARY_THREAD", log_msg);

    // --- LOGIC: REGISTER (Write Lock Users) ---
    if (strcmp(command, "Register") == 0) {
        pthread_rwlock_wrlock(&user_lock); // Exclusive Lock
        
        // Check if user already exists
        if (find_user_by_name(arg1) != -1) {
            strcpy(response, "failure (user already exists)");
        } else {
            // Create new user
            int new_id = user_count + 1;
            strcpy(users[user_count].name, arg1);
            users[user_count].id = new_id;
            user_count++;
            
            sprintf(response, "success %d", new_id);
        }
        pthread_rwlock_unlock(&user_lock);
    } 
    // --- LOGIC: LEND (Read User, Write Book) ---
    else if (strcmp(command, "Lend") == 0) {
        // 1. Verify User (Read Lock)
        pthread_rwlock_rdlock(&user_lock);
        int user_idx = find_user_by_name(arg2);
        int user_exists = (user_idx != -1);
        pthread_rwlock_unlock(&user_lock);

        if (!user_exists) {
            strcpy(response, "failure (user not found)");
        } else {
            // 2. Lend Book (Write Lock)
            pthread_rwlock_wrlock(&book_lock);
            int idx = find_book_index(arg1);
            if (idx != -1 && books[idx].available) {
                books[idx].available = 0;
                strcpy(response, "success");
            } else {
                strcpy(response, "failure (book not available)");
            }
            pthread_rwlock_unlock(&book_lock);
        }
    }
    // --- LOGIC: RETURN (Write Book) ---
    else if (strcmp(command, "Return") == 0) {
        pthread_rwlock_wrlock(&book_lock);
        int idx = find_book_index(arg1);
        if (idx != -1) {
            books[idx].available = 1;
            strcpy(response, "success");
        } else {
            strcpy(response, "failure (book not found)");
        }
        pthread_rwlock_unlock(&book_lock);
    }
    // --- LOGIC: ADD BOOK (Write Book) ---
    else if (strcmp(command, "AddBook") == 0) {
        pthread_rwlock_wrlock(&book_lock);
        if (book_count < MAX_BOOKS) {
            strcpy(books[book_count].title, arg1);
            books[book_count].available = 1;
            book_count++;
            strcpy(response, "success");
        } else {
            strcpy(response, "failure (library full)");
        }
        pthread_rwlock_unlock(&book_lock);
    } else {
        strcpy(response, "failure (unknown command)");
    }

    // Log response
    snprintf(log_msg, sizeof(log_msg), "Response sent: %s", response);
    write_log("LIBRARY_THREAD", log_msg);

    send(sock, response, strlen(response), 0);
    close(sock);
    return NULL;
}

int main() {
    int server_fd, new_socket, *new_sock;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    write_log("LIBRARY", "Library process starting...");

    // Initialize RW Locks
    pthread_rwlock_init(&user_lock, NULL);
    pthread_rwlock_init(&book_lock, NULL);

    // Pre-populate a few books for testing
    strcpy(books[0].title, "Harry_Potter"); 
    books[0].available = 1;
    strcpy(books[1].title, "1984"); 
    books[1].available = 1;
    strcpy(books[2].title, "To_Kill_a_Mockingbird"); 
    books[2].available = 1;
    book_count = 3;

    // Socket Setup
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("Socket creation failed");
        return 1;
    }

    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt failed");
        close(server_fd);
        return 1;
    }
    
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        close(server_fd);
        return 1;
    }

    if (listen(server_fd, 5) < 0) {
        perror("Listen failed");
        close(server_fd);
        return 1;
    }

    write_log("LIBRARY", "Server listening on port 8080...");

    while (1) {
        new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);
        if (new_socket < 0) {
            perror("Accept failed");
            continue;
        }

        pthread_t handler_thread;
        new_sock = malloc(sizeof(int));
        *new_sock = new_socket;
        
        // Spawn Thread for this request
        if (pthread_create(&handler_thread, NULL, client_handler, (void*)new_sock) < 0) {
            perror("Could not create thread");
            free(new_sock);
            close(new_socket);
            continue;
        }

        // Detach thread so resources are freed automatically
        pthread_detach(handler_thread);
    }

    close(server_fd);
    pthread_rwlock_destroy(&user_lock);
    pthread_rwlock_destroy(&book_lock);
    return 0;
}
