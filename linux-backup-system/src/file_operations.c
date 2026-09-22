#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include "file_operations.h"

#define BUFFER_SIZE 4096

int copy_file(const char *source, const char *destination)
{
    int source_fd;
    int destination_fd;

    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;
    ssize_t bytes_written;

    source_fd = open(source, O_RDONLY);

    if (source_fd == -1)
    {
        perror("Error opening source file");
        return -1;
    }

    destination_fd = open(
        destination,
        O_WRONLY | O_CREAT | O_TRUNC,
        0644
    );

    if (destination_fd == -1)
    {
        perror("Error opening destination file");
        close(source_fd);
        return -1;
    }

    while ((bytes_read = read(source_fd, buffer, BUFFER_SIZE)) > 0)
    {
        ssize_t total_written = 0;

        while (total_written < bytes_read)
        {
            bytes_written = write(
                destination_fd,
                buffer + total_written,
                bytes_read - total_written
            );

            if (bytes_written == -1)
            {
                perror("Error writing to destination file");

                close(source_fd);
                close(destination_fd);

                return -1;
            }

            total_written += bytes_written;
        }
    }

    if (bytes_read == -1)
    {
        perror("Error reading source file");

        close(source_fd);
        close(destination_fd);

        return -1;
    }

    close(source_fd);
    close(destination_fd);

    return 0;
}