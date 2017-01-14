#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

#define DEVICE "/dev/completion"
#define BUFFER_SIZE 4096

int main(int argc, char *argv[])
{
    int fd;
    char write_buf[] = "work hard with a good luck";
    char read_buf[BUFFER_SIZE];
    int n;
    pid_t write_pid;
    int i;

    fd = open(DEVICE, O_RDWR);
    if (fd < 0) {
            perror("open " DEVICE " failed");
            return errno;
    }
    if ((write_pid = fork()) == 0) {
        for (i = 0; i < 1; i++) {
            n = write(fd, write_buf, sizeof(write_buf));
            printf("write thread: n = %d\n", n);
        }
    } else {
        printf("read thread: fd = %d\n", fd);
        n = read(fd, read_buf, 27);
        read_buf[n] = '\0';
        printf("read thread: %s\n", read_buf);
    }

    close(fd);

    return 0;
}