#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>

typedef struct {
    char player[50];
    int score;
} UserScore;

int search_user(UserScore *list, int total, const char *name) {
    for (int i = 0; i < total; i++) {
        if (strcmp(list[i].player, name) == 0)
            return i;
    }
    return -1;
}

int is_valid_treasure_file(const char *name) {
    return name[0] != '.' &&
           strstr(name, "log") == NULL &&
           strcmp(name, "--add") != 0;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <hunt_folder>\n", argv[0]);
        return 1;
    }

    DIR *folder = opendir(argv[1]);
    if (!folder) {
        perror("Couldn't open hunt folder");
        return 1;
    }

    struct dirent *entry;
    UserScore users[100];
    int count = 0;

    while ((entry = readdir(folder)) != NULL) {
        if (entry->d_type != DT_REG || !is_valid_treasure_file(entry->d_name))
            continue;

        char fullpath[512];
        snprintf(fullpath, sizeof(fullpath), "%s/%s", argv[1], entry->d_name);

        FILE *file = fopen(fullpath, "r");
        if (!file)
            continue;

        char id[20], name[50], lat[20], lon[20], clue[50];
        int value;

        fscanf(file, "%s\n%s\n%s\n%s\n%s\n%d", id, name, lat, lon, clue, &value);
        fclose(file);

        int idx = search_user(users, count, name);
        if (idx == -1) {
            strcpy(users[count].player, name);
            users[count].score = value;
            count++;
        } else {
            users[idx].score += value;
        }
    }

    closedir(folder);

    for (int i = 0; i < count; i++) {
        printf("%s: %d\n", users[i].player, users[i].score);
    }

    return 0;
}
