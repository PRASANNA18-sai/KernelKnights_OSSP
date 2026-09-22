#include <stdio.h>
#include <unistd.h>

#include "ipc.h"

int create_backup_pipe(int pipe_fd[2])
{
    if (pipe(pipe_fd) == -1)
    {
        perror("pipe failed");
        return -1;
    }

    return 0;
}