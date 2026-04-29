#include <stdio.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <unistd.h>
#include "arena.h"

#define SHM_KEY 1234
#define SEM_KEY 5678

int main()
{
    int shmid = shmget(SHM_KEY, sizeof(Arena), IPC_CREAT | 0666);
    if (shmid == -1)
    {
        perror("shmget");
        return 1;
    }

    Arena *arena = shmat(shmid, NULL, 0);
    if (arena == (void *)-1)
    {
        perror("shmat");
        return 1;
    }

    int semid = semget(SEM_KEY, 1, IPC_CREAT | 0666);
    if (semid == -1)
    {
        perror("semget");
        return 1;
    }

    semctl(semid, 0, SETVAL, 1);

    printf("=== ORION READY ===\n");

    while (1)
        sleep(1);
}