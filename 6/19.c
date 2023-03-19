#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>
#include <semaphore.h>

#define buf_size 1024

int main(int argc, char *argv[])
{
    sem_t sem;
    sem_init(&sem, 0, 1);

    // create the pipe
    int fd[2];
    if (pipe(fd) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    // create the first child process
    pid_t pid1 = fork();
    if (pid1 == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    } else if (pid1 == 0) {
        // child1: reads input from console and writes to pipe
        sem_wait(&sem);

        // read from file in argv[1]

        int fdisc = open(argv[1], O_RDONLY | O_CREAT, 0644);

        char buf[buf_size];

        int read_bytes = read(fdisc, buf, buf_size);
        if (read_bytes == -1) {
            perror("read");
            exit(EXIT_FAILURE);
        }

        write(fd[1], buf, buf_size);
        close(fd[1]);
        close(fdisc);

        printf("Input exit\n");
        sem_post(&sem);

        usleep(10);
        
        sem_wait(&sem);

        int buf1[128];
        read(fd[0], buf1, sizeof(buf1));
        close(fd[0]);

        char buf_out[buf_size] = {0};

        int fd = open(argv[2], O_WRONLY | O_CREAT, 0644);
        int written = 0;
        for (int i = 32; i < 127; i++) {
            if (buf1[i] != 0) {
                written += sprintf(buf_out+written, "%c: %d\n", i, buf1[i]);
            }
        }

        write(fd, buf_out, written);

        close(fd);

        printf("Write exit\n");
        exit(EXIT_SUCCESS);
    }

    // create the second child process
    pid_t pid2 = fork();
    if (pid2 == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    } else if (pid2 == 0) {
        sem_wait(&sem);
        char buf[buf_size];
        read(fd[0], buf, buf_size);
        close(fd[0]);

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

        write(fd[1], counts, sizeof(counts));
        close(fd[1]);

        printf("Count exit\n");
        sem_post(&sem);

        exit(EXIT_SUCCESS);
    } 

    // parent process: wait for all child processes to exit

    waitpid(pid1, NULL, 0);
    waitpid(pid2, NULL, 0);

    close(fd[0]);
    close(fd[1]);

    sem_destroy(&sem);

    return 0;
}
