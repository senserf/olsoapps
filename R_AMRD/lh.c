#include <sys/file.h>

// cmp -b Image Image1 gives offset N
int main(int argc, char *argv[]) {
	int fd;
	char c;
	const off_t pos = 21205 -1; // offset N -1

	if (argc > 2) {
		printf ("Usage: lh [<lh>]\n");
		return 1;
	}
	if ((fd = open ("Image", O_RDWR, 0)) <= 0 ||
		lseek(fd, pos, 0) != pos) {
		printf ("Can't process Image\n");
		return 1;
	}
	if (argc == 1) {
		if (read(fd, &c, 1) == 1) {
			printf ("Read lh %d\n", c);
			close (fd);
			return 0;
		}
		printf ("Read failed\n");
		return 1;
	}

	c =atoi(argv[1]);
	if (write(fd, &c, 1) == 1) {
			printf ("Wrote lh %d\n", c);
			close (fd);
			return 0;
	}
	printf ("Write failed\n");
	return 1;
}
