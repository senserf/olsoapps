#include <sys/file.h>

// cmp -b Image Image1 gives offset N
int main(int argc, char *argv[]) {
	int fd;
	char c;
	const off_t pos = 27143 -1; // offset N -1

	if (argc > 2) {
		printf ("Usage: lh [<lh>]");
		return 1;
	}
	if ((fd = open ("Image", O_RDWR, 0)) <= 0 ||
		lseek(fd, pos, 0) != pos) {
		printf ("Can't process Image");
		return 1;
	}
	if (argc == 1) {
		if (read(fd, &c, 1) == 1) {
			printf ("Read lh %d", c);
			close (fd);
			return 0;
		}
		printf ("Read failed");
		return 1;
	}

	c =atoi(argv[1]);
	if (write(fd, &c, 1) == 1) {
			printf ("Wrote lh %d", c);
			close (fd);
			return 0;
	}
	printf ("Write failed");
	return 1;
}
