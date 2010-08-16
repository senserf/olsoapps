#include "sysio.h"
#include "ser.h"
#include "serf.h"

fsm output (const char*), root;

fsm output (const char *msg) {
	state WRITE_MSG:
		ser_out (WRITE_MSG, "I am ready!!\r\n");
	state WAIT_FOR_SIGNAL:
		when (getcpid (), WAKE_UP);
		release;
	state WAKE_UP:
		ser_outf (WAKE_UP, "Time %lu, %s\r\n", 
			seconds (), msg);
		proceed WAIT_FOR_SIGNAL;
}

sint out1, out2;

fsm root {
	state START:
		out1 = runfsm output ("message 1\r\n");
		out2 = runfsm output ("message 2\r\n");
	state DELAY:
		delay (1024, SIGNAL1);
		release;
	state SIGNAL1:
		trigger (out1);
		delay (1024, SIGNAL2);
		release;
	state SIGNAL2:
		trigger (out2);
		proceed DELAY;
}
