Quick start for tests:
======================

Execute mkmk WARSAW_LCD_TEST followed by make.

Execute genihex Image.a43 BACADEAD BACA0002 2 . This will create two Image
files: Image_BACA0002.a43 and Image_BACA0003.a43. Load the two files into
two boards. Connect the board flashed with BACA0002 to the USB UART and
determine the COM number.

------------------------------------------------------------------------------

I am assuming you are in Apps/SEAWOLF/LOADER. The two nodes are numbered
2 (BACA0002) and 3 (BACA0003). The numbers are the least 16 bits of the
ESNs.

------------------------------------------------------------------------------

Execute ./oss.tcl c, where n is the UART COM number. After a few seconds you
should see this:

received ESN: baca0002

which means that the board is talking to you over the UART.

------------------------------------------------------------------------------

Make sure EEPROM is cleared on the board. Do this:

erase 0 0

This will take 20 or so seconds, and you will get this at the end:

ACK OK (rqn 1 lnk 0002)

Sometimes, the message may not show up, so just wait for 30 seconds or so.

------------------------------------------------------------------------------

Now try to load an image into the board. Go like this:

get 1 0 1 0 PICTURES/gerry.nok

The expected response:

image file PICTURES/gerry.nok read, length 25379 bytes
geometry: <130,130>, 12 bpp
label: A to jest Zdzichu
470 chunks total

... here you will have to wait until you see :

transaction complete

Occasionally, you may get no response, so if nothing happens for 30 seconds or
so, try this:

retr 1 0

which should return the list of images stored in the device, something like:

image list 0, owner link id: 2, 0002
 image 1, size 470 chunks, 130 x 130 (12 bpp)
 "A to jest Zdzichu"
end of list

The image received number 1 (this is the third parameter of get). If you are
curious, those parameters mean:

1 - get the thing from ME, i.e., the oss script. It has the node Id 1. The
actual nodes are 2 and 3. You are connected to node 2 over the UART.

0 - the thing to get is an image (object type 0)

1 - the number to be assigned to the image

0 - just get it, do not display

------------------------------------------------------------------------------

Now, check if the display works. Do:

show 1

meaning display image number 1. You should see in a few seconds the picture on
the LCD.

------------------------------------------------------------------------------

Load another image:

get 1 0 2 0 PICTURES/pawel.nok

This one gets number 2. The response should be similar to what you had before.

When you now do:

retr 1 0

you will get:

image list 0, owner link id: 2, 0002
 image 1, size 470 chunks, 130 x 130 (12 bpp)
 "A to jest Zdzichu"
 image 2, size 470 chunks, 130 x 130 (12 bpp)
 "Pawel a.k.a. Sambo Blyskawica"
end of list

i.e., both images are there.

------------------------------------------------------------------------------

Loading and displaying at the same time. Do:

get 1 0 3 16 PICTURES/wlodek.nok

Response as before. You will see how the image fills the screen while being
loaded.

retr 1 0 

shows:

image list 0, owner link id: 2, 0002
 image 1, size 470 chunks, 130 x 130 (12 bpp)
 "A to jest Zdzichu"
 image 2, size 470 chunks, 130 x 130 (12 bpp)
 "Pawel a.k.a. Sambo Blyskawica"
 image 3, size 470 chunks, 130 x 130 (12 bpp)
 "To jest CEO"
end of list

------------------------------------------------------------------------------

Test the buttons:

They should scan through the images displaying them in turns forward and
backward. Wait for a while after pressing a button. Don't release it too
soon. There is no debouncing at present, so it may not be perfect.

------------------------------------------------------------------------------

Test the joystick:

Whenever you press it, you should see something like this:

DEBUG: JOY W <0,0>
DEBUG: JOY N <0,0>
DEBUG: JOY E <0,0>
DEBUG: JOY PUSH <0,0>

... and so on

------------------------------------------------------------------------------

Inter board communication.

------------------------------------------------------------------------------

Connect the UART to the second node. The first node remains on. Wait for this:

ESN changed: baca0002 -> baca0003

This means thet the oss script has recognized it is talking to a different
node. As before, do:

erase 0 0

------------------------------------------------------------------------------

Check if the node sees the neighbor:

retr 2 0

You should get the neighbor list:

Neighbor list 0, owner link id: 3, 0003
  -> baca0002
end of list

------------------------------------------------------------------------------

Tell the node to get an image from node 2:

get 2 0 1 0

This means: get from node 2 image number 1 without displaying it. This may
produce no response, so wait for 20 seconds or so and try:

ping

If you see a status message, like this:

STATUS:
 1 images, 60 free pages
 1 neighbors

The operation has been completed.

------------------------------------------------------------------------------

Try to display the image:

show 1

... and so on ...

Get another image from node 2:

get 2 0 2 0

...

------------------------------------------------------------------------------

Check the buttons and joystick. You need at least two images in the node to
see the effect of buttons.
