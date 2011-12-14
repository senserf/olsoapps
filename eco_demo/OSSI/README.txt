For a VUEE-based educational exercise do these steps:

Move up and compile the VUEE model:

	cd ..
	cp options.tagall options.sys
	vuee

Then start the model:

	side large_eco.xml

Move back here and run the OSSI script:

	cd OSSI
	./ossi.tcl -s sensors_large_eco.xml -d -v

Look into sensors_large_eco.xml for comments. Confront that file with the
dataset for the VUEE model (large_eco.xml). Note that the database description
has been commented out, so the OSSI script will not try to store any values in
a database. It will create these files (in this directory):

	log
	data_...... (the dots represent the current date) containing the list
		    of collected sensor values)
	values      the last readings of all sensor values

There is a way to randomly alter sensor values in the model. For that, run
this script (in this directory):

	./val_gen.tcl -s sensors_large_eco.xml

which will be periodically connecting to the model and updating the sensor
values.

Note: as they are, the scripts ossi.tcl and val_gen.tcl can only be called
from this directory (or, to be more exact, from an Apps subdirectory in the
PICOS tree). To create their location-independent versions (detached from the
PICOS tree - see NOTE.txt) do this:

	../../../../Scripts/isource ossi.tcl ossi_indep.tcl
	../../../../Scripts/isource val_gen.tcl val_gen_indep.tcl

