Here is a quick start for the new features in VUEE R090609A:
============================================================

Install VUEE as before. Make sure that SIDE has been set up as required.
Do not forget to execute ./mklinks in VUEE/PICOS.

Move to this directory and compile the praxis (i.e., execute vuee).

Execute ./build.sh to create the EEPROM image.

Run the praxis this way: side twonodes.xml. When you look inside the data
file, you will see that the EEPROM of both nodes is directly mapped to file
eeprom.nok created by build.sh. It is OK to share the image, as long as the
nodes do not modify it. Needless to say, you can create and use separate
copies for different nodes.

You will also see in the data file that both nodes declare an LCDG module. This
is the only way to declare that module at present.

Execute udaemon. Make sure it is run through Tk (wish) version at least 8.5.
To make it more convenient, create a stand-alone executable of udaemon
(run mkexe.sh in udaemon's directory).

Select Node 0 and connect to its LCDG. After a while you should see the
initial menu.

You may also connect to the node from the OSS script in this directory. To do
that, open the (preferably hex) UART in udaemon, set it into U-U mode, and
run oss.tcl CNCA0 115200 .

To emulate buttons, connect to the PINS module. You will see 7 pins
representing 7 buttons (in this order) BUTTON 0, BUTTON 1, JOYSTICK: N E S W,
JOYSTICK PUSH. They are declared (see twonodes.xml) as active low, so their
default values are high. You can simply change the pine value (in the bottom)
row to low and and then back to high. Alternatively, you can right click on
the button which will change it to low for 300 msec and then back to high
(emulating a button press).

Please be aware that the LCDG is occassionally slow to respond, so be patient.
If you see that a button does not un-press immediately (the pin turns back
red), it means that udaemon is busy receiving data, so just hold on.
