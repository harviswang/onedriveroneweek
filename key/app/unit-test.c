#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>

#define DEVICE "/dev/keys"
#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) (sizeof(x)/sizeof(x[0]))
#endif

int main(int argc, char **argv)
{
	int fd;
	char key_last_value[6] = {'0', '0', '0', '0', '0', '0'}; 
	char key_current_value[6]; 
	int err;
	int i;

	if ((fd = open(DEVICE, O_RDONLY)) < 0) {
		fprintf(stderr, "open(%s) filed !\n", DEVICE);
		exit(1);
	}
	
	while(1) {
		if ((err = read(fd, (void *)key_current_value, ARRAY_SIZE(key_current_value))) != ARRAY_SIZE(key_current_value)) {
			fprintf(stderr, "read(%d) failed !\n", fd);
			exit(2);
		}

		for (i = 0; i < 6; i++)
			if (key_current_value[i] != key_last_value[i]) {
				key_last_value[i] = key_current_value[i];	
				printf("KEY%d %s\n", i + 1, key_current_value[i] == '0' ? "up" : "down");
			}
		}

	close(fd);
}	
