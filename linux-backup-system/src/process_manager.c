#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>

#include "process_manager.h"
#include "file_operations.h"
#include "ipc.h"
#include "monitor.h"

int start_backup_process(const char *source_directory)
{
    int pipe_fd[2];

    /*
     * Create the pipe for communication
     * between monitor and backup process.
     */
    if (create_backup_pipe(pipe_fd) == -1)
    {
        return -1;
    }

    /*
     * Create the backup process.
     */
    pid_t pid = fork();

    if (pid == -1)
    {
        perror("fork failed");

        close(pipe_fd[0]);
        close(pipe_fd[1]);

        return -1;
    }

    /*
     * CHILD PROCESS
     * Receives file paths from the monitor
     * and creates backups.
     */
    if (pid == 0)
    {
        close(pipe_fd[1]);

        printf("\n[BACKUP PROCESS]\n");
        printf("Backup Process PID: %d\n", getpid());

        char buffer[1024];
        ssize_t bytes_read;

        /*
         * Keep reading while the monitor is running.
         *
         * When the monitor closes the pipe,
         * read() returns 0 and the child exits.
         */
        while ((bytes_read = read(
                    pipe_fd[0],
                    buffer,
                    sizeof(buffer) - 1)) > 0)
        {
            buffer[bytes_read] = '\0';

            /*
             * A newline separates file paths.
             */
            char *file_path = strtok(buffer, "\n");

            while (file_path != NULL)
            {
                printf(
                    "\n[BACKUP] Received file: %s\n",
                    file_path
                );

                /*
                 * Extract the filename from the path.
                 *
                 * Example:
                 * source/demo.txt
                 *        ↓
                 * demo.txt
                 */
                char *filename = strrchr(file_path, '/');

                if (filename != NULL)
                {
                    filename++;
                }
                else
                {
                    filename = file_path;
                }

                char destination[512];

                snprintf(
                    destination,
                    sizeof(destination),
                    "backup/%s",
                    filename
                );

                printf(
                    "[BACKUP] Copying to: %s\n",
                    destination
                );

                /*
                 * Use the file-operation module
                 * to copy the file.
                 */
                if (copy_file(file_path, destination) == 0)
                {
                    printf(
                        "[BACKUP] Backup completed: %s\n",
                        filename
                    );
                }
                else
                {
                    printf(
                        "[BACKUP] Backup failed: %s\n",
                        filename
                    );
                }

                file_path = strtok(NULL, "\n");
            }
        }

        if (bytes_read == -1)
        {
            perror("[BACKUP] Error reading from pipe");
        }

        close(pipe_fd[0]);

        printf("\n[BACKUP] Pipe closed by monitor.\n");
        printf("[BACKUP] Backup process exiting.\n");

        exit(0);
    }

    /*
     * PARENT PROCESS
     * Acts as the directory monitor.
     */
    else
    {
        close(pipe_fd[0]);

        printf("\n[MONITOR PROCESS]\n");
        printf("Monitor PID: %d\n", getpid());
        printf("Created Backup PID: %d\n", pid);

        /*
         * Continuously monitor the source directory.
         *
         * This function returns when Ctrl+C
         * is pressed.
         */
        monitor_directory(
            source_directory,
            pipe_fd[1]
        );

        /*
         * Closing the write end sends EOF to
         * the backup process.
         */
        close(pipe_fd[1]);

        printf("\n[MONITOR] Waiting for backup process...\n");

        int status;

        waitpid(pid, &status, 0);

        if (WIFEXITED(status))
        {
            printf(
                "[MONITOR] Backup process exited with status: %d\n",
                WEXITSTATUS(status)
            );
        }

        printf("[MONITOR] System operation completed.\n");
    }

    return 0;
}
