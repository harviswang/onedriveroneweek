#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>

#define DEVICE "/dev/mini2440-leds"
int main(int argc, char **argv)
{
	int led_id; /* just 1,2,3 and 4 is valid*/
	int led_switch;
	int fd;

	if (argc != 3 || 
		sscanf(argv[1], "%d", &led_id) != 1 || 
		sscanf(argv[2], "%d", &led_switch) != 1 ||
		led_id > 4 || led_id < 1 ||
		led_switch > 1 || led_switch < 0) {
		fprintf(stderr, "Usage: unit-test led_id<1,2,3 or 4> led_switch<0 or 1> \n");
		exit(1);
	}

	if ((fd = open(DEVICE, 0)) < 0) {
		fprintf(stderr, "open file:%s failed\n", DEVICE);
		exit(2);
	}
	
	printf("fd=%d led_witch=%d led_id=%d\n", fd, led_switch, led_id);
	ioctl(fd, led_switch, led_id - 1); /* user sapce indexed by 1, kernel space indexed by 0, so sub 1 here */

	close(fd);
}
