/* ============================================================= */
/* ============================================================= */
/* ============================================================= */

#include "sysio.h"
#include "tcvphys.h"
#include "tcvplug.h"

heapmem {100};

#include "phys_cc1100.h"
#include "phys_uart.h"

#define	XMIT_POWER	7

// The plugin

int tcv_ope_bridge (int, int, va_list);
int tcv_clo_bridge (int, int);
int tcv_rcv_bridge (int, address, int, int*, tcvadp_t*);
int tcv_frm_bridge (address, int, tcvadp_t*);
int tcv_out_bridge (address);
int tcv_xmt_bridge (address);

const tcvplug_t plug_bridge =
		{ tcv_ope_bridge, tcv_clo_bridge, tcv_rcv_bridge,
			tcv_frm_bridge, tcv_out_bridge, tcv_xmt_bridge, NULL,
				0x0091 /* Plugin Id */ };

int	RSFD, USFD;

#define	MAXPLEN			60

// The Plugin =================================================================

int 	*desc = NULL;

int tcv_ope_bridge (int phy, int fd, va_list plid) {
/*
 * This is very simple - we are allowed to have one descriptor per phy.
 */
	int i;

	if (desc == NULL) {
		desc = (int*) umalloc (sizeof (int) * TCV_MAX_PHYS);
		if (desc == NULL)
			syserror (EMALLOC, "plug_bridge tcv_ope_bridge");
		for (i = 0; i < TCV_MAX_PHYS; i++)
			desc [i] = NONE;
	}

	/* phy has been verified by TCV */
	if (desc [phy] != NONE)
		return ERROR;

	desc [phy] = fd;
	return 0;
}

int tcv_clo_bridge (int phy, int fd) {

	/* phy/fd has been verified */

	if (desc == NULL || desc [phy] != fd)
		return ERROR;

	desc [phy] = NONE;
	return 0;
}

#define	RSSI_MAX	220
#define	RSSI_MIN	(RSSI_MAX - 127)

int tcv_rcv_bridge (int phy, address p, int len, int *ses, tcvadp_t *bounds) {

	word rssi;

	if (desc == NULL)
		return TCV_DSP_PASS;

	if (phy == 0 && (((byte*)p) [2] & 0xf0) == 0x90) {
		// Convey RSSI scaled to 0-15
		rssi = ((byte*)p) [len - 1];
		if (rssi < RSSI_MIN)
			rssi = RSSI_MIN;
		else if (rssi > RSSI_MAX)
			rssi = RSSI_MAX;
		
		((byte*)p) [len - 3] = (((byte*)p) [len - 3] & 0xf0) |
			(rssi - RSSI_MIN) >> 3;
	}

	// Swap the sessions
	phy = 1 - phy;

	if ((*ses = desc [phy]) == NONE)
		return TCV_DSP_PASS;

	bounds->head = bounds->tail = 0;

	return TCV_DSP_XMT;
}

int tcv_frm_bridge (address p, int phy, tcvadp_t *bounds) {

	// Link Id + CRC
	return bounds->head = bounds->tail = 0;
}

int tcv_out_bridge (address p) {

	return TCV_DSP_XMT;

}

int tcv_xmt_bridge (address p) {

	return TCV_DSP_DROP;
}

// ============================================================================


// ============================================================================

thread (root)

#define	RS_INIT		0

    word scr;

    entry (RS_INIT)

	phys_cc1100 (0, MAXPLEN);
	phys_uart (1, MAXPLEN, 0);
	tcv_plug (0, &plug_bridge);

	RSFD = tcv_open (NONE, 0, 0);
	USFD = tcv_open (NONE, 1, 0);

	if (RSFD < 0 || USFD < 0)
		syserror (EHARDWARE, "open");

	scr = 0xffff;

	tcv_control (RSFD, PHYSOPT_SETSID, &scr);
	tcv_control (USFD, PHYSOPT_SETSID, &scr);

	tcv_control (RSFD, PHYSOPT_TXON, NULL);
	tcv_control (RSFD, PHYSOPT_RXON, NULL);
	scr = XMIT_POWER;
	tcv_control (RSFD, PHYSOPT_SETPOWER, &scr);

	tcv_control (USFD, PHYSOPT_TXON, NULL);
	tcv_control (USFD, PHYSOPT_RXON, NULL);

	finish;

endthread
