#include "vuee_rcv.h"

void NodeRcv::init () {

// ============================================================================

#define	__dcx_ini__
#include "app_rcv_data.h"
#undef	__dcx_ini__

// ============================================================================

	// Start application root
	appStart ();
}

void buildRcvNode (data_no_t *nddata) {

	create NodeRcv (nddata);
}
