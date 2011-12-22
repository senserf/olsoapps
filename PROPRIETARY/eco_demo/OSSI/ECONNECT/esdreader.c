#include <asm/types.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

typedef __u16 word;
typedef __s16 iword;
typedef __u32 lword;
typedef __s32 ilword;
typedef	unsigned char byte;
typedef	word nid_t;
typedef	byte Boolean;

// ============================================================================

#define	MAGIC	0x00ec
#define	BUFSIZE	512

#define	MINBLOCKS	(1048576- 32768) // 0.5G - epsilon
#define	MAXBLOCKS	(4194304+131072) // 2.0G + epsilon

#define	ONEMEG	(1024*1024)

const char *devname = NULL,		// Forced device name (or number)
	   *prgname = "esdump",		// How we have been called
	   *dmpname = NULL;		// Dump file name

char	   vname [64];			// Generated device name

int	   devskip = 0,			// Devices to skip/scan flag
	   nolabel = 0,			// Ignore label
	   oldvers = 0;			// version 1.2

FILE	   *df;				// Dump file

lword	slot_from = 0xffffffff,
	slot_to   = 0xffffffff;

int	sd_desc;
off_t	sd_size;

unsigned char buf [BUFSIZE];

word ee_read (lword addr, byte *buf, word len) {

	int nb;

	if (lseek (sd_desc, addr, SEEK_SET) != addr) {
		fprintf (stderr, "ee_read: seek failed at %1d\n", addr);
		exit (1);
	}

	while (len) {
		nb = read (sd_desc, buf, len);
		if (nb <= 0) {
			fprintf (stderr, "ee_read: read failed at %1d\n", addr);
			exit (1);
		}
		len -= (word) nb;
		buf += nb;
		addr += nb;
	}
	return 0;
}

// ============================================================================
// Data structures and functions for reading slots from the card ==============
// ============================================================================

#define AGG_ALL			0xE

#define EE_AGG_MAX	(lword)(sd_size / EE_AGG_SIZE -1)
#define EE_AGG_MIN	0L

#define MARK_EMPTY	7
#define MARK_BOOT	6
#define MARK_PLOT	5
#define MARK_SYNC	4
#define MARK_MCHG	3
#define MARK_DATE	2

typedef union {
	word b:8;
	struct {
		word emptym :1;
		word mark   :3;
		word status :4;
	} f;
	word spare:8;
} statu_t;

typedef union {
	ilword secs;
	struct {
		word f  :1;
		word yy :5;
		word dd :5;
		word h  :5; // just in case, no word crossing
		word mm :4;
		word m  :6;
		word s  :6;
	} dat;
} mdate_t;

#define SIY     (365 * 24 * 60 * 60)
#define SID     (24 * 60 * 60)

void s2d (mdate_t * in) {

	long l1;
	word w1, w2;

	if (in->secs < 0) {
		l1 = -in->secs;
		in->dat.f = 1;
	} else {
		l1 = in->secs;
		in->dat.f = 0;
	}

	w1 = (word)(l1 / SIY);
	l1 -= SIY * w1 + SID * (w1 >> 2); // leap days in prev. years

	if (l1 < 0) {
		l1 += SIY;
		if (w1-- % 4 == 0)
			l1 += SID;
	}

	w2 = (word)(l1 / SID); // days left base 0

	if (w2 < 31) {
		in->dat.mm = 1;
		l1 -= SID * w2++;
		goto Fin;
	}

	if (w2 < 59 || ((w1 +1) % 4 == 0 && w2 < 60)) { // stupid comp warnings
		in->dat.mm = 2;
		l1 -= SID * w2;
		w2 -= 30;
		goto Fin;
	}

	if ((w1 +1) % 4 != 0) {
		w2++; // equal non-leap years
		l1 += SID;
	}

	if (w2 < 91) {
		in->dat.mm = 3;
		l1 -= SID * w2; 
		w2 -= 59;
		goto Fin;
	}

	if (w2 < 121) {
		in->dat.mm = 4;
		l1 -= SID * w2;
		w2 -= 90;
		goto Fin;
	}

	if (w2 < 152) {
		in->dat.mm = 5;
		l1 -= SID * w2;
		w2 -= 120;
		goto Fin;
	}

	if (w2 < 182) {
		in->dat.mm = 6;
		l1 -= SID * w2;
		w2 -= 151;
		goto Fin;
	}

	if (w2 < 213) {
		in->dat.mm = 7;
		l1 -= SID * w2;
		w2 -= 181;
		goto Fin;
	}

	if (w2 < 244) {
		in->dat.mm = 8;
		l1 -= SID * w2;
		w2 -= 212;
		goto Fin;
	}

	if (w2 < 274) {
		in->dat.mm = 9;
		l1 -= SID * w2;
		w2 -= 243;
		goto Fin;
	}

	if (w2 < 305) {
		in->dat.mm = 10;
		l1 -= SID * w2;
		w2 -= 273;
		goto Fin;
	}

	 if (w2 < 335) {
		 in->dat.mm = 11;
		 l1 -= SID * w2;
		 w2 -= 304;
		 goto Fin;
	 }

	 if (w2 < 366) {
		 in->dat.mm = 12;
		 l1 -= SID * w2;
		 w2 -= 334;
		 goto Fin;
	 }

	// error
	in->dat.f = 0;
	in->dat.h = 0;
	in->dat.m = 0;
	in->dat.s = 0;
	in->dat.yy = 0;
	in->dat.mm = 1;
	in->dat.yy = 1;
	return;
Fin:
	in->dat.h = (word)(l1 / 3600);
	l1 %= 3600;
	in->dat.m = (word)(l1 / 60);
	in->dat.s = l1 % 60;
	in->dat.yy = w1;
	in->dat.dd = w2;
}

const char *markName (statu_t s) {

        switch (s.f.mark) {
		case 0:
		case MARK_EMPTY:   return "NONE";
                case MARK_BOOT: return "BOOT";
                case MARK_PLOT: return "PLOT";
                case MARK_SYNC: return "SYNC";
                case MARK_MCHG: return "MCHG";
                case MARK_DATE: return "DATE";
        }
        return "????";
}       

// ============================================================================
// Version-dependent structures and functions =================================
// ============================================================================

// Number of sensors (version 1.2)
#define	NUM_SENS_12	5

typedef struct {

	statu_t s; // 1st byte in ee slot

	word sval [NUM_SENS_12]; // aligned

	lword ds;
	lword t_ds;
	lword t_eslot;

	word  tag;
	word sspare[3];

} aggEEDataType_12;	// Size = 32 bytes

typedef struct {
	aggEEDataType_12 ee;
	lword	fr;
	lword	to;
	lword	ind;
	lword	cnt;
	nid_t	tag;
} aggEEDumpType_12;	// 20 + 32 = 52

// ============================================================================

// Number of sensors (version 1.3)
#define	NUM_SENS_13	6

typedef struct {

	statu_t s; 	// 1st 2 bytes in ee slot
	word	tag;
	lword 	ds;
	lword 	t_ds;
	lword	t_eslot;
	word 	sval [NUM_SENS_13];

} aggEEDataType_13; 	// 16 + NUM_SENS * 2 = 28

typedef struct {

	aggEEDataType_13 ee;

	lword	fr;
	lword	to;
	lword	ind;
	lword	cnt;
	nid_t	tag;

} aggEEDumpType_13;	// 20 + 28 = 48

// ============================================================================

#define	MAX_DUMP_STR_SIZE	64	// Maximum size of aggEEDumpType

lword 	agg_dump_s	[MAX_DUMP_STR_SIZE / sizeof (lword)];

// ============================================================================
// VERSION 1.2 ================================================================
// ============================================================================


#define	aggEEDataType	aggEEDataType_12
#define	aggEEDumpType	aggEEDumpType_12

#define	agg_dump ((aggEEDumpType*)(&agg_dump_s))


#define EE_AGG_SIZE	sizeof(aggEEDataType)

static inline int is_agg_empty_12 (const aggEEDumpType *ad, int ez) {

	int i;

	if ((ez && ad->ee.s.f.status == 0x0) ||
		(!ez && ad->ee.s.f.status == 0xF))
			return 1;

	for (i = 0; i < EE_AGG_SIZE; i++)
		if (((byte*)(&(ad->ee))) [i])
			goto NZ;

	return 1;
NZ:
	for (i = 0; i < EE_AGG_SIZE; i++)
		if (((byte*)(&(ad->ee))) [i] != 0xff)
			return 0;

	return 1;
}

#define IS_AGG_EMPTY	is_agg_empty_12 (agg_dump, ezero)

int esd_dump_12 (int ezero) {

	char * lbuf = NULL;
	mdate_t md, md2;
	lword ecount;

	memset (agg_dump, 0, sizeof(aggEEDumpType));

	printf ("dumping samples ... ");
	fflush (stdout);

	agg_dump->fr = slot_from;
	agg_dump->to = slot_to;

	if (agg_dump->fr > EE_AGG_MAX)
		agg_dump->fr = EE_AGG_MAX;

	if (agg_dump->fr < EE_AGG_MIN)
		agg_dump->fr = EE_AGG_MIN;

	if (agg_dump->to > EE_AGG_MAX)
		agg_dump->to = EE_AGG_MAX;

	if (agg_dump->to < EE_AGG_MIN)
		agg_dump->to = EE_AGG_MIN;

	agg_dump->ind = agg_dump->fr;

	ecount = 0;

	while (1) {
	
		ee_read (agg_dump->ind * EE_AGG_SIZE, (byte*)&agg_dump->ee,
			EE_AGG_SIZE);

		if (IS_AGG_EMPTY) {
			if (agg_dump->fr <= agg_dump->to) {
				ecount++;
				if (ecount >= 512)
					goto Done;
			}
			goto Continue;
		}

		ecount = 0;

		if (agg_dump->tag == 0 || agg_dump->ee.tag == agg_dump->tag ||
			agg_dump->ee.s.f.status == AGG_ALL) {

	    		if (agg_dump->ee.s.f.status == AGG_ALL) { // mark

				md.secs = agg_dump->ee.ds;
				s2d (&md);

				fprintf (df, "1011 "
					"%s %u %04u-%02u-%02u %02u:%02u:%02u "
					"%u %u %u\n",
					markName (agg_dump->ee.s),
					agg_dump->ind,
					md.dat.f ? 2009 + md.dat.yy :
						1001 + md.dat.yy,
					md.dat.mm, md.dat.dd, md.dat.h,
					md.dat.m, md.dat.s,
					agg_dump->ee.sval[0],
					agg_dump->ee.sval[1],
					agg_dump->ee.sval[2]);

	    		} else { // sens reading

				md.secs = agg_dump->ee.t_ds;
				s2d (&md);
				md2.secs = agg_dump->ee.ds;
				s2d (&md2);

				fprintf (df, "1007 Col %1u slot %1u (A: %1u) "
				"%04u-%02u-%02u %02u:%02u:%02u "
				"(A %04u-%02u-%02u %02u:%02u:%02u) "
				" %1d, %1d, %1d, %1d, %1d\n",

				agg_dump->ee.tag,
				agg_dump->ee.t_eslot,
				agg_dump->ind,

				md.dat.f ?  2009 + md.dat.yy : 1001 + md.dat.yy,
				md.dat.mm, md.dat.dd, md.dat.h, md.dat.m,
				md.dat.s,

				md2.dat.f ?  2009 + md2.dat.yy :
					1001 + md2.dat.yy,
				md2.dat.mm, md2.dat.dd, md2.dat.h, md2.dat.m,
				md2.dat.s,
				agg_dump->ee.sval[0],
				agg_dump->ee.sval[1],
				agg_dump->ee.sval[2],
				agg_dump->ee.sval[3],
				agg_dump->ee.sval[4]);
			}

	    		agg_dump->cnt++;

		}

Continue:
		if (agg_dump->fr <= agg_dump->to) {
			if (agg_dump->ind >= agg_dump->to)
				goto Done;
			else
				agg_dump->ind++;
		} else {
			if (agg_dump->ind <= agg_dump->to)
				goto Done;
			else
				agg_dump->ind--;
		}
	}
Done:
	printf ("done\n");
}

#undef	EE_AGG_SIZE
#undef	aggEEDataType
#undef	aggEEDumpType
#undef	agg_dump
#undef	IS_AGG_EMPTY

// ============================================================================
// VERSION 1.3 ================================================================
// ============================================================================

#define	aggEEDataType	aggEEDataType_13
#define	aggEEDumpType	aggEEDumpType_13

#define	agg_dump ((aggEEDumpType*)(&agg_dump_s))

#define IS_AGG_EMPTY		(agg_dump->ee.s.f.status == 0x0 || \
					agg_dump->ee.s.f.status == 0xF)

#define EE_AGG_SIZE	sizeof(aggEEDataType)

int esd_dump_13 () {

	char * lbuf = NULL;
	mdate_t md, md2;
	lword ecount;

	memset (agg_dump, 0, sizeof(aggEEDumpType));

	printf ("dumping samples ... ");
	fflush (stdout);

	agg_dump->fr = slot_from;
	agg_dump->to = slot_to;

	if (agg_dump->fr > EE_AGG_MAX)
		agg_dump->fr = EE_AGG_MAX;

	if (agg_dump->fr < EE_AGG_MIN)
		agg_dump->fr = EE_AGG_MIN;

	if (agg_dump->to > EE_AGG_MAX)
		agg_dump->to = EE_AGG_MAX;

	if (agg_dump->to < EE_AGG_MIN)
		agg_dump->to = EE_AGG_MIN;

	agg_dump->ind = agg_dump->fr;

	ecount = 0;

	while (1) {
	
		ee_read (agg_dump->ind * EE_AGG_SIZE, (byte*)&agg_dump->ee,
			EE_AGG_SIZE);

		if (IS_AGG_EMPTY) {
			if (agg_dump->fr <= agg_dump->to) {
				ecount++;
				if (ecount >= 512)
					goto Done;
			}
			goto Continue;
		}

		ecount = 0;

		if (agg_dump->tag == 0 || agg_dump->ee.tag == agg_dump->tag ||
			agg_dump->ee.s.f.status == AGG_ALL) {

	    		if (agg_dump->ee.s.f.status == AGG_ALL) { // mark

				md.secs = agg_dump->ee.ds;
				s2d (&md);

				fprintf (df, "1011 "
					"%s %u %04u-%02u-%02u %02u:%02u:%02u "
					"%u %u %u\n",
					markName (agg_dump->ee.s),
					agg_dump->ind,
					md.dat.f ? 2009 + md.dat.yy :
						1001 + md.dat.yy,
					md.dat.mm, md.dat.dd, md.dat.h,
					md.dat.m, md.dat.s,
					agg_dump->ee.sval[0],
					agg_dump->ee.sval[1],
					agg_dump->ee.sval[2]);

	    		} else { // sens reading

				md.secs = agg_dump->ee.t_ds;
				s2d (&md);
				md2.secs = agg_dump->ee.ds;
				s2d (&md2);

				fprintf (df, "1007 Col %1u slot %1u (A: %1u) "
				"%04u-%02u-%02u %02u:%02u:%02u "
				"(A %04u-%02u-%02u %02u:%02u:%02u) "
				" %1d, %1d, %1d, %1d, %1d, %1d\n",

				agg_dump->ee.tag,
				agg_dump->ee.t_eslot,
				agg_dump->ind,

				md.dat.f ?  2009 + md.dat.yy : 1001 + md.dat.yy,
				md.dat.mm, md.dat.dd, md.dat.h, md.dat.m,
				md.dat.s,

				md2.dat.f ?  2009 + md2.dat.yy :
					1001 + md2.dat.yy,
				md2.dat.mm, md2.dat.dd, md2.dat.h, md2.dat.m,
				md2.dat.s,
				agg_dump->ee.sval[0],
				agg_dump->ee.sval[1],
				agg_dump->ee.sval[2],
				agg_dump->ee.sval[3],
				agg_dump->ee.sval[4], agg_dump->ee.sval[5]);
			}

	    		agg_dump->cnt++;
		}

Continue:
		if (agg_dump->fr <= agg_dump->to) {
			if (agg_dump->ind >= agg_dump->to)
				goto Done;
			else
				agg_dump->ind++;
		} else {
			if (agg_dump->ind <= agg_dump->to)
				goto Done;
			else
				agg_dump->ind--;
		}
	}
Done:
	printf ("done\n");
}

#undef	EE_AGG_SIZE
#undef	aggEEDataType
#undef	aggEEDumpType
#undef	agg_dump
#undef	IS_AGG_EMPTY

// ============================================================================
// ============================================================================

void bad_args () {

	fprintf (stderr,
	    "Usage: %s [-d dname/skip] [-o] [-n] [-f sl] [-t sl] [dumpfile]\n"
	    "       %s -s [-n] [dumpfile]\n", prgname, prgname);
	exit (1);
}

void esd_show (int n) {
//
// Dump the contents of the first block
//
	int i, j;
	unsigned char *bp;
	char cc, cdmp [17];

	fprintf (df, "DEVICE NUMBER %1d <%s>, size = %u\n", n, vname, sd_size);

	bp = buf;
	cdmp [16] = '\0';
	for (i = 0; i < 32; i++) {
		for (j = 0; j < 16; j++) {
			cc = bp [j];
			if (isspace (cc) || !isprint (cc))
				cc = ' ';
			cdmp [j] = cc;
		}
		fprintf (df, "%03x: "
			" %02x %02x %02x %02x %02x %02x %02x %02x"
			" %02x %02x %02x %02x %02x %02x %02x %02x"
			" -- %s\n", i * 16,
				bp [ 0], bp [ 1], bp [ 2], bp [ 3],
				bp [ 4], bp [ 5], bp [ 6], bp [ 7],
				bp [ 8], bp [ 9], bp [10], bp [11],
				bp [12], bp [13], bp [14], bp [15],
					cdmp);
		bp += 16;
	}
	fprintf (df, "\n");
}

int esd_size () {

	unsigned int l;
	off_t nb, be, en;
	word m;

	// Assuming the initial block has been read into buf

	if (nolabel == 0) {
		// Check the label
		m =   (word) buf [0] | (((word) buf [1]) <<  8);
		if (m != MAGIC)
			return 1;
	}

	// Determine the length

	be = 0;
	// This is 4G / 512, i.e., more than the max number of block
	en = 1024 * 1024 * 4 * 2;

	while (be + 1 < en) {

		nb = (be + en) / 2;

		if (lseek (sd_desc, nb * BUFSIZE, SEEK_SET) < 0) {
			// Assume we are too far
TooFar:
			en = nb;
			continue;
		}

		if (read (sd_desc, buf, BUFSIZE) <= 0)
			// Too far?
			goto TooFar;

		be = nb;
	}

	// Trailing check

	lseek (sd_desc, be * BUFSIZE, SEEK_SET);
	if ((l = read (sd_desc, buf, BUFSIZE)) < 0)
		// This cannot happen
		return 1;

	if (l != 0) {
		// One more block
		be++;
		if ((l = read (sd_desc, buf, BUFSIZE)) < 0)
			return 1;
		if (l == BUFSIZE)
			be++;
	}

	if (be < MINBLOCKS || be > MAXBLOCKS)
		return 1;

	sd_size = be * BUFSIZE;

	return 0;
}

void esd_gendn (int i) {

	int j;

	j = i >> 1;
	sprintf (vname, "/dev/sd%c%s", 'a' + j, (i & 1) ? "1" : "");
}

int esd_verify () {

	if ((sd_desc = open (vname, O_RDONLY)) < 0)
		// Cannot open
		return 1;
	// Check if can read
	if (read (sd_desc, buf, BUFSIZE) != BUFSIZE) {
		// Cannot read
Ignore:
		close (sd_desc);
		return 1;
	}
	// Calculate the length
	if (esd_size ())
		goto Ignore;
	// Re-read block zero
	if (lseek (sd_desc, 0, SEEK_SET) < 0)
		goto Ignore;
	if (read (sd_desc, buf, BUFSIZE) != BUFSIZE)
		goto Ignore;

	return 0;
}

void esd_scan () {

	int i, n;

	for (n = i = 0; i < 18; i++) {
		esd_gendn (i);
		if (esd_verify () == 0) {
			esd_show (n);
			n++;
		}
	}
}

void esd_open () {

	int i, n;

	if (devname != NULL) {
		// Explicit name
		if (strlen (devname) > 63)
			bad_args ();
		strcpy (vname, devname);
		if (esd_verify ()) {
			fprintf (stderr, "cannot open device %s\n", devname);
			exit (1);
		}
		printf ("card size = %lld bytes\n", sd_size);
		return;
	}

	// Scanning
	for (n = i = 0; i < 18; i++) {
		esd_gendn (i);
		if (esd_verify () == 0) {
			if (devskip == 0 || n == devskip) {
				// Got it
				printf ("card device number %1d, name %s, "
					"size = %lld bytes\n",
						n, vname, sd_size);
				return;
			}
			n++;
		}
	}
	fprintf (stderr, "couldn't find a valid card\n");
	exit (1);
}


void do_args (int argc, const char **argv) {
//
// Handle arguments
//
	const char *av;

	if (argc < 1)
		return;

	prgname = *argv;

	argc--;

	while (argc) {

		argc--;
		argv++;

		if (**argv != '-') {
			// This can only be the dump file name
			if (dmpname != NULL)
				bad_args ();
			dmpname = *argv;
			continue;
		}

		switch (*(*argv + 1)) {

			case 's' :
				// Scan: report open-able devices
				if (devskip)
					bad_args ();
				devskip = -1;
				break;

			case 'n' :
				// No label check
				if (nolabel)
					bad_args ();
				nolabel = 1;
				break;

			case 'd' :
				// Explicit device indication
				if (argc == 0 || devname != NULL || devskip)
					bad_args ();
				argc--;
				argv++;
				devname = *argv;
				break;

			case 'f' :
				// From slot
				if (argc == 0 || slot_from != 0xffffffff)
					bad_args ();
				argc--;
				argv++;
				slot_from = atol (*argv);
				break;

			case 'o' :
	
				if (oldvers)
					bad_args ();

				oldvers = 1;

				if (argc == 0)
					break;

				// The next argument can be 0 or f
				av = *(argv+1);

				if (strcmp (av, "f") == 0 ||
				    strcmp (av, "F") == 0 ||
				    strcmp (av, "0") == 0 ) {

					if (strcmp (av, "0") == 0)
						oldvers = 2;

					argc--;
					argv++;
				}
				
				break;

			case 't' :
				// To slot
				if (argc == 0 || slot_to != 0xffffffff)
					bad_args ();
				argc--;
				argv++;
				slot_to = atol (*argv);
				break;

			// More may come later
			default:
				bad_args ();
		}
	}

	df = NULL;
	if (dmpname == NULL) {
		if (devskip < 0) {
			// Reporting - set it to stdout
			df = stdout;
		} else {
			// The default for dumping
			dmpname = "sddump.txt";
		}
	}

	if (df == NULL) {
		// Open the dump/report file
		if ((df = fopen (dmpname, "w")) == NULL) {
			fprintf (stderr, "cannot open dump file %s\n", dmpname);
			exit (1);
		}
	}

	if (slot_from == 0xffffffff)
		slot_from = 0;
	if (slot_to == 0xffffffff)
		slot_to = 999999999;
}

int main (int argc, const char *argv []) {

	const char *cp;

	do_args (argc, argv);

	if (devskip < 0) {
		// Scan
		if (devname != NULL)
			bad_args ();
		esd_scan ();
		exit (0);
	}

	if (devname != NULL) {
		// Check if this is in fact a skip count
		for (cp = devname; *cp != '\0'; cp++)
			if (!isdigit (*cp))
				goto DName;
		devskip = atoi (devname);
		devname = NULL;
	}
DName:
	esd_open ();

	if (oldvers)
		// Flag: empty is 0
		esd_dump_12 (oldvers == 2);
	else
		esd_dump_13 ();

	exit (0);
}
