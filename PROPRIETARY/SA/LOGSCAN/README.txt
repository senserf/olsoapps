This is extremely ad-hoc, to be enhanced, improved as needed

./scan.tcl -d format_function -q qual_function -f from -t to logfile > output

format_function: file with a function to format packets shown on output (see
                 file sample_format_function in this directory); if the
                 function returns "", the line will be ignored, so the function
                 can also act as a pre-qualifier

qual_function:   file with a function qualifying packets to show (see file
                 sample_qual_function); the function gets a formatted line
                 together with its time stamp; it calls show to send any
                 piece of text to output (and can generally do whatever it
                 pleases); only the lines explicitly written with show appear
                 in the output file

from, to:        starting and ending time, e.g., "Jan 30, 2015 23:55:00"

logfile:         the log file (there can be more than one)

For example, try this on Jasmien's log:

./scan.tcl -d sample_format_function -q sample_qual_function \
 -f "Jan 30, 2015 23:00:00" -t "Jan 31, 2015 00:15:00" jasmien.log > output.txt
