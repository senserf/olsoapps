#include "sysio.h"
#include "params.h"
#include "images.h"

// Last received image list
static olist_t *RIList = NULL;

static	word	NEPages,	// Number of pages in EEPROM
		INOld,		// Oldest used file number
		INNew;		// First available file number

// ===========================================================================

typedef struct {
//
// Transaction structure for sending an image
//
	croster_t	ros;
	word		n,	// The number of chunks
			cch,	// Current outgoing chunk number
			x, y;	// Geometry
	word		pp [0];	// Page numbers

} img_s_t;

#define	IMSData		((img_s_t*)DHook)
#define	imsd_n		(IMSData->n)
#define	imsd_pp		(IMSData->pp)
#define	imsd_ros	(IMSData->ros)
#define	imsd_cch	(IMSData->cch)
#define	imsd_x		(IMSData->x)
#define	imsd_y		(IMSData->y)

// ===========================================================================

typedef struct {
//
// For receiving an image
//
	lword		ccp;	// Chunk pointer to write to eeprom
	address		pap;	// Pointer to the chunk in the packet

	ctally_t	cta;
	word		x, y,	// geometry
	word		fc;	// first chunk behind the label
	byte		bpp;	// bits per pixel (nonzero == 8)
	Boolean		rdy;	// eeprom ready flag
	word		pp [0];	// page numbers

} img_r_t;

#define	IMRData		((img_r_t*)DHook)
#define	imrd_ccp	(IMRData->ccp)
#define	imrd_pap	(IMRData->pap)
#define	imrd_cta	(IMRData->cta)
#define	imrd_x		(IMRData->x)
#define	imrd_y		(IMRData->y)
#define	imrd_fc		(IMRData->fc)
#define	imrd_bpp	(IMRData->bpp)
#define	imrd_rdy	(IMRData->rdy)
#define	imrd_pp		(IMRData->pp)

// ============================================================================

void images_init () {
//
// Initializes the page map
//
	lword ep, ch;
	word pn, min, max;

	NEPages = (word)(ee_size () >> IMG_PAGE_SHIFT);

	max = 0;
	min = MAX_WORD;

	for (pn = 0; pn < NEPages; pn++) {
		ep = pntopa (pn);
		ee_read (ep + IMG_PO_MAGIC, (byte*)(&ch), 4);
		if (ch != IMG_MAGIC_FIRST)
			// Not the first page of an image
			continue;
		ee_read (ep + IMG_PO_NUMBER, (byte*)(&cp), 2);
		if ((cp & 0x8000))
			// Special, ignore
			continue;
		if (cp > max)
			max = cp;
		if (cp < min)
			min = cp;
	}

	if (min == MAX_WORD) {
		// No images
		INOld = INNew = 0;
		return;
	}

	// Do a second pass
	
	INOld = max;
	INNew = min;
	for (pn = 0; pn < NEPages; pn++) {
		ep = pntopa (pn);
		ee_read (ep + IMG_PO_MAGIC, (byte*)(&ch), 4);
		if (ch != IMG_MAGIC_FIRST)
			// Not the first page of an image
			continue;
		ee_read (ep + IMG_PO_NUMBER, (byte*)(&cp), 2);
		if ((cp & 0x8000))
			// Special, ignore
			continue;

		if (cp < INOld && (INOld - cp) < IMG_MAX)
			// Determine the number of the oldest image: the
			// smallest value distant no more from max than
			// the admissible range
			INOld = cp;

		if (cp > INNew && (cp - INNew) < IMG_MAX)
			INNew = cp;
	}
	// Make it first free
	if (++INNew == 0x8000)
		INNew = 0;
}

static void image_delete (word num, word *pa, word max) {
//
// Erase the image with the specified number, return the number of released
// pages
//
	lword ep,
	word cw, pn;

	for (pn = 0; pn < NEPages; pn++) {
		ep = pntopa (pn);
		ee_read (ep + IMG_PO_MAGIC, (byte*)(&cw), 2);
		if (cw == IMAGE_MAGIC_USED) {
			ee_read (ep + IMG_PO_NUMBER, (byte*)(&cw), 2);
			if (cw == num) {
				cw = IMG_MAGIC_FREE;
				ee_write (ep + IMG_PO_MAGIC, (byte*)(&cw), 2);
				if (pa && max) {
					*pa++ = pn;
					max--;
				}
			}
		}
	}
	return max;
}

static void linum (word wc) {
//
// Transform image number from internal to logical
//
	if (wc == IMG_NUM_OWN)
		return 0;

	if (wc < INOld)
		return wc + (0x8001 - INOld);

	return wc - INOld + 1;
}

static void iinum (word wc) {
//
// Transform image number from logical to internal
//
	if (wc == 0)
		// Image zero 
		return IMG_NUM_OWN;
	wc--;
	if ((wc += INOld) >= 0x8000)
		wc -= 0x8000;

	return wc;
}

void images_status (address packet) {
//
// Return image-related status (into an OSS_DONE packet) - two words
//
	word wc;

	// The number of images (excluding own)
	if (INNew >= INOld)
		wc = INNew - INOld;
	else
		wc = (0x8000 - INNew) + INOld - 1;

	put2 (packet, wc);

	if (RIList != NULL)
		wc = wofb (RIList->buf);
	else
		wc = 0;

	put2 (packet, wc);
}

void images_clean (byte what) {
//
// Cleans up images
//
	lword ep;
	word pn, cw;

	if ((what & (CLEAN_IMG_OWN | CLEAN_IMG))) {
		// Erase images in eeprom
		for (pn = 0; pn < NEPages; pn++) {
			ep = pntopa (pn);
			ee_read (ep + IMG_PO_MAGIC, (byte*)(&cw), 2);
			if (cw != IMAGE_MAGIC_USED)
				// No need to touch this one
				continue;
			if ((what & (CLEAN_IMG_OWN | CLEAN_IMG)) !=
			    (CLEAN_IMG_OWN | CLEAN_IMG)) {
				// Have to check what it is
				ee_read (ep + IMG_PO_NUMBER, (byte*)(&cw), 2);
				if ((what & CLEAN_IMG_OWN) && cw != IMG_NUM_OWN)
					continue;
				if ((what & CLEAN_IMG_OWN) == 0 &&
				    cw == IMG_NUM_OWN)
					continue;
			}
			// Erase it
			cw = IMG_MAGIC_FREE;
			ee_write (ep + IMG_PO_MAGIC, (byte*)(&cw), 2);
		}
		if ((what & CLEAN_IMG))
			INOld = INNew = 0;
	}

	if ((what & CLEAN_IMG_LIST)) {
		// Erase the list received from somebody else
		if (RIList != NULL) {
			ufree (RIList);
			RIList = NULL;
		}
	}
}

// ============================================================================
// Functions for sending/receiving the image list =============================
// ============================================================================

static word do_imglist (byte *buf) {
//
// Calculate the list length and (optionally) fill the list is pointer != NULL
//
	lword ep, ch;
	word size, wc, pn, wls;

	size = 2;
	// The minimum size is 2. The first word contains the link Id of the
	// owner.

	if (buf)
		*((word*)buf) = MCN;

	for (pn = 0; pn < NEPages; pn++) {
		ep = pntopa (pn);
		ee_read (ep + IMG_PO_MAGIC, (byte*)(&ch), 4);
		if (ch != IMG_MAGIC_FIRST)
			continue;
		// Label size
		ee_read (ep + IMG_PO_LL, (byte*)(&wls), 2);
		if (wls) {
			// There is a label, its actual length is in the first
			// word of the image
			ee_read (ep + IMG_PO_CHUNKS, (byte*)(&wls), 2);
			if (wls > 64)
				// This is in words
				wls = 64;
		}
		if (buf) {
			// Total description size; the size word doesn't
			// count
			*((word*)(buf+size)) = wls + 8;
			size += 2;

			// Image number
			ee_read (ep + IMG_PO_NUMBER, wc, 2);
			// Transform into external
			wc = linum (wc);
			*((word*)(buf+size)) = wc;
			size += 2;

			// File size
			ee_read (ep + IMG_PO_NCHUNKS, buf + size, 2);
			size += 2;

			// Geometry
			ee_read (ep + IMG_PO_X, by + size, 4);
			size += 4;

			// The label
			if (wls)
				ee_read (ep + IMG_PO_CHUNKS+1, buf + size, wls);
		} else {
			// - size word (next pointer)
			// - node's file number
			// - file size (number of chunks)
			// - x geometry
			// - y geometry
			size += 10;
		}
		size += wls;
	}
	return size;
}

static Boolean imgl_lkp_snd (word oid) {
//
// OID = 0 -> own list, OID = 1 -> last received list
//
	word size;
	
	if (oid == 0) {

		// Extract own list and store in memory

		size = do_imglist (NULL);
		if (size > 4096)
			// Keep it decent
			return NO;

		if (alloc_dhook (sizeof (old_s_t) + size) == NULL)
			// No memory, refuse
			return NO;

		do_imglist (&olsd_cbf);
	
	} else if (oid == 1) {

		// This means the list you have acquired from another node
		if (RIList == NULL)
			return NO;

		size = RIList->size;
		if (alloc_dhook (sizeof (old_s_t) + size) == NULL)
			// For now, try to copy the list to a separate area;
			// needless to say, we can be smarter about this later
			return NO;

		memcpy (olsd_cbf, RIList->buf, size);

	} else {
		// Illegal object, only 0 and 1 supported at present
		return NO;
	}

	olsd_size = size;
	return YES;
}

// ============================================================================

static Boolean imgl_ini_rcp (address packet) {
//
// Initialize for reception
//
	return objl_ini_rcp (packet, &RIList);
}

static Boolean imgl_cnk_rcp (address packet, word st) {
//
// Receive a chunk (state is ignored, we are always ready)
//
	return objl_cnk_rcp (packet, &RIList);
}

static Boolean imgl_stp_rcp () {
//
// Terminate reception
//
	return objl_stp_rcp (&RIList);
}

// ============================================================================
// Functions for sending/receiving images =====================================
// ============================================================================

static img_lkp_snd (word oid) {
//
// The Id is the 'logical' image number, e.g., as conveyed in the image list
//
	lword ep;
	word cw, ic, np, pn;

	// Logical to internal
	oid = iinum (oid);

	for (np = pn = 0; pn < NEPages; pn++) {
		ep = pntopa (pn);
		ee_read (ep + IMG_PO_MAGIC, (byte*)(&cw), 2);
		if (cw == IMG_MAGIC_USED) {
			ee_read (ep + IMG_PO_NUMBER, (byte*)(&cw), 2);
			if (cw == oid)
				np++;
		}
	}

	if (np == 0)
		return NO;

	if (alloc_dhook (sizeof (img_s_t) + sizeof (word) * np) == NULL)
		return NO;

	for (cw = 0; cw < np; cw++)
		// To detect inconsistencies
		imsd_pp [cw] = WNONE;

	// The second round
	for (pn = 0; pn < NEPages; pn++) {
		ep = pntopa (pn);
		ee_read (ep + IMG_PO_MAGIC, (byte*)(&cw), 2);
		if (cw != IMG_MAGIC_USED)
			continue;
		ee_read (ep + IMG_PO_NUMBER, (byte*)(&cw), 2);
		if (cw != oid)
			continue;
		// One of ours: get the page number
		ee_read (ep + IMG_PO_PN, (byte*)(&cw), 2);
		if (cw >= np || imsd_pp [cw] != WNONE) {
			// Illegal
BadImg:
			ufree (DHook);
			DHook = NULL;
			return NO;
		}
		imsd_pp [cw] = pn;
		if (cw == 0) {
			// Image parameters
			ee_read (ep + IMG_PO_NCHUNKS, (byte*)(&imsd_n), 2);
			if (chunks_to_pages (imsd_n) != np)
				goto BadImg;
			ee_read (ep + IMG_PO_X, (byte*)(&imsd_x), 2);
			ee_read (ep + IMG_PO_Y, (byte*)(&imsd_y), 2);
			// Label bypass (needed temporarily)
			ee_read (ep + IMG_PO_LL, (byte*)(&imsd_cch), 2);
		}
	}

	for (cw = 0; cw < np; cw++)
		if (imsd_pp [cw] == WNONE)
			goto BadImg;

	return YES;
}

static word img_rts_snd (address packet) {
//
// Send the image parameters; for now, it is only the total number of chunks
//
	if (packet != NULL) {
		put2 (packet, imsd_n);
		put2 (packet, imsd_x);
		put2 (packet, imsd_y);
		put2 (packet, imsd_cch);
	}
	return 8;
}

static Boolean img_cls_snd (address packet) {
//
// Unpack the chunk list
//
	return croster_init (&imsd_ros, packet);
}

static word img_cnk_snd (address packet) {
//
// Handle the next outgoing chunk
//
	word pn, cn;

	if (packet == NULL) {
		if ((imsd_cch = croster_next (&imsd_ros)) == NONE)
			return 0;
		return CHUNKSIZE;
	}

	cn = imsd_cch;
	put2 (packet, cn);

	pn = 0;
	// Avoid division: this is max 4 turns
	while (cn >= IMG_CHK_PAGE) {
		pn++;
		cn -= IMG_CHK_PAGE;
	}

	ee_read (pntopa (imsd_pp [pn]) + cn * CHUNKSIZE, (byte*)(packet+3),
		CHUNKSIZE);

	return CHUNKSIZE;
}

// ============================================================================

static Boolean img_ini_rcp (address packet) {
//
// Initialize for image reception
// 
	lword ep;
	word cw, pn, nc, np;

	if (tcv_left (packet) < 8)
		// The expected length of parameters
		return NO;

	// The total number of chunks
	get2 (packet, nc);
	if (nc == 0)
		// Something wrong
		return NO;

	// The number of pages required (at least 1)
	np = chunks_to_pages (nc);

	// Allocate the operation structure
	if (alloc_dhook (sizeof (img_r_t) + sizeof (word) * np) == NULL)
		// No memory
		return NO;

	ctally_init (&imrd_cta, nc);

	// X
	get2 (packet, nc);
	imrd_bpp = (nc & 0x8000) ? LCDGF_8BPP : 0;
	imrd_x = (nc & 0x7fff);

	// Y
	get2 (packet, imrd_y);

	// Label bypass
	get2 (packet, imrd_fc);

	if ((RQFlag & IRT_OWN)) {
		// Replace own image: erase old version
		image_delete (IMG_NUM_OWN, NULL, 0);
	}

	// Need np free pages

	for (nc = pn = 0; pn < NEPages; pn++) {
		ep = pntopa (pn);
		ee_read (ep + IMG_PO_MAGIC, (byte*)(&cw), 2);
		if (cw != IMG_MAGIC_USED) {
			// A free page
			imrd_pp [nc++] = pn;
			if (nc == np)
				// All we need
				break;
	}

	while (nc < np) {
		// Delete old files
		if (INOld == INNew)
			// This cannot happen
Drop:
			ufree (DHook);
			DHook = NULL;
			return NO;
		}
		// Delete image number INOld
		nc = np - image_delete (INOld, imrd_pp + nc, np - nc);

		if (++INOld == 0x8000)
			INOld = 0;
	}

	if ((RQFlag & IRT_SHOW)) {
		lcdg_set (0, 0, 0, 0, 0);
		lcdg_clear (COLOR_BLACK);
		// This will also clip (for the last chunk)
		lcdg_set (0, 0, imrd_x, imrd_y, imrd_bpp);
	}

	imrd_rdy = YES;

	return OK;
}

static Boolean img_stp_rcp () {
//
// Terminate image reception
// 
	if (ctally_full (&imrd_cta))
		return YES;

	if ((RQFlag & IRT_SHOW))
		// The image is bad
		lcdg_set (0, 0, 0, 0, 0);
		lcdg_clear (COLOR_BLACK);

	return NO;
}

static Boolean img_cnk_rcp (address packet, word st) {
//
// Receive a chunk of image
//
	word cn, pc, cs, rc;

	if (imrd_rdy) {
		// Starting write
		if ((cn = tcv_left (packet)) != CHUNKSIZE+2) {
			// It can only be zero or max
			if (cn != 0) {
				// Just ignore
				return YES;
		}

		// The chunk number
		get2 (packet, cn);

		if (cn >= imrd_n)
			// Illegal, ignore
			return YES;

		// Tally it in
		ctally_add (&imrd_cta, cn);

		// Where the chunk starts
		imrd_pap = packet + 3;

		if ((RQFlag & IRT_SHOW) && cn >= imrd_fc) {
			// Displaying while we go
			pc = imrd_bpp ? PICPERCHK8 : PIXPERCHK12;
			// Starting pixel number of the chunk
			cs = (ch - imrd_fc) * pc;
			// Is there a way to avoid division? Have to calculate
			// starting row and column
			rc = cs / imrd_x;
			cs = cs % imrd_x;
			lcdg_render ((byte)cs, (byte)rc, (byte*)imrd_pap, pc);
		}

		// Prepare for possible blocking
		imrd_rdy = NO;

		// Calculate where the chunk goes in the eeprom
		pc = 0;
		while (cn >= IMG_CHK_PAGE) {
			pc++;
			cn -= IMG_CHK_PAGE;
		}

		imrd_cpp = pntopa (imrd_pp [pn]) + cn * CHUNKSIZE;
	}

	ee_write (st, imrd_cpp, imrd_pap, CHUNKSIZE);
	// If we succeed
	imrd_rdy = YES;

	return YES;
}

static word img_cls_rcp (address packet) {
//
// Return the length of the chunk list and (optionally) fill the packet
//
	return ctally_fill (&imrd_cta, packet) << 1;
}

// ============================================================================

const fun_pak_t imgl_pak = {
				imgl_lkp_snd,
				objl_rts_snd,
				objl_cls_snd,
				objl_cnk_snd,
				imgl_ini_rcp,
				imgl_stp_rcp,
				imgl_cnk_rcp,
				objl_cls_rcp
			   },

		img_pak  = {
				img_lkp_snd,
				img_rts_snd,
				img_cls_snd,
				img_cnk_snd,
				img_ini_rcp,
				img_stp_rcp,
				img_cnk_rcp,
				img_cls_rcp
		};

//=============================================================================
