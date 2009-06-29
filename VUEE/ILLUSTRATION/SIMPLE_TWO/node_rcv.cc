#include "vuee_rcv.h"

void NodeRcv::init () {

// ============================================================================

#include "app_rcv_data_init.h"

// ============================================================================

	// Start application root
	appStart ();
}

void buildRcvNode (data_no_t *nddata) {

	create NodeRcv (nddata);
}
