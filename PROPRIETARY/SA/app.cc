//
// A generalized spectrum analizer, or rather a neighborhood analyzer
//
#include "sysio.h"
#include "rflib.h"
#include "oss.h"

// Sampling parameters
static lword	FreqStart,	// Frequency range to scan
		FreqStep,	// Increment
		FreqMax,

		FreqPrev,	// Previous and current frequency (to avoid
		FreqCurr;	// unnecessary register updates)

static word	SamplesToAverage,	// Samples per take
		SampleDelay,		// Inter-sample delay
		CurrentSample;

static word	Mode;		// 0-band, 1-single freq, 2-single+receiving

// Samples
static byte Samples [OSS_SAMPLES];

// Packet to inject and its length
static byte *PTI = NULL, PTIL;

#define	MAX_CTS_TRIES	1024

// ============================================================================

#define	LED_RED		0
#define	LED_GREEN	1
#define	LED_YELLOW	2
static byte led_stat [3];

static led_toggle (word led) {

	led_stat [led] = 1 - led_stat [led];
	leds (led, led_stat [led]);
}

static leds_clear () {

	leds_all (0);
	fastblink (0);
	led_stat [0] = led_stat [1] = led_stat [2] = 0;
}

// ============================================================================

static lword unpack3 (byte *b) {

	return (lword)(b[0]) | ((lword)(b[1]) << 8) | ((lword)(b[2]) << 16);
}

static word unpack2 (byte *b) {

	return (word)(b[0]) | ((word)(b[1]) << 8);
}

static void pack3 (byte *b, lword val) {

	b [0] = (byte) val;
	b [1] = (byte) (val >> 8);
	b [2] = (byte) (val >> 16);
}

static void pack2 (byte *b, word val) {

	b [0] = (byte) val;
	b [1] = (byte) (val >> 8);
}

static void out_fifo (word st) {

	sint len;
	byte *buf;

	if ((len = rrf_rx_status ()) <= 0)
		return;

	if ((buf = oss_outu (st, len + 2)) == NULL)
		return;

	buf [0] = OSS_CMD_U_PKT;

	rrf_get_reg_burst (CCxxx0_RXFIFO, buf + 2, buf [1] = (byte) len);

	oss_send (buf);
}

static void flush_samples (word st) {

	byte *buf;

	if ((buf = oss_outu (st, OSS_SAMPLES+1)) == NULL)
		// This can only happen if st id WNONE
		return;

	buf [0] = OSS_CMD_U_SMP;
	memcpy (buf+1, Samples, OSS_SAMPLES);
	oss_send (buf);
}

// ============================================================================

static void set_freq () {
//
// Sets the RF frequency to FreqCurr
//
	lword diff;

	rrf_enter_idle ();
	diff = FreqCurr ^ FreqPrev;
	if ((diff & 0xff) != 0)
		// FREQ0 changed
		rrf_set_reg (CCxxx0_FREQ0, (byte) FreqCurr);
	if ((diff & 0xff00) != 0)
		// FREQ1 changed
		rrf_set_reg (CCxxx0_FREQ1, (byte) (FreqCurr >> 8));
	if ((diff & 0xff0000) != 0)
		// FREQ2 changed
		rrf_set_reg (CCxxx0_FREQ2, (byte) (FreqCurr >> 16));
	// Note: we do not update mirror registers
	rrf_enter_rx ();
	FreqPrev = FreqCurr;
}

static void update_frequency () {
//
// Increment the frequency accounting for a possible wrap around
//
	if (++CurrentSample >= OSS_SAMPLES) {
		CurrentSample = 0;
		FreqCurr = FreqStart;
	} else {
		FreqCurr += FreqStep;
	}
}

static lword init_freq () {
//
// Initializes the starting scan frequency at random from within the sampled
// range
//
	FreqCurr = FreqStart + FreqStep * (CurrentSample = lrnd () & 0x7f);
}

fsm band_sampler {
//
// Samples the specified band
//
	word sn, sacc, scnt;

	state LOOP:

		init_freq ();
		sn = 0;
		led_toggle (LED_YELLOW);
		leds (LED_GREEN, 0);
		leds (LED_RED, 0);

	state FREQ_UPDATE:

		set_freq ();
		sacc = scnt = 0;
		if (SampleDelay) {
			delay (SampleDelay, SAMPLE);
			release;
		}

	state SAMPLE:

		sacc += (rrf_get_reg (CCxxx0_RSSI) + 128);
		if (RX_FIFO_READY)
			// make sure we are not stuck; this will seldom happen
			rrf_enter_rx ();
		scnt++;

		if (scnt < SamplesToAverage)
			proceed SAMPLE;

		Samples [CurrentSample] = (sacc + (scnt >> 1)) / scnt;
		sn++;

		if (sn < OSS_SAMPLES) {
			// keep going
			update_frequency ();
			proceed FREQ_UPDATE;
		}

	state FLUSH:

		flush_samples (FLUSH);
		proceed LOOP;
}

fsm freq_sampler {

	word cs, sn, max, sacc, scnt;

	state INIT:

		FreqCurr = FreqStart;
		set_freq ();

	state LOOP:

		max = sn = 0;
		led_toggle (LED_YELLOW);
		leds (LED_GREEN, 0);
		leds (LED_RED, 0);

	state WAIT_SAMPLE:

		if (PTI != NULL)
			// There is a packet to inject; we check this at sample
			// boundaries
			proceed INJECT;
			
		sacc = scnt = 0;
		delay (SampleDelay, SAMPLE);
		release;

	state SAMPLE:

		sacc += (rrf_get_reg (CCxxx0_RSSI) + 128);
		scnt++;

	state FIFO_FLUSH:

		if (RX_FIFO_READY) {
			leds (LED_GREEN, 1);
			if (Mode > 1)
				out_fifo (FIFO_FLUSH);
			rrf_enter_rx ();
		}

		if (scnt < SamplesToAverage)
			proceed SAMPLE;

		Samples [sn++] = (sacc + (scnt >> 1)) / scnt;

		if (sn < OSS_SAMPLES-1)
			// keep going
			proceed WAIT_SAMPLE;

		// This is a dummy, we only use 128 slots
		Samples [sn] = 0;

	state FLUSH:

		flush_samples (FLUSH);
		proceed LOOP;

	state INJECT:

		// Wait for channel assessment
		leds (LED_RED, 1);
		sacc = 0;

	state CCLEAR:

		if (!rrf_cts ()) {
			if (sacc >= MAX_CTS_TRIES) {
				// Failed
				ufree (PTI);
				PTI = NULL;
				rrf_enter_rx ();
				proceed INJECT_FAIL;
			}
			sacc++;
			delay (1, CCLEAR);
			release;
		}

		// Channel OK, start sending
		rrf_send (PTI, PTIL);
		// Wait until done
WaitTX:
		delay (1, INJECT_TX_CHECK);
		release;

	state INJECT_TX_CHECK:

		if (rrf_status () == CC1100_STATE_TX)
			goto WaitTX;

		ufree (PTI);
		PTI = NULL;
		rrf_enter_rx ();

	state INJECT_OK:

		oss_ack (INJECT_OK, OSS_CMD_ACK);
		proceed WAIT_SAMPLE;

	state INJECT_FAIL:

		oss_ack (INJECT_FAIL, OSS_CMD_NAK);
		proceed WAIT_SAMPLE;
}			

static void reset_all () {

	killall (band_sampler);
	killall (freq_sampler);
	if (PTI) {
		ufree (PTI);
		PTI = NULL;
	}
	leds_clear ();
}

static void start_all () {

	rrf_rx_reset ();
	// Initialize previous freq for faster assessment of what needs to be
	// changed
	FreqPrev =  ((lword) rrf_get_reg (CCxxx0_FREQ2) << 16) |
		    ((lword) rrf_get_reg (CCxxx0_FREQ1) <<  8) |
		    ((lword) rrf_get_reg (CCxxx0_FREQ0)      );

	leds_clear ();
	if (Mode)
		runfsm freq_sampler;
	else
		runfsm band_sampler;
}

// ============================================================================

static void rq_handler (word st, byte *cmd, word cmdlen) {

	switch (*cmd) {

		case OSS_CMD_RSC: {
#if 1
			reset ();
#else
			// Kill the sampler process
			reset_all ();
			oss_ack (st, OSS_CMD_ACK);
#endif
			return;
		}

		case OSS_CMD_D_SMP: {

			// A new sampling request
			reset_all ();

			// Command parameters:
			//	mode (1) = 0, 1, 2
			// 	Freq start (3)
			//	Freq step (3)
			//	Samples to average (1)
			//	Sample delay (1)

			if (cmdlen < 10) {
SErr:
				oss_ack (st, OSS_CMD_NAK);
				leds (LED_RED, 0);
				return;
			}

			Mode = cmd [1];
			if (Mode > 2)
				goto SErr;

			FreqStart 	= unpack3 (cmd +  2);
			FreqStep 	= unpack3 (cmd +  5);
			FreqMax		= FreqStart + FreqStep * OSS_SAMPLES;

			SamplesToAverage	= cmd [8];
			SampleDelay 		= cmd [9];

			oss_ack (st, OSS_CMD_ACK);

			start_all ();
			return;
		}

		case OSS_CMD_D_SRG: {

			// Set single-value register(s):
			//	len (1) - number of regs
			//	(addr, val)+

			word i;
			sint len;

			leds (LED_RED, 1);
			// The minimum is cmd npairs + 2 bytes
			if (cmdlen < 4)
				goto SErr;

			len = 2 + 2 * ((sint) cmd [1]);

			if (len < 4 || len > cmdlen)
				goto SErr;

			oss_ack (st, OSS_CMD_ACK);

			for (i = 2; i < len; i += 2)
				rrf_mod_reg (cmd [i], cmd+1+i, 0);

			leds (LED_RED, 0);
			return;
		}

		case OSS_CMD_D_GRG: {

			// Read single-valued register(s):
			//	len (1)
			//	addresses
			sint len;
			word i;
			byte *buf;

			leds (LED_RED, 1);
			if (cmdlen < 3)
				goto SErr;

			len = 2 + ((sint) cmd [1]);

			if (len < 3 || len > cmdlen)
				goto SErr;

			if ((buf = oss_outr (st, len)) == NULL)
				// Cannot happen
				goto SErr;

			buf [0] = OSS_CMD_U_GRG;
			buf [1] = (byte) (len - 2);

			for (i = 2; i < len; i++)
				buf [i] = rrf_get_reg (cmd [i]);
			oss_send (buf);
			leds (LED_RED, 0);
			return;
		}

		case OSS_CMD_D_SRGB:

			// Set register burst
			//	addr
			//	len
			//	bytes

			leds (LED_RED, 1);
			if (cmdlen < 4)
				goto SErr;

			if (cmd [2] == 0 || cmd [2] > cmdlen - 3)
				goto SErr;

			oss_ack (st, OSS_CMD_ACK);

			rrf_mod_reg (cmd [1], cmd + 3, cmd [2]);
			leds (LED_RED, 0);
			return;

		case OSS_CMD_D_GRGB: {

			// Get register burst
			//	addr
			//	len
			byte *buf;

			leds (LED_RED, 1);
			if (cmdlen < 3 || cmd [2] > OSS_SAMPLES)
				goto SErr;

			if ((buf = oss_outr (st, cmd [2] + 2)) == NULL)
				goto SErr;

			buf [0] = OSS_CMD_U_GRGB;
			buf [1] = cmd [2];

			rrf_get_reg_burst (cmd [1], buf + 2, cmd [2]);
			oss_send (buf);
			leds (LED_RED, 0);
			return;
		}

		case OSS_CMD_D_POL: {

			byte *buf;

			leds (LED_RED, 1);
			if ((buf = oss_outr (st, 4)) == NULL)
				// Cannot happen
				goto SErr;

			buf [0] = OSS_CMD_U_POL;
			buf [1] = OSS_MAG0;
			buf [2] = OSS_MAG1;
			buf [3] = OSS_MAG2;

			oss_send (buf);
			leds (LED_RED, 0);
			return;
		}

		case OSS_CMD_D_INJT:

			// Len
			// Bytes to be written to TX FIFO

			leds (LED_RED, 1);
			if (cmdlen < 3)
				goto SErr;

			if (cmd [1] == 0 || cmd [1] > cmdlen - 2)
				goto SErr;

			if (cmd [1] > MAX_TOTAL_PL + 1)
				goto SErr;

			if (!running (freq_sampler) || PTI)
				goto SErr;

			if ((PTI = (byte*) umalloc (cmd [1])) == NULL)
				goto SErr;

			PTIL = cmd [1];
			memcpy (PTI, cmd + 2, PTIL);
			return;

		default:
			goto SErr;

	}
}

fsm root {

	state INIT:

		leds_clear ();
		oss_init (rq_handler);
		// Init RF, power down, default registers
		rrf_init ();

	state RELAX:

		// Hold on to your slot
		when (root, RELAX);
		leds (LED_GREEN, 1);
}
