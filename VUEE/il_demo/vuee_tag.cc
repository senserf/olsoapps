#include "app_tag.h"
#include "msg_tag.h"
#include "vuee_tag.h"

void NodeTag::init () {

// ============================================================================

#define	__dcx_ini__
#include "app_tag_data.h"
#undef	__dcx_ini__

// ============================================================================

	// Start application root
	appStart ();
}

void buildTagNode (data_no_t *nddata) {

	create NodeTag (nddata);
}

