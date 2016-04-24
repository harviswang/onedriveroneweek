#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/param.h> /* PAGE_SIZE */

int main(int argc, char **argv)
{
	int fd;
	int i;
	unsigned char *p_mmap;
	
	/* open device */
	fd = open("/dev/mymmap", O_RDWR);
	if (fd < 0) {
		printf("open() failed\n");
		return errno;
	} else {
		/* map memory */
		p_mmap = (unsigned char *)mmap(0, PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
		if (p_mmap == MAP_FAILED) {
			printf("mmap() failed\n");
			return errno;
		} else {
			/* print ten bytes */
			for (i = 0; i < 10; i++) {
				printf("%d\n", p_mmap(i);
			}
			return 0;
		}
	}
}	
