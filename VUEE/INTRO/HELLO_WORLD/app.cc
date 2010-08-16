#include "sysio.h"
#include "ser.h"

fsm root {
	state START:
		ser_out (START, "Hello world!!\r\n");
		finish;
}
