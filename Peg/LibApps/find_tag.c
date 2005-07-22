#include "sysio.h"
#include "app.h"
#include "lib_apps.h"

int find_tag (lword tag) {
	int i = 0;
	lword mask = 0xffffffff;
	
	if (!(tag & 0xff000000))  // rss irrelevant
		mask &= 0x00ffffff;

	if (!(tag & 0x00ff0000)) // power level 
		mask &= 0xff00ffff;

	while (i < tag_lim) {
		if ((tagArray[i].id & mask) == tag)
			return i;
		i++;
	}
	return -1;
}

