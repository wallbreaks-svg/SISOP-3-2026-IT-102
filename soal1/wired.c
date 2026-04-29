#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <time.h>
#include "protocol.h"

// socket client
int user_socket[CLIENT_LIMIT];

// username client
char username[CLIENT_LIMIT][50];

// admin flag
int is_admin[CLIENT_LIMIT];

// buffer pesan
char pesan[MAX_BUFFER];

// broadcast
void kirim_ke_semua(int pengirim, char pesan[])
{
    for (int j = 0; j < CLIENT_LIMIT; j++)
    {
        if (user_socket[j] != 0 && user_socket[j] != pengirim)
        {
            send(user_socket[j], pesan, strlen(pesan), 0);
        }
    }
}

int main()
{
    int server_fd, addrlen;
    struct sockaddr_in address;
    fd_set readfds;

    time_t start_time = time(NULL);

    // inisialisasi
    for (int i = 0; i < CLIENT_LIMIT; i++)
    {
        user_socket[i] = 0;
        is_admin[i] = 0;
        strcpy(username[i], "");
    }

    // buat socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    // setting address
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(SERVER_PORT);

    addrlen = sizeof(address);

    // bind & listen
    bind(server_fd, (struct sockaddr *)&address, sizeof(address));
    listen(server_fd, 5);

    printf("Server berjalan di port %d...\n", SERVER_PORT);
    tulis_log("System", "SERVER ONLINE");

    while (1)
    {
        FD_ZERO(&readfds);
        FD_SET(server_fd, &readfds);

        int max_sd = server_fd;

        for (int i = 0; i < CLIENT_LIMIT; i++)
        {
            int sd = user_socket[i];

            if (sd > 0)
                FD_SET(sd, &readfds);

            if (sd > max_sd)
                max_sd = sd;
        }

        select(max_sd + 1, &readfds, NULL, NULL, NULL);

        // koneksi baru
        if (FD_ISSET(server_fd, &readfds))
        {
            int koneksi_baru = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen);

            char nama[50];
            recv(koneksi_baru, nama, 50, 0);
            nama[strcspn(nama, "\n")] = 0;

            int sudah_ada = 0;
            for (int i = 0; i < CLIENT_LIMIT; i++)
            {
                if (strcmp(username[i], nama) == 0)
                {
                    sudah_ada = 1;
                    break;
                }
            }

            if (sudah_ada)
            {
                char *msg = "Username sudah dipakai!\n";
                send(koneksi_baru, msg, strlen(msg), 0);
                close(koneksi_baru);
            }
            else
            {
                for (int i = 0; i < CLIENT_LIMIT; i++)
                {
                    if (user_socket[i] == 0)
                    {
                        user_socket[i] = koneksi_baru;
                        strcpy(username[i], nama);
                        is_admin[i] = 0;
                        break;
                    }
                }

                printf("User %s masuk\n", nama);

                char logmsg[100];
                snprintf(logmsg, sizeof(logmsg), "User '%s' connected", nama);
                tulis_log("System", logmsg);
            }
        }

        // handle client
        for (int i = 0; i < CLIENT_LIMIT; i++)
        {
            int sd = user_socket[i];

            if (FD_ISSET(sd, &readfds))
            {
                int valread = recv(sd, pesan, MAX_BUFFER, 0);

                if (valread == 0)
                {
                    printf("User %s keluar\n", username[i]);

                    char logmsg[100];
                    snprintf(logmsg, sizeof(logmsg), "User '%s' disconnected", username[i]);
                    tulis_log("System", logmsg);

                    close(sd);
                    user_socket[i] = 0;
                    strcpy(username[i], "");
                    is_admin[i] = 0;
                }
                else
                {
                    pesan[valread] = '\0';

                    // AUTH
                    if (strncmp(pesan, "/auth ", 6) == 0)
                    {
                        char password[50];
                        sscanf(pesan, "/auth %s", password);

                        if (strcmp(password, "1234") == 0)
                        {
                            is_admin[i] = 1;
                            send(sd, "Admin mode aktif\n", 18, 0);
                            tulis_log("Admin", "AUTH SUCCESS");
                        }
                        else
                        {
                            send(sd, "Password salah\n", 15, 0);
                        }
                        continue;
                    }

                    // GET USERS
                    if (strncmp(pesan, "/get_users", 10) == 0 && is_admin[i])
                    {
                        int count = 0;
                        for (int j = 0; j < CLIENT_LIMIT; j++)
                        {
                            if (user_socket[j] != 0 && !is_admin[j])
                                count++;
                        }

                        char msg[50];
                        snprintf(msg, sizeof(msg), "User aktif: %d\n", count);
                        send(sd, msg, strlen(msg), 0);

                        tulis_log("Admin", "RPC_GET_USERS");
                        continue;
                    }

                    // GET UPTIME
                    if (strncmp(pesan, "/get_uptime", 11) == 0 && is_admin[i])
                    {
                        time_t now = time(NULL);

                        char msg[100];
                        snprintf(msg, sizeof(msg), "Uptime: %ld detik\n", now - start_time);
                        send(sd, msg, strlen(msg), 0);

                        tulis_log("Admin", "RPC_GET_UPTIME");
                        continue;
                    }

                    // SHUTDOWN
                    if (strncmp(pesan, "/shutdown", 9) == 0 && is_admin[i])
                    {
                        tulis_log("Admin", "RPC_SHUTDOWN");
                        tulis_log("System", "EMERGENCY SHUTDOWN INITIATED");

                        for (int j = 0; j < CLIENT_LIMIT; j++)
                        {
                            if (user_socket[j] != 0)
                            {
                                send(user_socket[j], "Server shutdown\n", 17, 0);
                                close(user_socket[j]);
                            }
                        }

                        close(server_fd);
                        exit(0);
                    }

                    // EXIT
                    if (strncmp(pesan, "/exit", 5) == 0)
                    {
                        printf("User %s keluar\n", username[i]);

                        char notif[100];
                        snprintf(notif, sizeof(notif), "%s keluar dari chat\n", username[i]);
                        kirim_ke_semua(sd, notif);

                        char logmsg[100];
                        snprintf(logmsg, sizeof(logmsg), "User '%s' disconnected", username[i]);
                        tulis_log("System", logmsg);

                        close(sd);
                        user_socket[i] = 0;
                        strcpy(username[i], "");
                        is_admin[i] = 0;

                        continue;
                    }

                    // CHAT
                    char kirim[MAX_BUFFER + 100];
                    snprintf(kirim, sizeof(kirim), "[%s]: %s", username[i], pesan);

                    printf("%s", kirim);
                    kirim_ke_semua(sd, kirim);

                    char logmsg[MAX_BUFFER + 100];
                    snprintf(logmsg, sizeof(logmsg), "[%s]: %s", username[i], pesan);
                    tulis_log("User", logmsg);
                }
            }
        }
    }

    return 0;
}