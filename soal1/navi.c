#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "protocol.h"

int sock;
char pesan[MAX_BUFFER];

// thread untuk kirim pesan
void *kirim_pesan(void *arg)
{
    while (1)
    {
        fgets(pesan, MAX_BUFFER, stdin);
        send(sock, pesan, strlen(pesan), 0);
    }
}

// thread untuk menerima pesan
void *terima_pesan(void *arg)
{
    char buffer[MAX_BUFFER];

    while (1)
    {
        int valread = recv(sock, buffer, MAX_BUFFER, 0);

        if (valread > 0)
        {
            buffer[valread] = '\0';
            printf("%s", buffer);
        }
    }
}

int main()
{
    struct sockaddr_in server_addr;

    // buat socket
    sock = socket(AF_INET, SOCK_STREAM, 0);

    // setting server
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    // connect ke server
    connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr));

    printf("Terhubung ke server!\n");

    char nama[50];

    printf("Masukkan username: ");
    fgets(nama, 50, stdin);

    // kirim ke server
    send(sock, nama, strlen(nama), 0);
    // buat thread
    pthread_t t1, t2;

    pthread_create(&t1, NULL, kirim_pesan, NULL);
    pthread_create(&t2, NULL, terima_pesan, NULL);

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);

    return 0;
}