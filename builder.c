// builder.c
#include "common.h"
#include <sys/wait.h>
#include <signal.h>

int main() {
    // 1. Reset Log File
    FILE *fp = fopen(LOG_FILE, "w");
    if (fp) {
        fprintf(fp, "=== Library Management System Simulation ===\n\n");
        fclose(fp);
    }

    write_log("BUILDER", "Simulation Started.");

    // 2. Fork Library Process
    pid_t lib_pid = fork();
    if (lib_pid == 0) {
        // Child 1: Run Library
        execl("./library", "library", (char *)NULL);
        perror("Library exec failed");
        exit(1);
    } else if (lib_pid < 0) {
        perror("Library fork failed");
        exit(1);
    }

    // Give library time to bind port and start listening
    sleep(2);

    // 3. Fork Client Process
    pid_t client_pid = fork();
    if (client_pid == 0) {
        // Child 2: Run Client with user files
        execl("./client", "client", "user1.txt", "user2.txt", (char *)NULL);
        perror("Client exec failed");
        exit(1);
    } else if (client_pid < 0) {
        perror("Client fork failed");
        exit(1);
    }

    // 4. Wait for client to finish
    int client_status;
    waitpid(client_pid, &client_status, 0);
    
    if (WIFEXITED(client_status)) {
        write_log("BUILDER", "Client process exited successfully.");
    } else {
        write_log("BUILDER", "Client process terminated abnormally.");
    }

    // 5. Give library a moment to finish pending requests, then kill it
    sleep(1);
    kill(lib_pid, SIGTERM);
    
    int lib_status;
    waitpid(lib_pid, &lib_status, 0);
    
    if (WIFEXITED(lib_status)) {
        write_log("BUILDER", "Library process exited successfully.");
    } else {
        write_log("BUILDER", "Library process terminated.");
    }

    write_log("BUILDER", "Simulation Finished.");
    printf("Simulation complete. Check log.txt for details.\n");
    
    return 0;
}
