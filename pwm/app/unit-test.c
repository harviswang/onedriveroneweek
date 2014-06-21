#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

#define PWM_IOCTL_SET_FREQ 1
#define PWM_IOCTL_STOP     0

#define ESC_KEY 0x1b
#define DEVICE "/dev/pwm"

static int fd = -1;

static int getkey(void);
static void buzzer_open(void);
static void buzzer_close(void);
static void buzzer_set_freq(int freq);
static void buzzer_stop(void);
static void buzzer_help(void);

int main(int argc, char **argv)
{
	int freq = 1000;

	buzzer_help();
	buzzer_open();

	while (1) {
		int key;

		buzzer_set_freq(freq);
		printf("\tFreq = %d\n", freq);

		key = getkey();
		switch(key) {
			case '+':
				if (freq < 20000)
					freq += 10;
				break;
			case '-':
				if (freq > 11)
					freq -= 10;
				break;
			case ESC_KEY:
			case EOF:
				buzzer_stop();
				return 0;
			default:
				buzzer_help();
				continue;
		}
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

static void buzzer_open(void)
{
	if ((fd = open(DEVICE, 0)) < 0) {
		fprintf(stderr, "open(%s) failed\n", DEVICE);
		exit(5);
	}

	/* Any function exit will call buzzer_close() function */
	atexit(buzzer_close);
}

static void buzzer_close(void)
{
	if (fd >= 0) {
		ioctl(fd, PWM_IOCTL_STOP);
		close(fd);
		fd = -1;
	}
}

static void buzzer_set_freq(int freq)
{
	int ret;

	if (fd >= 0) {
		ret = ioctl(fd, PWM_IOCTL_SET_FREQ, freq);
		if (ret < 0) {
			fprintf(stderr, "ioctl() failed\n");
			exit(5);
		}
	}
}

static void buzzer_stop(void)
{
	int ret;
	if (fd >= 0) {
		ret = ioctl(fd, PWM_IOCTL_STOP);
		if (ret < 0) {
			fprintf(stderr, "ioctl() failed\n");
			exit(5);
		}
	}
}

static void buzzer_help(void)
{
	printf("\nBUZZER TEST (PWM Control)\n");
	printf("Press +/- to increase/decrease the frequency of the BUZZER\n");
	printf("Press 'ESC' key to exit this program\n\n");
}

