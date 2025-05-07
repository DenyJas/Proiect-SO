#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>

int running = 1;

void handle_usr1(int sig) {
    FILE *f = fopen("monitor_command.cmd", "r");
    if (!f) {
        perror("Failed to open command file");
        return;
    }

    char command[128];
    if (fgets(command, sizeof(command), f)) {
        command[strcspn(command, "\n")] = 0; 
        printf("Monitor received command: %s\n", command);

        if (strcmp(command, "stop") == 0) {
            printf("Monitor stopping...\n");
            running = 0;
        }
    }
    fclose(f);
}

int main() {
    struct sigaction sa;
    sa.sa_handler = handle_usr1;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGUSR1, &sa, NULL);

    printf("Monitor started with PID %d. Waiting for commands...\n", getpid());

    while (running) {
        pause(); 
    }

    printf("Monitor exiting.\n");
    return 0;
}
