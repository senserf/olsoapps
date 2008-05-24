#ifndef	__sea_plug_h
#define	__sea_plug_h

// The Plugin =================================================================

int tcv_ope_sea (int, int, va_list);
int tcv_clo_sea (int, int);
int tcv_rcv_sea (int, address, int, int*, tcvadp_t*);
int tcv_frm_sea (address, int, tcvadp_t*);
int tcv_out_sea (address);
int tcv_xmt_sea (address);

const tcvplug_t plug_sea =
		{ tcv_ope_sea, tcv_clo_sea, tcv_rcv_sea, tcv_frm_sea,
			tcv_out_sea, tcv_xmt_sea, NULL,
				0x0083 /* Plugin Id */ };
static int* desc = NULL;

int tcv_ope_sea (int phy, int fd, va_list plid) {
/*
 * This is very simple - we are allowed to have one descriptor per phy.
 */
	int i;

	if (desc == NULL) {
		desc = (int*) umalloc (sizeof (int) * TCV_MAX_PHYS);
		if (desc == NULL)
			syserror (EMALLOC, "plug_sea tcv_ope_sea");
		for (i = 0; i < TCV_MAX_PHYS; i++)
			desc [i] = NONE;
	}

	/* phy has been verified by TCV */
	if (desc [phy] != NONE)
		return ERROR;

	desc [phy] = fd;
	return 0;
}

int tcv_clo_sea (int phy, int fd) {

	/* phy/fd has been verified */

	if (desc == NULL || desc [phy] != fd)
		return ERROR;

	desc [phy] = NONE;
	return 0;
}

int tcv_rcv_sea (int phy, address p, int len, int *ses, tcvadp_t *bounds) {

	if (desc == NULL || (*ses = desc [phy]) == NONE)
		return TCV_DSP_PASS;

	bounds->head = bounds->tail = 0;

	return TCV_DSP_RCV;
}

int tcv_frm_sea (address p, int phy, tcvadp_t *bounds) {

	// Link Id + CRC
	return bounds->head = bounds->tail = 2;
}

int tcv_out_sea (address p) {

	return TCV_DSP_XMT;

}

int tcv_xmt_sea (address p) {

	return TCV_DSP_DROP;
}

#endif
