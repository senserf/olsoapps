#include "sysio.h"
#include "board.h"
#include "board.cc"

void buildTestNode (data_no_t*);

process Root : BoardRoot {

	void buildNode (const char *tp, data_no_t *nddata) {
		buildTestNode (nddata);
	};
};
