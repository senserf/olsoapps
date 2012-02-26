#include "app_ert.h"
#include "vuee_ert.h"

void NodeErt::init () {

// ============================================================================

#define	__dcx_ini__
#include "app_ert_data.h"
#undef	__dcx_ini__

// ============================================================================

	// Start application root
	appStart ();
}

void buildErtNode (data_no_t *nddata) {

	create NodeErt (nddata);
}
