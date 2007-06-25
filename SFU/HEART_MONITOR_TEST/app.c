/* ============================================================= */
/* ============================================================= */
/* ==              P. Gburzynski, June 2007                   == */
/* ============================================================= */
/* ============================================================= */

// This praxis is used to test the prototype boards, specifically,
// radio, LCD, flash and UART

#include "sysio.h"
#include "tcvphys.h"
#include "tcvplug.h"
#include "board_rf.h"

heapmem {100};

#include "form.h"
#include "serf.h"

#include "phys_cc1100.h"

// The plugin

int tcv_ope_heart (int, int, va_list);
int tcv_clo_heart (int, int);
int tcv_rcv_heart (int, address, int, int*, tcvadp_t*);
int tcv_frm_heart (address, int, tcvadp_t*);
int tcv_out_heart (address);
int tcv_xmt_heart (address);

const tcvplug_t plug_heart =
		{ tcv_ope_heart, tcv_clo_heart, tcv_rcv_heart, tcv_frm_heart,
			tcv_out_heart, tcv_xmt_heart, NULL,
				0x0081 /* Plugin Id */ };

#define	MAXPLEN			56
#define	MINPLEN			16
#define	XMIT_POWER		7	// Out of 7
#define	PACKET_QUEUE_LIMIT	8

int	RSFD = NONE;	// Radio interface SID

word Sent, Rcvd;

char cbuf [18];

// The Plugin =================================================================

int 	*desc = NULL;

int tcv_ope_heart (int phy, int fd, va_list plid) {
/*
 * This is very simple - we are allowed to have one descriptor per phy.
 */
	int i;

	if (desc == NULL) {
		desc = (int*) umalloc (sizeof (int) * TCV_MAX_PHYS);
		if (desc == NULL)
			syserror (EMALLOC, "plug_heart tcv_ope_heart");
		for (i = 0; i < TCV_MAX_PHYS; i++)
			desc [i] = NONE;
	}

	/* phy has been verified by TCV */
	if (desc [phy] != NONE)
		return ERROR;

	desc [phy] = fd;
	return 0;
}

int tcv_clo_heart (int phy, int fd) {

	/* phy/fd has been verified */

	if (desc == NULL || desc [phy] != fd)
		return ERROR;

	desc [phy] = NONE;
	return 0;
}

int tcv_rcv_heart (int phy, address p, int len, int *ses, tcvadp_t *bounds) {

	if (desc == NULL || (*ses = desc [phy]) == NONE)
		return TCV_DSP_PASS;

	bounds->head = bounds->tail = 0;

	return TCV_DSP_RCV;
}

int tcv_frm_heart (address p, int phy, tcvadp_t *bounds) {

	// Link Id + CRC
	return bounds->head = bounds->tail = 2;
}

int tcv_out_heart (address p) {

	return TCV_DSP_XMT;

}

int tcv_xmt_heart (address p) {

	return TCV_DSP_DROP;
}

// ============================================================================

#ifdef EEPROM_PRESENT

void memory_test () {

#define	N_FLASH_SAMPLES	2000		// We only try this many samples ...
#define	N_FLASH_SAMPLE_SIZE 48		// ... of this size

	lword nb, ms;
	byte *sbuf;
	word si, cn;
	byte el;

	lcd_clear (0, 0);
	lcd_write (0, "TESTING FLASH:");

	sbuf = (byte*) umalloc (N_FLASH_SAMPLE_SIZE);

	nb = ee_size (NULL, NULL);

	// Increment
	si = (word) (nb / N_FLASH_SAMPLES);

	lcd_write (16, "WRITING: ");

	nb = 0;
	for (cn = 0; cn < N_FLASH_SAMPLES; cn++) {
		form (cbuf, "%ld", nb);
		// Determine the number of initial spaces
		el = 7 - (byte) strlen (cbuf);
		lcd_write (16 + 9 + el, cbuf);
		while (el) {
			el--;
			lcd_write (16 + 9 + el, " ");
		}

		// Prepare the buffer to write
		for (el = 0; el < N_FLASH_SAMPLE_SIZE; el++)
			sbuf [el] = 0xA5;

		ee_write (WNONE, nb, sbuf, N_FLASH_SAMPLE_SIZE);
		nb += si;
	}

	lcd_clear (0, 0);
	mdelay (500);

	lcd_write (0,  "READING: ");
	lcd_write (16, "MISREAD:       0");

	nb = 0;
	ms = 0;
	for (cn = 0; cn < N_FLASH_SAMPLES; cn++) {
		form (cbuf, "%ld", nb);
		el = 7 - (byte) strlen (cbuf);
		lcd_write (0 + 9 + el, cbuf);
		while (el) {
			el--;
			lcd_write (0 + 9 + el, " ");
		}

		ee_read (nb, sbuf, N_FLASH_SAMPLE_SIZE);

		// Check
		for (el = 0; el < N_FLASH_SAMPLE_SIZE; el++) {
			if (sbuf [el] != 0xA5)
				ms++;
		}

		form (cbuf, "%ld", ms);
		el = 7 - (byte) strlen (cbuf);
		lcd_write (16 + 9 + el, cbuf);
		while (el) {
			el--;
			lcd_write (16 + 9 + el, " ");
		}
		nb += si;
	}

	ufree (sbuf);

	lcd_clear (0, 0);
	lcd_write (0, "FLASH DONE!");
	mdelay (2000);
}

#endif /* EEPROM_PRESENT */

static word gen_packet_length (void) {

	word pl;

#if MINPLEN >= MAXPLEN
	return MINPLEN;
#else
	pl = ((rnd () % (MAXPLEN - MINPLEN + 1)) + MINPLEN) & 0xFFE;
	if (pl < MINPLEN)
		return MINPLEN;
	else if (pl > MAXPLEN)
		return MAXPLEN;
	return pl;
#endif

}

static void enc5 (word n) {

	byte i;

	cbuf [5] = '\0';

	i = 4;
	while (1) {
		if (n == 0) {
			if (i == 4)
				cbuf [i] = '0';
			else
				cbuf [i] = ' ';
		} else {
			cbuf [i] = (char) ((n % 10) + '0');
			n /= 10;
		}
		if (i == 0)
			break;
		i--;
	}
}

#define	SN_LOOP		0
#define	SN_WPA		1
#define	SN_OUT		2

thread (sender)

    address packet;
    static word plen;

    entry (SN_LOOP) 

	plen = gen_packet_length ();

    entry (SN_WPA)

	packet = tcv_wnp (SN_WPA, RSFD, plen);

	packet [0] = 0;
	packet [1] = Sent;

	tcv_endp (packet);

	enc5 (Sent);
	Sent++;

	lcd_write (7, cbuf);

    entry (SN_OUT)

	ser_outf (SN_OUT, "SND: %u %u\r\n", Sent, plen);
	delay (1024, SN_LOOP);

endthread

#define	RC_WAIT		0
#define	RC_OUT		1

thread (receiver)

    address packet;

    entry (RC_WAIT)

	packet = tcv_rnp (RC_WAIT, RSFD);
	Rcvd = packet [1];
	tcv_endp (packet);

	enc5 (Rcvd);
	lcd_write (16 + 7, cbuf);

    entry (RC_OUT)

	ser_outf (RC_OUT, "RCV: %u\r\n", Rcvd);
	proceed (RC_WAIT);

endthread

// ============================================================================

thread (root)

#define	RS_INIT		0

    word scr;

    entry (RS_INIT)


	lcd_on (0);
	lcd_clear (0, 0);

	memory_test ();

	lcd_clear (0, 0);
	lcd_write (0, "OPENING RF      INTERFACE");

	phys_cc1100 (0, 0);
	tcv_plug (0, &plug_heart);
	RSFD = tcv_open (NONE, 0, 0);
	if (RSFD < 0) {
		lcd_write (0, "FAILED TO START RF INTERFACE!");
		while (1);
	}

	lcd_clear (0, 0);

	lcd_write (0,  "SENT: ");
	lcd_write (16, "RCVD: ");

	tcv_control (RSFD, PHYSOPT_TXON, NULL);
	tcv_control (RSFD, PHYSOPT_RXON, NULL);

	runthread (sender);
	runthread (receiver);

	finish;

endthread
