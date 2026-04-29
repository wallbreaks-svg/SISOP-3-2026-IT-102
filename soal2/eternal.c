#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include "arena.h"

#define SHM_KEY 1234
#define SEM_KEY 5678

Arena *arena;
int semid;

/* ================= UTIL ================= */

void wait_enter()
{
    int ch;
    while ((ch = getchar()) != '\n' && ch != EOF)
        ;
    printf("\nPress ENTER to continue...");
    getchar();
}

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

Player *find(char *u)
{
    for (int i = 0; i < arena->user_count; i++)
    {
        if (strcmp(arena->users[i].username, u) == 0)
            return &arena->users[i];
    }
    return NULL;
}

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

/* ================= REGISTER ================= */

void register_user()
{
    char u[50], p[50];

    printf("\n=== REGISTER ===\n");
    printf("Username: ");
    scanf("%s", u);

    lock();
    if (find(u))
    {
        printf("Username sudah ada!\n");
        unlock();
        return;
    }

    printf("Password: ");
    scanf("%s", p);

    Player *pl = &arena->users[arena->user_count++];

    strcpy(pl->username, u);
    strcpy(pl->password, p);

    pl->xp = 0;
    pl->gold = 150;
    pl->level = 1;
    pl->weapon = 0;
    pl->online = 0;
    pl->in_battle = 0;
    pl->searching = 0;

    unlock();

    printf("Register berhasil!\n");
}

/* ================= LOGIN ================= */

Player *login_user()
{
    char u[50], p[50];

    printf("\n=== LOGIN ===\n");
    printf("Username: ");
    scanf("%s", u);
    printf("Password: ");
    scanf("%s", p);

    lock();
    Player *pl = find(u);

    if (!pl || strcmp(pl->password, p) != 0)
    {
        printf("Login gagal!\n");
        unlock();
        return NULL;
    }

    if (pl->online)
    {
        printf("User sudah login!\n");
        unlock();
        return NULL;
    }

    pl->online = 1;
    unlock();

    printf("Login berhasil!\n");
    return pl;
}

/* ================= ARMORY ================= */

void armory(Player *p)
{
    int c;
    int dmg[] = {5, 15, 30, 60};
    int price[] = {100, 300, 600, 1500};

    while (1)
    {
        printf("\n=== ARMORY ===\nGold: %d\n", p->gold);

        for (int i = 0; i < 4; i++)
        {
            printf("%d. Weapon +%d (%d gold)\n", i + 1, dmg[i], price[i]);
        }

        printf("0. Back\nChoice: ");
        scanf(" %d", &c);

        if (c == 0)
            break;

        int i = c - 1;

        if (p->gold >= price[i])
        {
            p->gold -= price[i];
            if (dmg[i] > p->weapon)
                p->weapon = dmg[i];

            printf("Berhasil beli!\n");
        }
        else
        {
            printf("Gold tidak cukup!\n");
        }
    }
}

/* ================= HISTORY ================= */

void show_history()
{
    system("clear");

    printf("=========== HISTORY ===========\n\n");

    if (arena->log_count == 0)
    {
        printf("Belum ada riwayat pertandingan.\n");
    }
    else
    {
        for (int i = 0; i < arena->log_count; i++)
        {
            printf("%d. %s vs %s | %s | +%d XP\n",
                   i + 1,
                   arena->logs[i].user,
                   arena->logs[i].enemy,
                   arena->logs[i].result,
                   arena->logs[i].xp);
        }
    }

    wait_enter();
}

/* ================= MATCHMAKING ================= */

Player *matchmaking(Player *me)
{

    lock();
    me->searching = 1;
    me->in_battle = 0;
    unlock();

    printf("Searching for opponent...\n");

    // 🔥 WAJIB nunggu dulu biar semua sempat masuk mode searching
    sleep(1);

    Player *enemy = NULL;

    for (int t = 0; t < 35; t++)
    {

        lock();

        for (int i = 0; i < arena->user_count; i++)
        {
            Player *p = &arena->users[i];

            if (p != me &&
                p->online &&
                p->searching &&
                !p->in_battle)
            {

                p->in_battle = 1;
                me->in_battle = 1;

                p->searching = 0;
                me->searching = 0;

                enemy = p;
                break;
            }
        }

        unlock();

        if (enemy)
            return enemy;

        sleep(1);
    }

    // fallback BOT
    static Player bot;
    strcpy(bot.username, "BOT");
    bot.xp = 0;
    bot.weapon = 0;

    lock();
    me->searching = 0;
    unlock();

    return &bot;
}

/* ================= BATTLE ================= */

void battle(Player *me)
{
    Player *enemy = matchmaking(me);

    int hp1 = get_hp(me);
    int hp2 = get_hp(enemy);

    int atk1 = get_atk(me);
    int atk2 = get_atk(enemy);

    char log[5][100];
    int idx = 0;

    for (int i = 0; i < 5; i++)
        strcpy(log[i], "-");

    char cmd;

    while (hp1 > 0 && hp2 > 0)
    {
        system("clear");

        printf("=========== ARENA ===========\n");
        printf("%s (Lv %d)\n[HP: %d]\n\n",
               enemy->username, get_lvl(enemy), hp2);

        printf("VS\n\n");

        printf("%s (Lv %d) | Weapon: %s\n[HP: %d]\n\n",
               me->username,
               get_lvl(me),
               me->weapon ? "YES" : "NONE",
               hp1);

        printf("----- Combat Log -----\n");
        for (int i = 0; i < 5; i++)
            printf("> %s\n", log[i]);

        printf("\n[a] attack | [u] ultimate\nInput: ");
        scanf(" %c", &cmd);

        if (cmd == 'a')
        {
            hp2 -= atk1;
            sprintf(log[idx % 5], "You hit %d", atk1);
            idx++;
        }
        else if (cmd == 'u')
        {
            if (me->weapon)
            {
                int dmg = atk1 * 3;
                hp2 -= dmg;
                sprintf(log[idx % 5], "ULTIMATE %d", dmg);
                idx++;
            }
            else
            {
                sprintf(log[idx % 5], "No weapon!");
                idx++;
            }
        }

        if (hp2 <= 0)
            break;

        hp1 -= atk2;
        sprintf(log[idx % 5], "Enemy hit %d", atk2);
        idx++;

        sleep(1);
    }

    lock();

    system("clear");

    if (hp1 > 0)
    {
        printf("===== VICTORY =====\n");
        me->xp += 50;
        me->gold += 120;

        strcpy(arena->logs[arena->log_count].result, "WIN");
        arena->logs[arena->log_count].xp = 50;
    }
    else
    {
        printf("===== DEFEAT =====\n");
        me->xp += 15;
        me->gold += 30;

        strcpy(arena->logs[arena->log_count].result, "LOSE");
        arena->logs[arena->log_count].xp = 15;
    }

    strcpy(arena->logs[arena->log_count].user, me->username);
    strcpy(arena->logs[arena->log_count].enemy, enemy->username);
    arena->log_count++;

    me->level = get_lvl(me);
    me->in_battle = 0;

    unlock();

    wait_enter();
}

/* ================= MAIN ================= */

int main()
{
    int shmid = shmget(SHM_KEY, sizeof(Arena), 0666);
    if (shmid == -1)
    {
        printf("ORION belum jalan!\n");
        return 1;
    }

    arena = shmat(shmid, NULL, 0);
    if (arena == (void *)-1)
    {
        printf("Gagal attach memory\n");
        return 1;
    }

    semid = semget(SEM_KEY, 1, 0666);

    Player *me = NULL;
    int c;

    while (1)
    {
        printf("\n1. Register\n2. Login\n3. Exit\nChoice: ");
        scanf(" %d", &c);

        if (c == 1)
            register_user();
        else if (c == 2)
        {
            me = login_user();
            if (me)
                break;
        }
        else
            return 0;
    }

    while (1)
    {
        system("clear");

        printf("=== BATTLE ETERION ===\n");
        printf("%s | Gold:%d | XP:%d | Lvl:%d\n\n",
               me->username, me->gold, me->xp, me->level);

        printf("1. Battle\n2. Armory\n3. History\n4. Logout\nChoice: ");
        scanf(" %d", &c);

        if (c == 1)
            battle(me);
        else if (c == 2)
            armory(me);
        else if (c == 3)
            show_history();
        else
            break;
    }

    me->online = 0;
}