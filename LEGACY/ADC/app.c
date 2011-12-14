/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2005                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "sysio.h"
/*
 * This application reads values from the AD converter.
 *
 * There is no TCV, so all dynamic memory goes to umalloc
 */
heapmem {100};

#include "led.h"
#include "lcd.h"
#include "ser.h"
#include "serf.h"

int	mask, interval;
char 	dbuffer [34], enc [8];

#define	DS_INIT		00

process (display, void*)

  static char enc [8];

  int i, j, k, v;
  static int len;

  nodata;

  entry (DS_INIT)

	len = 1;
	for (i = 0; i < 4; i++) {
		if ((mask & (1 << i)) == 0)
			continue;
		v = adc_read (i) + 2048;
		form (enc, "%d", v);
		k = j = strlen (enc);
		while (k < 5) {
			dbuffer [len++] = ' ';
			k++;
		}
		strncpy (dbuffer + len, enc, j);
		if ((len += j) == 16)
			len = 17;
	}

	upd_lcd (dbuffer);

	delay (interval, DS_INIT);

endprocess

void stop_adc (void) {

	while (running (display))
		kill (running (display));
}

void start_adc (int ma, int del) {

	int i;

	for (i = 0; i < 32; i++)
		dbuffer [i] = ' ';

	stop_adc ();
	mask = ma;
	interval = del;
	fork (display, NULL);
}

#define	RS_INIT		00
#define	RS_RCMD		10
#define	RS_ERROR	15
#define	RS_SET		20
#define	RS_READ		30
#define	RS_DISPLAY	40
#define	RS_STOP		50

process (root, void*)
/* =========================================== */
/* This is the main program of the application */
/* =========================================== */

	static char cmd;
	static int nv, ma, iv;
	static unsigned vu;

	nodata;

  entry (RS_INIT)

	dsp_lcd ("PicOS ready     ADC", YES);

  entry (RS_RCMD-1)

	ser_out (RS_RCMD-1, "\r\n"
		"Welcome to PicOS!\r\n"
		"Commands: 's 0-5 mask intvl' (set ADC mode)\r\n"
		"          'r 0-3' (read ADC value)\r\n"
		"          'd mask intvl' (display ADC values)\r\n"
		"          'e' (stop displaying)\r\n");

  entry (RS_RCMD)

	nv = ma = iv = 0;
	if (ser_inf (RS_RCMD, "%c %d %d %d", &cmd, &nv, &ma, &iv) < 1)
		proceed (RS_RCMD-1);

	if (cmd == 's')
		proceed (RS_SET);
	if (cmd == 'r')
		proceed (RS_READ);
	if (cmd == 'd')
		proceed (RS_DISPLAY);
	if (cmd == 'e')
		proceed (RS_STOP);

  entry (RS_ERROR)

	ser_out (RS_ERROR, "Illegal command or parameter\r\n");
	proceed (RS_RCMD-1);

  entry (RS_SET)

	if (nv < 0 || nv > 5)
		nv = 0;
	if ((ma & 0xf) == 0)
		ma = 0xf;
	if (iv < 1)
		iv = 1;

  entry (RS_SET+1)

	ser_outf (RS_SET+1, "Setting ADC: mode = %d, mask = %x, delay = %d\r\n",
		nv, ma, iv);
	adc_start (nv, ma, iv);
	proceed (RS_RCMD);

  entry (RS_READ)

	if (nv < 0 || nv > 3)
		nv = 0;
	vu = adc_read (nv);

  entry (RS_READ+1)

	ser_outf (RS_READ+1, "ADC value [%d] = %u (%x)\r\n", nv, vu, vu);
	proceed (RS_RCMD);

  entry (RS_DISPLAY)

	iv = ma; ma = nv;
	if ((ma & 0xf) == 0)
		ma = 1;
	if (iv < 128)
		iv = 128;

	start_adc (ma, iv);
	proceed (RS_RCMD);

  entry (RS_STOP)

	stop_adc ();
	proceed (RS_RCMD);

endprocess
