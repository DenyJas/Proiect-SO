#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <time.h>

typedef struct comoara {
    char ID[10];
    char nume[20];
    double lat;
    double lng;
    char indiciu[50];
    int valoare;
} Comoara_t;

void add(char *huntID, Comoara_t comoara) {
    mkdir(huntID, 0755); // Dacă există deja, nu e problemă

    char filename[100];
    snprintf(filename, sizeof(filename), "%s/%s", huntID, comoara.ID);

    FILE *f = fopen(filename, "w+");
    if (!f) {
        perror("Eroare la deschiderea fișierului");
        return;
    }

    fprintf(f, "%s\n%s\n%lf\n%lf\n%s\n%d\n",
            comoara.ID,
            comoara.nume,
            comoara.lat,
            comoara.lng,
            comoara.indiciu,
            comoara.valoare);

    fclose(f);
    printf("Comoara '%s' a fost adăugată în vânătoarea '%s'.\n", comoara.ID, huntID);
}

int list(char *huntID) {
    DIR *dir = opendir(huntID);
    if (!dir) {
        perror("Nu am putut deschide directorul");
        return 1;
    }

    struct dirent *entry;
    struct stat fileStat;
    long long totalSize = 0;
    time_t lastModTime = 0;

    printf("Hunt: %s\n", huntID);

    while ((entry = readdir(dir)) != NULL) {
        // Verifică că nu sunt directoare speciale "." și ".."
        if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            char filepath[300];
            snprintf(filepath, sizeof(filepath), "%s/%s", huntID, entry->d_name);

            // Verifică dacă fișierul este un fișier regulat
            if (stat(filepath, &fileStat) == 0 && S_ISREG(fileStat.st_mode)) {
                totalSize += fileStat.st_size;
                if (fileStat.st_mtime > lastModTime)
                    lastModTime = fileStat.st_mtime;
            }
        }
    }

    printf("Total file size: %lld bytes\n", totalSize);
    printf("Last modified: %s", ctime(&lastModTime));

    rewinddir(dir);
    printf("Treasures:\n");

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            char filepath[300];
            snprintf(filepath, sizeof(filepath), "%s/%s", huntID, entry->d_name);

            // Verifică dacă fișierul este un fișier regulat
            if (stat(filepath, &fileStat) == 0 && S_ISREG(fileStat.st_mode)) {
                printf("- %s\n", entry->d_name);
            }
        }
    }

    closedir(dir);
    return 0;
}

void view(char *huntID, char *ID) {
    char filename[100];
    snprintf(filename, sizeof(filename), "%s/%s", huntID, ID);

    FILE *f = fopen(filename, "r");
    if (!f) {
        perror("Eroare la deschiderea fișierului");
        return;
    }

    char buffer[256];
    while (fgets(buffer, sizeof(buffer), f)) {
        printf("%s", buffer);
    }

    fclose(f);
}

void removeTreasure(char *huntID, char *ID) {
    char filepath[100];
    snprintf(filepath, sizeof(filepath), "%s/%s", huntID, ID);

    if (remove(filepath) == 0)
        printf("Comoara '%s' a fost ștearsă.\n", ID);
    else
        perror("Eroare la ștergerea comorii");
}

void removeHunt(char *huntID) {
    DIR *dir = opendir(huntID);
    if (!dir) {
        perror("Nu am putut deschide vânătoarea");
        return;
    }

    struct dirent *entry;
    char filepath[300];

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            snprintf(filepath, sizeof(filepath), "%s/%s", huntID, entry->d_name);
            remove(filepath);
        }
    }

    closedir(dir);

    if (rmdir(huntID) == 0)
        printf("Vânătoarea '%s' a fost ștearsă complet.\n", huntID);
    else
        perror("Eroare la ștergerea directorului");
}

void logger(char **comanda, int argc) {
    char filepath[100];
    snprintf(filepath, sizeof(filepath), "%s/logged_hunt", comanda[2]);

    FILE *f = fopen(filepath, "a");
    if (!f) {
        perror("Eroare la scrierea logului");
        return;
    }

    for (int i = 0; i < argc; ++i) {
        fprintf(f, "%s ", comanda[i]);
    }
    fprintf(f, "\n");
    fclose(f);

    char symlinkName[100];
    snprintf(symlinkName, sizeof(symlinkName), "logged_hunt-%s", comanda[2]);
    symlink(filepath, symlinkName);
}

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Nu ai specificat nicio comandă.\n");
        return 1;
    }

    if (strcmp(argv[1], "--add") == 0) {
        Comoara_t c;
        printf("Introdu id, nume, lat, lng, indiciu, valoare:\n");
        scanf("%9s %19s %lf %lf %49s %d", c.ID, c.nume, &c.lat, &c.lng, c.indiciu, &c.valoare);
        add(argv[2], c);
        logger(argv, argc);
    } else if (strcmp(argv[1], "--list") == 0) {
        list(argv[2]);
        logger(argv, argc);
    } else if (strcmp(argv[1], "--view") == 0) {
        view(argv[2], argv[3]);
        logger(argv, argc);
    } else if (strcmp(argv[1], "--removeTreasure") == 0) {
        removeTreasure(argv[2], argv[3]);
        logger(argv, argc);
    } else if (strcmp(argv[1], "--removeHunt") == 0) {
        removeHunt(argv[2]);
        logger(argv, argc);
    } else {
        fprintf(stderr, "Comandă necunoscută.\n");
    }

    return 0;
}
