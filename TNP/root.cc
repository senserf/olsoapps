#include "common.h"
#include "board.cc"

process Root : BoardRoot {

	void buildNode (
		const char *tp,		// Type
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
		Long	ifps,
		Long	UMODE,		// UART mode
		Long	UBS,		// UART buffer size
		Long	USP,		// UART rate
		char	*UIDV,		// Input device for UART
		char	*UODV		// Output device for UART
	) {

		if (strcmp (tp, "tag") == 0)

		    buildTagNode (
			mem,			// Memory
			X,
			Y,
			XP, RP,
			BCmin, BCmax,
			LBTDel, LBTThs,
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

		else if (strcmp (tp, "peg") == 0)

		    buildPegNode (
			mem,			// Memory
			X,
			Y,
			XP, RP,
			BCmin, BCmax,
			LBTDel, LBTThs,
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

		else
			excptn ("Root: illegal node type: %s", tp);
	};
};
