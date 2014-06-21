#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#define DEVICE "/dev/mini2440-adc"
int main(int argc, char **argv)
{
	int fd;
	int count = 0;

	fd = open(DEVICE, 0);
	if (fd < 0) {
		fprintf(stderr, "open(%s) failed\n", DEVICE);
		exit(1);
	}

	while (1) {
		int ret;
		char buff[10];

		ret = read(fd, buff, 10);
		if (ret > 0) {
			buff[ret] = '\0';
			printf("[%d]ADC current value is %s\n", ++count, buff);
			sleep(1);
		} else {
			continue;
		}
	}

	close(fd);

	return 0;
}
