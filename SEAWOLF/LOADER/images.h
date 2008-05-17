#ifndef __images_pg_h
#define	__images_pg_h

#include "sysio.h"
#include "params.h"

#define	IMG_PAGE_SIZE	8192
#define	IMG_PAGE_SHIFT	13
#define	IMG_MAGIC_USED	0xB0C3		// USED
#define	IMG_MAGIC_FREE	0xB1C4

#define	pntopa(a)	(((lword)(a)) << IMG_PAGE_SHIFT)
#define	IMG_MAGIC_FIRST	lofw (IMG_MAGIC_USED, 0)

#define	IMG_MAXSIZE	25350
#define	IMG_CHK_PAGE	151		// Chunks per page
#define	IMG_FOFFSET	38		// File offset in page
#define	IMG_MAX		128
#define	IMG_MAXPPI	4		// Max pages per image

#define	IMG_NUM_OWN	WNONE		// Own image number

#define	PIXPERCHK8	CHUNKSIZE
#define	PIXPERCHK12	((CHUNKSIZE*8)/12)

// Image file format ==========================================================

// Bytes 38-8191 are chunks: this is the part actually sent and received.
// 8192-38=8154 and 8154/54=151, so this is the (integer) number of chunks per
// page.
//
//	Bytes 0-3:	MAGIC, second word is the page number within the image
//	Bytes 4-5:	The total number of chunks, i.e., this many chunks will
//			be sent
//	Bytes 6-9:	GEOMETRY, two words (X and Y), X is or'red with 0x8000
//			if the image is 8bpp
//	Bytes 10-11:	Number of chunks in the label, i.e., this many chunks
//			from the front are to be ignored
//	Bytes 12:	Image number
//	Bytes 38-8191:	The chunks
//
//	The label itself begins with the length word
//
// ============================================================================

#define	IMG_PO_MAGIC	0
#define	IMG_PO_PN	2		// W page number within image
#define	IMG_PO_NCHUNKS	4		// W size: the number of chunks
#define	IMG_PO_X	6		// W X-geometry
#define	IMG_PO_Y	8		// W Y-geometry
#define	IMG_PO_LL	10		// W label length (chunks)
#define	IMG_PO_NUMBER	12		// W number (WNONE == OWN)
#define	IMG_PO_CHUNKS	38

// ============================================================================

#define	chunks_to_pages(n)	(((n) + IMG_CHK_PAGE - 1) / IMG_CHK_PAGE)


// Flags for an image download request
#define	IRT_OWN		0x01	// Download OWN image
#define	IRT_SHOW	0x10	// Display while downloading

// Cleanup flags
#define	CLEAN_IMG_OWN	0x04	// Own
#define	CLEAN_IMG_LIST	0x02	// List
#define	CLEAN_IMG	0x01	// Acquired

void images_init ();
void images_status (address);
void images_clean (byte);

extern const fun_pak_t img_pak, imgl_pak;

#endif
