#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <string.h>
#include "protocol.h"

int main()
{
    int sock;
    struct sockaddr_in server;
    fd_set fds;

    sock = socket(AF_INET, SOCK_STREAM, 0);

    server.sin_family = AF_INET;
    server.sin_port = htons(WIRED_PORT);
    inet_pton(AF_INET, WIRED_HOST, &server.sin_addr);

    // cek koneksi
    if (connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0)
    {
        perror("Connect failed");
        return 1;
    }

    // input nama
    char name[64];
    printf("Enter your name: ");
    fgets(name, sizeof(name), stdin);
    name[strcspn(name, "\n")] = 0;

    send(sock, name, strlen(name), 0);

    // tampilkan prompt pertama
    printf("> ");
    fflush(stdout);

    while (1)
    {
        FD_ZERO(&fds);
        FD_SET(STDIN_FILENO, &fds);
        FD_SET(sock, &fds);

        select(sock + 1, &fds, NULL, NULL, NULL);

        // input user
        if (FD_ISSET(STDIN_FILENO, &fds))
        {
            char msg[WIRED_BUF];

            fgets(msg, WIRED_BUF, stdin);
            msg[strcspn(msg, "\n")] = 0;

            send(sock, msg, strlen(msg), 0);

            if (strcmp(msg, CMD_EXIT) == 0)
                break;
        }

        // output dari server
        if (FD_ISSET(sock, &fds))
        {
            char buf[WIRED_BUF];
            int n = read(sock, buf, WIRED_BUF - 1);

            if (n <= 0)
                break;

            buf[n] = '\0';

            // 🔥 FIX UI BIAR RAPI
            printf("\r%s\n> ", buf);
            fflush(stdout);
        }
    }

    close(sock);
    return 0;
}