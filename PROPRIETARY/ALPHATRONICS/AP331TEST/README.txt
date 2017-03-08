This is a test for AP331 whose purpose is to quickly check whether these three components of the node:

- radio
- loop detector
- accelerometer

work (or rather basically work).

You flash Image_ap.a43 into a WARSAW node. You switch the node on and leave it
in peace. Once per second the node will blink the green LED indicating that it
is sending a poll packet to the AP331.

The second image (Image_tag.a43) is to be flashed into the AP331 device under
test. Start the device. If the radio works, you will see on the WARSAW box the remaining two LEDs (red and yellow) blink for about three seconds. From then
on, every green blink on the WARSAW box will be quickly followed by a second
green blink indicating a response received from AP331 to every poll packet.

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

The WARSAW box writes some output to the UART (at 115200), but you don't
have to see it.

Note that without the WARSAW box you can still test the accelerometer and loop
detector on the AP331 (the red LED blinks 3/5 times), but you won't test the
radio.