// client.c
#include "common.h"

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
        
        sscanf(line, "%s %s %s", cmd, arg1, arg2);

        if (strcmp(cmd, "Sleep") == 0) {
            float sleep_time = atof(arg1);
            snprintf(log_buf, sizeof(log_buf), "Sleeping for %.1f seconds", sleep_time);
            write_log("CLIENT_THREAD", log_buf);
            usleep((useconds_t)(sleep_time * 1000000)); // usleep takes microseconds
        } else {
            // NETWORK REQUEST
            int sock = 0;
            struct sockaddr_in serv_addr;
            char buffer[BUFFER_SIZE] = {0};

            if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
                write_log("CLIENT_ERR", "Socket creation error");
                continue;
            }

            serv_addr.sin_family = AF_INET;
            serv_addr.sin_port = htons(PORT);
            
            // Convert IPv4 and IPv6 addresses from text to binary form
            if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
                write_log("CLIENT_ERR", "Invalid address conversion");
                close(sock);
                continue;
            }

            // Try to connect with retries
            int retry_count = 0;
            while (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
                retry_count++;
                if (retry_count > 5) {
                    write_log("CLIENT_ERR", "Connection failed after retries");
                    close(sock);
                    break;
                }
                usleep(100000); // Wait 100ms before retry
            }

            if (retry_count <= 5) {
                // Send Command (e.g., "Lend Harry_Potter user1")
                send(sock, line, strlen(line), 0);
                
                memset(buffer, 0, sizeof(buffer));
                ssize_t bytes_read = read(sock, buffer, BUFFER_SIZE - 1);
                if (bytes_read > 0) {
                    buffer[bytes_read] = '\0';
                    snprintf(log_buf, sizeof(log_buf), "Command: %s | Response: %s", line, buffer);
                    write_log("CLIENT_THREAD", log_buf);
                } else {
                    write_log("CLIENT_ERR", "Failed to read response");
                }
                
                close(sock);
            }
        }
    }

    fclose(fp);
    snprintf(log_buf, sizeof(log_buf), "User thread finished: %s", config_file);
    write_log("CLIENT_THREAD", log_buf);
    
    return NULL;
}

int main(int argc, char *argv[]) {
    // Expecting: ./client <file1> <file2> ...
    if (argc < 2) {
        printf("Usage: %s <file1> <file2> ...\n", argv[0]);
        return 1;
    }

    int num_users = argc - 1;
    pthread_t threads[num_users];

    char log_msg[BUFFER_SIZE];
    snprintf(log_msg, sizeof(log_msg), "Starting %d client threads...", num_users);
    write_log("CLIENT_PROC", log_msg);

    // Create a thread for each user file 
    for (int i = 0; i < num_users; i++) {
        pthread_create(&threads[i], NULL, simulate_user, (void*)argv[i+1]);
    }

    // Wait for all users to finish
    for (int i = 0; i < num_users; i++) {
        pthread_join(threads[i], NULL);
    }

    write_log("CLIENT_PROC", "All user threads finished.");
    return 0;
}
