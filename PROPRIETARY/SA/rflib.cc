/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#include "sysio.h"
#include "rflib.h"

byte rrf_regs [] = {
//
// We start from the fixed registers, i.e., those whose values we should never
// be fooling around with
//
	CCxxx0_IOCFG2,	0x2f,	// Unused and grounded
	CCxxx0_IOCFG1,	0x2f,	// Unused and grounded
	CCxxx0_IOCFG0,	0x01,	// FIFO nonempty, packet received
	CCxxx0_FIFOTHR,	0x0F,	// 64 bytes in FIFO
	// Initial setting of the sync word upper byte, lower byte
	CCxxx0_SYNC1,	((RADIO_SYSTEM_IDENT >> 8) & 0xff),
	CCxxx0_SYNC0,	((RADIO_SYSTEM_IDENT     ) & 0xff),
	// The default setting for PKTLEN is max FIFO - the byte length;
	// this is because the default packet params are for "formatted"
	// packets with the length sent as the first byte
        CCxxx0_PKTLEN,   63,
	// 5-7	PQT preamble threshold; nonzero value increases preamble
	//	quality requirements before accepting sync, if zero, sync
	//	is always accepted
	// 3	AUTOFLUSH (do not use)
	// 2	APPEND STATUS ON (default ON)
	// 0-1	ADDRESS CHECK (don't use, set to zero)
	CCxxx0_PKTCTRL1, 0x04,
	// 6	Whitening ON (default, xmit only)
	// 4-5	PKT_FORMAT: should always be zero (NORMAL, use FIFO)
	// 2	CRC ON
	// 0-1	LENGTH_CONFIG (we use only 0 and 1):
	//		0 - fixed (length in PKTLEN reg)
	//		1 - first byte after sync (default)
	//		2 - infinite
	//		3 - reserved
	CCxxx0_PKTCTRL0,(0x41 | CRC_FLAGS), // Whitening, pkt lngth follows sync
        CCxxx0_ADDR,    0x00,   	// Device address.
	CCxxx0_CHANNR,  0x00,		// Channel number
	// 0-4	The IF (intermediate frequency) in the receiver.
	//	The formula is: IF = (f_xosc * VALUE) / 2^10
        CCxxx0_FSCTRL1, 0x0C,
	// Frequency offset to be added, if we want to use manual frequency
	// offset, e.g., as determined by the frequency compensation loop;
	// we won't use it, I guess
        CCxxx0_FSCTRL0, 0x00,
	// Frequency default setting -> to what is being used by the driver
	CCxxx0_FREQ2,   CC1100_FREQ_FREQ2_VALUE,
        CCxxx0_FREQ1,   CC1100_FREQ_FREQ1_VALUE,
        CCxxx0_FREQ0,   CC1100_FREQ_FREQ0_VALUE,
	// 6-7	CHANBW_E
	// 4-5	CHANBW_M
	// 	Together they determine channel filter bandwidth, i.e., the
	//	selectivity of the channel; the doc says that you can reduce
	//	this (to improve sensitivity) if you know that the match is
	//	good, or you have compensated (by an offset, see above)
	//	The two together, treated as a single value (4-7), yield:
	//
 	//	 0 => 812 kHz
 	//	 1 => 650
 	//	 2 => 541
 	//	 3 => 464
 	//	 4 => 406
 	//	 5 => 325
 	//	 6 => 270
 	//	 7 => 232
 	//	 8 => 203
 	//	 9 => 162
	//	10 => 135
	//	11 => 116
	//	12 => 102
	//	13 =>  81
	//	14 =>  68
	//	15 =>  58
	//
	// 0-3	DRATE_E (see below)
       	CCxxx0_MDMCFG4, 0x68,   // MDMCFG4 	(4.8 kbps)
	// DRATE_M: (with DRATE_E) determines data rate (bps):
	// 	R_DATA = (((256 + DRATE_M) * 2^DRATE_E) * f_osc) / 2^28
	// Conversely:
	// 	DRATE_E = floor ( log2 ((R_DATA * 2^20)/f_osc ) )
	// 	DRATE_M = ((R_DATA * 2^28) / (f_osc * 2^DRATE_E)) - 256
	// Default settings: BW = 270 kHz, rate = 9992.6 bps
       	CCxxx0_MDMCFG3, 0x93,
	// 7	Disable DC-blocking filter (for irrelevant current savings)
	//	should be zero
	// 4-6	MOD_FORMAT (encoding):
	//	0	2-FSK
	//	1	GFSK
	//	3	ASK/OOK
	//	4	4-FSK
	//	7	MSK
	// 3	Manchester ON
	// 0-2	SYNC_MODE:
	//	0	no preamble/sync word
	//	1	15 of 16 sync word bits
	//	2	16 of 16
	//	3	30 of 32
	//	4	no preamble/sync CS above threshold (see AGCCTRL1,2)
	//	5	15 of 16 CS above threshold
	//	6	16 of 16 CS above ...
	//	7	30 of 32 .....
	// Default: 2-FSK, Manchester OFF, 30 of 32
        CCxxx0_MDMCFG2, 0x03,
	// 7	FEC_EN	(enable FEC)
	// 4-6	Minimum preamble bits to xmit:
	//	0	3
	//	1	3
	//	2	4
	//	3	6
	//	4	8
	//	5	12
	//	6	16
	//	7	24
	// 0-1	CHANSPC_E: Exponent of channel spacing, see below
	// Default: 8 bit preamble
        CCxxx0_MDMCFG1, 0x42,
	// Channel spacing mantissa: CHANSPC_M, the formula (Hertz):
	//	DELTA = (f_xosc * (256 + M) * 2^E) / 2^18
	// Default: 199951.17 Hz
        CCxxx0_MDMCFG0, 0xF8,
	// 4-6	Deviation Exp
	// 0-2	Deviation Mant
	// This specifies the frequency deviation for a 0 [nominal-dev] and
	// 1 [nominal + dev], calculated as:
	//	dev = (f_xosc * (8 + M) * 2^E) / 2^17
	// Note: the above formula only applies to 2-FSK, GFSK, 4-FSK
	// for MSK specifies some phase depth, for ASK/OSK, ignored
	// Default: 19043 (isn't this too little? Reset default is 47 = 47607.
	CCxxx0_DEVIATN, 0x34,   // DEVIATN	47 -> 40 -> 34
 	// Radio control state machine
	// 4	RX_TIME_RSSI: terminate RX based on RSSI (CS), see AGCCTRL1,2
	// 3	RX_TIME_QUAL: add to sustain (non-termination condition)
	//	preamble qualifier PQI above PQT (this is for WOR functions,
	//	so we shouldn't care, I guess, keep them zero)
	// 0-2	the timer; we shall ignore it; there's table on p. 80
	// This is the default setting, also reset setting:
	CCxxx0_MCSM2,	0x07,
	// 4-5	CCA_MODE: clear channel indication for XMIT (LBT):
	//	0	always
	//	1	RSSI below threshold
	//	2	unless receiving a packet
	//	3	both
	// 2-3	RXOFF_MODE: what to do after reception
	//	0	IDLE
	//	1	FSTXON
	//	2	TX
	//	3	Stay in RX
	// 0-1	TXOFF_MODE: what to do after TX
	//	0	IDLE
	//	1	FSTXON
	//	2	Stay in TX
	//	3	RX
	// Driver's default depends on LBT mode, we ignore LBT, after RX we
	// go IDLE until the packet has been accepted and RX has been opened
	// explicitly by the driver
	CCxxx0_MCSM1,	0x03,
	// 4-5	Autocalibration:
	//	0	don't, manual only (SCAL)
	//	1	when going from IDLE to RX or TX
	//	2	when going from RX or TX to IDLE
	//	3	every 4th time on RX/TX -> IDLE
	// 2-3	Timeout to stabilize oscillator on power up
	//	0	2.3-2.4 us
	//	1	37-39 us
	//	2	149-155	us
	//	3	597-620	us
	// 1	Pin radio control option (0)
	// 0	Force oscillator to stay on in SLEEP (perhaps makes sense in
	//	the analyzer?
#ifdef	__CC430__
        CCxxx0_MCSM0,   0x10,
#else
        CCxxx0_MCSM0,   0x18,
#endif       
	// Frequency Offset Compensation Configuration
	// 5	Carrier Sense asserted. Freeze compensation until CS goes
	//	high (according to the CS thresholding [absolute/relative]
	//	described in AGCCTRL1,2). Useful when there are long gaps
	//	between reception with RF switched on (otherwise offset may
	// 	go to boundaries while tracking noise.
	//	Note: may be useful in our case!
	// 3-4	Compensation gain before sync word: 0 => K, 1 => 2K, 2 => 3K,
	//	3 => 4K (whatever that means); how much effort to spend before
	//	sync
	// 2	Gain after sync detected: 0 => same as FOC_PRE_K, 1 => K/2
	// 0-1	Saturation point for the compensation algorithm (when to stop):
	//	0 the whole algorithm switched off (required for ASK/OOK)
	//	1 => +-BW_chan/8
	//	2 => +-BW_chan/4
	//	3 => +-BW_chan/2
	//	where BW_chan is the channel filter bandwidth (MDMCFG4)
	// Default: 3K before sync, after K/2, BW/8 (perhaps should be
	// widened for the analyzer
        CCxxx0_FOCCFG,  0x15,
	// Bit synchronization (bit rate offset compensation)
	// 6-7	Clock recovery loop integral before sync (like for MCSM0, 3-4)
	// 4-5	Clock recovery loop proportional gain before sync (like 6-7)
	// 3	------------------- integral after sync (like MCSM0, 2)
	// 2    ------------------- proportional after sync (0 same, 1 Kp)
	// 0-1	Saturation point:
	//	0 OFF
	//	1 +- 3.125%
	//	2 +- 6.25%
	//	3 +- 12.5%
	// Default is off, as we can see, should it?
        CCxxx0_BSCFG,   0x6C,
	// AGC control: primarily determining CS for clear channel assessment,
	// but also for triggering CS-based reception events, if used
	// 6-7	MAX_DVGA_GAIN: reduces the maximum available DVGA gain
	//	0 => all gain settings can be used
	//	1 => the highest cannot be used
	//	2 => two highest ....
	//	3 => three highest
	// 3-5	MAX_LNA_GAIN: sets the maximum allowable LNA + LNA 2 gain
	//	relative to maximum gain:
	//	0 => max possible
	//	1 => 2.6 dB below max
	//	2 => 6.1 dB below
	//	3 => 7.4
	//	4 => 9.2
	//	5 => 11.5
	//	6 => 14.6
	//	7 => 17.1
	//	The gain is described by tables 32 and 33 (depending on bit
	//	rate and MAGN_TARGET), e.g., 2.4kbps, 2 (33dB), 868 MHz:
	//		00	01 	10 	11 
	//	-------------------------------------
	//	000 	-97.5 	-91.5 	-85.5 	-79.5 
	//	001 	-94 	-88 	-82.5 	-76
	//	010 	-90.5 	-84.5 	-78.5 	-72.5
	//	011 	-88 	-82.5 	-76.5 	-70.5
	//	100 	-85.5 	-80 	-73.5 	-68
	//	101 	-84 	-78 	-72 	-66
	//	110 	-82 	-76 	-70 	-64
	//	111 	-79 	-73.5 	-67 	-61 
	//
	//	or, 250 kbps, 7 (42dB), 868 MHz
	//		00 	01 	10 	11 
	//	000 	-90.5 	-84.5 	-78.5 	-72.5 
	//	001 	-88 	-82 	-76 	-70
	//	010 	-84.5 	-78.5 	-72 	-66
	//	011 	-82.5 	-76.5 	-70 	-64
	//	100 	-80.5 	-74.5 	-68 	-62
	//	101 	-78 	-72 	-66 	-60
	//	110 	-76.5 	-70 	-64 	-58
	//	111 	-74.5 	-68 	-62 	-56 
	//	To set high (only strong signals are wanted), first reduce the
	//	LNA value and then DVGA
	//
	// 0-2	MAGN_TARGET: sets the target value from the digital filter;
	//	increasing this value reduces close-in selectivity, but
	//	improves sensitivity; I guess this only applies to scenarios
	//	where CS is actually used
	//	0 => 24 dB
	//	1 => 27
	//	2 => 30
	//	3 => 33
	//	4 => 36
	//	5 => 38
	//	6 => 40
	//	7 => 42
	// Default is: 0x03 (Note: still not sure if this is right for the
	// kind of LBT operation we implement
        CCxxx0_AGCCTRL2,0x03,
	// 6	LNA_PRIORITY: selects between two strategies of LNA gains
	//	adjustments, use 1 and never touch it (I guess)
	// 4-5	CS relative threshold (for triggering CS)
	//	0 => disabled
	//	1 => 6 dB increase
	//	2 => 10
	//	3 => 14
	// 0-3	CS absolute threshold (RSSI relative to MAGN_TARGET)
	//	-8 (1000) disabled
	//	-7 (1001) 7 dB below MAGN_TARGET
	//	...
	//	-1 (1111) 1 dB below
	//	 0 (0000) at MAGN_TARGET
	//	 1 (0001) 1 dB above
	//	...
	//	 7 (0111) 7 dB above
	// Default: depends on LBT setting, we use 0x40 (disabling CS)
	CCxxx0_AGCCTRL1, 0x40,
	// 6-7	Hysteresis level for CS trigger (I think)
	//	0 - none (no hysteresis), 3 - largest
	// 4-5	Wait time (number of passed samples before accumulating)
	//	0 - 8, 1 - 16, 2 - 24, 3 - 32
	// 2-3	When to freeze AGC
	//	0 - never
	//	1 - on sync word found
	//	2 - manual override analogue gain setting
	//	3 - manual override both analogue and digital gain setting ???
	// 0-1	FILTER_LENGTH:
	//	2-FSK, 4-FSK, MSK: Sets the averaging length for the amplitude
	//	from the channel filter.  
	//	ASK, OOK: Sets the OOK/ASK decision boundary for OOK/ASK
	//	reception. 
	//	       Samples  Decision boundary
	//	0 (00) 	8  	4 dB
	//	1 (01) 16  	8 dB
	//	2 (10) 32 	12 dB
	//	3 (11) 64 	16 dB 
	// Default: 0x91
        CCxxx0_AGCCTRL0, 0x91,
	// --------------------------------------------------------------------
	CCxxx0_WOREVT1,  0x87,
	CCxxx0_WOREVT0,  0x6B,
	CCxxx0_WORCTRL,  0x01,
	// --------------------------------------------------------------------
        CCxxx0_FREND1,   0x56,	// Current control
	// Power control
	// 4-5 set to 1 and don't touch
	// 0-2 PATABLE index (XMT power setting)
	CCxxx0_FREND0,	 0x10,
        CCxxx0_FSCAL3,   0xA9,	// Frequency Synthesizer Calibration
        CCxxx0_FSCAL2,   0x2A,	// Default for 200kbps was 0x0A
	CCxxx0_FSCAL1,	 0x00,
        CCxxx0_FSCAL0,   0x0D,
	// --------------------------------------------------------------------
#if 0
        CCxxx0_RCCTRL1,  0x00,
        CCxxx0_RCCTRL0,  0x00,
        CCxxx0_FSTEST,   0x59,
        CCxxx0_PTEST,    0x7F,
        CCxxx0_AGCTEST,  0x3F,
        CCxxx0_TEST2,    0x88,
        CCxxx0_TEST1,    0x31,
        CCxxx0_TEST0,    0x02,
#endif
	// ====================================================================

	0xff };

byte rrf_patable [8] = CC1100_PATABLE;

void rrf_mod_reg (byte addr, byte *val, word length) {
//
// Resets the register value both in the module as well as in the table
//
	sint i;
	byte r;

	if ((r = (addr & 0x3F)) >= CCxxx0_RCCTRL1 && r <= CCxxx0_TEST0)
		// Ignore write to test registers
		return;

	if (length == 0) {
		// a simple register (at least this is what we suspect); also
		// update the table copy, if address present
		for (i = 0; rrf_regs [i] != 255; i += 2) {
			if (rrf_regs [i] == addr) {
				rrf_regs [i+1] = *val;
				break;
			}
		}
		rrf_set_reg (addr, *val);
		return;
	}

	// Burst write
	if (addr == CCxxx0_PATABLE) {
		if (length > sizeof (rrf_patable))
			length = sizeof (rrf_patable);
		memcpy (rrf_patable, val, length);
	}

	rrf_set_reg_burst (addr, val, length);
}
