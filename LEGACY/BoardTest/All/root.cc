#include "node.h"

#include "board.cc"

process Root : BoardRoot {

	void buildNode (const char *tp, data_no_t *nddata) {
		// Type ignored; we only handle a single type in this case
		create Node (nddata);
	};
};
