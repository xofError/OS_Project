// client.c
#include "common.h"




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
