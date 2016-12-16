#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/user.h> /* PAGE_SIZE */
#include <errno.h> /* errno */

int main(int argc, char **argv)
{
	int fd;
	int i;
	unsigned char *p_mmap;
	
	/* open device */
	fd = open("/dev/mymmap", O_RDWR);
	if (fd < 0) {
		perror("open() failed");
		return errno;
	} else {
		/* map memory */
		p_mmap = (unsigned char *)mmap(0, PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
		if (p_mmap == MAP_FAILED) {
			perror("mmap() failed\n");
			return errno;
		} else {
			/* print ten bytes */
			for (i = 0; i < PAGE_SIZE; i++) {
				printf("%d\n", p_mmap[i]);
			}
			return 0;
		}
	}
}	
