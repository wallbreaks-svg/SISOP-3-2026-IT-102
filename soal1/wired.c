#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <time.h>
#include "protocol.h"

int clients[WIRED_MAX];
char names[WIRED_MAX][64];
time_t start_time;

void log_to_file(const char *type, const char *msg)
{
    FILE *f = fopen("history.log", "a");
    if (!f)
        return;

    char buf[256];
    build_log(buf, type, msg);
    fprintf(f, "%s\n", buf);
    fclose(f);
}

int name_exists(char *name)
{
    for (int i = 0; i < WIRED_MAX; i++)
        if (clients[i] && strcmp(names[i], name) == 0)
            return 1;
    return 0;
}

void broadcast_msg(char *msg, int sender)
{
    for (int i = 0; i < WIRED_MAX; i++)
        if (clients[i] && clients[i] != sender)
            send(clients[i], msg, strlen(msg), 0);
}

void remove_client(int i)
{
    if (!clients[i])
        return;

    char logmsg[128];
    sprintf(logmsg, "User '%s' disconnected", names[i]);
    log_to_file("System", logmsg);

    close(clients[i]);
    clients[i] = 0;
    memset(names[i], 0, sizeof(names[i]));
}

void handle_admin(int fd)
{
    send(fd, "Enter Password: ", 16, 0);

    char pass[64];
    int n = read(fd, pass, sizeof(pass) - 1);
    if (n <= 0)
        return;
    pass[n] = '\0';

    if (strcmp(pass, "protocol7") != 0)
    {
        send(fd, "[System] Wrong password\n", 24, 0);
        return;
    }

    send(fd, "[System] Authentication Successful.\n", 36, 0);

    char menu[] =
        "--- THE KNIGHTS CONSOLE ---\n"
        "1. Check Active Entities (Users)\n"
        "2. Check Server Uptime\n"
        "3. Execute Emergency Shutdown\n"
        "4. Disconnect\n";

    send(fd, menu, strlen(menu), 0);

    // 🔥 LOOP ADMIN (INI YANG DIPERBAIKI)
    while (1)
    {
        send(fd, "Command >> ", 11, 0);

        char cmd[10];
        int n = read(fd, cmd, sizeof(cmd) - 1);
        if (n <= 0)
            break;

        cmd[n] = '\0';

        if (cmd[0] == '1')
        {
            int count = 0;
            for (int i = 0; i < WIRED_MAX; i++)
                if (clients[i])
                    count++;

            char out[64];
            sprintf(out, "Active Users: %d\n", count);
            send(fd, out, strlen(out), 0);

            log_to_file("Admin", "RPC_GET_USERS");
        }

        else if (cmd[0] == '2')
        {
            int uptime = time(NULL) - start_time;

            char out[64];
            sprintf(out, "Uptime: %d seconds\n", uptime);
            send(fd, out, strlen(out), 0);

            log_to_file("Admin", "RPC_GET_UPTIME");
        }

        else if (cmd[0] == '3')
        {
            log_to_file("Admin", "RPC_SHUTDOWN");
            log_to_file("System", "EMERGENCY SHUTDOWN INITIATED");

            broadcast_msg("[System] Server shutting down...\n", fd);
            exit(0);
        }

        else if (cmd[0] == '4')
        {
            send(fd, "[System] Disconnecting from admin console\n", 43, 0);
            break;
        }

        else
        {
            send(fd, "[System] Invalid command\n", 26, 0);
        }
    }
}

void handle_client(int i)
{
    char buf[WIRED_BUF];
    int fd = clients[i];

    int n = read(fd, buf, WIRED_BUF - 1);
    if (n <= 0)
    {
        remove_client(i);
        return;
    }

    buf[n] = '\0';

    if (strcmp(buf, CMD_EXIT) == 0)
    {
        remove_client(i);
        return;
    }

    if (strcmp(buf, CMD_ADMIN) == 0)
    {
        handle_admin(fd);
        return;
    }

    char msg[WIRED_BUF];
    snprintf(msg, sizeof(msg), "[%s]: %.900s", names[i], buf);

    char logmsg[WIRED_BUF];
    snprintf(logmsg, sizeof(logmsg), "[[%s]: %.900s]", names[i], buf);

    log_to_file("User", logmsg);
    broadcast_msg(msg, fd);
}

void accept_client(int server_fd)
{
    int newfd = accept(server_fd, NULL, NULL);
    if (newfd < 0)
        return;

    char name[64];
    int n = read(newfd, name, sizeof(name) - 1);
    if (n <= 0)
    {
        close(newfd);
        return;
    }
    name[n] = '\0';

    if (name_exists(name))
    {
        send(newfd, "[System] Name already used\n", 29, 0);
        close(newfd);
        return;
    }

    for (int i = 0; i < WIRED_MAX; i++)
    {
        if (!clients[i])
        {
            clients[i] = newfd;
            strcpy(names[i], name);

            char logmsg[128];
            sprintf(logmsg, "User '%s' connected", name);
            log_to_file("System", logmsg);

            char welcome[128];
            sprintf(welcome, "--- Welcome to The Wired, %s ---\n", name);
            send(newfd, welcome, strlen(welcome), 0);
            break;
        }
    }
}

int main()
{
    int server_fd;
    struct sockaddr_in addr;
    fd_set readfds;

    start_time = time(NULL);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(WIRED_PORT);

    bind(server_fd, (struct sockaddr *)&addr, sizeof(addr));
    listen(server_fd, 5);

    log_to_file("System", "SERVER ONLINE");

    while (1)
    {
        FD_ZERO(&readfds);
        FD_SET(server_fd, &readfds);

        int maxfd = server_fd;

        for (int i = 0; i < WIRED_MAX; i++)
        {
            if (clients[i])
            {
                FD_SET(clients[i], &readfds);
                if (clients[i] > maxfd)
                    maxfd = clients[i];
            }
        }

        select(maxfd + 1, &readfds, NULL, NULL, NULL);

        if (FD_ISSET(server_fd, &readfds))
            accept_client(server_fd);

        for (int i = 0; i < WIRED_MAX; i++)
            if (clients[i] && FD_ISSET(clients[i], &readfds))
                handle_client(i);
    }
}