#include "sysio.h"
#include "params.h"
#include "neighbors.h"

// Neighbor table: autoinitialized to all zeros
static neighbor_t NTable [MAXNEIGHBORS];

// Last received neighbor list
static olist_t *RNList = NULL;

// ===========================================================================

void neighbors_status (address packet) {
//
// Return neighbors-related status (two words)
//
	word i, wc;

	for (i = wc = 0; i < MAXNEIGHBORS; i++)
		if (NTable [i] . esn != 0)
			wc++;

	put2 (packet, wc);
	if (RNList != NULL)
		wc = wofb (RNList->buf);
	else
		wc = 0;
	put2 (packet, wc);
}

void neighbors_clean (byte what) {
//
// Cleans up neighbor info
//
	word i;

	if ((what & CLEAN_NEI)) {
		// Own neighbor list
		for (i = 0; i < MAXNEIGHBORS; i++)
			NTable [i] . esn = 0;
	}

	if ((what & CLEAN_NEI_LIST)) {
		// Acquired list of neighbors
		if (RNList != NULL) {
			ufree (RNList);
			RNList = NULL;
		}
	}
}

void hello_in (address p) {
//
// Receive a HELLO packet
//
	lword e;
	word i, j;

	if (tcv_left (p) < 5)
		return;

	// Ignore the request number byte
	get1 (p, e);
	get4 (p, e);
	for (j = MAXNEIGHBORS, i = 0; i < MAXNEIGHBORS; i++) {
		// First try to look up this one
		if (NTable [i] . esn == e) {
			NTable [i] . when = (word) seconds ();
			return;
		}
		if (NTable [i] . esn == 0)
			j = i;
	}

	// Use a free slot
	if (j != MAXNEIGHBORS) {
		NTable [j] . esn = e;
		NTable [j] . when = (word) seconds ();
	}
}

// ============================================================================

static word do_neilist (byte *buf) {
//
// Calculate the list length and (optionally) fill the buffer
//
	word size, i;

	size = 2;	// Link Id of the owner

	if (buf)
		*((word*)buf) = MCN;

	for (i = 0; i < MAXNEIGHBORS; i++) {
		if (NTable [i] . esn != 0) {
			if (buf)
				memcpy (buf + size, (byte*)(&(NTable[i].esn)),
					4);
			size += 4;
		}
	}

	return size;
}

static Boolean neil_lkp_snd (word oid) {
//
// Prepare to send the neigbor list
//
	word size;
	
	if (oid == 0) {

		// Own list, size first
		size = do_neilist (NULL);

		if (alloc_dhook (sizeof (old_s_t) + size) == NULL)
			// No memory, refuse
			return NO;

		do_neilist (olsd_cbf);
	
	} else if (oid == 1) {

		// This means the list you have acquired from another node
		if (RNList == NULL)
			return NO;

		size = RNList->size;
		if (alloc_dhook (sizeof (old_s_t) + size) == NULL)
			return NO;

		memcpy (olsd_cbf, RNList->buf, size);

	} else {
		// Illegal object, only 0 and 1 supported at present
		return NO;
	}

	olsd_size = size;
	return YES;
}

static Boolean neil_ini_rcp (address packet) {
//
// Initialize for neighbor list reception
//
	return objl_ini_rcp (packet, &RNList);
}

static Boolean neil_cnk_rcp (word st, address packet) {
//
// Receive a chunk
//
	return objl_cnk_rcp (packet, &RNList);
}

static Boolean neil_stp_rcp () {
//
// Terminate reception
//
	return objl_stp_rcp (&RNList);
}

// ============================================================================

thread (nager)

#define	NAG_RUN	0

    word i, s, t;

    entry (NAG_RUN)

	s = seconds ();
	for (i = 0; i < MAXNEIGHBORS; i++) {
		if (NTable [i] . esn) {
			if ((s >= (t = NTable [i] . when) && s < t + MAXNAGE) ||
			    (s < t && t + MAXNAGE > s))
				// Delete
				NTable [i] . esn = 0;
		}
	}

	delay (NAGFREQ, NAG_RUN);
	release;

endthread

// ============================================================================

const fun_pak_t nei_pak = {
				neil_lkp_snd,
				objl_rts_snd,
				objl_cls_snd,
				objl_cnk_snd,
				neil_ini_rcp,
				neil_stp_rcp,
				neil_cnk_rcp,
				objl_cls_rcp
			   };

// ============================================================================
