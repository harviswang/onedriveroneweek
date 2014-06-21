#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

#define PWM_IOCTL_SET_FREQ 1
#define PWM_IOCTL_STOP     0

#define ESC_KEY 0x1b


static int getkey(void);
int main(int argc, char **argv)
{
	int freq = 1000;


	while (1) {
		int key;


		key = getkey();
		printf("key = %c\n", key);
	}
}


static int getkey(void)
{
	struct termios oldt, newt;
	int ch;

	if (!isatty(STDIN_FILENO)) {
		fprintf(stderr, "This problem should be run at a terminal\n");
		exit(1);
	}

	/* Save terminal setting */
	if (tcgetattr(STDIN_FILENO, &oldt) < 0) {
		fprintf(stderr, "Save termianl setting failed\n");
		exit(2);
	}

	/* Set terminal as need */
	newt = oldt;
	newt.c_lflag &= ~(ICANON | ECHO);

	/* Using new terminal setting */
	if (tcsetattr(STDIN_FILENO, TCSANOW, &newt) < 0) {
		fprintf(stderr, "tcsetattr() failed\n");
		exit(3);
	}

	printf("before getchar()\n");
	ch = getchar();
	printf("after getchar()\n");

	/* Restore old terminal setting */
	if (tcsetattr(STDIN_FILENO, TCSANOW, &oldt) < 0) {
		fprintf(stderr, "tcsetarrt() filed\n");
		exit(4);
	}

	return ch;
}

