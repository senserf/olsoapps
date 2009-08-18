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

// These are the same as in ..
// Should be encapsulated into a single file and stored in one place

#define	NUM_SENS	6

typedef union {
	word b:8;
	struct {
		word emptym :1;
		word mark   :3;
		word status :4;
	} f;
	word spare:8;
} statu_t;

typedef struct aggEEDataStruct {

	statu_t s; 	// 1st 2 bytes in ee slot
	word	tag;
	lword 	ds;
	lword 	t_ds;
	lword	t_eslot;
	word 	sval [NUM_SENS];

} aggEEDataType; 	// 16 + NUM_SENS * 2

typedef struct aggDataStruct {

	aggEEDataType ee;
	lword eslot;

} aggDataType;

typedef struct aggEEDumpStruct {

	aggEEDataType ee;

	lword	fr;
	lword	to;
	lword	ind;
	lword	cnt;
	nid_t	tag;
	word	dfin:1;
	word	upto:15;

} aggEEDumpType;

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

#define IS_BYTE_EMPTY(x)	((x) == 0 || (x) == 0xFF)
#define IS_AGG_EMPTY(x)		((x) == 0 || (x) == 0xF)
#define AGG_EMPTY		0xF
#define AGG_COLLECTED		0x8
#define AGG_CONFIRMED   	1
#define AGG_ALL			0xE

#define EE_AGG_SIZE	sizeof(aggEEDataType)
#define EE_AGG_MAX	(lword)(sd_size / EE_AGG_SIZE -1)
#define EE_AGG_MIN	0L

#define MARK_EMPTY	7
#define MARK_BOOT	6
#define MARK_PLOT	5
#define MARK_SYNC	4
#define MARK_MCHG	3
#define MARK_DATE	2

#define SIY     (365 * 24 * 60 * 60)
#define SID     (24 * 60 * 60)

// ============================================================================

#define	MAGIC	0x00ec
#define	BUFSIZE	(512*16)

#define	ONEMEG	(1024*1024)

const char *devname = NULL,
	   *prgname = "esdump",
	   *dmpname = NULL;

lword	slot_from = 0xffffffff,
	slot_to   = 0xffffffff;

int	sd_desc;
off_t	sd_size;

aggDataType agg_data;
aggEEDumpType agg_dump_s;
aggEEDumpType *agg_dump = &agg_dump_s;

unsigned char gbuf [BUFSIZE];

void esd_open () {

	int i, j;
	char vname [64];
	unsigned char buf [512];
	unsigned int l;
	off_t nb, be, en;
	word m;
	Boolean fnd;
	

	if (devname != NULL) {
		if ((sd_desc = open (devname, O_RDONLY)) < 0) {
			fprintf (stderr, "cannot open device %s\n", devname);
			exit (1);
		}
		// Check for magic
		if (read (sd_desc, buf, 2) != 2) {
			close (sd_desc);
			fprintf (stderr, "cannot read from %s\n", devname);
			exit (1);
		}
		m =   (word) buf [0] 		|
		    (((word) buf [1]) <<  8);

		if (m != MAGIC) {
			printf ("warning: the card has opened, but it has a "
					"bad header\n");
		}
		fnd = 0;
	} else {
		for (i = 0; i < 32; i++) {
			j = i >> 1;
			sprintf (vname, "/dev/sd%c%s",
				'b' + j, (i & 1) ? "1" : "");
			if ((sd_desc = open (vname, O_RDONLY)) < 0)
				continue;
			// Read the signature
			if (read (sd_desc, buf, 2) != 2) {
				close (sd_desc);
				continue;
			}

			m =   (word) buf [0] 		|
			    (((word) buf [1]) <<  8);

			if (m == MAGIC) {
				fnd = 1;
				printf ("card found as %s\n", vname);
				goto Found;
			}
// printf ("MAGIC: %02x %02x %04x\n", buf [0], buf [1], m);
		}

		fprintf (stderr, "couldn't find a valid card\n");
		exit (1);
	}
Found:
	// Locate the end
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

		if (read (sd_desc, buf, 512) <= 0)
			// Too far?
			goto TooFar;

		be = nb;
	}

	// Trailing check

	lseek (sd_desc, be * 512, SEEK_SET);
	if ((l = read (sd_desc, buf, 512)) < 0) {
		// This cannot happen
Err:
		fprintf (stderr, "cannot read from card\n");
		exit (1);
	}

	if (l != 0) {
		// One more block
		be++;
		if ((l = read (sd_desc, buf, 512)) < 0) {
			goto Err;
		}
		if (l == 512)
			be++;
	}

	sd_size = be * 512;
	printf ("card size = %lld bytes\n", sd_size);
}

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
		
void esd_prescan () {
//
// Locate the boundaries of data
//
	lword l, u, m;
	byte b;

	ee_read ((lword)EE_AGG_MIN * EE_AGG_SIZE, &b, 1);

	memset (&agg_data, 0, sizeof (aggDataType));

	if (IS_BYTE_EMPTY (b)) { // normal operations
		agg_data.eslot = EE_AGG_MIN;
		agg_data.ee.s.f.mark = MARK_EMPTY;
		agg_data.ee.s.f.status = AGG_EMPTY;
		printf ("the card is empty\n");
		exit (0);
	}

	l = EE_AGG_MIN; u = EE_AGG_MAX;

	while ((u - l) > 1) {

		m = l + ((u -l) >> 1);

		ee_read (m * EE_AGG_SIZE, &b, 1);

		if (IS_BYTE_EMPTY (b))
			u = m;
		else
			l = m;
	}

	ee_read (u * EE_AGG_SIZE, &b, 1);

	if (IS_BYTE_EMPTY (b)) {
		if (l < u) {
			ee_read (l * EE_AGG_SIZE, &b, 1);

			if (IS_BYTE_EMPTY (b)) {
				fprintf (stderr, "illegal card contents\n");
				exit (1);
			}

			agg_data.eslot = l;
		}
	} else
		agg_data.eslot = u;

	ee_read (agg_data.eslot * EE_AGG_SIZE, (byte *)&agg_data.ee,
				EE_AGG_SIZE);
	printf ("last slot = %1u\n", agg_data.eslot);
}

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

int esd_dump () {

	char * lbuf = NULL;
	mdate_t md, md2;
	FILE *df;

	// Open the dump file
	if ((df = fopen (dmpname, "w")) == NULL) {
		fprintf (stderr, "cannot open dump file %s\n", dmpname);
		exit (1);
	}

	memset (agg_dump, 0, sizeof(aggEEDumpType));

	printf ("dumping samples ... ");
	fflush (stdout);

	agg_dump->fr = slot_from;
	agg_dump->to = slot_to;

	if (agg_dump->fr > agg_data.eslot)
		agg_dump->fr = agg_data.eslot;

	if (agg_dump->fr < EE_AGG_MIN)
		agg_dump->fr = EE_AGG_MIN;

	if (agg_dump->to > agg_data.eslot)
		agg_dump->to = agg_data.eslot; //EE_AGG_MAX;

	if (agg_dump->to < EE_AGG_MIN)
		agg_dump->to = EE_AGG_MIN;

	agg_dump->ind = agg_dump->fr;

	while (1) {
	
		ee_read (agg_dump->ind * EE_AGG_SIZE, (byte*)&agg_dump->ee,
			EE_AGG_SIZE);

		if (IS_AGG_EMPTY (agg_dump->ee.s.f.status)) {
			if (agg_dump->fr <= agg_dump->to) {
				goto Done;
			}
			goto Continue;
		}

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

	    		if (agg_dump->upto != 0 && agg_dump->upto <=
			    agg_dump->cnt) {
				goto Done;
			}
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

void bad_args () {

	fprintf (stderr, "Usage: %s [-d devname] [-f sl] [-t sl] [dumpfile]\n",
		prgname);
	exit (1);
}

void do_args (int argc, const char **argv) {
//
// Handle arguments
//
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

			case 'd' :
				// Explicit device indication
				if (argc == 0 || devname != NULL)
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

	if (dmpname == NULL)
		dmpname = "sddump.txt";

	if (slot_from == 0xffffffff)
		slot_from = 0;
	if (slot_to == 0xffffffff)
		slot_to = 999999999;
}

int main (int argc, const char *argv []) {

	do_args (argc, argv);
	esd_open ();
	esd_prescan ();
	esd_dump ();
	exit (0);
}
