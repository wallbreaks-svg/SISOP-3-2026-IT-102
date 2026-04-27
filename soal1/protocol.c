#include <stdio.h>
#include <time.h>
#include "protocol.h"

void tulis_log(const char *tipe, const char *isi)
{
    FILE *file_log = fopen("history.log", "a");

    time_t waktu = time(NULL);
    struct tm *info = localtime(&waktu);

    fprintf(file_log,
            "[%04d-%02d-%02d %02d:%02d:%02d] [%s] [%s]\n",
            info->tm_year + 1900,
            info->tm_mon + 1,
            info->tm_mday,
            info->tm_hour,
            info->tm_min,
            info->tm_sec,
            tipe,
            isi);

    fclose(file_log);
}