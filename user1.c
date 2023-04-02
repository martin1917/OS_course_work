#include <sys/shm.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/sem.h>

#define SHM_KEY_1 12
#define SHM_KEY_2 34
#define SHM_SIZE 1024

union semafSettings {
    int val;
    struct semid_ds *buf;
    unsigned short *array;
    struct seminfo *__buf;
};

typedef struct 
{
    char data[SHM_SIZE];
} mailbox_t;

// общая память
int shmid_1;
mailbox_t *mbox_1;
int shmid_2;
mailbox_t *mbox_2;

// операции над семафорами
struct sembuf lock_res_w = {0, -1, 0};
struct sembuf rel_res_w = {0, 1, 0};
struct sembuf lock_res_r = {1, -1, 0};
struct sembuf rel_res_r = {1, 1, 0};

// id семафора
int semafor;

void* handle_read(void* args)
{
    while (1)
    {
        semop(semafor, &lock_res_r, 1);
        printf("\nrecived: %s", mbox_2->data);
        semop(semafor, &rel_res_r, 1);
    }
}

int main()
{
    // Создание или получение идентификатора разделяемой памяти (запись)
    if ((shmid_1 = shmget(SHM_KEY_1, sizeof(mailbox_t), IPC_CREAT | 0666)) < 0) {
        perror("shmget_1");
        exit(1);
    }

    // Присоединение разделяемой памяти к адресному пространству процесса
    if ((mbox_1 = (mailbox_t *) shmat(shmid_1, NULL, 0)) == (mailbox_t *) -1) {
        perror("shmat_1");
        exit(1);
    }

    // Создание или получение идентификатора разделяемой памяти (чтение)
    if ((shmid_2 = shmget(SHM_KEY_2, sizeof(mailbox_t), IPC_CREAT | 0666)) < 0) {
        perror("shmget_2");
        exit(1);
    }

    // Присоединение разделяемой памяти к адресному пространству процесса
    if ((mbox_2 = (mailbox_t *) shmat(shmid_2, NULL, 0)) == (mailbox_t *) -1) {
        perror("shmat_2");
        exit(1);
    }

    // создание ключа для получения семафора
    key_t key = ftok("user1.c", 0);
    if (key == -1) {
        perror("Error create key");
        exit(1);
    }

    // получение семафоров
    semafor = semget(key, 2, IPC_CREAT | 0644);
    if (semafor == -1) {
        perror("Can't create semafor");
        exit(1);
    }

    // установка значения в 0 для семафора на запись
    union semafSettings arg;
    arg.val = 0;
    semctl(semafor, 0, SETVAL, arg);

    pthread_t thread;
    pthread_create(&thread, NULL, handle_read, NULL);
    
    while (1)
    {
        printf("> ");
        char buf[SHM_SIZE];
        fgets(buf, SHM_SIZE, stdin);

        strcpy(mbox_1->data, buf);
        semop(semafor, &rel_res_w, 1);
        semop(semafor, &lock_res_w, 1);
    }

    if (semctl(semafor, 0, IPC_RMID) < 0) {
        perror("Невозможно удалить семафор\n");
        exit(1);
    }
    return 0;
}