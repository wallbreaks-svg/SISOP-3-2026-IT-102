#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "protocol.h"

int sock;

// thread kirim pesan
void *kirim_pesan(void *arg)
{
    char pesan[MAX_BUFFER];

    while (1)
    {
        fgets(pesan, MAX_BUFFER, stdin);

        // hapus newline biar rapi
        pesan[strcspn(pesan, "\n")] = 0;

        if (strlen(pesan) == 0)
            continue;

        send(sock, pesan, strlen(pesan), 0);

        // kalau exit
        if (strcmp(pesan, "/exit") == 0)
        {
            close(sock);
            exit(0);
        }
    }

    return NULL;
}

// thread terima pesan
void *terima_pesan(void *arg)
{
    char buffer[MAX_BUFFER];

    while (1)
    {
        int valread = recv(sock, buffer, MAX_BUFFER, 0);

        if (valread <= 0)
        {
            printf("Disconnected from server\n");
            close(sock);
            exit(0);
        }

        buffer[valread] = '\0';
        printf("%s\n", buffer);
    }

    return NULL;
}

// MAIN
int main()
{
    struct sockaddr_in server_addr;

    sock = socket(AF_INET, SOCK_STREAM, 0);

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("connect failed");
        return 1;
    }

    printf("Terhubung ke server!\n");

    char nama[50];

    printf("Masukkan username: ");
    fgets(nama, 50, stdin);
    nama[strcspn(nama, "\n")] = 0;

    send(sock, nama, strlen(nama), 0);

    pthread_t t1, t2;

    pthread_create(&t1, NULL, kirim_pesan, NULL);
    pthread_create(&t2, NULL, terima_pesan, NULL);

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);

    return 0;
}