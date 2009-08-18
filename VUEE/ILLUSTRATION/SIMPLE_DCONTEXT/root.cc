#include "sysio.h"
#include "board.h"
#include "board.cc"

void buildSndNode (data_no_t*);
void buildRcvNode (data_no_t*);

process Root : BoardRoot {

	void buildNode (const char *tp, data_no_t *nddata) {
		if (strcmp (tp, "snd") == 0)
			buildSndNode (nddata);
		else
			buildRcvNode (nddata);
	};
};
