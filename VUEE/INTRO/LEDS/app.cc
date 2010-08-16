#include "sysio.h"
#include "ser.h"

typedef struct {
	byte led, state;
} led_status_t;

fsm blinker (led_status_t *lstat) {

	state CHECK_STATUS:

		if (lstat->state < 2) {
			leds (lstat->led, 0);
			if (lstat->state) {
				lstat->state = 2;
				delay (512, CHECK_STATUS);
			}
		} else {
			leds (lstat->led, 1);
			lstat->state = 1;
			delay (768, CHECK_STATUS);
		}

		when (lstat, CHECK_STATUS);
}

void blink (led_status_t *lstat, Boolean on) {
	lstat->state = on ? 2 : 0;
	trigger (lstat);
}

fsm root {

	led_status_t *my_led;

	state START:

		ser_out (START,
			"Commands:\r\n"
			"  on\r\n"
			"  off\r\n"
		);
			
		my_led = (led_status_t*)umalloc (sizeof (led_status_t));
		my_led -> led = 1;
		blink (my_led, NO);
		runfsm blinker (my_led);

	state INPUT:

		char cmd [4];

		ser_in (INPUT, cmd, 4);
		if (strncmp (cmd, "on", 2) == 0)
			blink (my_led, YES);
		else if (strncmp (cmd, "of", 2) == 0)
			blink (my_led, NO);
		else
			proceed BAD_INPUT;

	state OKMSG:

		ser_out (OKMSG, "OK\r\n");
		proceed INPUT;

	state BAD_INPUT:

		ser_out (BAD_INPUT, "Illegal command\r\n");
		proceed INPUT;
}
