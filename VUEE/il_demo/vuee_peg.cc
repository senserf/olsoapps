#include "app_peg.h"
#include "msg_peg.h"
#include "vuee_peg.h"

void NodePeg::init () {

// ============================================================================

#define	__dcx_ini__
#include "app_peg_data.h"
#undef	__dcx_ini__

// ============================================================================

	// Start application root
	appStart ();
}

void buildPegNode (data_no_t *nddata) {

	create NodePeg (nddata);
}

