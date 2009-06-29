#include "vuee_snd.h"

void NodeSnd::init () {

// ============================================================================

#include "app_snd_data_init.h"

// ============================================================================

	// Start application root
	appStart ();
}

void buildSndNode (data_no_t *nddata) {

	create NodeSnd (nddata);
}
