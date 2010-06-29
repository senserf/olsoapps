#include "sysio.h"
#include "board.cc"

void buildPegNode (data_no_t*);
void buildTagNode (data_no_t*);

process Root : BoardRoot {

	void buildNode (const char *tp,	data_no_t *nddata) {

		if (strcmp (tp, "tag") == 0)
			buildTagNode (nddata);
		else if (strcmp (tp, "peg") == 0)
			buildPegNode (nddata);
		else
			excptn ("Root: illegal node type: %s", tp);
	};
};
