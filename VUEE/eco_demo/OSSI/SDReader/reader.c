#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

#define	MAGIC	0x000a00ec
#define	BUFSIZE	(512*16)

#define	ONEMEG	(1024*1024)

int	sd_desc;
off_t	sd_size;

unsigned char gbuf [BUFSIZE];

int open_eco_sd (const char *fn) {

	int i, j;
	char vname [512];
	unsigned char buf [4];
	unsigned int l;
	off_t nb, be, en;

	if (fn != NULL) {
		if ((sd_desc = open (fn, O_RDONLY)) < 0) {
			perror ("cannot open this device");
			return sd_desc = -1;
		}
	} else {
		for (i = 0; i < 32; i++) {
			j = i >> 1;
			sprintf (vname, "/dev/sd%c%s",
				'b' + j, (i & 1) ? "1" : "");
			if ((sd_desc = open (vname, O_RDONLY)) < 0)
				continue;
			// Read the signature
			if (read (sd_desc, buf, 4) != 4) {
				close (sd_desc);
				continue;
			}

			l =   (unsigned int) buf [0] 		|
			    (((unsigned int) buf [1]) <<  8) 	|
			    (((unsigned int) buf [2]) << 16) 	|
			    (((unsigned int) buf [3]) << 24);

			if (l != MAGIC)
				continue;

			printf ("found card at %s\n", vname);
			goto Found;
		}

		fprintf (stderr, "couldn't find a valid card\n");
		return sd_desc = -1;
	}

Found:
	// Locate the end
	printf ("Locating EOF on card ...\n");

	be = 0;
	// This is 4G / 512, i.e., more than the max number of block
	en = 1024 * 1024 * 4 * 2;

	while (be + 1 < en) {

		nb = (be + en) / 2;

		if (lseek (sd_desc, nb * 512, SEEK_SET) < 0) {
			// Assume we are too far
TooFar:
			en = nb;
			continue;
		}

		if (read (sd_desc, vname, 512) <= 0)
			// Too far?
			goto TooFar;

		be = nb;
	}

	// Trailing check

	lseek (sd_desc, be * 512, SEEK_SET);
	if ((l = read (sd_desc, vname, 512)) < 0) {
		// This cannot happen
		perror ("trailer read failed (1)");
Err:
		close (sd_desc);
		return sd_desc = -1;
	}

	if (l != 0) {
		// One more block
		be++;
		if ((l = read (sd_desc, vname, 512)) < 0) {
			perror ("read 2 failed");
			goto Err;
		}
		if (l == 512)
			be++;
	}

	sd_size = be * 512;
	printf ("EOF reached, card size = %lld bytes\n", sd_size);

	return sd_desc;
}

char *mgetl () {
//
// Read line from standard input
//
	int cl, lp, i, c;
	char *ln, *aux;

	while (1) {
		c = getchar ();
		if (c == EOF)
			return NULL;
		if (!isspace (c))
			break;
	}

	// First non-blank

	if ((ln = malloc (cl = 80)) == NULL)
		return NULL;

	ln [0] = (char) c;
	lp = 1;

	while ((c = getchar ()) != EOF && c != '\n') {
		if (lp + 1 == cl) {
			// Grow
			if ((aux = malloc (cl = cl + cl)) == NULL) {
				free (ln);
				return NULL;
			}
			for (i = 0; i < lp; i++)
				aux [i] = ln [i];

			free (ln);
			ln = aux;
		}
		ln [lp++] = c;
	}

	while (isspace (ln [lp - 1])) lp--;

	ln [lp] = '\0';

	return ln;
}
			
void menu () {

	printf ("Commands:\n");
	printf ("      d from nbytes [fname]\n");
	printf ("      f from nbytes [-]hexbytes\n");
}

char nspace (const char **ln) {

	while (isspace (**ln)) (*ln)++;
	return **ln;
}

off_t nparse (const char **ln) {

	off_t res;

	nspace (ln);

	for (res = 0; isdigit (**ln); res = res * 10 + ((**ln) - '0'), (*ln)++);

	return res;
}

unsigned char xval (char c) {

	if (c <= 'f' && c >= 'a')
		return (unsigned char)(c - 'a' + 10);

	if (c <= 'F' && c >= 'A')
		return (unsigned char)(c - 'A' + 10);

	if (c <= '9' && c >= '0')
		return (unsigned char)(c - '0');

	return 0;
}

unsigned char *bparse (const char **ln, int *size) {

	const char *lptr;
	unsigned char *res;
	int i;

	for (lptr = *ln, i = 0; *lptr != '\0'; lptr++)
		if (isxdigit (*lptr))
			i++;

	if (i == 0 || (i & 1))
		// Must be an even number of hex digits
		return NULL;

	if ((res = (unsigned char*) malloc (*size = (i >> 1))) == NULL)
		return NULL;

	for (lptr = *ln, i = 0; i < *size; i++) {

		while (!isxdigit (*lptr))
			lptr++;

		res [i] = xval (*lptr) << 4;
		lptr++;

		while (!isxdigit (*lptr))
			lptr++;

		res [i] |= xval (*lptr);
		lptr++;
	}

	*ln = lptr;

	return res;
}

void show_left (off_t left) {

	if (left >= ONEMEG && (left & (ONEMEG - 1)) == 0) {
		printf ("\rleft %8d", (int) (left >> 20));
		fflush (stdout);
	}
}

void do_dump (const char *ln) {

	const char *lptr;
	off_t from, to;
	size_t len, rl;
	int fd;

	lptr = ln + 1;

	from = nparse (&lptr);
	to = nparse (&lptr);

	if (sd_size && from > sd_size) {
		fprintf (stderr, "from (%ld) > size (%ld)\n", from, sd_size);
		return;
	}

	if (sd_size && from + to > sd_size)
		to = sd_size - from;

	to += from;

	while (isspace (*lptr)) lptr++;

	if (lseek (sd_desc, from, SEEK_SET) != from) {
		fprintf (stderr, "cannot seek\n");
		return;
	}

	fd = -1;
	if (*lptr != '\0') {
		// Dump to file
		if ((fd = open (lptr, O_WRONLY|O_CREAT|O_TRUNC, 0777)) < 0) {
			perror ("cannot open dump file");
			return;
		}

		while (from < to) {
			if ((len = to - from) > BUFSIZE)
				len = BUFSIZE;
			if ((rl = read (sd_desc, gbuf, len)) != len) {
				printf ("read length %ld < len %ld\n", rl, len);
DF:				close (fd);
				return;
			}
			if ((rl = write (fd, gbuf, len)) != len) {
				printf ("written length %ld < len %ld\n", rl,
					len);
				goto DF;
			}

			show_left (to - from);
			from += len;
		}
		close (fd);
		printf ("Done\n");
		return;
	}

	// Dump to screen

	while (from < to) {
		if ((len = to - from) > 16)
			len = 16;
		if ((rl = read (sd_desc, gbuf, len)) != rl) {
			printf ("read length %ld < len %ld\n", rl, len);
			goto DF;
		}
		printf ("%11lud: ", from);
		for (rl = 0; rl < len; rl++)
			printf (" %02x", gbuf [rl]);
		printf ("\n");
		from += len;
	}

	close (fd);
}

int ssearch (const unsigned char *buf, int bufl,
	     const unsigned char *b, int bl, int neg) {

	int nt, off;

	nt = bufl - bl;

	if (neg) {
		if (nt < 0)
			return 0;

		for (off = 0; off <= nt; off++) {
			if (bcmp ((const char*)(buf + off), (const char*) b,
				bl) != 0)
					return off;
		}
		return -1;
	} else {
		if (nt < 0)
			return -1;
		for (off = 0; off <= nt; off++) {
			if (bcmp ((const char*)(buf + off), (const char*) b,
				bl) == 0)
					return off;
		}
		return -1;
	}
}
		
void do_find (const char *ln) {

	const char *lptr;
	unsigned char *bytes;
	int bsize, off;
	off_t from, to;
	size_t rl;
	int negative;

	lptr = ln + 1;

	from = nparse (&lptr);
	to = nparse (&lptr);

	if (sd_size && from > sd_size) {
		fprintf (stderr, "from (%ld) > size (%ld)\n", from, sd_size);
		return;
	}

	if (sd_size && from + to > sd_size)
		to = sd_size - from;

	to += from;

	if ((negative = (nspace (&lptr) == '-')))
		lptr ++;

	if ((bytes = bparse (&lptr, &bsize)) == NULL) {
		fprintf (stderr, "illegal format of byte sequence\n");
		return;
	}

	if (bsize > 512) {
		fprintf (stderr, "too many bytes, 512 is max\n");
RF:
		free (bytes);
		return;
	}

	if (lseek (sd_desc, from, SEEK_SET) != from) {
		fprintf (stderr, "cannot seek\n");
		goto RF;
	}

	while (from < to) {
		if ((rl = read (sd_desc, gbuf, 512)) != 512) {
			printf ("read length %ld < 512\n", rl);
			goto RF;
		}
		if ((off = ssearch (gbuf, 512, bytes, bsize, negative)) >= 0)
			break;
		show_left (to - from);
		from += 512;
	}

	free (bytes);

	if (from >= to ) {
		printf ("not found\n");
	} else {
		printf ("found in block %lld at offset %d, position %lld\n",
			from / 512, off, from + off);
	}
}
	
void cmd () {

	char *ln;

	if ((ln = mgetl ()) == NULL) {
		printf ("Done\n");
		exit (0);
	}

	switch (*ln) {

		case 'd' : do_dump (ln); break;
		case 'f' : do_find (ln); break;

		default: 
			printf ("no such command\n");
			menu ();
	}

	free (ln);
}

main (int argc, const char *argv []) {

	if (open_eco_sd (argc > 1 ? argv [1] : NULL) < 0) {
		exit (99);
	}

	menu ();

	while (1)
		cmd ();
		
}
