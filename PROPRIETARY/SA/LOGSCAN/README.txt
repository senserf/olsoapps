This is extremely ad-hoc, to be enhanced, improved as needed

./scan.tcl -d format_function -ee expressions -f from -t to logfile > output

format_function: file with a function to format packets shown on output (see
                 file sample_format_function in this directory)

expressions:     a file with regular expressions applicable to the formatted
                 output, one expression per line, the expressions are AND-ed
                 (see file sample_expressions)

from, to:        starting and ending time, e.g., "Jan 30, 2015 23:55:00"

logfile:         the log file (there can be more than one)

For example, try this on Jasmien's log:

./scan.tcl -d sample_format_function -ee sample_expressions -f "Jan 30, 2015 23:00:00" -t "Jan 31, 2015 00:15:00" jasmien.log > output.txt


