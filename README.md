# SISOP Modul 3

## Soal 1 - The Wired

**Nama:** Achmad Rifqy Aqila
**NRP:** 5027251102

---

## Penjelasan Program `navi.c` (Client / NAVI)

Program `navi.c` berperan sebagai client dalam sistem komunikasi *The Wired*. Program ini memungkinkan pengguna untuk terhubung ke server, mengirim pesan, serta menerima pesan dari pengguna lain secara real-time. Seluruh komunikasi dilakukan menggunakan mekanisme socket berbasis TCP.

Pada bagian awal program, dilakukan import beberapa library yang dibutuhkan untuk keperluan input-output, komunikasi jaringan, serta pengelolaan data. Selain itu, file `protocol.h` digunakan sebagai tempat penyimpanan konfigurasi seperti alamat IP, port, serta command yang digunakan dalam program.

```c
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <string.h>
#include "protocol.h"
```

Selanjutnya, program membuat sebuah socket menggunakan protokol TCP dengan domain IPv4. Socket ini nantinya digunakan sebagai jalur komunikasi antara client dan server.

```c
int sock;
struct sockaddr_in server;

sock = socket(AF_INET, SOCK_STREAM, 0);
```

Setelah socket berhasil dibuat, program kemudian mengatur alamat tujuan server, yaitu dengan menentukan jenis alamat, port yang digunakan, serta alamat IP server. Nilai port dan host diambil dari file `protocol.h` agar lebih fleksibel.

```c
server.sin_family = AF_INET;
server.sin_port = htons(WIRED_PORT);
inet_pton(AF_INET, WIRED_HOST, &server.sin_addr);
```

Setelah konfigurasi selesai, client akan mencoba melakukan koneksi ke server. Jika koneksi gagal, program akan menampilkan pesan error dan langsung berhenti.

```c
if (connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0)
{
    perror("Connect failed");
    return 1;
}
```

Ketika koneksi berhasil, pengguna diminta untuk memasukkan nama sebagai identitas. Nama ini akan dikirim ke server untuk diverifikasi agar tidak terjadi duplikasi dengan pengguna lain.

```c
char name[64];
printf("Enter your name: ");
fgets(name, sizeof(name), stdin);
name[strcspn(name, "\n")] = 0;

send(sock, name, strlen(name), 0);
```

Setelah itu, program menampilkan prompt `>` sebagai penanda bahwa user sudah bisa mulai mengirim pesan.

```c
printf("> ");
fflush(stdout);
```

Bagian utama dari program berjalan di dalam sebuah loop tak hingga. Pada bagian ini digunakan fungsi `select()` untuk memungkinkan client membaca dua sumber input secara bersamaan, yaitu dari keyboard (input user) dan dari socket (pesan dari server). Hal ini memungkinkan komunikasi berjalan secara asynchronous tanpa menggunakan `fork` atau thread.

```c
while (1)
{
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);
    FD_SET(sock, &fds);

    select(sock + 1, &fds, NULL, NULL, NULL);
```

Ketika user mengetik sesuatu di terminal, program akan membaca input tersebut, menghapus karakter newline, lalu mengirimkannya ke server. Jika user mengetik perintah `/exit`, maka koneksi akan ditutup.

```c
if (FD_ISSET(STDIN_FILENO, &fds))
{
    char msg[WIRED_BUF];

    fgets(msg, WIRED_BUF, stdin);
    msg[strcspn(msg, "\n")] = 0;

    send(sock, msg, strlen(msg), 0);

    if (strcmp(msg, CMD_EXIT) == 0)
        break;
}
```

Di sisi lain, jika server mengirimkan pesan, client akan membaca data tersebut dan menampilkannya ke layar. Jika koneksi terputus, maka program akan keluar dari loop.

```c
if (FD_ISSET(sock, &fds))
{
    char buf[WIRED_BUF];
    int n = read(sock, buf, WIRED_BUF - 1);

    if (n <= 0)
        break;

    buf[n] = '\0';
```

Agar tampilan tetap rapi, digunakan teknik formatting sederhana dengan `\r` untuk memastikan pesan baru tidak merusak input yang sedang diketik oleh user, serta tetap menampilkan prompt `>` setelah pesan muncul.

```c
printf("\r%s\n> ", buf);
fflush(stdout);
```

Setelah loop selesai (biasanya karena user keluar atau koneksi terputus), socket akan ditutup untuk mengakhiri koneksi secara aman.

```c
close(sock);
return 0;
```

Secara keseluruhan, program ini berhasil mengimplementasikan client yang mampu berkomunikasi dengan server secara real-time, mendukung pengiriman dan penerimaan pesan secara bersamaan, serta menyediakan antarmuka sederhana berbasis terminal yang mudah digunakan.

## Penjelasan Program `protocol.h`

File `protocol.h` berfungsi sebagai pusat konfigurasi dan utilitas yang digunakan baik oleh server maupun client dalam sistem *The Wired*. Dengan adanya file ini, nilai-nilai penting seperti port, alamat server, ukuran buffer, serta command dapat dikelola dalam satu tempat sehingga memudahkan pengembangan dan perawatan program.

Selain berisi konstanta, file ini juga menyediakan sebuah fungsi bantu (`helper function`) yang digunakan untuk membangun format log sesuai dengan kebutuhan sistem.

```c id="7fz6ls"
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
```

Pada bagian awal, digunakan *header guard* (`#ifndef`, `#define`, `#endif`) untuk mencegah file ini di-include lebih dari satu kali yang dapat menyebabkan error saat proses kompilasi.

Selanjutnya, didefinisikan beberapa konstanta penting. `WIRED_PORT` digunakan sebagai port komunikasi server, `WIRED_HOST` sebagai alamat IP tujuan, `WIRED_MAX` sebagai batas maksimum client yang dapat terhubung, serta `WIRED_BUF` sebagai ukuran buffer untuk menampung data. Selain itu, terdapat juga definisi command seperti `/exit` untuk keluar dan `/admin` untuk masuk ke mode admin.

Bagian terakhir dari file ini adalah fungsi `build_log`, yaitu fungsi yang digunakan untuk membentuk format log secara otomatis. Fungsi ini akan mengambil waktu saat ini, kemudian menyusunnya menjadi format `[YYYY-MM-DD HH:MM:SS] [Type] [Message]`. Dengan adanya fungsi ini, pencatatan aktivitas (logging) menjadi konsisten dan sesuai dengan format yang diminta pada soal.

Penggunaan fungsi secara langsung di dalam header (dengan `static inline`) bertujuan agar fungsi dapat digunakan oleh banyak file tanpa perlu membuat file `.c` terpisah, sekaligus menghindari masalah duplikasi definisi saat proses linking.

Secara keseluruhan, file `protocol.h` berperan penting dalam menjaga konsistensi konfigurasi dan mempermudah proses pengelolaan log pada sistem.

## Penjelasan Program `wired.c` (Server / The Wired)

File `wired.c` merupakan inti dari sistem *The Wired* yang berperan sebagai server. Program ini bertugas untuk menerima koneksi dari banyak client (NAVI), mengelola komunikasi antar client, serta menyediakan fitur tambahan seperti broadcast, admin RPC, dan logging aktivitas.

Pada bagian awal program, digunakan beberapa library yang mendukung komunikasi jaringan, pengelolaan socket, serta manipulasi waktu untuk keperluan logging.

```c id="b1r6dx"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <time.h>
#include "protocol.h"
```

Program kemudian mendeklarasikan struktur data utama, yaitu array `clients` untuk menyimpan file descriptor dari setiap client yang terhubung, serta array `names` untuk menyimpan nama masing-masing client. Selain itu, terdapat variabel `start_time` yang digunakan untuk menghitung uptime server.

```c id="qk9v5r"
int clients[WIRED_MAX];
char names[WIRED_MAX][64];
time_t start_time;
```

Untuk memenuhi kebutuhan logging, dibuat fungsi `log_to_file` yang akan menuliskan setiap aktivitas ke dalam file `history.log`. Format log mengikuti aturan yang sudah ditentukan, yaitu menggunakan timestamp dan kategori pesan.

```c id="u8h1ok"
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
```

Agar tidak terjadi duplikasi nama, digunakan fungsi `name_exists` untuk mengecek apakah nama client yang baru sudah digunakan oleh client lain.

```c id="4dx6gs"
int name_exists(char *name)
{
    for (int i = 0; i < WIRED_MAX; i++)
        if (clients[i] && strcmp(names[i], name) == 0)
            return 1;
    return 0;
}
```

Dalam sistem ini, setiap pesan yang dikirim oleh satu client akan diteruskan ke seluruh client lain melalui mekanisme broadcast. Hal ini diatur oleh fungsi `broadcast_msg`.

```c id="0w1h1n"
void broadcast_msg(char *msg, int sender)
{
    for (int i = 0; i < WIRED_MAX; i++)
        if (clients[i] && clients[i] != sender)
            send(clients[i], msg, strlen(msg), 0);
}
```

Ketika client keluar atau koneksi terputus, server akan menghapus data client tersebut dan mencatatnya ke dalam log melalui fungsi `remove_client`.

```c id="o2r3hv"
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
```

Salah satu fitur penting dalam soal adalah adanya mode admin (*The Knights*). Fungsi `handle_admin` digunakan untuk menangani proses autentikasi dan eksekusi perintah admin seperti melihat jumlah user, mengecek uptime, serta melakukan shutdown server.

```c id="e9z6fm"
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
```

Setelah berhasil login sebagai admin, program akan menampilkan menu interaktif dan memproses command secara berulang menggunakan loop.

```c id="x7m2yt"
    while (1)
    {
        send(fd, "Command >> ", 11, 0);

        char cmd[10];
        int n = read(fd, cmd, sizeof(cmd) - 1);
        if (n <= 0)
            break;

        cmd[n] = '\0';
```

Admin dapat menjalankan beberapa perintah penting seperti:

* Melihat jumlah user aktif
* Melihat uptime server
* Mematikan server (shutdown)
* Keluar dari mode admin

Semua aktivitas admin juga akan dicatat dalam log.

Selanjutnya, fungsi `handle_client` digunakan untuk memproses pesan dari setiap client. Fungsi ini akan membaca pesan, mengecek apakah itu command khusus seperti `/exit` atau `/admin`, atau pesan biasa yang akan dibroadcast.

```c id="b8y2sq"
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
```

Jika pesan berupa chat biasa, maka server akan memformat pesan tersebut dan mengirimkannya ke client lain, sekaligus menyimpannya ke dalam log.

```c id="k4r9pw"
char msg[WIRED_BUF];
snprintf(msg, sizeof(msg), "[%s]: %.900s", names[i], buf);

char logmsg[WIRED_BUF];
snprintf(logmsg, sizeof(logmsg), "[[%s]: %.900s]", names[i], buf);

log_to_file("User", logmsg);
broadcast_msg(msg, fd);
```

Untuk menerima koneksi baru, digunakan fungsi `accept_client` yang akan menerima client, membaca nama, melakukan validasi, lalu menyimpannya ke dalam sistem.

```c id="n1x7de"
void accept_client(int server_fd)
{
    int newfd = accept(server_fd, NULL, NULL);
    if (newfd < 0)
        return;
```

Jika nama sudah digunakan, maka client akan ditolak. Jika valid, client akan diterima dan mendapatkan pesan sambutan.

Bagian utama program berada pada fungsi `main`, di mana server dibuat, di-bind ke port tertentu, lalu mulai mendengarkan koneksi.

```c id="j7v2qo"
int main()
{
    int server_fd;
    struct sockaddr_in addr;
    fd_set readfds;

    start_time = time(NULL);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
```

Server menggunakan `select()` untuk memantau banyak file descriptor sekaligus, sehingga dapat menangani banyak client tanpa blocking.

```c id="p9z3wy"
select(maxfd + 1, &readfds, NULL, NULL, NULL);
```

Jika ada koneksi baru, server akan memanggil `accept_client`, sedangkan jika ada aktivitas dari client, maka akan diproses oleh `handle_client`.

Secara keseluruhan, program ini berhasil mengimplementasikan server yang mampu menangani banyak client secara bersamaan, melakukan broadcast pesan, menyediakan fitur admin berbasis RPC, serta mencatat seluruh aktivitas ke dalam log sesuai dengan spesifikasi soal.


## Soal 2 - The Arena

## Penjelasan Program `arena.h` (Struktur Data Arena)

File `arena.h` berfungsi sebagai definisi struktur data utama yang digunakan dalam sistem arena pada Soal 2. File ini menjadi fondasi untuk menyimpan seluruh informasi terkait pemain (*player*), riwayat pertandingan, serta kondisi keseluruhan arena.

Dengan memusatkan semua struktur data dalam satu file header, program menjadi lebih terorganisir dan memudahkan penggunaan data yang sama di berbagai file seperti `eternal.c` dan `orion.c`.

```c id="1wq8kp"
#ifndef ARENA_H
#define ARENA_H

#define MAX_USER 50
#define MAX_LOG 200

typedef struct
{
    char username[50];
    char password[50];

    int online;
    int in_battle;
    int searching;

    int level;
    int xp;
    int gold;

    int weapon;
} Player;

typedef struct
{
    char user[50];
    char enemy[50];
    char result[10];
    int xp;
} History;

typedef struct
{
    Player users[MAX_USER];
    int user_count;

    History logs[MAX_LOG];
    int log_count;
} Arena;

#endif
```

Pada bagian awal, digunakan *header guard* untuk mencegah file ini di-include lebih dari satu kali yang dapat menyebabkan error saat proses kompilasi.

Selanjutnya, didefinisikan dua konstanta utama, yaitu `MAX_USER` sebagai batas maksimal jumlah pemain yang dapat terdaftar dalam sistem, serta `MAX_LOG` sebagai batas maksimal jumlah riwayat pertandingan yang dapat disimpan.

Struktur `Player` digunakan untuk menyimpan seluruh informasi terkait seorang pemain. Di dalamnya terdapat atribut seperti `username` dan `password` untuk autentikasi, serta beberapa status seperti `online`, `in_battle`, dan `searching` yang digunakan untuk mengetahui kondisi pemain saat ini. Selain itu, terdapat juga atribut perkembangan pemain seperti `level`, `xp`, `gold`, dan `weapon` yang menggambarkan progres permainan.

Struktur `History` digunakan untuk mencatat riwayat pertandingan antar pemain. Data yang disimpan meliputi nama pemain (`user`), nama lawan (`enemy`), hasil pertandingan (`result`), serta jumlah pengalaman (`xp`) yang diperoleh dari pertarungan tersebut.

Struktur utama `Arena` berfungsi sebagai wadah keseluruhan sistem. Struktur ini menyimpan array dari `Player` dan `History`, serta variabel penghitung (`user_count` dan `log_count`) untuk mengetahui jumlah data yang sedang aktif.

Secara keseluruhan, file `arena.h` menjadi komponen penting dalam sistem karena mengatur bagaimana data disimpan, diakses, dan dikelola selama program berjalan.

## Penjelasan Program `eternal.c` (Client / Battle System)

File `eternal.c` merupakan program utama yang digunakan oleh pemain untuk berinteraksi dengan sistem arena. Program ini berjalan sebagai client yang terhubung ke shared memory yang sama dengan server (`orion.c`), sehingga memungkinkan banyak user bermain dalam satu sistem yang sama secara bersamaan.

Pada bagian awal program, digunakan beberapa library tambahan seperti `sys/shm.h` dan `sys/sem.h` yang berfungsi untuk mengakses shared memory serta semaphore. Mekanisme ini digunakan agar data dapat dibagi antar proses dengan aman tanpa terjadi konflik.

```c id="c9f2xk"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include "arena.h"
```

Program menggunakan shared memory dengan key tertentu untuk mengakses data arena yang berisi seluruh player dan history pertandingan.

```c id="t7n1qw"
#define SHM_KEY 1234
#define SEM_KEY 5678
```

Untuk menjaga agar tidak terjadi race condition saat banyak user mengakses data secara bersamaan, digunakan mekanisme semaphore melalui fungsi `lock` dan `unlock`.

```c id="v3k8po"
void lock()
{
    struct sembuf op = {0, -1, 0};
    semop(semid, &op, 1);
}

void unlock()
{
    struct sembuf op = {0, 1, 0};
    semop(semid, &op, 1);
}
```

Program juga memiliki beberapa fungsi utilitas seperti pencarian user (`find`) serta perhitungan atribut player seperti attack, health, dan level berdasarkan XP yang dimiliki.

```c id="m2q9re"
int get_atk(Player *p)
{
    return 10 + (p->xp / 50) + p->weapon;
}

int get_hp(Player *p)
{
    return 100 + (p->xp / 10);
}

int get_lvl(Player *p)
{
    return (p->xp / 100) + 1;
}
```

Pada fitur register, user dapat membuat akun baru dengan username dan password. Sistem akan memastikan bahwa username tidak boleh duplikat sebelum menyimpan data ke dalam arena.

```c id="d8p4tz"
void register_user()
{
    char u[50], p[50];

    printf("\n=== REGISTER ===\n");
    printf("Username: ");
    scanf("%s", u);
```

Setelah berhasil register, user dapat login. Sistem akan memverifikasi username dan password serta memastikan user tidak sedang login di tempat lain.

```c id="q5k1vz"
Player *login_user()
{
    char u[50], p[50];

    printf("\n=== LOGIN ===\n");
```

Fitur armory memungkinkan player membeli senjata menggunakan gold yang dimiliki. Setiap senjata memberikan tambahan damage yang berbeda, dan hanya senjata dengan damage lebih besar yang akan menggantikan senjata sebelumnya.

```c id="p1z7lh"
void armory(Player *p)
{
    int c;
    int dmg[] = {5, 15, 30, 60};
    int price[] = {100, 300, 600, 1500};
```

Selain itu, terdapat fitur history yang menampilkan riwayat pertandingan yang pernah terjadi di arena.

```c id="r4w2yo"
void show_history()
{
    system("clear");
```

Salah satu bagian penting dalam sistem ini adalah matchmaking, yaitu proses mencari lawan secara otomatis. Player yang sedang mencari akan dipasangkan dengan player lain yang juga dalam kondisi searching. Jika dalam waktu tertentu tidak ditemukan lawan, maka sistem akan memberikan lawan berupa BOT.

```c id="k8x3mw"
Player *matchmaking(Player *me)
{
    lock();
    me->searching = 1;
```

Setelah mendapatkan lawan, proses battle dimulai. Sistem akan menghitung HP dan attack masing-masing player, kemudian menjalankan pertarungan berbasis giliran hingga salah satu kalah.

```c id="u9s2bc"
void battle(Player *me)
{
    Player *enemy = matchmaking(me);
```

Selama battle berlangsung, player dapat memilih untuk melakukan serangan biasa atau menggunakan ultimate (jika memiliki senjata). Semua aksi akan ditampilkan dalam combat log sederhana.

Di akhir pertarungan, hasil akan ditentukan dan player akan mendapatkan reward berupa XP dan gold. Hasil pertandingan juga akan disimpan ke dalam history.

```c id="z6v1hn"
if (hp1 > 0)
{
    printf("===== VICTORY =====\n");
    me->xp += 50;
```

Pada fungsi `main`, program akan terlebih dahulu mencoba mengakses shared memory yang sudah dibuat oleh server. Jika server belum berjalan, maka program akan menampilkan pesan error.

```c id="y2n8fq"
int shmid = shmget(SHM_KEY, sizeof(Arena), 0666);
if (shmid == -1)
{
    printf("ORION belum jalan!\n");
    return 1;
}
```

Setelah berhasil terhubung, user akan diberikan menu utama untuk melakukan register, login, hingga masuk ke dalam sistem arena.

```c id="w3k9pz"
while (1)
{
    printf("\n1. Register\n2. Login\n3. Exit\nChoice: ");
```

Setelah login, player dapat memilih berbagai fitur seperti battle, armory, melihat history, atau logout.

```c id="n7c4xd"
printf("1. Battle\n2. Armory\n3. History\n4. Logout\nChoice: ");
```

Secara keseluruhan, program `eternal.c` berhasil mengimplementasikan sistem permainan berbasis arena yang interaktif, mendukung banyak user, serta menggunakan shared memory untuk berbagi data secara real-time antar proses.

## Penjelasan Program `orion.c` (Server / Memory Manager)

File `orion.c` berperan sebagai server utama dalam sistem arena yang bertugas untuk menyiapkan dan mengelola shared memory serta semaphore. Program ini menjadi fondasi agar seluruh client (`eternal.c`) dapat saling berbagi data secara real-time dalam satu ruang memori yang sama.

Berbeda dengan pendekatan socket pada Soal 1, pada Soal 2 komunikasi antar proses dilakukan menggunakan mekanisme **shared memory**, sehingga semua data seperti player dan history tersimpan secara terpusat.

Pada bagian awal program, digunakan beberapa library khusus yang mendukung penggunaan shared memory (`sys/shm.h`) dan semaphore (`sys/sem.h`).

```c id="c1k9xp"
#include <stdio.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <unistd.h>
#include "arena.h"
```

Program kemudian mendefinisikan dua buah key, yaitu `SHM_KEY` untuk shared memory dan `SEM_KEY` untuk semaphore. Key ini harus sama dengan yang digunakan pada client agar dapat mengakses resource yang sama.

```c id="x4n2rf"
#define SHM_KEY 1234
#define SEM_KEY 5678
```

Langkah pertama yang dilakukan adalah membuat atau mengambil shared memory menggunakan fungsi `shmget`. Jika shared memory belum ada, maka akan dibuat dengan flag `IPC_CREAT`.

```c id="b7q3vz"
int shmid = shmget(SHM_KEY, sizeof(Arena), IPC_CREAT | 0666);
if (shmid == -1)
{
    perror("shmget");
    return 1;
}
```

Setelah shared memory berhasil dibuat, program akan melakukan attach ke address space proses menggunakan `shmat`, sehingga data di dalamnya dapat diakses seperti variabel biasa.

```c id="m9w2ka"
Arena *arena = shmat(shmid, NULL, 0);
if (arena == (void *)-1)
{
    perror("shmat");
    return 1;
}
```

Selanjutnya, program membuat semaphore yang digunakan sebagai mekanisme penguncian (locking) untuk mencegah konflik akses data ketika banyak client berjalan secara bersamaan.

```c id="p8t5yd"
int semid = semget(SEM_KEY, 1, IPC_CREAT | 0666);
if (semid == -1)
{
    perror("semget");
    return 1;
}
```

Semaphore kemudian diinisialisasi dengan nilai 1, yang berarti hanya satu proses yang dapat mengakses data pada satu waktu (mutual exclusion).

```c id="z2l7hk"
semctl(semid, 0, SETVAL, 1);
```

Setelah semua inisialisasi selesai, server akan menampilkan pesan bahwa sistem sudah siap digunakan.

```c id="n6x1jf"
printf("=== ORION READY ===\n");
```

Program kemudian masuk ke dalam loop tak hingga menggunakan `sleep`, yang bertujuan untuk menjaga agar proses tetap berjalan dan shared memory tetap tersedia selama client masih membutuhkan akses.

```c id="r3k8mw"
while (1)
    sleep(1);
```

Secara keseluruhan, `orion.c` tidak melakukan interaksi langsung dengan user, melainkan bertugas sebagai penyedia resource utama berupa shared memory dan semaphore. Tanpa program ini, client tidak akan dapat berjalan karena tidak memiliki tempat untuk menyimpan dan berbagi data.

Dengan adanya `orion.c`, sistem arena dapat berjalan secara multi-process dengan data yang konsisten dan terkoordinasi dengan baik.