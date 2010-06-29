/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2007                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "sysio.h"
#include "adc_sampler.h"
#include "tcvphys.h"

heapmem {100};

#include "ser.h"
#include "serf.h"
#include "form.h"

#if CC1100
#include "phys_cc1100.h"
#include "plug_null.h"
#endif

void hrc_start (), hrc_stop ();

#define	RF_MAXPLEN	48
#define	RF_MSTYPE_HR	1
#define	RF_MSTYPE_SM	2
#define	RF_MSTYPE_SML	7		// Last sample
#define	RF_MSTYPE_AK	3
#define	RF_FRAME	(2 + 4)		// CRC + netid + type + count (mod 256)
#define	RF_NETID	0xCABA		// To make us distinct
#define	RF_RTIMEOUT	2048
#define	RF_HRATE_INT	1024		// Heart rate send interval

char	ibuf [82];
word	sbuf [ADCS_SAMPLE_LENGTH], dbuf [ADCS_SAMPLE_LENGTH];
int	sfd;
word	err;

lword	samples;
lword	overflows;
lword	flash_addr,
	flash_samples;

lword	a, b;

lword	Time;

char	SRate [12];

Boolean	FBusy = NO, Load = NO, Talk = YES;

byte	PCount;

#define	SE_NEX		0
#define	SE_ANP		1
#define	SE_WAK		2
#define	SE_LOP		3
#define	SE_DON		4

process (sender, void)
/*
 * This is a most naive version that requires every packet to be acked
 */
  address packet;

  entry (SE_NEX)

	if (flash_samples == 0) {
		// Fin
		packet = tcv_wnp (SE_ANP, sfd, RF_FRAME);
		packet [1] = (RF_MSTYPE_SM  << 8) | PCount;
		tcv_endp (packet);
		// Done, finish
		goto SDone;
	}

	// Note: perhaps ACKs should be handled in the plugin in the final
	// version? That way we wouldn't have to rebuild the packet before
	// every retransmission.

	flash_samples--;
	ee_read (flash_addr, (byte*)dbuf, ADCS_SAMPLE_LENGTH * 2);
	// Be RT-friendly
	proceed (SE_ANP);

  entry (SE_ANP)

	packet = tcv_wnp (SE_ANP, sfd, ADCS_SAMPLE_LENGTH * 2 + RF_FRAME);

	packet [0] = RF_NETID;

	if (flash_samples)
		packet [1] = (RF_MSTYPE_SM  << 8) | PCount;
	else
		packet [1] = (RF_MSTYPE_SML << 8) | PCount;

	memcpy (packet + 2, dbuf, ADCS_SAMPLE_LENGTH * 2);
	tcv_endp (packet);
	proceed (SE_WAK);

  entry (SE_WAK)

	delay (RF_RTIMEOUT, SE_ANP);
	packet = tcv_rnp (SE_WAK, sfd);
	unwait (WNONE);

	if ((packet [1] & 0xff00) != (RF_MSTYPE_AK << 8)) {
		tcv_endp (packet);
		proceed (SE_WAK);
	}

	if ((packet [1] & 0xff) != PCount) {
		// Retransmit immediately
		tcv_endp (packet);
		proceed (SE_ANP);
	}

	tcv_endp (packet);

	// Acknowledged
	proceed (SE_LOP);

  entry (SE_LOP)

	flash_addr += ADCS_SAMPLE_LENGTH * 2;
	PCount ++;
	proceed (SE_NEX);

  entry (SE_DON)

SDone:
	if (Talk)
		ser_out (SE_DON, "TRANSMISSION FINISHED\r\n");
	FBusy = NO;
	tcv_control (sfd, PHYSOPT_RXOFF, NULL);
	finish;

endprocess (1)

#define	HR_NEX		0

process (hrate, void)

  address packet;

  entry (HR_NEX)

	packet = tcv_wnp (HR_NEX, sfd, RF_FRAME + 2);
	packet [0] = RF_NETID;
	packet [1] = RF_MSTYPE_HR << 8;
	packet [2] = HeartRateCounter;
	tcv_endp (packet);

	delay (RF_HRATE_INT, HR_NEX);

endprocess (1)
	
#define	RE_WAIT		0
#define	RE_OUT		1
#define	RE_SYN		2
#define	RE_DUMP		3

process (reader, void)

  entry (RE_WAIT)

Again:
	adcs_get_sample (RE_WAIT, sbuf);
	samples++;

	if (Load == NO) {
		if ((samples & 0x3ff) == 0)
			goto Dump;
		else
			goto Again;
	}

  entry (RE_OUT)

	ee_write (RE_OUT, flash_addr, (byte*) sbuf, ADCS_SAMPLE_LENGTH * 2);

	if (--flash_samples == 0)
		goto Sync;
	else
		flash_addr += ADCS_SAMPLE_LENGTH * 2;
	goto Again;

  entry (RE_SYN)

Sync:
	ee_sync (RE_SYN);
	Load = NO;
	FBusy = NO;
	goto Again;

  entry (RE_DUMP)

Dump:
	if (Talk)
		ser_outf (RE_DUMP, "S: %x %x %x %x %x %x\r\n",
			sbuf [0],
			sbuf [1],
			sbuf [2],
			sbuf [3],
			sbuf [4],
			sbuf [5]);
	goto Again;

endprocess (1);

#define	MO_RUN		0
#define	MO_LOA		1
#define	MO_IDLE		2

process (monitor, void) 

  word Rate1, Rate2;
  lword TotalTime;

  entry (MO_RUN)

	overflows += adcs_overflow ();

	if (!Load)
		goto Idle;

	// Loading
  entry (MO_LOA)

	if (Talk)
		ser_outf (MO_LOA, "LOADING: %lx [%lu] v = %lu\r\n",
			flash_addr, flash_samples, overflows);

	delay (10 * 1024, MO_RUN);
	release;
Idle:
	if ((TotalTime = seconds () - Time) != 0) {
		Rate1 = (word) (samples / TotalTime);
		Rate2 = (word)(((samples % TotalTime) * 1000) / TotalTime);
		form (SRate, "%d.%c%c%c", Rate1,
			((Rate2 / 100)    ) + '0',
			((Rate2 / 10) % 10) + '0',
			((Rate2     ) % 10) + '0');
	} else {
		SRate [0] = '\0';
	}

  entry (MO_IDLE)

	delay (10 * 1024, MO_RUN);
	
	if (Talk)
		ser_outf (MO_IDLE, "IDLE: %lu (%s/s) v = %lu hr = %u\r\n",
			samples, (word) SRate, overflows, HeartRateCounter);

	lcd_clear (0, 15);
	form (ibuf, "%lu", samples);
	lcd_write (0, ibuf);
	lcd_clear (16, 31);
	form (ibuf, "%s %lu", SRate, overflows);
	lcd_write (16, ibuf);

endprocess (1);

#define	DU_RUN		0
#define	DU_DIS		1

process (dumper, void)

  entry (DU_RUN)

	ee_read (flash_addr, (byte*) dbuf, ADCS_SAMPLE_LENGTH * 2);

  entry (DU_DIS)

	ser_outf (DU_DIS, "%lx: %x %x %x %x %x %x\r\n",
		flash_addr,
		dbuf [0],
		dbuf [1],
		dbuf [2],
		dbuf [3],
		dbuf [4],
		dbuf [5]);

	if (--flash_samples == 0) {
		FBusy = NO;
		finish;
	}

	flash_addr += ADCS_SAMPLE_LENGTH * 2;

	delay (100, DU_RUN);

endprocess (1);

#define	RS_INIT		0
#define	RS_RCMF		1
#define	RS_RCMD		2
#define	RS_RCME		3
#define	RS_STA		4
#define	RS_DON		5
#define	RS_STO		6
#define	RS_ERA		7
#define	RS_ERB		8
#define	RS_REA		9
#define	RS_WRI		10
#define	RS_XMT		11
#define	RS_HEA		12
#define	RS_HEK		13
#define	RS_TAK		14

process (root, int)

  word bs;

  entry (RS_INIT)

	fork (reader, NULL);
	fork (monitor, NULL);
	lcd_on (0);

#if CC1100
	phys_cc1100 (0, RF_MAXPLEN);
	tcv_plug (0, &plug_null);
	sfd = tcv_open (NONE, 0, 0);

	if (sfd < 0) {
		diag ("RF open failed");
		halt ();
	}

	tcv_control (sfd, PHYSOPT_TXON, NULL);
	err = RF_NETID;
	tcv_control (sfd, PHYSOPT_SETSID, &err);
#endif

  entry (RS_RCMF)

	ser_out (RS_RCMF,
		"\r\nADCS Test\r\n"
		"Commands:\r\n"
		"s bs         -> start\r\n"
		"q            -> stop\r\n"
		"e from upto  -> erase flash\r\n"
		"w addr n     -> write n samples to flash\r\n"
		"r addr n     -> read n samples from flash\r\n"
		"x addr n     -> transmit n samples from flash\r\n"
		"h            -> start heart rate monitor\r\n"
		"k            -> stop heart rate monitor\r\n"
		"t            -> toggle talk/quiet\r\n"
	);

  entry (RS_RCMD)

	err = 0;
	ser_in (RS_RCMD, ibuf, 132-1);

	switch (ibuf [0]) {
		case 's': proceed (RS_STA);
		case 'q': proceed (RS_STO);
		case 'e': proceed (RS_ERA);
		case 'w': proceed (RS_WRI);
	  	case 'r': proceed (RS_REA);
	  	case 'x': proceed (RS_XMT);
	  	case 'h': proceed (RS_HEA);
	  	case 'k': proceed (RS_HEK);
	  	case 't': proceed (RS_TAK);
	}
	
  entry (RS_RCME)

	ser_out (RS_RCME, "?????????\r\n");
	proceed (RS_RCMF);

  entry (RS_STA)

	bs = 0;
	scan (ibuf + 1, "%u", &bs);
	if (bs == 0)
		proceed (RS_RCME);
	samples = 0;
	overflows = 0;
	err = adcs_start (bs);
	Time = seconds ();

  entry (RS_DON)

	ser_outf (RS_DON, "Done %u\r\n", err);
	proceed (RS_RCMD);

  entry (RS_STO)

	adcs_stop ();
	proceed (RS_DON);

  entry (RS_ERA)

	if (FBusy)
		proceed (RS_RCME);

	FBusy = YES;
	scan (ibuf + 1, "%lu %lu", &a, &b);

  entry (RS_ERB)

	err = ee_erase (RS_ERB, a, b);
	FBusy = NO;
	proceed (RS_DON);

  entry (RS_REA)

	if (FBusy)
		proceed (RS_RCME);

	flash_samples = 0;
	scan (ibuf + 1, "%lu %lu", &flash_addr, &flash_samples);
	if (flash_samples == 0)
		proceed (RS_RCME);

	FBusy = YES;
	fork (dumper, NULL);
	proceed (RS_DON);

  entry (RS_WRI)

	if (FBusy)
		proceed (RS_RCME);

	flash_samples = 0;
	scan (ibuf + 1, "%lu %lu", &flash_addr, &flash_samples);
	if (flash_samples == 0)
		proceed (RS_RCME);

	FBusy = YES;
	Load = YES;
	proceed (RS_DON);

  entry (RS_XMT)

	if (FBusy)
		proceed (RS_RCME);

	flash_samples = 0;
	scan (ibuf + 1, "%lu %lu", &flash_addr, &flash_samples);
	if (flash_samples == 0)
		proceed (RS_RCME);

	FBusy = YES;
	PCount = 0;
	tcv_control (sfd, PHYSOPT_RXON, NULL);
	fork (sender, NULL);
	proceed (RS_DON);

  entry (RS_HEA)

	if (running (hrate))
		proceed (RS_RCME);

	hrc_start ();
	fork (hrate, NULL);
	proceed (RS_DON);

  entry (RS_HEK)

	if (!running (hrate))
		proceed (RS_RCME);

	hrc_stop ();
	killall (hrate);
	proceed (RS_DON);

  entry (RS_TAK)

	Talk = Talk ? NO : YES;
	proceed (RS_DON);

endprocess (1);
