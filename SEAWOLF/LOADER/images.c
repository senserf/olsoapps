#include "sysio.h"
#include "params.h"
#include "images.h"

// Last received image list
static olist_t *RIList = NULL;

static	word	NEPages;		// Number of pages in EEPROM
static	word	CImage;			// For scaning through images

// ===========================================================================

typedef struct {
//
// Transaction structure for sending an image
//
	croster_t	ros;
	word		nu;			// Number
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
#define	imsd_nu		(IMSData->nu)
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
	byte		chk [CHUNKSIZE];
	word		pp [IMG_MAXPPI];	// page numbers

} img_d_t;

#define	IMDData		((img_d_t*)DHook)
#define	imdd_x		(IMDData->x)
#define	imdd_y		(IMDData->y)
#define	imdd_cx		(IMDData->cx)
#define	imdd_fc		(IMDData->fc)
#define	imdd_n		(IMDData->n)
#define	imdd_chk	(IMDData->chk)
#define	imdd_pp		(IMDData->pp)

// ============================================================================

static const word errw = 0xffff;

// ============================================================================

void images_init () {
//
// Initializes the page map
//
	NEPages = (word)(ee_size (NULL, NULL) >> IMG_PAGE_SHIFT);
	CImage = NEPages;
}

word image_find (word num) {
//
// Find the specified image, return the number of pages
//
	lword ep;
	word pn, cw;
	img_sig_t *sig;

	if ((sig = (img_sig_t*) umalloc (IMG_SIGSIZE)) == NULL)
		return WNONE;

	for (pn = 1; pn < NEPages; pn++) {
		ep = pntopa (pn);
		ee_read (ep + IMG_PO_MAGIC, (byte*)(&cw), 2);
		if (cw == 0x7f00) {
			ee_read (ep + IMG_PO_NUMBER, (byte*)(&cw), 2);
			if (cw == num)
				goto Found;
		}
	}

	// Not found
	ufree (sig);
	return 0;

Found:
	ee_read (ep + IMG_PO_SIG, (byte*)sig, IMG_SIGSIZE);
	cw = sig->np;
	ufree (sig);
	
	if (cw == 0 || cw > IMG_MAXPPI) {
		SFlags |= FLG_EEPR;
		return 0;
	}

	return cw;
}

Boolean images_haveilist () {
//
	return RIList != NULL;
}

static word image_delete (word num) {
//
// Erase the image with the specified number, return the number of pages
// reclaimed
//
	lword ep;
	word pn, cw, ci;
	img_sig_t *sig;

	if ((sig = (img_sig_t*) umalloc (IMG_SIGSIZE)) == NULL)
		return WNONE;

	for (pn = 1; pn < NEPages; pn++) {
		ep = pntopa (pn);
		ee_read (ep + IMG_PO_MAGIC, (byte*)(&cw), 2);
		if (cw == 0x7f00) {
			ee_read (ep + IMG_PO_NUMBER, (byte*)(&cw), 2);
			if (cw == num)
				goto Erase;
		}
	}

	// Not found
	ufree (sig);
	return 0;
Erase:
	ee_read (ep + IMG_PO_SIG, (byte*)sig, IMG_SIGSIZE);
	if (sig->ppt [0] != pn) {
		// Basic consistency check
		SFlags |= FLG_EEPR;
		ufree (sig);
		return WNONE;
	}

	for (cw = 0; cw < sig->np; cw++) {
		pn = sig->ppt [cw];
		ep = pntopa (pn);
		ee_write (WNONE, ep + IMG_PO_MAGIC, (byte*)(&errw), 2);
	}

	ufree (sig);
	ee_sync (WNONE);
	return cw;
}

void images_status (address packet) {
//
// Return image-related status (into an OSS_DONE packet) - two words
//
	lword ep;
	word pn, cw, fr;
	byte ni;

	fr = 0;
	ni = 0;
	for (pn = 1; pn < NEPages; pn++) {
		ep = pntopa (pn);
		ee_read (ep + IMG_PO_MAGIC, (byte*)(&cw), 2);
		if (cw == 0x7f00)
			ni++;
		else if (cw == errw)
			fr++;
	}

	put1 (packet, SFlags);
	put1 (packet, ni);
	put2 (packet, fr);

	if (RIList != NULL)
		cw = wofb (RIList->buf);
	else
		cw = 0;

	put2 (packet, cw);
}

void images_clean (address lst, word n) {
//
// Cleans up images
//
	lword ep;
	word cw;

	// diagg ("CLEAN", n, lst [0]);
	if (n == 0) {
		// Delete all images
		for (n = 1; n < NEPages; n++) {
			ep = pntopa (n);
			ee_read (ep + IMG_PO_MAGIC, (byte*)(&cw), 2);
			if (cw != errw) {
				ee_write (WNONE, ep + IMG_PO_MAGIC,
					(byte*)(&errw), 2);
			}
		}
		ee_sync (WNONE);
		return;
	}

	do {
		n--;
		image_delete (lst [n]);
	} while (n);
}

void images_cleanil () {
//
// Clean image list
//
	if (RIList != NULL) {
		ufree (RIList);
		RIList = NULL;
	}
}

static Boolean image_display (lword ep) {
//
// Display image pointed to by the page address
//
	word pn, cw, cx, cy;
	img_sig_t *sig;

	if ((sig = (img_sig_t*) umalloc (sizeof (img_sig_t))) == NULL)
		return NO;

	ee_read (ep + IMG_PO_SIG, (byte*)sig, IMG_SIGSIZE);
	if (sig->np > IMG_MAXPPI) {
		// Something wrong
		ufree (sig);
		return NO;
	}

	lcdg_off ();
	lcdg_set (0, 0, LCDG_MAXX, LCDG_MAXY);
	lcdg_setc (COLOR_BLACK, COLOR_WHITE);
	lcdg_clear ();
	lcdg_set (0, 0, (byte)(sig->x - 1), (byte)(sig->y - 1));

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
		cx = sig->cn * PIXPERCHK12;
		cy = cx / sig->x;
		cx = cx % sig->x;
		lcdg_render ((byte)cx, (byte)cy, sig->chunk, PIXPERCHK12);
		sig->nc--;
	}
	lcdg_on (0);
	// Done
	ufree (sig);
	return YES;
}

void images_show_next () {
//
// Display next image
//
	lword ep;
	word cw, cv;

	for (cw = 1; cw < NEPages; cw++) {
		CImage++;
		if (CImage >= NEPages)
			CImage = 0;
		ep = pntopa (CImage);
		ee_read (ep + IMG_PO_MAGIC, (byte*)(&cv), 2);
		if (cv == 0x7f00)
			goto Render;
	}

	// No image found
	return;
Render:
	image_display (ep);
}

void images_show_previous () {
//
// Display previous image
//
	lword ep;
	word cw, cv;

	for (cw = 1; cw < NEPages; cw++) {
		if (CImage == 0)
			CImage = NEPages - 1;
		else
			CImage--;
		ep = pntopa (CImage);
		ee_read (ep + IMG_PO_MAGIC, (byte*)(&cv), 2);
		if (cv == 0x7f00)
			goto Render;
	}

	// No image found
	return;
Render:
	image_display (ep);
}

Boolean image_show (word in) {
//
// Show the image with the given number
//
	lword ep;
	word pn, cw;

	// Locate the first page of the image
	for (pn = 1; pn < NEPages; pn++) {
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
	return image_display (ep);
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
		*((word*)buf) = MyLink;

	for (pn = 1; pn < NEPages; pn++) {
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
			// For now, try to copy the list to a separate area
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
	lword ep;
	word cw, np, pn, ws;

	for (pn = 1; pn < NEPages; pn++) {
		ep = pntopa (pn);
		ee_read (ep + IMG_PO_MAGIC, (byte*)(&cw), 2);
		if (cw != 0x7f00)
			continue;
		ee_read (ep + IMG_PO_NUMBER, (byte*)(&cw), 2);
		if (cw == oid)
			break;
	}

	if (pn == NEPages)
		return NO;

	ee_read (ep + IMG_PO_NCHUNKS, (byte*)(&cw), 2);
	if (cw == 0 || cw > IMG_MAXCHUNKS) {
		SFlags |= FLG_EEPR;
		return NO;
	}

	if (alloc_dhook (sizeof (img_s_t) + (cw << 1)) == NULL)
		return NO;

	imsd_nu = oid;

	// Build the chunk map
	ee_read (ep + IMG_PO_SIG, (byte*)(&imsd_n), IMG_SIGSIZE+IMG_MAX_LABEL);

	for (cw = 0; cw < imsd_n; cw++)
		imsd_cmap [cw] = WNONE;

	ep = pntopa (imsd_ppt [pn = 0]);
	ws = CHUNKSIZE + 2;
	for (cw = 0; cw < imsd_n; cw++) {
		if (ws > IMG_MAX_COFF) {
			pn++;
			ep = pntopa (imsd_ppt [pn]);
			ws = 0;
		}

		ee_read (ep + ws, (byte*)(&np), 2);

		if (np >= imsd_n || imsd_cmap [np] != WNONE) {
			// Bad image
			ufree (DHook);
			DHook = NULL;
			SFlags |= FLG_EEPR;
			return NO;
		}

		imsd_cmap [np] = (pn << IMG_PAGE_SHIFT) | ws;
		ws += CHUNKSIZE + 2;
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
		put2 (packet, imsd_nu);
		put2 (packet, imsd_x);
		put2 (packet, imsd_y);
		memcpy ((byte*)(packet + 6), imsd_lab, ll);
	}

	return 8 + ll;
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
		if ((imsd_cch = croster_next (&imsd_ros)) == NONE) {
			// diagg ("EOR", 0, 0);
			return 0;
		}
		return CHUNKSIZE;
	}

	// Note: this assumes that the variant with packet != NULL is
	// called immediately after one with packet == NULL; thus, imsd_cch
	// is set in the previous turn

	put2 (packet, imsd_cch);

	// Chunk coordinates
	cn = imsd_cmap [imsd_cch];

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

	// Image number
	get2 (packet, np);
	if (image_find (np)) {
Drop:
		ufree (DHook);
		DHook = NULL;
		return NO;
	}

	// X
	get2 (packet, imrd_x);

	// Y
	get2 (packet, imrd_y);

	// Need free pages
	for (nc = 0, pn = 1; pn < NEPages; pn++) {
		ep = pntopa (pn);
		ee_read (ep + IMG_PO_MAGIC, (byte*)(&cw), 2);
		if (cw == errw) {
			// A free page
			imrd_ppt [nc++] = pn;
			if (nc == imrd_np)
				// All we need
				goto GotThem;
		}
	}

	// Not enough memory
	goto Drop;

GotThem:
	// Intialize the out pointer
	imrd_cp = 0;
	imrd_cpp = pntopa (imrd_ppt [imrd_cp]);

	// Start the first page
	cw = 0x7f00;
	ee_write (WNONE, imrd_cpp + IMG_PO_MAGIC, (byte*)(&cw), 2);

	// The number
	ee_write (WNONE, imrd_cpp + IMG_PO_NUMBER, (byte*)(&np), 2);

	// The sigature
	ee_write (WNONE, imrd_cpp + IMG_PO_SIG, (byte*)(&imrd_n), IMG_SIGSIZE);

	// The label
	cw = tcv_left (packet);
	nc = 0;
	if (cw) {
		ee_write (WNONE, imrd_cpp + IMG_PO_LABEL, (byte*)(packet + 6),
			cw);
	}
	ee_write (WNONE, imrd_cpp + IMG_PO_LABEL + cw, (byte*)(&nc), 1);

	imrd_cpp += CHUNKSIZE + 2;

	if ((RQFlag & IRT_SHOW)) {
		lcdg_on (0);
		lcdg_set (0, 0, LCDG_MAXX, LCDG_MAXY);
		lcdg_setc (COLOR_BLACK, COLOR_WHITE);
		lcdg_clear ();
		// This will also clip (for the last chunk)
		lcdg_set (0, 0, imrd_x - 1, imrd_y - 1);
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
		// diagg ("RCP OK", 0, 0);
		ee_sync (WNONE);
		return YES;
	}
	// diagg ("RCP FAILED", ctally_absent (&imrd_cta), 0);

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
	word cn, cs, rc;

	if (imrd_rdy) {
		// Starting write
		if ((cn = tcv_left (packet)) != CHUNKSIZE+2) {
			// It can only be zero or max
			if (cn != 0)
				// Just ignore
				return YES;
			return NO;
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
			cs = cn * PIXPERCHK12;
			// Is there a way to avoid division? Have to calculate
			// starting row and column
			rc = cs / imrd_x;
			cs = cs % imrd_x;
			lcdg_render ((byte)cs, (byte)rc, (byte*)(packet + 3),
				PIXPERCHK12);
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
