#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

volatile sig_atomic_t signal_flag = 0;

void signal_handler(int sig) {
    signal_flag = 1;
}

int is_valid_treasure(const char *name) {
    return strcmp(name, ".") != 0 &&
           strcmp(name, "..") != 0 &&
           strstr(name, "log") == NULL;
}

int is_valid_hunt(const char *name) {
    return name[0] != '.' &&
           strcmp(name, "logged_hunt") != 0 &&
           strcmp(name, ".vscode") != 0;
}

void enumerate_hunts() {
    DIR *root = opendir(".");
    if (!root) {
        perror("Cannot open current directory");
        return;
    }

    struct dirent *item;
    while ((item = readdir(root)) != NULL) {
        if (item->d_type == DT_DIR &&
            is_valid_hunt(item->d_name)) {

            char folder_path[512];
            snprintf(folder_path, sizeof(folder_path), "%s", item->d_name);

            DIR *hunt = opendir(folder_path);
            if (!hunt) continue;

            int treasure_total = 0;
            struct dirent *entry;
            while ((entry = readdir(hunt)) != NULL) {
                if (entry->d_type == DT_REG && is_valid_treasure(entry->d_name)) {
                    treasure_total++;
                }
            }

            printf("Hunt: %s | Treasures: %d\n", item->d_name, treasure_total);
            closedir(hunt);
        }
    }

    closedir(root);
    fflush(stdout);
}

void enumerate_treasures(const char *hunt_name) {
    DIR *hunt_dir = opendir(hunt_name);
    if (!hunt_dir) {
        printf("Hunt '%s' not found.\n", hunt_name);
        return;
    }

    printf("Treasures in hunt '%s':\n", hunt_name);
    struct dirent *file;
    while ((file = readdir(hunt_dir)) != NULL) {
        if (file->d_type == DT_REG && is_valid_treasure(file->d_name)) {
            printf(" - %s\n", file->d_name);
        }
    }

    closedir(hunt_dir);
    fflush(stdout);
}

void show_treasure_content(const char *hunt, const char *treasure_id) {
    char full_path[256];
    snprintf(full_path, sizeof(full_path), "%s/%s", hunt, treasure_id);

    FILE *f = fopen(full_path, "r");
    if (!f) {
        printf("Could not open treasure '%s' from hunt '%s'.\n", treasure_id, hunt);
        return;
    }

    printf("Contents of treasure '%s' in hunt '%s':\n", treasure_id, hunt);
    char buffer[256];
    while (fgets(buffer, sizeof(buffer), f)) {
        printf("%s", buffer);
    }

    fclose(f);
    fflush(stdout);
}

void process_command(const char *cmd) {
    char action[64], param1[64], param2[64];
    int arg_count = sscanf(cmd, "%s %s %s", action, param1, param2);

    if (strcmp(action, "list_hunts") == 0) {
        enumerate_hunts();
    } else if (strcmp(action, "list_treasures") == 0 && arg_count >= 2) {
        enumerate_treasures(param1);
    } else if (strcmp(action, "view_treasure") == 0 && arg_count == 3) {
        show_treasure_content(param1, param2);
    } else if (strcmp(action, "stop") == 0) {
        printf("Shutting down treasure monitor...\n");
        fflush(stdout);
        exit(0);
    } else {
        printf("Invalid or unknown command: '%s'\n", cmd);
        fflush(stdout);
    }
}

int main() {
    struct sigaction sa;
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;

    if (sigaction(SIGUSR1, &sa, NULL) == -1) {
        perror("sigaction");
        return 1;
    }

    while (1) {
        if (signal_flag) {
            signal_flag = 0;

            FILE *cmd_file = fopen("monitor_command.cmd", "r");
            if (!cmd_file) {
                perror("Failed to open command file");
                continue;
            }

            char command[256];
            if (fgets(command, sizeof(command), cmd_file)) {
                command[strcspn(command, "\n")] = '\0';
                process_command(command);
            }

            fclose(cmd_file);
        }

        usleep(100000);
    }

    return 0;
}
