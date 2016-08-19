#include "sysio.h"
#include "blink.h"

static blinkrq_t	*RList [MAX_LEDS];

fsm blinker (word led) {

	state LOOP:

		if (RList [led] == NULL)
			finish;

	state RUN:

		if (RList [led]->count == 0) {
			// Done
			delay (RList [led]->space, DONE);
			release;
		}

		RList [led]->count--;

		leds (led, 1);
		delay (RList [led]->on, OFF);
		release;

	state OFF:

		leds (led, 0);
		delay (RList [led]->off, RUN);
		release;

	state DONE:

		blinkrq_t *p;

		p = RList [led];
		RList [led] = (blinkrq_t*)(p->next);
		ufree (p);
		sameas LOOP;
}

void blink (byte led, byte times, word on, word off, word space) {

	word rc;
	blinkrq_t *p, *q;

	for (rc = 0, p = NULL, q = RList [led]; q != NULL;
		p = q, q = (blinkrq_t*)(q->next), rc++);

	if (rc >= MAX_BLINK_REQUESTS)
		return;

	if ((q = (blinkrq_t*) umalloc (sizeof (blinkrq_t))) == NULL)
		return;

	q->count = times;
	q->on = on;
	q->off = off;
	q->space = space;
	q->next = NULL;

	if (p == NULL) {
		RList [led] = q;
		runfsm blinker (led);
	} else
		p->next = (address) q;
}
