This is a test for AP331 whose purpose is to quickly check whether these three components of the node:

- radio
- loop detector
- accelerometer

work (or rather basically work).

Note: I am not 100% sure about the LED colors on your Warsaw boxes, but you
will figure them out.

You flash Image_peg_test_warsaw.a43 into a WARSAW node. You switch the node on
and leave it in peace. Once per second the node will blink the green LED
indicating that it is sending a poll packet to the AP331.

The second image (Image_tag_test_ap331.a43) is to be flashed into the AP331
device under test. Start the device. If the radio works, you will see on the
WARSAW box the remaining two LEDs (red and yellow) blink for about three
seconds. From then on, every green blink on the WARSAW box will be quickly
followed by a second green blink indicating a response received from AP331 to
every poll packet.

Note: test only one AP331 device at a time! Make sure it is the only device
running the test image that is turned on in the neighborhood.

When you move the node (rapidly enough) you will trigger an accelerometer
event. The red LED on AP331 will quickly blink 3 times and you will also
see the red LED on the WARSAW box blink for 2-3 seconds.

When you get the device close to the loop transmitter (preferably by moving
the transmitter towards the AP331, as opposed to the other way around, to
avoid simultaneous accellerometer events, so you see the loop event more
clearly) the red LED on AP331 will quickly blink 5 times and the yellow LED
on the WARSAW box will blink for 2-3 seconds. It is possible to see both
events at the same time: on the WARSAW box both LEDs (red and yellow) will
blink simultaneously. On the AP331 you will see series of 3 blinks
interleaved with 5 blinks (but not as legibly).

The WARSAW box writes some output to the UART (at 115200). You don't absolutely
have to see it, but here is what it means:

A line arriving in respone to the Peg's ping looks like this:

R: AAAA BBBB CCCC DDDD EEEE FFFF

where AAAA, BBBB, etc. denote 4-digit hex numbers.

AAAA    is the number of seconds since the last motion event was reported by
        the Tag; every motion event resets the count to zero

BBBB    is the number of seconds since the last loop event was reported by the
        the Tag; every loop event resets the count to zero

CCCC    is the number of seconds since the last button press was detected by
        the Tag; every button press resets the count to zero

DDDD    is the raw reading of the voltage sensor (the new battery sensor); to
        interpret it, convert the hex value to decimal and apply this formula:

		V = (r / 4095) * 2.5 * (260 / 100) or, equivalently:
		V = r * 0.001587

EEEE    is the raw reading of the internal supply voltage sensor:

		V = r * 0.001221

FFFF    is the raw reading of the charge voltage sensor:

		V = r * 0.001575

Note that without the WARSAW box you can still test the accelerometer and loop
detector on the AP331 (the red LED blinks 3/5 times), but you won't test the
radio.
