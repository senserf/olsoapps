#ifndef __images_pg_h
#define	__images_pg_h

#include "sysio.h"
#include "params.h"

#define	IMG_PAGE_SIZE	8192
#define	IMG_PAGE_SHIFT	13
#define	IMG_PAGE_MASK	((1<<IMG_PAGE_SHIFT)-1)

#define	pntopa(a)	(((lword)(a)) << IMG_PAGE_SHIFT)

#define	IMG_MAXCHUNKS	720
#define	IMG_CHK_PAGE	146		// Chunks per page
#define	IMG_MAX		128
#define	IMG_MAXPPI	5		// Max pages per image (hard limit)
					// Max chunk offset
#define	IMG_MAX_COFF	((IMG_CHK_PAGE-1)*(CHUNKSIZE+2))

#define	IMG_NUM_OWN	0x8001		// Own image number


#define	PIXPERCHK12	((CHUNKSIZE*8)/12)

// First chunk layout =========================================================
//
// Bytes 0-1:		0x7f00
// Bytes 2-3:		Image number
//
// Bytes 4-5:           The total number of chunks in file
// Bytes 6-7:		The number of pages (including this one)
// Bytes 8-11:		GEOMETRY, two words (X and Y)
// Bytes 12-21:		Page numbers of the remaining pages
// Bytes 22-55: 	The label
//
// ============================================================================

typedef	struct {

	word	nc;	// Number of chunks
	word	x;	// X
	word	y;	// Y
	word	np;	// Number of pages
	word	ppt [IMG_MAXPPI];

	// --- optional ---
	word	cn;
	byte	chunk [CHUNKSIZE];

} img_sig_t;

#define	IMG_SIGSIZE	((4 + IMG_MAXPPI) * 2)

#define	IMG_PO_MAGIC	0
#define	IMG_PO_NUMBER	2		// W number (WNONE == OWN)
#define	IMG_PO_SIG	4		// Signature
#define	IMG_PO_NCHUNKS	4
#define	IMG_PO_X	6
#define	IMG_PO_LABEL	22

#define	IMG_MAX_LABEL	(CHUNKSIZE+2-IMG_PO_LABEL)

// ============================================================================

// The actual number of chunks is n+1
#define	chunks_to_pages(n)	(((n) + IMG_CHK_PAGE) / IMG_CHK_PAGE)

// Flags for an image download request
#define	IRT_OWN		0x01	// Download OWN image
#define	IRT_SHOW	0x10	// Display while downloading

// Cleanup flags
#define	CLEAN_IMG_OWN	0x04	// Own
#define	CLEAN_IMG_LIST	0x02	// List
#define	CLEAN_IMG	0x01	// Acquired

void images_init ();
word image_find (word);
void images_status (address);
void images_clean (address, word);
void images_cleanil ();
Boolean image_show (word);
Boolean images_haveilist ();
void images_show_next ();
void images_show_previous ();

extern const fun_pak_t img_pak, imgl_pak;

#endif
