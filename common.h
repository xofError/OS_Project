// common.h
#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/file.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>

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

#endif
