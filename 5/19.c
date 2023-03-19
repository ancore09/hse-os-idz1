#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#define buf_size 1024

int main(int argc, char *argv[])
{
    char name[] = "fifo.fifo";

    remove(name);

    // create the pipe
    (void)umask(0);
    mknod(name, S_IFIFO | 0666, 0);

    // create the first child process
    pid_t pid1 = fork();
    if (pid1 == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    } else if (pid1 == 0) {
        // read from file in argv[1]

        int fdisc = open(argv[1], O_RDONLY | O_CREAT, 0644);

        char buf[buf_size];

        int read_bytes = read(fdisc, buf, buf_size);
        if (read_bytes == -1) {
            perror("read");
            exit(EXIT_FAILURE);
        }

        int fd = open(name, O_WRONLY);

        write(fd, buf, buf_size);
        close(fd);
        close(fdisc);

        printf("Input exit\n");
        exit(EXIT_SUCCESS);
    }

    // create the second child process
    pid_t pid2 = fork();
    if (pid2 == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    } else if (pid2 == 0) {
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
        exit(EXIT_SUCCESS);
    } 

    // create the third child process
    pid_t pid3 = fork();
    if (pid3 == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    } else if (pid3 == 0) {
        // child3: reads from pipe and writes to console
        //close(fd[1]);
        int buf[128];
        int fd = open(name, O_RDONLY);
        read(fd, buf, sizeof(buf));
        close(fd);

        char buf_out[buf_size] = {0};

        int fdisk = open(argv[2], O_WRONLY | O_CREAT, 0644);
        int written = 0;
        for (int i = 32; i < 127; i++) {
            if (buf[i] != 0) {
                written += sprintf(buf_out+written, "%c: %d\n", i, buf[i]);
            }
        }

        write(fdisk, buf_out, written);

        close(fdisk);

        printf("Write exit\n");
        exit(EXIT_SUCCESS);
    }

    // parent process: wait for all child processes to exit

    waitpid(pid1, NULL, 0);
    waitpid(pid2, NULL, 0);
    waitpid(pid3, NULL, 0);

    return 0;
}
