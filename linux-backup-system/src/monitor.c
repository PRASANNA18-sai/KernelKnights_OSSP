#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>

#include "monitor.h"

#define MAX_FILES 100

typedef struct
{
    char name[256];
    off_t size;
    time_t modified_time;
} FileInfo;

volatile sig_atomic_t running = 1;

void handle_signal(int signal_number)
{
    if (signal_number == SIGINT)
    {
        running = 0;
    }
}

int monitor_directory(const char *directory, int pipe_fd)
{
    struct sigaction sa;

    memset(&sa, 0, sizeof(sa));

    sa.sa_handler = handle_signal;

    sigemptyset(&sa.sa_mask);

    sa.sa_flags = 0;

    if (sigaction(SIGINT, &sa, NULL) == -1)
    {
        perror("sigaction failed");
        return -1;
    }

    FileInfo previous_files[MAX_FILES];
    int previous_count = 0;

    printf("\n[MONITOR] Starting real-time directory monitor.\n");
    printf("[MONITOR] Press Ctrl+C to stop.\n");

    while (running)
    {
        DIR *dir;
        struct dirent *entry;

        FileInfo current_files[MAX_FILES];
        int current_count = 0;

        dir = opendir(directory);

        if (dir == NULL)
        {
            perror("Error opening directory");
            return -1;
        }

        printf("\n[MONITOR] Scanning: %s\n", directory);

        while ((entry = readdir(dir)) != NULL)
        {
            if (!running)
            {
                break;
            }

            if (strcmp(entry->d_name, ".") == 0 ||
                strcmp(entry->d_name, "..") == 0)
            {
                continue;
            }

            if (current_count >= MAX_FILES)
            {
                printf("[MONITOR] Maximum file limit reached.\n");
                break;
            }

            char file_path[512];

            snprintf(
                file_path,
                sizeof(file_path),
                "%s/%s",
                directory,
                entry->d_name
            );

            struct stat file_info;

            if (stat(file_path, &file_info) == -1)
            {
                perror("stat failed");
                continue;
            }

            if (!S_ISREG(file_info.st_mode))
            {
                continue;
            }

            strcpy(
                current_files[current_count].name,
                entry->d_name
            );

            current_files[current_count].size = file_info.st_size;

            current_files[current_count].modified_time =
                file_info.st_mtime;

            int is_new_or_modified = 1;

            for (int i = 0; i < previous_count; i++)
            {
                if (strcmp(
                        previous_files[i].name,
                        entry->d_name
                    ) == 0)
                {
                    if (previous_files[i].size ==
                            file_info.st_size &&
                        previous_files[i].modified_time ==
                            file_info.st_mtime)
                    {
                        is_new_or_modified = 0;
                    }

                    break;
                }
            }

            if (is_new_or_modified)
            {
                printf(
                    "[MONITOR] New/Modified file: %s\n",
                    file_path
                );

                ssize_t bytes_written = write(
                    pipe_fd,
                    file_path,
                    strlen(file_path)
                );

                if (bytes_written == -1)
                {
                    perror("[MONITOR] Pipe write failed");
                    closedir(dir);
                    return -1;
                }

                write(pipe_fd, "\n", 1);
            }

            current_count++;
        }

        closedir(dir);

        /*
         * Save the current directory state.
         * It will be compared with the next scan.
         */
        memcpy(
            previous_files,
            current_files,
            sizeof(FileInfo) * current_count
        );

        previous_count = current_count;

        if (running)
        {
            printf("[MONITOR] Waiting 3 seconds...\n");
            sleep(3);
        }
    }

    printf("\n[MONITOR] SIGINT received.\n");
    printf("[MONITOR] Shutting down safely...\n");
    printf("[MONITOR] Monitor stopped successfully.\n");

    return 0;
}
