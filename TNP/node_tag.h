#ifndef __node_tag_h__
#define	__node_tag_h__

#ifdef	__node_peg_h__
#error "node_tag.h and node_peg.h cannot be included together"
#endif

#include "board.h"
#include "chan_shadow.h"
#include "plug_tarp.h"

station	NodeTag : TNode {

	/*
	 * Session (application) specific data
	 */
#include "attribs_tag.h"

	/*
	 * Application starter
	 */
	void appStart ();

	void setup (
		word mem,
		double	X,		// Coordinates
		double  Y,
		double	XP,		// Power
		double	RP,
		Long	BCmin,		// Backoff
		Long	BCmax,
		Long	LBTDel, 	// LBT delay (ms) and threshold (dBm)
		double	LBTThs,
		RATE	rate,
		Long	eesz,		// EEPROM size
		Long	ifsz,		// IFLASH size
		Long	ifps,		// IFLASH page size
		Long	PRE,		// Preamble
		Long	UMODE,		// UART mode
		Long	UBS,		// UART buffer size
		Long	USP,		// UART rate
		char	*UIDV,		// Input device for UART
		char	*UODV		// Output device for UART
	);

	void _da (reset) ();
	void init ();
};

#endif
