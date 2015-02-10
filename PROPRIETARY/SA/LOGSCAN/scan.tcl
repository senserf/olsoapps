#!/bin/sh
#####################\
exec tclsh "$0" "$@"

proc msg { m } {

	puts stderr $m
}

proc abt { m } {

	msg $m
	exit 1
}

proc usage { } {

	global argv0

	abt "usage: $argv0 -d fmtproc -f date -t date -q quaproc\
		... file ... file "
}

proc get_file_contents { fn } {

	set fd [open $fn "r"]

	if [catch { read $fd } res] {
		catch { close $fd }
		error $res
	}

	catch { close $fd }

	return [string trim $res]
}

proc get_file_firstline { fn } {

	set fd [open $fn "r"]

	if [catch { gets $fd line } sta] {
		catch { close $fd }
		error $sta
	}

	catch { close $fd }

	if { $sta < 0 } {
		error "the file is empty"
	}

	return $line
}
		

proc date_header { ln } {

	if ![regexp "####.*Today is (\[^#\]+)####" $ln jnk da] {
		return ""
	}

	set da [string trim $da]

	if [catch { clock scan $da } res] {
		return ""
	}

	return $res
}

proc date_daystart { sec } {

	return [clock format $sec -format %D]
}

###############################################################################

proc fmt { bytes } {
#
# The default format procedure
#
	set output ""

	foreach b $bytes {
		append output " [format %02x $b]"
	}

	return [string trimleft $output]
}

proc qua { ts fpkt } {
#
# The default qualifier
#
	show $fpkt
}

###############################################################################

proc show { line } {

	global time_hdr

	puts "$time_hdr $line"
}

proc scan_files { fl tf tt } {

	if { $tf == "" } {
		set tf 0
	}

	if { $tt == "" } {
		set tt [expr { 0xffffffff }]
	}

	foreach f $fl {

		if [scan_file [lindex $f 1] $tf $tt] {
			return
		}
	}
}

proc scan_file { fn tf tt } {

	global time_hdr

	set fd [open $fn "r"]

	# just in case
	set day [date_daystart [clock seconds]]

	while { [gets $fd line] >= 0 } {

		# check for date change
		set ts [date_header $line]
		if { $ts != "" } {
			if { $ts > $tt } {
				return 1
			}
			set day [date_daystart $ts]
			continue
		}

		if ![regexp "^(..:..:..) <= (.*)" $line jnk time_hdr byt] {
			# ignore
			continue
		}

		if [catch { clock scan "$day $time_hdr" } ts] {
			abt "illegal time stamp: $time_hdr"
		}

		if { $ts < $tf } {
			continue
		}

		if { $ts > $tt } {
			return 1
		}

		# format the line
		set bytes ""

		set byt [split $byt " "]

		foreach b $byt {

			if { $b == "" } {
				continue
			}
			lappend bytes [expr 0x$b]
		}

		set line [fmt $bytes]

		if { $line == "" } {
			continue
		}

		qua $ts $line
	}

	return 0
}

###############################################################################

proc fundec { name func } {

	uplevel #0 [concat proc $name $func]
}

proc main { } {

	global argv

	set file_list ""
	set format_code ""
	set qualifier_code ""
	set time_from ""
	set time_to ""

	set lfo 1

	while { $argv != "" } {

		set opt [lindex $argv 0]
		set argv [lrange $argv 1 end]

		if { $lfo && [string index $opt 0] == "-" } {
			# an option
			set opt [string range $opt 1 end]
			if { $opt == "-" } {
				set lfo 0
				continue
			}
			set arg [lindex $argv 0]
			set argv [lrange $argv 1 end]
			if { $arg == "" } {
				usage
			}

			switch $opt {

			"d" {
				# formatting
				if { $format_code != "" } {
					usage
				}

				if [catch { get_file_contents $arg } \
				    format_code] {
					abt "-d $arg, $format_code"
				}


				if [catch { fundec fmt $format_code } err] {
					abt "-d, cannot evaluate, $err"
				}
				continue
			}

			"f" {
				# start date/time
				if { $time_from != "" } {
					usage
				}
				if [catch { clock scan $arg } time_from] {
					abt "-f, illegal date/time"
				}
				continue
			}

			"t" {
				# stop date/time
				if { $time_to != "" } {
					usage
				}
				if [catch { clock scan $arg } time_to] {
					abt "-t, illegal date/time"
				}
				continue
			}

			"q" {
				# formatting
				if { $qualifier_code != "" } {
					usage
				}

				if [catch { get_file_contents $arg } \
				    qualifier_code] {
					abt "-q $arg, $qualifier_code"
				}

				if [catch { fundec qua $qualifier_code } err] {
					abt "-q, cannot evaluate, $err"
				}
				continue
			}
			}

			usage
		}

		if [catch { get_file_firstline $opt } line] {
			abt "log file $opt, $line"
		}

		set da [date_header $line]

		if { $da == "" } {
			abt "log file $opt, bad file header (not a log file)"
		}

		lappend file_list [list $da $opt]
	}

	unset format_code qualifier_code

	#######################################################################

	# sort the files according to header time
	set file_list [lsort -integer -index 0 $file_list]

	if { $file_list == "" } {
		usage
	}

	scan_files $file_list $time_from $time_to
}

main
