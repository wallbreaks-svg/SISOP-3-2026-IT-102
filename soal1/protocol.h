#ifndef WIRED_PROTOCOL_H
#define WIRED_PROTOCOL_H

#include <time.h>
#include <stdio.h>
#include <string.h>

#define WIRED_PORT 8080
#define WIRED_MAX 100
#define WIRED_BUF 1024
#define WIRED_HOST "127.0.0.1"

#define CMD_EXIT "/exit"
#define CMD_ADMIN "/admin"

// 🔥 function langsung di header (inline style)
static inline void build_log(char *out, const char *type, const char *msg)
{
    time_t now = time(NULL);
    struct tm *t = localtime(&now);

    sprintf(out, "[%04d-%02d-%02d %02d:%02d:%02d] [%s] [%s]",
            t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
            t->tm_hour, t->tm_min, t->tm_sec,
            type, msg);
}

#endif