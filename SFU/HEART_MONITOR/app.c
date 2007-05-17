/* ============================================================= */
/* ============================================================= */
/* ==              P. Gburzynski, May 2007                    == */
/* ============================================================= */
/* ============================================================= */

#include "sysio.h"
#include "adc_sampler.h"
#include "tcvphys.h"
#include "tcvplug.h"

//#define	DONT_DISPLAY

heapmem {100};

#include "form.h"

#ifndef BLUETOOTH_PRESENT
#include "phys_cc1100.h"
#endif

#include "phys_uart.h"

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

const lword ESN = 0x80000001;

#define	MAXPLEN			60
#define	ROSTER_SIZE		16
#define	PACKET_QUEUE_LIMIT	8
#define	XMIT_POWER		2	// Out of 7

// The command table ==========================================================

const	byte

	PT_HELLO      = 0x80,
	PT_SDATA      = 0xC0,
	PT_HRATE      = 0xD0;

// ====================================

#define PT_STATUS	0x90
#define	PT_BIND		0x00
#define	PT_UNBIND	0x40
#define	PT_STOP		0x20
#define	PT_REPORT	0x30
#define	PT_RESET	0x10
#define	PT_SAMPLE	0x50
#define	PT_SEND		0x60
#define	PT_HRMCTRL	0x70

#define	ABORT		0x01	// Offset to PT_STOP

// ============================================================================

#define	HR_ON		0x00	// Offsets to PT_HRMCTRL
#define	HR_OFF		0x01

#define	SM_XMIT		0x01	// Offset to PT_SAMPLE

#define	ST_READY	0x00	// These must work as offsets to PT_STATUS
#define	ST_SAMPLING	0x02	// ...
#define	ST_SENDING	0x04	// ...

#define	HRM_ACTIVE	(TheHRMonitor != 0) // ... and this too

#define	INTV_HELLO	(4096 - 0x1f + (rnd () & 0xff))
#define	INTV_PERSTAT	7168
#define	INTV_EOR	512	// End-Of-Round retransmissions

#define	INTV_DISPLAY	512

#define	MAX_SAMPLES	8	// The max number of stored samples

#define	SAMPLE_SIZE		(ADCS_SAMPLE_LENGTH * 2)// This is 12 bytes
#define	SAMPLES_PER_PACKET	4			// == 48 bytes
#define	SAMPLES_PER_SECOND	(ADCS_TA_FREQUENCY / SAMPLES_PER_PACKET)

#define	BUFFER_STORAGE_UNIT	(SAMPLE_SIZE * SAMPLES_PER_PACKET)
#define	MULT_STORAGE_UNIT(a)	(((a) << 5) + ((a) << 4))
#define	MULT_SAMPLE_SIZE(a)	(((a) << 3) + ((a) << 2))
#define	MULT_SAMPLES_PACKET(a)	((a) << 2)

// This is in storage units
#define	SAMPLE_BUFFER_SIZE	60

#define	put1(p,b)	tcv_write (p, (const char*)(&(b)), 1)
#define	get1(p,b)	tcv_read (p, (char*)(&(b)), 1)

#define	put2(p,b)	tcv_write (p, (const char*)(&(b)), 2)
#define	get2(p,b)	tcv_read (p, (char*)(&(b)), 2)

#define	put3(p,b)	tcv_write (p, (const char*)(&(b)), 3)
#define	get3(p,b)	tcv_read (p, (char*)(&(b)), 3)

#define	put4(p,b)	tcv_write (p, (const char*)(&(b)), 4)
#define	get4(p,b)	tcv_read (p, (char*)(&(b)), 4)

#ifdef	DONT_DISPLAY
#define	DISPIT		do { } while (0)
#else
#define	DISPIT		ptrigger (Display, 0) 		// Display event
#endif

#define	SENDIT		do { \
				if (TheSender != 0) \
					ptrigger (TheSender, 0); \
				else if (TheSampler != 0) \
					ptrigger (TheSampler, 0); \
			} while (0)

#define	BINDIT		trigger (&BSFD)			// Bind event

lword	SRoster	[ROSTER_SIZE];				// Transmission list
lword	BufferIn, BufferLimit, SendFrom, SendUpto;

lword	SampStart [MAX_SAMPLES];
lword	SampCount [MAX_SAMPLES];

int 	*desc = NULL;
word	QPackets = 0;

byte	SampIdent [MAX_SAMPLES];

byte	Status = ST_READY;
byte	ThisSampleId;
byte	ThisSampleSlot;
byte	NextSampleSlot;
byte	SRL, SRP;		// Roster length/ptr
byte	DispToggle = 0;
byte	LostSamples = 0,
	SamplesToStat,
	XWS;			// Transmitting sample packets

word	HeartRate = 0;

// ============================================================================

word	SBuf [ADCS_SAMPLE_LENGTH * SAMPLES_PER_PACKET];

int	Display = 0, TheSampler = 0, TheSender = 0, TheHRMonitor = 0;

int	USFD, RSFD,	// Session IDs UART / RF
	BSFD = NONE;	// Which one is bound

// The Plugin =================================================================

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

int tcv_rcv_heart (int phy, address p, int len, int *ses,
							     tcvadp_t *bounds) {

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

	QPackets++;
	return TCV_DSP_XMT;

}

int tcv_xmt_heart (address p) {

	QPackets--;
	if (QPackets == PACKET_QUEUE_LIMIT)
		SENDIT;
	return TCV_DSP_DROP;
}

// ============================================================================

void send_status (int msfd) {
/*
 * Send a status packet:
 *
 * 	Status byte, [smp, count] * 6
 *      Sampling, sample Id, collected, left
 *      Sending, sample Id, last sent, left, last acked
 *
 */
	address packet;
	byte i, n;

	if (msfd != BSFD)
		// Lost a race
		return;

	switch (Status) {

	    case ST_READY:

		// Calculate packet size based on the sample pool
		for (i = 0, n = 2; i < MAX_SAMPLES; i++)
			if (SampCount [i])
				n += 4;

		if ((packet = tcv_wnp (WNONE, msfd, n)) == NULL)
			return;

		n = PT_STATUS | ST_READY | HRM_ACTIVE;
		put1 (packet, n);

		for (i = 0; i < MAX_SAMPLES; i++) {
			if (SampCount [i]) {
				put1 (packet, SampIdent [i]);
				put3 (packet, SampCount [i]);
			}
		}

		tcv_endp ((address) packet);
		return;

	    case ST_SAMPLING:

		if ((packet = tcv_wnp (WNONE, msfd, 6)) == NULL)
			return;

		n = PT_STATUS | ST_SAMPLING | HRM_ACTIVE;
		put1 (packet, n);
		put1 (packet, ThisSampleId);
		put3 (packet, SampCount [ThisSampleSlot]);

		tcv_endp (packet);
		return;

	    case ST_SENDING:

		if ((packet = tcv_wnp (WNONE, msfd, 6)) == NULL)
			return;

		n = PT_STATUS | ST_SENDING | HRM_ACTIVE;
		put1 (packet, n);
		put1 (packet, ThisSampleId);
		put3 (packet, SendFrom);

		tcv_endp (packet);
		return;

	}
}

word find_free_sample_slot () {

	word w;

	for (w = 0; w < MAX_SAMPLES; w++)
		if (SampCount [w] == 0)
			return w;

	return WNONE;
}

void kill_samples (lword n) {
/*
 * Removes the samples that will be overwritten by the new sample
 */
	lword lpo, lpp;
	word w;

	lpo = BufferIn + n;

	for (w = 0; w < MAX_SAMPLES; w++) {
		if (SampCount [w] == 0)
			continue;
		if (lpo > SampStart [w] && BufferIn < SampStart [w] +
		    SampCount [w])
			SampCount [w] = 0;
	}
}

void erase_all_samples () {

	int i;

	for (i = 0; i < MAX_SAMPLES; i++)
		SampCount [i] = 0;

	NextSampleSlot = 0;
}

void init_storage () {

	lword sz;

	BufferIn = 0;

	BufferLimit = ee_size (NULL, NULL);

	// Turn this into samples
	BufferLimit /= BUFFER_STORAGE_UNIT;
}

void stop_all (Boolean ab) {
/*
 * Equivalent to a soft reset
 */
	if (ab && HRM_ACTIVE) {
		hrc_stop ();
		kill (TheHRMonitor);
		TheHRMonitor = 0;
		HeartRate = 0;
	}

	if (Status == ST_SAMPLING)
		// Have to stop the ADC sampler as well
		adcs_stop ();

	if (TheSampler != 0) {
		kill (TheSampler);
		TheSampler = 0;
	}

	if (TheSender != 0) {
		kill (TheSender);
		TheSender = 0;
	}

	Status = ST_READY;
	XWS = 0;
}

// ============================================================================

#define	DS_SIGNAL	0
#define	DS_LOOP		1

thread (display)

    char cb [12];
    word w;
    int i;

    entry (DS_SIGNAL)

	// Force "on" state of all flashing items
	DispToggle = 1;

    entry (DS_LOOP)

	if (BSFD == NONE) {
		// Unbound
		lcd_clear (0, 0);
		if (DispToggle)
			lcd_write (0, "UNBOUND");
	} else {
		// We are bound: fixed items first
		lcd_write (15, BSFD == USFD ? "S" : "W");
		for (w = 0, i = 0; i < MAX_SAMPLES; i++) 
			if (SampCount [i])
				w++;
		cb [0] = (char) (w + '0');
		cb [1] = '\0';
		lcd_write (13, cb);

		if (TheHRMonitor) {
			// Display the heart rate
			if (DispToggle) {
				form (cb, "%d", HeartRate);
				i = 4 - strlen (cb);
				if (i > 0) {
					lcd_clear (16, i);
					lcd_write (16 + i, cb);
				}
			} else {
				lcd_clear (16, 4);
			}
		}

		if (LostSamples && DispToggle) {
			form (cb, "%d", LostSamples);
			lcd_write (21, cb);
		} else 
			lcd_clear (21, 2);

		if (Status == ST_READY) {

			// No flashing in this state unless HeartRate != 0
			lcd_write (0, "READY   ");
			lcd_clear (9, 3);
			lcd_clear (32-8, 8);
			if (TheHRMonitor || LostSamples) {
				DispToggle = 1 - DispToggle;
				delay (INTV_DISPLAY, DS_LOOP);
			}
			when (0, DS_SIGNAL);
			release;

		} else {

			if (DispToggle)
				lcd_write (0, Status == ST_SAMPLING ?
					"SAMPLING" : "SENDING ");
			else
				lcd_clear (0, 8);

			// Sample number
			form (cb, "%d", ThisSampleId);
			i = 3 - strlen (cb);
			if (i > 0)
				lcd_clear (9, i);
			lcd_write (9 + i, cb);

			form (cb, "%lu", SendFrom);
			i = 8 - strlen (cb);
			if (i <= 0) {
				i = 0;
			} else {
				lcd_clear (32-8, i);
			}
			lcd_write (32-8 + i, cb);
		}
	}

	DispToggle = 1 - DispToggle;
	delay (INTV_DISPLAY, DS_LOOP);
	when (0, DS_SIGNAL);

endthread

// ============================================================================

#define	SN_LOOP		0
#define	SN_SEND		1
#define	SN_EOR		2

thread (sender)

    lword sa;
    address packet;

    entry (SN_LOOP)

SN_loop:

	SendUpto = SRoster [SRP++];

	if (SRP != SRL) {
		// Try the next one
		if ((SendFrom = SRoster [SRP]) <= SendUpto)
			SRP++;
		else
			SendFrom = SendUpto;
	} else
		SendFrom = SendUpto;

	// Send samples from SendFrom upto SendUpto inclusively

    entry (SN_SEND)

SN_send:

	if (QPackets > PACKET_QUEUE_LIMIT) {
		when (0, SN_SEND);
		release;
	}

	packet = tcv_wnp (SN_SEND, BSFD, 6 + BUFFER_STORAGE_UNIT);
	put1 (packet, PT_SDATA);
	put1 (packet, ThisSampleId);
	put1 (packet, HeartRate);
	put3 (packet, SendFrom);

	if ((sa = SampStart [ThisSampleSlot] + SendFrom) >= BufferLimit)
		sa -= BufferLimit;

	ee_read (MULT_STORAGE_UNIT (sa), ((byte*) packet) + 8,
		BUFFER_STORAGE_UNIT);

	tcv_endp (packet);

	if (SendFrom != SendUpto) {
		SendFrom++;
		goto SN_send;
	}

	if (SRP != SRL)
		goto SN_loop;

    entry (SN_EOR)

	if (QPackets > PACKET_QUEUE_LIMIT) {
		when (0, SN_EOR);
		release;
	}

	if ((packet = tcv_wnp (WNONE, BSFD, 4)) != NULL) {
		put1 (packet, PT_SDATA);
		put1 (packet, ThisSampleId);
		put1 (packet, HeartRate);
		tcv_endp (packet);
	}

	delay (INTV_EOR, SN_EOR);

endthread

// ============================================================================

#define	SM_INIT		0
#define	SM_ACQUIRE	1
#define	SM_ACQUIRF	2
#define	SM_ACQUIRG	3
#define	SM_ACQUIRH	4
#define	SM_OUT		5
#define	SM_EOR		6

thread (sampler)

    word ovf;
    address packet;

    entry (SM_INIT)

	if (adcs_start (SAMPLE_BUFFER_SIZE * SAMPLES_PER_PACKET) == ERROR) {
		// No memory, not likely to happen but be prepared
		TheSampler = 0;
		Status = ST_READY;
		SampCount [ThisSampleSlot] = 0;
		send_status (BSFD);
		DISPIT;
		finish;
	}

	// We have succeeded

    entry (SM_ACQUIRE)

NextSample:

	adcs_get_sample (SM_ACQUIRE, SBuf + (ADCS_SAMPLE_LENGTH * 0));

    entry (SM_ACQUIRF)

	adcs_get_sample (SM_ACQUIRF, SBuf + (ADCS_SAMPLE_LENGTH * 1));

    entry (SM_ACQUIRG)

	adcs_get_sample (SM_ACQUIRG, SBuf + (ADCS_SAMPLE_LENGTH * 2));

    entry (SM_ACQUIRH)

	adcs_get_sample (SM_ACQUIRH, SBuf + (ADCS_SAMPLE_LENGTH * 3));

    entry (SM_OUT)

	ee_write (SM_OUT, MULT_STORAGE_UNIT (BufferIn), (byte*) SBuf,
		BUFFER_STORAGE_UNIT);

	// Update the IN pointer
	if (++BufferIn == BufferLimit)
		BufferIn = 0;

	if ((ovf = adcs_overflow ()) != 0) {
		if ((ovf += LostSamples) > 99)
			LostSamples = 99;
		else
			LostSamples = (byte) ovf;
	}

	if (XWS) {
		// Sending while sampling
		if (QPackets <= PACKET_QUEUE_LIMIT) {
			// Send this sample
			packet = tcv_wnp (WNONE, BSFD, 6 + BUFFER_STORAGE_UNIT);
			if (packet != NULL) {
				put1 (packet, PT_SDATA);
				put1 (packet, ThisSampleId);
				put1 (packet, HeartRate);
				put3 (packet, SampCount [ThisSampleSlot]);
				memcpy (((byte*) packet) + 8, (byte*) SBuf,
					BUFFER_STORAGE_UNIT);
				tcv_endp (packet);
			}
		}

	} else {
		// Just sampling
		if (SamplesToStat-- == 0) {
			SamplesToStat = SAMPLES_PER_SECOND;
			send_status (BSFD);
		}
	}

	SampCount [ThisSampleSlot]++;

	if (--SendFrom)
		goto NextSample;

	// Done
	adcs_stop ();

	if (XWS) {
		XWS = 2;
		proceed (SM_EOR);
	}

	// This is also used as a flag == Heart Rate piggybacked onto sample
	// packets

	Status = ST_READY;

	TheSampler = 0;
	send_status (BSFD);
	DISPIT;
	finish;

    entry (SM_EOR)

	if (QPackets > PACKET_QUEUE_LIMIT) {
		when (0, SM_EOR);
		release;
	}

	if ((packet = tcv_wnp (WNONE, BSFD, 4)) != NULL) {
		put1 (packet, PT_SDATA);
		put1 (packet, ThisSampleId);
		put1 (packet, HeartRate);
		tcv_endp (packet);
	}

	delay (INTV_EOR, SM_EOR);

endthread

// ============================================================================

#define	HR_INIT		0
#define	HR_SEND		1

thread (hrate)

    address packet;

    entry (HR_INIT)

	delay (3 * 1024, HR_SEND);
	release;

    entry (HR_SEND)

	if ((HeartRate = hrc_get ()) > 255)
		HeartRate = 255;
	
	if (XWS == 0 && (packet = tcv_wnp (WNONE, BSFD, 2)) != NULL) {
		// Do not send if the sample packets are being sent
		put1 (packet, PT_HRATE);
		// We are little endian
		put1 (packet, HeartRate);
		tcv_endp (packet);
	}

	proceed (HR_INIT);

endthread
	
// ============================================================================

#define	LI_INIT		0
#define	LI_WBIND	1
#define	LI_GETCMD	2
#define	LI_PERSTAT	3

#define	MYFD	((int)data)

strand (listener, int)

    lword lwsc;
    address packet;
    word wsc;
    byte cmd, bsc;

    entry (LI_INIT)

	// This is where we start listening. The first thing we should do
	// is to bind ourselves somehwere.

	if (BSFD == MYFD)
		// We are bound already on this interface
		proceed (LI_GETCMD);

	if (BSFD == NONE) {

		// We are not bound at all. Send a HELLO packet. We don't do
		// that if we are bound on the other interface, but we listen
		// on this one in case somebody wants to re-bind us here.

		packet = tcv_wnp (LI_INIT, MYFD, 6);

		put1 (packet, PT_HELLO);
		put4 (packet, ESN);

		tcv_endp (packet);
	}

    entry (LI_WBIND)

	// Keep waiting
	delay (INTV_HELLO, LI_INIT);
	packet = tcv_rnp (LI_WBIND, MYFD);
	if (Status) {
		// Ignore if we are busy (well we cannot be, can we?)
BIgn:
		tcv_endp (packet);
		proceed (LI_WBIND);
	}

	if (tcv_left (packet) < 5)
		goto BIgn;

	// NetId
	wsc = packet [0];

	get1 (packet, cmd);
	if (cmd != PT_BIND)
		// Only BIND command expected
		goto BIgn;

	get4 (packet, lwsc);
	if (lwsc != ESN)
		// Not my ESN
		goto BIgn;

	// Bind
	BSFD = MYFD;

	// Set the NetId
	tcv_control (MYFD, PHYSOPT_SETSID, &wsc);

	tcv_endp (packet);
	BINDIT;
	send_status (MYFD);
	DISPIT;

	// Note: BIND commands are not acked. An unbound node will keep
	// sending HELLO on the interface, so once it stops, the binding
	// AP will know that the command has succeeded

    entry (LI_GETCMD)

	// This is the main request acquisition loop
	if (BSFD != MYFD)
		// Check before every read if the binding is still ON
		proceed (LI_INIT);

	// re-check when bound on the oher interface
	when (&BSFD, LI_GETCMD);
	// do periodic status reports even if nobody asks
	delay (INTV_PERSTAT, LI_PERSTAT);

	packet = tcv_rnp (LI_GETCMD, MYFD);

	if (tcv_left (packet) < 2) {
LRet:
		tcv_endp (packet);
		proceed (LI_GETCMD);
	}

	get1 (packet, cmd);

	switch (cmd & 0xF0) {

	    case PT_UNBIND:

		// Unbind forces abort. This is not acknowledged.
		stop_all (YES);
		BSFD = NONE;
		// Become promiscuous to listen to bind commands
		wsc = 0;
		tcv_control (MYFD, PHYSOPT_SETSID, &wsc);
		tcv_endp (packet);
		DISPIT;
		proceed (LI_INIT);

	    case PT_RESET:

		// Erase all samples
		erase_all_samples ();

	    case PT_STOP:

		// Stop whatever you are doing
		stop_all (cmd & ABORT);
		DISPIT;

	    case PT_BIND:
	    case PT_REPORT:
Report:
		// Do this first to reduce memory contention on tcv_rnp
		tcv_endp (packet);
		// Send status
		send_status (MYFD);
		proceed (LI_GETCMD);

	    case PT_SAMPLE:

		// Note: if we are not idle, we simply ignore the request.
		// At the end, we send a status report, which will tell the
		// AP what has happend. If the status says that we are
		// doing the requested sample, the AP will know that we
		// have accepted the command. If we are done before the
		// report makes it to the AP, we will be idle, and the sample
		// will show up on the list.

		if (Status != ST_READY)
			goto Report;

		// The sample Id
		get1 (packet, bsc);

		// Check if have this sample already
		for (wsc = 0; wsc < MAX_SAMPLES; wsc++)
			if (SampCount [wsc] && SampIdent [wsc] == bsc)
				// Just report the status
				goto Report;

		XWS = (cmd & SM_XMIT) ? 1 : 0;
		QPackets = 0;

		// No such sample; start a new one
		get3 (packet, SendFrom);

		if (SendFrom >= BufferLimit)
			// Sanity check: do not exceed buffer capacity
			SendFrom = BufferLimit - 1;

		// Delete the samples that will overlap with us
		kill_samples (SendFrom);

		// Get a free slot
		if ((wsc = find_free_sample_slot ()) == WNONE) {
			ThisSampleSlot = NextSampleSlot;
			SampCount [ThisSampleSlot] = 0;
			if (++NextSampleSlot == MAX_SAMPLES)
				NextSampleSlot = 0;
		} else {
			ThisSampleSlot = (byte) wsc;
		}

		SampIdent [ThisSampleSlot] = ThisSampleId = bsc;
		SampStart [ThisSampleSlot] = BufferIn;

		LostSamples = 0;
		SamplesToStat = SAMPLES_PER_SECOND;

		Status = ST_SAMPLING;
		TheSampler = runthread (sampler);
		DISPIT;
		goto Report;

	    case PT_SEND:

		if (Status != ST_READY) {
			// Doing something
			if (Status != ST_SENDING) {
				if (Status == ST_SAMPLING && XWS > 1) {
					// Sampling at EOR
					XWS = 0;
					if (TheSampler != 0) {
						kill (TheSampler);
						TheSampler = 0;
					}
					Status = ST_READY;
					goto StartX;
				}
				// Actually busy
				goto Report;
			}

			if (SRP < SRL) {
				// We are sending the round, just keep going
				tcv_endp (packet);
				proceed (LI_GETCMD);
			}

			// Kill the sender
			kill (TheSender);
			TheSender = 0;
			// This is for consistency; we shall promptly restart it
			Status = ST_READY;
		}
StartX:
		LostSamples = 0;

		// Sample Id
		get1 (packet, bsc);
                for (ThisSampleSlot = 0; ThisSampleSlot < MAX_SAMPLES;
                    ThisSampleSlot++)
                        if (SampCount [ThisSampleSlot] &&
                            SampIdent [ThisSampleSlot] == bsc)
                                break;

                if (ThisSampleSlot == MAX_SAMPLES)
                        // Not found
                        goto Report;

		ThisSampleId = bsc;

		// Unpack the request
		SRL = 0;

		while (tcv_left (packet) >= 3) {
			get3 (packet, SRoster [SRL]);
			SRL++;
		}

		if (SRL == 0)
			// Just in case
			goto Report;

		tcv_endp (packet);
		SRP = 0;

		Status = ST_SENDING;

		QPackets = 0;
		XWS = 1;
		TheSender = runthread (sender);
		DISPIT;
		proceed (LI_GETCMD);

	    case PT_HRMCTRL:

		if ((cmd & HR_OFF)) {
			// Switch off the heart rate monitor
			if (HRM_ACTIVE) {
				hrc_stop ();
				kill (TheHRMonitor);
				TheHRMonitor = 0;
				HeartRate = 0;
				DISPIT;
			}
		} else {
			// Switch on the heart rate monitor
			if (!HRM_ACTIVE) {
				hrc_start ();
				TheHRMonitor = runthread (hrate);
				DISPIT;
			}
		}

		goto Report;
	}

	// Unrecognizable
	proceed (LI_GETCMD);

    entry (LI_PERSTAT)

	if (Status == ST_READY && !HRM_ACTIVE && BSFD == MYFD)
		// Periodic status report
		send_status (MYFD);

	proceed (LI_GETCMD);

endstrand

// ============================================================================

thread (root)

#define	RS_INIT		0

    word scr;

    entry (RS_INIT)

	hrc_stop ();
	erase_all_samples ();
	init_storage ();

	lcd_on (0);
	lcd_clear (0, 0);

#ifdef BLUETOOTH_PRESENT
	// UART_B as the primary interface via BlueTooth
	phys_uart (0, MAXPLEN, BLUETOOTH_PRESENT - 1);
	phys_uart (1, MAXPLEN, BLUETOOTH_PRESENT > 1 ? 0 : 1);
#else
	phys_cc1100 (0, MAXPLEN);
	phys_uart (1, MAXPLEN, 0);
#endif
	tcv_plug (0, &plug_heart);

	RSFD = tcv_open (NONE, 0, 0);
	USFD = tcv_open (NONE, 1, 0);

	if (RSFD < 0 || USFD < 0) {
		lcd_write (0, "FAILED TO START INTERFACES!");
		finish;
	}

	diag ("STARTED");

	scr = 0;
	tcv_control (RSFD, PHYSOPT_SETSID, &scr);
	tcv_control (USFD, PHYSOPT_SETSID, &scr);

	tcv_control (RSFD, PHYSOPT_TXON, NULL);
	tcv_control (RSFD, PHYSOPT_RXON, NULL);

	tcv_control (USFD, PHYSOPT_TXON, NULL);
	tcv_control (USFD, PHYSOPT_RXON, NULL);

#ifndef BLUETOOTH_PRESENT
	scr = XMIT_POWER;
	tcv_control (RSFD, PHYSOPT_SETPOWER, &scr);
#endif

	runstrand (listener, USFD);
	runstrand (listener, RSFD);

#ifndef	DONT_DISPLAY
	Display = runthread (display);
#endif

	// We are not needed any more. This will provide a high-priority slot
	// for the sampler.
	finish;

endthread
