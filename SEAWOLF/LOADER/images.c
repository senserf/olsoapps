#include "sysio.h"
#include "params.h"
#include "images.h"

// Last received image list
static olist_t *RIList = NULL;

static	word	NEPages,	// Number of pages in EEPROM
		INOld,		// Oldest used file number
		INNew;		// First available file number

static	byte	Flags;		// Internal flags (one used at present)

#define	FLAG_GOTOWN	0x01	// Own image present (here's the one)

// ===========================================================================

typedef struct {
//
// Transaction structure for sending an image
//
	croster_t	ros;
// ----------------
	word		n;			// The number of chunks
	word		x;
	word		y;
	word		np;
	word		ppt [IMG_MAXPPI];	// Page numbers
	byte		lab [IMG_MAX_LABEL];
// ----------------
	word		cch;			// Current outgoing chunk number
	word		cmap [0];		// Chunk map

} img_s_t;

#define	IMSData		((img_s_t*)DHook)
#define	imsd_n		(IMSData->n)
#define	imsd_ppt	(IMSData->ppt)
#define	imsd_cmap	(IMSData->cmap)
#define	imsd_ros	(IMSData->ros)
#define	imsd_lab	(IMSData->lab)
#define	imsd_cch	(IMSData->cch)
#define	imsd_x		(IMSData->x)
#define	imsd_y		(IMSData->y)

// ===========================================================================

typedef struct {
//
// For receiving an image
//
	lword		cpp;	// Chunk pointer to write to eeprom
// ----------------
	word		n;	// This looks like the image signature
	word		x, y;	// geometry
	word		np;	// Number or pages
	word		ppt [IMG_MAXPPI];	// Page numbers
// ----------------
	word		cp;	// Current page index
	byte		bpp;	// bits per pixel (nonzero == 8)
	Boolean		rdy;	// eeprom ready flag

	ctally_t	cta;

} img_r_t;

#define	IMRData		((img_r_t*)DHook)
#define	imrd_cpp	(IMRData->cpp)
#define	imrd_cp		(IMRData->cp)
#define	imrd_n		(IMRData->n)
#define	imrd_x		(IMRData->x)
#define	imrd_y		(IMRData->y)
#define	imrd_np		(IMRData->np)
#define	imrd_bpp	(IMRData->bpp)
#define	imrd_rdy	(IMRData->rdy)
#define	imrd_ppt	(IMRData->ppt)
#define	imrd_cta	(IMRData->cta)

// ===========================================================================

typedef struct {
//
// For rendering an image from EEPROM
//
	word		x, y;			// geometry
	word		cx;			// current pixel to render
	word		fc;			// first chunk behind the label
	word		n;			// Number of chunks
	byte		bpp;			// bits per pixel (nonzero == 8)
	byte		chk [CHUNKSIZE];
	word		pp [IMG_MAXPPI];	// page numbers

} img_d_t;

#define	IMDData		((img_d_t*)DHook)
#define	imdd_x		(IMDData->x)
#define	imdd_y		(IMDData->y)
#define	imdd_cx		(IMDData->cx)
#define	imdd_fc		(IMDData->fc)
#define	imdd_n		(IMDData->n)
#define	imdd_bpp	(IMDData->bpp)
#define	imdd_chk	(IMDData->chk)
#define	imdd_pp		(IMDData->pp)

// ============================================================================

static const word errw = 0xffff;

// ============================================================================

void images_init () {
//
// Initializes the page map
//
	lword ep;
	word ch, pn, cp, min, max;

	NEPages = (word)(ee_size (NULL, NULL) >> IMG_PAGE_SHIFT);

	max = 0;
	min = MAX_WORD;
	Flags = 0;

	for (pn = 0; pn < NEPages; pn++) {
		ep = pntopa (pn);
		ee_read (ep + IMG_PO_MAGIC, (byte*)(&ch), 2);
		if (ch != 0x7f00)
			continue;
		ee_read (ep + IMG_PO_NUMBER, (byte*)(&cp), 2);
		if ((cp & 0x8000)) {
			Flags |= FLAG_GOTOWN;
			continue;
		}
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
		ee_read (ep + IMG_PO_MAGIC, (byte*)(&ch), 2);
		if (ch != 0x7f00)
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

static word image_delete (word num, word *pa, word max) {
//
// Erase the image with the specified number, return the number of pages
// that remain needed after the operation
//
	lword ep;
	word pn, cw;
	img_sig_t *sig;

	if ((sig = (img_sig_t*) umalloc (IMG_SIGSIZE)) == NULL)
		return max;

	for (pn = 0; pn < NEPages; pn++) {
		ep = pntopa (pn);
		ee_read (ep + IMG_PO_MAGIC, (byte*)(&cw), 2);
		if (cw == 0x7f00) {
			ee_read (ep + IMG_PO_NUMBER, (byte*)(&cw), 2);
			if (cw != num)
				continue;
			ee_read (ep + IMG_PO_SIG, (byte*)sig, IMG_SIGSIZE);
			if (sig->ppt [0] != pn)
				// Basic consistency check
				continue;

			goto Erase;
		}
	}

	// Not found
	ufree (sig);
	return max;
Erase:
	while (sig->np) {
		--(sig->np);
		pn = sig->ppt [sig->np];
		if (pa && max) {
			*pa++ = pn;
			max--;
		}
		ep = pntopa (pn);
		ee_write (WNONE, ep + IMG_PO_MAGIC, (byte*)(&errw), 2);
	}

	ufree (sig);
	ee_sync (WNONE);
	return max;
}

static word linum (word wc) {
//
// Transform image number from internal to logical
//
	if (wc == IMG_NUM_OWN)
		return 0;

	if (wc < INOld)
		return wc + (0x8001 - INOld);

	return wc - INOld + 1;
}

static word iinum (word wc) {
//
// Transform image number from logical to internal
//
	word ix;

	if (wc == 0)
		return IMG_NUM_OWN;

	ix = INOld;
	while (1) {
		if (ix == INNew)
			return WNONE;
		if (--wc == 0)
			return ix;
		if (++ix == 0x8000)
			ix = 0;
	}
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
	img_sig_t *sig;

	if ((sig = (img_sig_t*) umalloc (IMG_SIGSIZE)) == NULL)
		return;

	if ((what & (CLEAN_IMG_OWN | CLEAN_IMG))) {
		// Erase images in eeprom
		for (pn = 0; pn < NEPages; pn++) {
			ep = pntopa (pn);
			ee_read (ep + IMG_PO_MAGIC, (byte*)(&cw), 2);
			if (cw != 0xf700) 
				// Continue
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

			ee_read (ep + IMG_PO_SIG, (byte*)sig, IMG_SIGSIZE);
			if (sig->ppt [0] != pn || sig->np > IMG_MAXPPI)
				// Something wrong
				continue;

			// Erase pages
			while (sig->np) {
				--(sig->np);
				ep = pntopa (sig->ppt [sig->np]);
				ee_write (WNONE, ep + IMG_PO_MAGIC,
					(byte*)(&errw), 2);
			}
			ee_sync (WNONE);
		}
		if ((what & CLEAN_IMG))
			INOld = INNew = 0;
		if ((what & CLEAN_IMG_OWN))
			Flags &= ~FLAG_GOTOWN;
	}

	if ((what & CLEAN_IMG_LIST)) {
		// Erase the list received from somebody else
		if (RIList != NULL) {
			ufree (RIList);
			RIList = NULL;
		}
	}
	ufree (sig);
}

Boolean images_show (word in) {
//
// Show the image with the given logical number; note that logical 0 stands for
// 'own' image
//
	lword ep;
	word pn, cw, cx, cy;
	img_sig_t *sig;
	byte bpp;

	if ((in = iinum (in)) == WNONE)
		return NO;

	// Locate the first page of the image
	for (pn = 0; pn < NEPages; pn++) {
		ep = pntopa (pn);
		ee_read (ep + IMG_PO_MAGIC, (byte*)(&cw), 2);
		if (cw != 0x7f00)
			continue;
		ee_read (ep + IMG_PO_NUMBER, (byte*)(&cw), 2);
		if (cw == in)
			goto Render;
	}

	// Not found
	return NO;
Render:
	if ((sig = (img_sig_t*) umalloc (sizeof (img_sig_t))) == NULL)
		return NO;

	ee_read (ep + IMG_PO_SIG, (byte*)sig, IMG_SIGSIZE);
	if (sig->ppt [0] != pn || sig->np > IMG_MAXPPI) {
		// Something wrong
		ufree (sig);
		return NO;
	}

	bpp = (sig->x & 0x8000) != 0;
	sig->x &= ~0x8000;

	lcdg_set (0, 0, 0, 0, 0);
	lcdg_clear (COLOR_BLACK);
	lcdg_set (0, 0, (byte)(sig->x), (byte)(sig->y), bpp);

	bpp = bpp ? PIXPERCHK8 : PIXPERCHK12;

	// Initial offset on first page
	cw = 0;
	ep = pntopa (sig->ppt [pn = 0]);
	while (sig->nc) {
		if ((cw += CHUNKSIZE+2) > IMG_MAX_COFF) {
			pn++;
			ep = pntopa (sig->ppt [pn]);
			cw = 0;
		}
		// Render this chunk
		ee_read (ep + cw, (byte*)(&(sig->cn)), CHUNKSIZE + 2);

		// Starting pixel of the chunk
		cx = sig->cn * bpp;
		cy = cx / sig->x;
		cx = cx % sig->x;
		lcdg_render ((byte)cx, (byte)cy, sig->chunk, bpp);
		sig->nc--;
	}

	// Done
	ufree (sig);
	return YES;
}

// ============================================================================
// Functions for sending/receiving the image list =============================
// ============================================================================

static word do_imglist (byte *buf) {
//
// Calculate the list length and (optionally) fill the list if pointer != NULL
//
	lword ep;
	word size, wc, pn, wls;
	byte bc;

	size = 2;
	// The minimum size is 2. The first word contains the link Id of the
	// owner.

	if (buf)
		*((word*)buf) = MCN;

	for (pn = 0; pn < NEPages; pn++) {
		ep = pntopa (pn);
		ee_read (ep + IMG_PO_MAGIC, (byte*)(&wc), 2);
		if (wc != 0x7f00)
			continue;

		// Label size, not extremely efficient
		for (wls = 0; wls < IMG_MAX_LABEL; wls++) {
			ee_read (ep + IMG_PO_LABEL, &bc, 1);
			if (bc == 0)
				break;
		}

		if (wls < IMG_MAX_LABEL)
			// Sentinel included
			wls++;
		if ((wls & 1))
			wls++;

		if (buf) {
			// Total description size; the size word doesn't
			// count
			*((word*)(buf+size)) = wls + 8;
			size += 2;

			// Image number
			ee_read (ep + IMG_PO_NUMBER, (byte*)(&wc), 2);
			// Transform into external
			wc = linum (wc);
			*((word*)(buf+size)) = wc;
			size += 2;

			// File size
			ee_read (ep + IMG_PO_NCHUNKS, buf + size, 2);
			size += 2;

			// Geometry
			ee_read (ep + IMG_PO_X, buf + size, 4);
			size += 4;

			// The label
			ee_read (ep + IMG_PO_LABEL, buf + size, wls);
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

		do_imglist (olsd_cbf);
	
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

static Boolean imgl_cnk_rcp (word st, address packet) {
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

static Boolean img_lkp_snd (word oid) {
//
// The Id is the 'logical' image number, e.g., as conveyed in the image list
//
	lword ep;
	word cw, np, pn, ws;

	// Logical to internal
	if ((oid = iinum (oid)) == WNONE)
		return NO;

	for (np = pn = 0; pn < NEPages; pn++) {
		ep = pntopa (pn);
		ee_read (ep + IMG_PO_MAGIC, (byte*)(&cw), 2);
		if (cw != 0x7f00)
			continue;
		ee_read (ep + IMG_PO_NUMBER, (byte*)(&cw), 2);
		if (cw != oid)
			break;
	}

	if (pn == NEPages)
		return NO;

	ee_read (ep + IMG_PO_NCHUNKS, (byte*)(&cw), 2);
	if (cw == 0 || cw > IMG_MAXCHUNKS)
		return NO;

	if (alloc_dhook (sizeof (img_s_t) + (cw >> 1)) == NULL)
		return NO;

	// Build the chunk map
	ee_read (ep + IMG_PO_SIG, (byte*)(&imsd_n), IMG_SIGSIZE+IMG_MAX_LABEL);

	for (cw = 0; cw < imsd_n; cw++)
		imsd_cmap [cw] = WNONE;

	ep = pntopa (imsd_ppt [pn = 0]);
	for (ws = cw = 0; cw < imsd_n; cw++) {
		if (ws > IMG_MAX_COFF) {
			pn++;
			ep = pntopa (imsd_ppt [pn]);
			ws = 0;
		}

		ee_read (ep + ws, (byte*)(&np), 2);

		if (imsd_cmap [cw] != WNONE) {
			// Bad image
			ufree (DHook);
			DHook = NULL;
			return NO;
		}

		imsd_cmap [cw] = (pn << IMG_PAGE_SHIFT) | ws;
	}

	return YES;
}

static word img_rts_snd (address packet) {
//
// Send the image parameters
//
	word ll;

	for (ll = 0; ll < IMG_MAX_LABEL; ll++)
		if (imsd_lab [ll] == 0)
			break;

	// No sentinel
	if ((ll & 1))
		ll++;

	if (packet != NULL) {
		put2 (packet, imsd_n);
		put2 (packet, imsd_x);
		put2 (packet, imsd_y);
		memcpy ((byte*)(packet + 10), imsd_lab, ll);
	}

	return 6 + ll;
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

	// Note: this assumes that the variant with packet != NULL is
	// called immediately after one with packet == NULL; thus, imsd_cch
	// is set in the previous turn

	put2 (packet, imsd_cch);

	// Chunk coordinates
	cn = imsd_ppt [imsd_cch];
	pn = cn >> IMG_PAGE_SHIFT;
	cn &= IMG_PAGE_MASK;

	ee_read (pntopa (imsd_ppt [pn]) + cn + 2, (byte*)(packet+3),
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

	// The number of pages required (at least 1)
	if (nc == 0 || (np = chunks_to_pages (nc)) > IMG_MAXPPI)
		// Illegal image size
		return NO;

	// Allocate the operation structure
	if (alloc_dhook (sizeof (img_r_t) + ctally_ctsize (nc)) == NULL)
		// No memory
		return NO;

	imrd_np = np;
	imrd_n = nc;
	ctally_init (&imrd_cta, nc);

	// X
	get2 (packet, imrd_x);
	imrd_bpp = (imrd_x & 0x8000) ? LCDGF_8BPP : 0;

	// Y
	get2 (packet, imrd_y);

	if ((RQFlag & IRT_OWN)) {
		// Replace own image: erase old version
		image_delete (IMG_NUM_OWN, NULL, 0);
		Flags &= ~FLAG_GOTOWN;
	}

	// Need np free pages

	for (nc = pn = 0; pn < NEPages; pn++) {
		ep = pntopa (pn);
		ee_read (ep + IMG_PO_MAGIC, (byte*)(&cw), 2);
		if (cw == errw) {
			// A free page
			imrd_ppt [nc++] = pn;
			if (nc == np)
				// All we need
				break;
		}
	}

	while (nc < np) {
		// Delete old files
		if (INOld == INNew) {
			// This cannot happen
Drop:
			ufree (DHook);
			DHook = NULL;
			return NO;
		}
		// Delete image number INOld
		nc = np - image_delete (INOld, imrd_ppt + nc, np - nc);

		if (++INOld == 0x8000)
			INOld = 0;
	}

	// Intialize the out pointer
	imrd_cp = 0;
	imrd_cpp = pntopa (imrd_ppt [imrd_cp]);

	// Start the first page
	cw = 0x7f00;
	ee_write (WNONE, imrd_cpp + IMG_PO_MAGIC, (byte*)(&cw), 2);

	// The number
	cw = (RQFlag & IRT_OWN) ? IMG_NUM_OWN : INNew;
	ee_write (WNONE, imrd_cpp + IMG_PO_NUMBER, (byte*)(&cw), 2);

	// The sigature
	ee_write (WNONE, imrd_cpp + IMG_PO_SIG, (byte*)(&imrd_n), IMG_SIGSIZE);

	// The label
	cw = tcv_left (packet);
	nc = 0;
	if (cw) {
		ee_write (WNONE, imrd_cpp + IMG_PO_LABEL, (byte*)(packet + 5),
			cw);
	}
	ee_write (WNONE, imrd_cpp + IMG_PO_LABEL + cw, (byte*)(&nc), 1);

	imrd_x &= 0x7fff;
	imrd_cpp += CHUNKSIZE + 2;

	if ((RQFlag & IRT_SHOW)) {
		lcdg_set (0, 0, 0, 0, 0);
		lcdg_clear (COLOR_BLACK);
		// This will also clip (for the last chunk)
		lcdg_set (0, 0, imrd_x, imrd_y, imrd_bpp);
	}

	imrd_rdy = YES;

	return YES;
}

static Boolean img_stp_rcp () {
//
// Terminate image reception
// 
	lword ep, ch;
	word pn, fn, wc;

	if (ctally_full (&imrd_cta)) {
		// Reception OK
		if ((RQFlag & IRT_OWN)) {
			Flags |= FLAG_GOTOWN;
		} else {
			if (++INNew == 0x8000)
				INNew = 0;
		}
		ee_sync (WNONE);
		return YES;
	}

	// Release the pages back
	for (pn = 0; pn < imrd_np; pn++) {
		ep = pntopa (imrd_ppt [pn]);
		ee_write (WNONE, ep + IMG_PO_MAGIC, (byte*)(&errw), 2);
	}
	ee_sync (WNONE);
	return NO;
}

static Boolean img_cnk_rcp (word st, address packet) {
//
// Receive a chunk of image
//
	word cn, pc, cs, rc;

	if (imrd_rdy) {
		// Starting write
		if ((cn = tcv_left (packet)) != CHUNKSIZE+2) {
			// It can only be zero or max
			if (cn != 0)
				// Just ignore
				return YES;
		}

		// The chunk number
		get2 (packet, cn);

		if (cn >= imrd_cta.n)
			// Illegal, ignore
			return YES;

		// Tally it in
		if (ctally_add (&imrd_cta, cn))
			// Void, present already
			return YES;

		if ((RQFlag & IRT_SHOW)) {
			// Displaying while we go
			pc = imrd_bpp ? PIXPERCHK8 : PIXPERCHK12;
			// Starting pixel number of the chunk
			cs = cn * pc;
			// Is there a way to avoid division? Have to calculate
			// starting row and column
			rc = cs / imrd_x;
			cs = cs % imrd_x;
			lcdg_render ((byte)cs, (byte)rc, (byte*)(packet + 3),
				pc);
		}

		// Prepare for possible blocking
		imrd_rdy = NO;
	}

	ee_write (st, imrd_cpp, (byte*)(packet+2), CHUNKSIZE+2);
	// If we succeed
	imrd_rdy = YES;

	// Advance the pointer
	if ((word)(imrd_cpp & IMG_PAGE_MASK) >= IMG_MAX_COFF) {
		if (++imrd_cp < imrd_np)
			imrd_cpp = pntopa (imrd_ppt [imrd_cp]);
	} else {
		imrd_cpp += CHUNKSIZE + 2;
	}
	return YES;
}

static word img_cls_rcp (address packet) {
//
// Return the length of the chunk list and (optionally) fill the packet
//
	return ctally_fill (&imrd_cta, packet) << 1;
}

const fun_pak_t imgl_pak = {
				imgl_lkp_snd,
				objl_rts_snd,
				objl_cls_snd,
				objl_cnk_snd,
				imgl_ini_rcp,
				imgl_stp_rcp,
				imgl_cnk_rcp,
				objl_cls_rcp
			   };

const fun_pak_t img_pak  = {
				img_lkp_snd,
				img_rts_snd,
				img_cls_snd,
				img_cnk_snd,
				img_ini_rcp,
				img_stp_rcp,
				img_cnk_rcp,
				img_cls_rcp
		};
