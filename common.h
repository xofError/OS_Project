// common.h
#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <sys/select.h>

#define PORT 8080
#define BUFFER_SIZE 1024
#define LOG_FILE "log.txt"

// Mutex to protect file locking within the same process
static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

void write_log(const char *process_name, const char *message) {
    pthread_mutex_lock(&log_mutex); // Protect threads in THIS process

    int fd = open(LOG_FILE, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd == -1) {
        perror("Log Open Error");
        pthread_mutex_unlock(&log_mutex);
        return;
    }

    // Lock file for INTER-PROCESS safety
    flock(fd, LOCK_EX);
    
    char buffer[BUFFER_SIZE];
    snprintf(buffer, sizeof(buffer), "[%s] %s\n", process_name, message);
    write(fd, buffer, strlen(buffer));

    flock(fd, LOCK_UN);
    close(fd);

    pthread_mutex_unlock(&log_mutex);
}
// Thread function for ONE user
void *simulate_user(void *arg) {
    char *config_file = (char*)arg;
    FILE *fp = fopen(config_file, "r");
    
    if (!fp) {
        char error_msg[BUFFER_SIZE];
        snprintf(error_msg, sizeof(error_msg), "Could not open config file: %s", config_file);
        write_log("CLIENT_ERROR", error_msg);
        return NULL;
    }

    char log_buf[BUFFER_SIZE];
    snprintf(log_buf, sizeof(log_buf), "User thread started, processing: %s", config_file);
    write_log("CLIENT_THREAD", log_buf);

    // IMPROVEMENT: Persistent connection (reused socket)
    int sock = -1;
    int connected = 0;

    char line[256];
    while (fgets(line, sizeof(line), fp)) {
        // Remove newline
        line[strcspn(line, "\n")] = 0;
        
        // Skip empty lines
        if (strlen(line) == 0) continue;

        char cmd[50], arg1[50], arg2[50];
        memset(cmd, 0, sizeof(cmd));
        memset(arg1, 0, sizeof(arg1));
        memset(arg2, 0, sizeof(arg2));
        
        sscanf(line, "%49s %49s %49s", cmd, arg1, arg2);
        if (strcmp(cmd, "Sleep") == 0) {
            float sleep_time = atof(arg1);
            snprintf(log_buf, sizeof(log_buf), "Sleeping for %.1f seconds", sleep_time);
            write_log("CLIENT_THREAD", log_buf);
            usleep((unsigned int)(sleep_time * 1000000)); // usleep takes microseconds
        } else {
            // NETWORK REQUEST
            char buffer[BUFFER_SIZE] = {0};

            // Establish connection if not already connected
            if (!connected) {
                sock = socket(AF_INET, SOCK_STREAM, 0);
                if (sock < 0) {
                    write_log("CLIENT_ERR", "Socket creation error");
                    continue;
                }

                struct sockaddr_in serv_addr;
                serv_addr.sin_family = AF_INET;
                serv_addr.sin_port = htons(PORT);
                
                if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
                    write_log("CLIENT_ERR", "Invalid address conversion");
                    close(sock);
                    sock = -1;
                    continue;
                }

                // Try to connect with retries
                int retry_count = 0;
                while (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
                    retry_count++;
                    if (retry_count > 5) {
                        write_log("CLIENT_ERR", "Connection failed after retries");
                        close(sock);
                        sock = -1;
                        break;
                    }
                    usleep(100000); // Wait 100ms before retry
                }

                if (sock >= 0) {
                    connected = 1;
                    char conn_msg[256];
                    snprintf(conn_msg, sizeof(conn_msg), "Connected to library (socket %d)", sock);
                    write_log("CLIENT_THREAD", conn_msg);
                }
            }

            if (connected && sock >= 0) {
                // Send Command on persistent connection
                send(sock, line, strlen(line), 0);
                
                memset(buffer, 0, sizeof(buffer));
                ssize_t bytes_read = read(sock, buffer, BUFFER_SIZE - 1);
                if (bytes_read > 0) {
                    buffer[bytes_read] = '\0';
                    snprintf(log_buf, sizeof(log_buf), "Command: %s | Response: %s", line, buffer);
                    write_log("CLIENT_THREAD", log_buf);
                } else if (bytes_read == 0) {
                    // Server closed connection, need to reconnect
                    write_log("CLIENT_THREAD", "Server closed connection, will reconnect");
                    close(sock);
                    sock = -1;
                    connected = 0;
                } else {
                    write_log("CLIENT_ERR", "Failed to read response");
                }
            }
        }
    }

    // Close persistent connection when done
    if (connected && sock >= 0) {
        close(sock);
        write_log("CLIENT_THREAD", "Connection closed");
    }

    fclose(fp);
    snprintf(log_buf, sizeof(log_buf), "User thread finished: %s", config_file);
    write_log("CLIENT_THREAD", log_buf);
    
    return NULL;
}

#endif
