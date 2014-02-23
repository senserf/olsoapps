/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2014                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "diag.h"

char * get_mem (word len, Boolean r) {

        char * buf = (char *)umalloc (len);

        if (buf == NULL) {
                app_diag_S ("No mem");
                if (r)
                        reset();
        }
        return buf;
}

