#include "sysio.h"
#include "ser.h"

fsm root {
	state START:
		ser_out (START, "Hello world!!\r\n");
		delay (1024, STOP);
		release;
	state STOP:
		halt ();
}
