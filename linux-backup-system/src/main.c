#include <stdio.h>

#include "process_manager.h"

int main()
{
    printf("========================================\n");
    printf(" Linux Real-Time File Backup System\n");
    printf("========================================\n");

    start_backup_process("source");

    return 0;
}