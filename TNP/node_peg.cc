#include "diag.h"
#include "app_peg.h"
#include "msg_peg.h"

#include "node_peg.h"

void NodePeg::setup (
		word mem,
		double	X,		// Coordinates
		double  Y,
		double	XP,		// Power
		double	RP,
		Long	BCmin,		// Backoff
		Long	BCmax,
		Long	LBTDel, 	// LBT delay (ms) and threshold (dBm)
		double	LBTThs,
		RATE	rate,		// Transmission rate
		Long	PRE,		// Preamble
		Long	eesz,
		Long	ifsz,
		Long	ifps,
		Long	UMODE,		// UART mode
		Long	UBS,		// UART buffer size
		Long	USP,		// UART rate
		char	*UIDV,		// Input device for UART
		char	*UODV		// Output device for UART
    ) {

	PicOSNode::setup (mem, X, Y, XP, RP, BCmin, BCmax, LBTDel, LBTThs,
		rate, PRE, eesz, ifsz, ifps, UMODE, UBS, USP, UIDV, UODV);

	TNode::setup ();

	init ();
}

void NodePeg::init () {

#include "attribs_init_peg.h"

	// Start application root
	appStart ();
}

__PUBLF (NodePeg, void, reset) () {

	TNode::reset ();

	init ();
}

void buildPegNode (
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
		Long	PRE,		// Preamble
		Long	eesz,
		Long	ifsz,
		Long 	ifps,
		Long	UMODE,		// UART mode
		Long	UBS,		// UART buffer size
		Long	USP,		// UART rate
		char	*UIDV,		// Input device for UART
		char	*UODV		// Output device for UART
	) {
/*
 * The purpose of this is to isolate the two applications, such that their
 * specific "type" files don't have to be used together in any module.
 */
		create NodePeg (
			mem,
			X,
			Y,
			XP,
			RP,
			BCmin,
			BCmax,
			LBTDel,
			LBTThs,
			rate,
			PRE,
			eesz,
			ifsz,
			ifps,
			UMODE,
			UBS,
			USP,
			UIDV,
			UODV
		);
}
