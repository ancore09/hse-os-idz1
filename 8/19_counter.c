#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#define buf_size 1024

int main()
{
    int semid;
    key_t key;
    struct sembuf sem_wait = {0, -1, SEM_UNDO};
    struct sembuf sem_signal = {0, 1, SEM_UNDO};

    key = 5678;

    char name[] = "fifo.fifo";

    // создаем семафор для синхронизации
    if ((semid = semget(key, 1, IPC_CREAT | 0666)) == -1) {
        perror("semget");
        exit(1);
    }

    // ждем сигнала от клиента
    if (semop(semid, &sem_wait, 1) == -1) {
        perror("semop");
        exit(1);
    }

    char buf[buf_size];

    int fd = open(name, O_RDWR);
    read(fd, buf, buf_size);

    int counts[128] = {0};
    for (int i = 0; i < buf_size; i++) {
        if (buf[i] == '\0') {
            break;
        }
        if (buf[i] < 32 || buf[i] >= 127) {
            continue;
        }
        counts[buf[i]]++;
    }

    write(fd, counts, sizeof(counts));
    close(fd);

    printf("Count exit\n");

    if (semop(semid, &sem_signal, 1) == -1) {
        perror("semop");
        exit(1);
    }

    return 0;
}
