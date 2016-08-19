typedef struct {

	word	count, on, off, space;
	address next;

} blinkrq_t;

void blink (byte, byte, word, word, word);

#define	MAX_BLINK_REQUESTS	4
#define	MAX_LEDS		4

//+++ "blink.cc"
