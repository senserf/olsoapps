#include "vuee.h"

void NodeTest::init () {

// ============================================================================
#define	__dcx_ini__

#include "app_data.h"

#undef	__dcx_ini__

// ============================================================================

	// Start application root
	appStart ();
}

void buildTestNode (data_no_t *nddata) {

	create NodeTest (nddata);
}
