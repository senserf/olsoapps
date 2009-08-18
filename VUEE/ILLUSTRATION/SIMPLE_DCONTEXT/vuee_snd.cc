#include "vuee_snd.h"

void NodeSnd::init () {

// ============================================================================
#define	__dcx_ini__

#include "app_snd_data.h"

#undef	__dcx_ini__

// ============================================================================

	// Start application root
	appStart ();
}

void buildSndNode (data_no_t *nddata) {

	create NodeSnd (nddata);
}
