#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

#define DEVICE "/dev/complete"

int main(int argc, char *argv[])
{
	int fd;
	char buf[128];
	int n;
	
	fd = open(DEVICE, O_RDWR);
	if (fd < 0) {
		perror("open " DEVICE " failed");
		return errno;
	}
	
	printf("fd = %d\n", fd);
	n = read(fd, buf, 8);
	buf[n] = '\0';

	printf("buf:%s n:%d\n", buf, n);
	close(fd);

	return 0;
}