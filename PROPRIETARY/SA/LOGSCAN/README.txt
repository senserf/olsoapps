This is extremely ad-hoc, to be enhanced, improved as needed

./scan.tcl -d format_function -ee expressions -q qual_function \
	-f from -t to logfile > output

format_function: file with a function to format packets shown on output (see
                 file sample_format_function in this directory); if the
                 function returns "", the line is ignored

qual_function:   file with a function qualifying packets to show (in addition
                 to expressions (see below); see file sample_qualifier in this
                 directory

expressions:     a file with regular expressions applicable to the formatted
                 output, one expression per line, the expressions are AND-ed
                 (see file sample_expressions); if the first character of an
                 expression is !, then the matching is reversed, i.e., the
                 packet must NOT much the expression to qualify

                 a single expression can be specified with -e (single e)

from, to:        starting and ending time, e.g., "Jan 30, 2015 23:55:00"

logfile:         the log file (there can be more than one)

For example, try this on Jasmien's log:

./scan.tcl -d sample_format_function -ee sample_expressions -f "Jan 30, 2015 23:00:00" -t "Jan 31, 2015 00:15:00" jasmien.log > output.txt
