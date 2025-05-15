#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/wait.h>
#include <sys/types.h>

int fd_pipe[2];
pid_t pid_monitor = -1;
int running = 0;
int shutting_down = 0;

void on_sigchld(int sig)
{
    int stat;
    pid_t ended = waitpid(pid_monitor, &stat, WNOHANG);
    if (ended > 0)
    {
        printf("Monitor process ended with code %d\n", WEXITSTATUS(stat));
        pid_monitor = -1;
        running = 0;
    }
}

void read_output()
{
    char buff[1024];
    ssize_t size = read(fd_pipe[0], buff, sizeof(buff) - 1);
    if (size > 0)
    {
        buff[size] = '\0';
        printf("%s", buff);
    }
}

void send_command(const char *text)
{
    FILE *cmdfile = fopen("monitor_command.cmd", "w");
    if (!cmdfile)
    {
        perror("Can't write command file");
        return;
    }

    fprintf(cmdfile, "%s\n", text);
    fclose(cmdfile);

    kill(pid_monitor, SIGUSR1);
    usleep(200000);
    read_output();
}

int main()
{
    struct sigaction sa;
    sa.sa_handler = on_sigchld;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGCHLD, &sa, NULL);

    char input[128];

    while (1)
    {
        printf("hub> ");
        fflush(stdout);

        if (!fgets(input, sizeof(input), stdin))
            break;

        input[strcspn(input, "\n")] = '\0';

        if (strcmp(input, "start_monitor") == 0)
        {
            if (running)
            {
                printf("Monitor already active (PID %d)\n", pid_monitor);
                continue;
            }

            if (pipe(fd_pipe) == -1)
            {
                perror("pipe failed");
                continue;
            }

            pid_monitor = fork();
            if (pid_monitor == 0)
            {
                dup2(fd_pipe[1], STDOUT_FILENO);
                close(fd_pipe[0]);
                execl("./treasure_manager", "treasure_manager", NULL);
                perror("exec error");
                exit(1);
            }
            else
            {
                close(fd_pipe[1]);
                running = 1;
                printf("Monitor launched (PID %d)\n", pid_monitor);
            }
        }
        else if (strcmp(input, "stop_monitor") == 0)
        {
            if (!running)
            {
                printf("No monitor running.\n");
                continue;
            }

            send_command("stop");
            shutting_down = 1;
        }
        else if (strncmp(input, "list_", 5) == 0 || strncmp(input, "view_", 5) == 0)
        {
            if (!running)
            {
                printf("Please start monitor first.\n");
                continue;
            }
            if (shutting_down)
            {
                printf("Monitor is shutting down. Wait...\n");
                continue;
            }

            send_command(input);
        }
        else if (strcmp(input, "calculate_score") == 0)
        {
            DIR *cur = opendir(".");
            if (!cur)
            {
                perror("opendir");
                continue;
            }

            struct dirent *d;
            while ((d = readdir(cur)) != NULL)
            {
                if (d->d_type == DT_DIR &&
                    d->d_name[0] != '.' && // ignoră directoare ascunse
                    strcmp(d->d_name, "vscode") != 0)
                {

                    int localpipe[2];
                    if (pipe(localpipe) == -1)
                    {
                        perror("pipe");
                        continue;
                    }

                    pid_t p = fork();
                    if (p == 0)
                    {
                        close(localpipe[0]);
                        dup2(localpipe[1], STDOUT_FILENO);
                        execl("./score_calculator", "score_calculator", d->d_name, NULL);
                        perror("calculator exec error");
                        exit(1);
                    }
                    else
                    {
                        close(localpipe[1]);
                        char line[256];
                        ssize_t len;

                        printf("Scores for %s:\n", d->d_name);
                        while ((len = read(localpipe[0], line, sizeof(line) - 1)) > 0)
                        {
                            line[len] = '\0';
                            printf("%s", line);
                        }

                        close(localpipe[0]);
                        waitpid(p, NULL, 0);
                    }
                }
            }

            closedir(cur);
        }
        else if (strcmp(input, "exit") == 0)
        {
            if (running)
            {
                printf("Can't exit while monitor is still running.\n");
            }
            else
            {
                break;
            }
        }
        else
        {
            printf("Invalid command: %s\n", input);
        }
    }

    return 0;
}
