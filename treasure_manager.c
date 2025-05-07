#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <dirent.h>

volatile sig_atomic_t got_command = 0;

void sigusr1_handler(int sig) {
    got_command = 1;
}

void list_hunts() {
    DIR *d = opendir(".");
    if (!d) {
        perror("Could not open directory");
        return;
    }
    struct dirent *entry;
    while ((entry = readdir(d))) {
        if (entry->d_type == DT_DIR && strcmp(entry->d_name, ".") && strcmp(entry->d_name, "..")) {
            int count = 0;
            char subdir[256];
            snprintf(subdir, sizeof(subdir), "%s", entry->d_name);
            DIR *sub = opendir(subdir);
            if (sub) {
                struct dirent *e;
                while ((e = readdir(sub))) {
                    if (strcmp(e->d_name, ".") && strcmp(e->d_name, "..")) {
                        count++;
                    }
                }
                closedir(sub);
            }
            printf("Hunt: %s - Treasures: %d\n", entry->d_name, count);
        }
    }
    closedir(d);
}

void list_treasures(const char *hunt) {
    DIR *d = opendir(hunt);
    if (!d) {
        printf("Hunt '%s' not found.\n", hunt);
        return;
    }

    printf("Treasures in '%s':\n", hunt);
    struct dirent *entry;
    while ((entry = readdir(d))) {
        if (strcmp(entry->d_name, ".") && strcmp(entry->d_name, "..")) {
            printf("- %s\n", entry->d_name);
        }
    }
    closedir(d);
}

void view_treasure(const char *hunt, const char *id) {
    char path[256];
    snprintf(path, sizeof(path), "%s/%s", hunt, id);
    FILE *f = fopen(path, "r");
    if (!f) {
        printf("Treasure '%s' not found in hunt '%s'\n", id, hunt);
        return;
    }

    printf("Details of '%s' in '%s':\n", id, hunt);
    char line[128];
    while (fgets(line, sizeof(line), f)) {
        printf("%s", line);
    }
    fclose(f);
}

void handle_command(const char *cmd) {
    char op[32], arg1[64], arg2[64];
    int parts = sscanf(cmd, "%s %s %s", op, arg1, arg2);

    if (strcmp(op, "list_hunts") == 0) {
        list_hunts();
    } else if (strcmp(op, "list_treasures") == 0 && parts >= 2) {
        list_treasures(arg1);
    } else if (strcmp(op, "view_treasure") == 0 && parts == 3) {
        view_treasure(arg1, arg2);
    } else if (strcmp(op, "stop") == 0) {
        printf("Shutting down monitor...\n");
        usleep(500000);  // 0.5 sec
        exit(0);
    } else {
        printf("Invalid or malformed command: %s\n", cmd);
    }
}

int main() {
    struct sigaction sa;
    sa.sa_handler = sigusr1_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGUSR1, &sa, NULL);

    printf("Monitor running with PID %d\n", getpid());

    while (1) {
        if (got_command) {
            got_command = 0;

            FILE *f = fopen("monitor_command.cmd", "r");
            if (!f) {
                perror("Could not read command file");
                continue;
            }

            char line[256];
            if (fgets(line, sizeof(line), f)) {
                line[strcspn(line, "\n")] = 0;
                handle_command(line);
            }
            fclose(f);
        }
        usleep(100000);
    }

    return 0;
}
