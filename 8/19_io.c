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

int main(int argc, char *argv[])
{
    int semid;
    key_t key;
    struct sembuf sem_wait = {0, -1, SEM_UNDO};
    struct sembuf sem_signal = {0, 1, SEM_UNDO};

    key = 5678;

    char name[] = "fifo.fifo";

    remove(name);

    (void)umask(0);
    mknod(name, S_IFIFO | 0666, 0);

    if ((semid = semget(key, 1, IPC_CREAT | 0666)) == -1) {
        perror("semget");
        exit(1);
    }

    if (semctl(semid, 0, SETVAL, 0) == -1) {
        perror("semctl");
        exit(1);
    }

    int fdisc = open(argv[1], O_RDONLY | O_CREAT);

    char buf[buf_size];

    int read_bytes = read(fdisc, buf, buf_size);
    if (read_bytes == -1) {
        perror("read");
        exit(EXIT_FAILURE);
    }

    int fd = open(name, O_RDWR);

    write(fd, buf, buf_size);

    close(fdisc);

    printf("Input exit\n");

    if (semop(semid, &sem_signal, 1) == -1) {
        perror("semop");
        exit(1);
    }

    usleep(10);

    if (semop(semid, &sem_wait, 1) == -1) {
        perror("semop");
        exit(1);
    }

    int buf1[128];
    read(fd, buf1, sizeof(buf1));

    close(fd);

    char buf_out[buf_size] = {0};

    int fdisk = open(argv[2], O_WRONLY | O_CREAT);
    int written = 0;
    for (int i = 32; i < 127; i++) {
        if (buf1[i] != 0) {
            written += sprintf(buf_out+written, "%c: %d\n", i, buf1[i]);
        }
    }

    write(fdisk, buf_out, written);

    close(fdisk);

    printf("Write exit\n");

    if (semctl(semid, 0, IPC_RMID, 0) == -1) {
        perror("semctl");
        exit(1);
    }

    return 0;
}
