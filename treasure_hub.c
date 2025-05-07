// treasure_hub.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <signal.h>

pid_t monitor_pid = -1;
int monitor_running = 0;
int monitor_exiting = 0;

void sigchld_handler(int sig) {
    int status;
    pid_t pid = waitpid(monitor_pid, &status, WNOHANG);
    if (pid > 0) {
        printf("Monitor terminated with status %d\n", WEXITSTATUS(status));
        monitor_running = 0;
        monitor_exiting = 0;
        monitor_pid = -1;
    }
}

void send_command(const char *cmd) {
    FILE *f = fopen("monitor_command.cmd", "w");
    if (!f) {
        perror("Failed to write command file");
        return;
    }
    fprintf(f, "%s\n", cmd);
    fclose(f);
    kill(monitor_pid, SIGUSR1);
}

int main() {
    struct sigaction sa;
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGCHLD, &sa, NULL);

    char input[128];

    while (1) {
        printf("hub> ");
        fflush(stdout);

        if (!fgets(input, sizeof(input), stdin)) break;

        input[strcspn(input, "\n")] = 0;

        if (strcmp(input, "start_monitor") == 0) {
            if (monitor_running) {
                printf("Monitor already running (PID %d)\n", monitor_pid);
                continue;
            }
            monitor_pid = fork();
            if (monitor_pid == 0) {
                execl("./treasure_monitor", "treasure_monitor", NULL);
                perror("Failed to start monitor");
                exit(1);
            } else if (monitor_pid > 0) {
                monitor_running = 1;
                printf("Started monitor with PID %d\n", monitor_pid);
            }

        } else if (strcmp(input, "list_hunts") == 0 ||
                   strncmp(input, "list_treasures", 14) == 0 ||
                   strncmp(input, "view_treasure", 13) == 0) {
            if (!monitor_running) {
                printf("Monitor is not running.\n");
                continue;
            }
            if (monitor_exiting) {
                printf("Monitor is exiting. Please wait...\n");
                continue;
            }
            send_command(input);

        } else if (strcmp(input, "stop_monitor") == 0) {
            if (!monitor_running) {
                printf("Monitor is not running.\n");
                continue;
            }
            send_command("stop");
            monitor_exiting = 1;

        } else if (strcmp(input, "exit") == 0) {
            if (monitor_running) {
                printf("Error: Monitor still running. Use stop_monitor first.\n");
                continue;
            }
            break;

        } else {
            printf("Unknown command: %s\n", input);
        }
    }

    return 0;
}
