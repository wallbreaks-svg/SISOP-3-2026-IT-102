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