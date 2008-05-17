#ifndef	__params_pg_h
#define	__params_pg_h

// ============================================================================
// Global constants ===========================================================
// ============================================================================

// Fixed component of every packet (excluding the plugin-defined frame, i.e.,
// Link Id and CRC). It covers the type and request number, which yield 2 bytes.
#define	PSIZE_FRAME		2
#define	PSIZE_BACK		(PSIZE_FRAME+0)	// No extra content
#define	PSIZE_BACK_D		(PSIZE_BACK +8) // + status
#define	PSIZE_NAK		(PSIZE_FRAME+2)	// Error code
#define	PSIZE_RTS		(PSIZE_FRAME+0)	// Varies
#define	PSIZE_GO		(PSIZE_FRAME+0)	// Varies
#define	PSIZE_WTR		(PSIZE_FRAME+6)	// RLink + otype + oid
#define	PSIZE_CHK		(PSIZE_FRAME+0)	// Varies
#define	PSIZE_OSS		(PSIZE_FRAME+0)	// Varies

#define	XMIT_POWER		7			// Default
#define	MAXPLEN			60			// Payload
#define	MAXTRIES		12			// Retries until ack
#define	MAXCHUNKLIST		56			// Bytes
#define	MAXCHUNKS		(MAXCHUNKLIST/2)	// Actual chunks
#define	MAXNEIGHBORS		16			// Neighbor table size
#define	CHUNKSIZE		54

// Object types
#define	OTYPE_IMAGE	0	// Image
#define	OTYPE_ILIST	1	// Image list
#define	OTYPE_NLIST	2	// Neighbor list

// OSS codes (one nibble)
#define	OSS_DONE	0
#define	OSS_FAILED	1
#define	OSS_BAD		2
#define	OSS_BUSY	3
#define	OSS_UNIMPL	4
#define	OSS_GETI	8
#define	OSS_CLEAN	9
#define	OSS_PING	10

// Endianness dependencies

#if LITTLE_ENDIAN
// First half of a long (meaning literally, i.e., in a byte string)
#define	fhol(a)		((word)((a)      ))
// Second half of a long
#define	shol(a)		((word)((a) >> 16))
// Two words to long
#define lofw(a,b)	((lword)(a) | ((lword)(b) << 16))
// Word of bytes (a is a byte pointer)
#define	wofb(a)		((word)(*(a  )) << 8) | *(a+1))
#else
#define	fhol(a)		((word)((a) >> 16))
#define	shol(a)		((word)((a)      ))
#define lofw(a,b)	((lword)(b) | ((lword)(a) << 16))
#define	wofb(a)		((word)(*(a+1)) << 8) | *(a  ))
#endif	/* LITTLE_ENDIAN */

// Randomized HELLO interval
#define	INTV_HELLO	(8192 - 0x7f + (rnd () & 0xff))

// Macros to extract/fill packet contents
#define	put1(p,b)	tcv_write (p, (const char*)(&(b)), 1)
#define	get1(p,b)	tcv_read (p, (char*)(&(b)), 1)
#define	put2(p,b)	tcv_write (p, (const char*)(&(b)), 2)
#define	get2(p,b)	tcv_read (p, (char*)(&(b)), 2)
#define	put4(p,b)	tcv_write (p, (const char*)(&(b)), 4)
#define	get4(p,b)	tcv_read (p, (char*)(&(b)), 4)

// Function package "plugin" to handle transmission/reception for a given
// object type
typedef struct {

	Boolean (fun_lkp_snd*)(word);		// Object lookup for xmission
	word    (fun_rts_snd*)(address);	// Send object params
	Boolean (fun_cls_snd*)(address);	// Unpack chunk list
	word	(fun_cnk_snd*)(address);	// Fill chunk and advance

	Boolean (fun_ini_rcp*)(address);	// Initialize reception
	Boolean (fun_stp_rcp*)();		// Stop reception (OK or bad)
	Boolean (fun_cnk_rcp*)(address, word);	// Receive a chunk
	word    (fun_cls_rcp*)(address);	// Remaining chunk list

} fun_pak_t;

typedef	struct {
//
// A received object list: to be passed to whoever asked it from us
//
	word		size;		// Total size
	byte		buf [0];	// The content
	// The first two bytes contain the channel Id of the original sender

} olist_t;

// Structures for handling "chunks" related to the transaction in progress
typedef struct {
//
// List of chunks to send (as requested by the recipient)
//
	word	list [MAXCHUNKS];
	word	p, n;

} croster_t;

typedef struct {
//
// The tally of received chunks
//
	word	n,		// Total number of chunks
		f;		// First missing
	byte	cstat [0];

} ctally_t;

// Transaction structures for sending/receiving generic lists of objects
typedef	struct {
//
// For sending a list
//
	croster_t	ros;
	word		size, cch, ccs;
	byte		cbf [0];

} old_s_t;

#define	OLSData		((old_s_t*)DHook)
#define olsd_size	(OLSData->size)
#define olsd_cbf	(OLSData->cbf)
#define	olsd_ros	(OLSData->ros)
#define	olsd_cch	(OLSData->cch)
#define	olsd_ccs	(OLSData->ccs)

typedef struct {
//
// For receiving a list
//
	ctally_t	cta;

} old_r_t;

#define	OLRData 	((old_r_t*)DHook)
#define	olrd_cta	(OLRData->cta)

extern const 	lword host_id;

#define	ESN host_id
#define MCN ((word)ESN)

// Common data hook for current-transaction-related dynamic structures
extern void *DHook;

// Current request type and flags
extern byte RQType, RQFlag;

// Generic transaction functions for list transcation plug-ins
word objl_rts_snd (address);
Boolean objl_cls_snd (address);
word objl_cnk_snd (address);
Boolean objl_ini_rcp (address, olist_t**);
Boolean objl_cnk_rcp (address, olist_t**);
word objl_cls_rcp (packet);
Boolean objl_stp_rcp (olist_t**);

#endif
