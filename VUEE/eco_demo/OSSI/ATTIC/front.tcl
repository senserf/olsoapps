#!/bin/sh
###########################\
exec tclsh "$0" "$@"

#
# UART front for master aggregator
#

###############################################################################

proc u_start { udev speed dfun } {
#
# Initialize UART
#
	global ST

	if { $udev == "" } {
		global argv
		# take from arguments
		set udev [lindex $argv 0]
	}

	if { $udev == "" } {
		set devlist ""
		for { set udev 0 } { $udev < 32 } { incr udev } {
			lappend devlist "COM${udev}:"
			lappend devlist "/dev/ttyUSB$udev"
			lappend devlist "/dev/tty$udev"
		}
	} else {
		if [catch { expr $udev } cn] {
			# must be a complete device
			set devlist \
			    [list $udev ${udev}: "/dev/$udev" "/dev/tty$udev"]
		} else {
			# com number or tty number
			set devlist [list "COM${udev}:" "/dev/ttyUSB$udev" \
				"/dev/tty$udev"]
		}
	}

	set fail 1
	foreach udev $devlist {
		if ![catch { open $udev "r+" } ST(SFD)] {
			set fail 0
			break
		}

	}

	if $fail {
		error "u_start: cannot open UART, device(s) $devlist"
	}

	fconfigure $ST(SFD) -mode "$speed,n,8,1" -handshake none \
		-buffering line -blocking 0

	fileevent $ST(SFD) readable u_rdline
}

proc u_rdline { } {

	global ST

	if [catch { gets $ST(SFD) line } nc] {
		puts "device has been re-opened"
		return
	}

	set line [string trim $line]
	if { $line == "" } {
		return
	}

	if $ST(ECH) {
		# echo everything received
		puts "-> $line"
	}

	if { $ST(EXP) != "" } {
		# expecting something
		if [regexp -nocase $ST(EXP) $line] {
			set ST(LLI) $line
		}
	}

	if $ST(SSE) {
		# show sensor values
		if { [regexp "^1002 .* Col (\[0-9\]+).*time: (\[^ \]+) *S0:" \
		    $line j col tst] && ![catch { expr $col } col] } {
			set sv ""
			set sn 0
			while 1 {
				if ![regexp " S${sn}: (\[0-9\]+)" $line j v] {
					break
				}
				if [catch { expr $v } v] {
					break
				}
				lappend sv $v
				incr sn

			}
			show_sensors $col $tst $sv
		}
	}

	if { $ST(DUM) != "" } {
		if { [regexp \
		  "^1007 Col (\[0-9\]+).*A: (\[0-9\]+).*time: (\[^ \]+).*S0:" \
		    $line j col slo tst] && ![catch { expr $col } col] &&
		      ![catch { expr $slo } slo] } {
			set sv ""
			set sn 0
			while 1 {
				if ![regexp " S${sn}: (\[0-9\]+)" $line j v] {
					break
				}
				if [catch { expr $v } v] {
					break
				}
				lappend sv $v
				incr sn

			}
			dump_sensors $col $slo $tst $sv
			set ST(LLI) "."
		} elseif [regexp "^1008 Did" $line] {
			# end signal
			set ST(LLI) "="
		}
	}
}

proc tstamp { ts } {
#
# Re-format the time stamp
#
	if ![regexp "^(\[0-9\])+\\.(\[0-9\]+):(\[0-9\]+):(\[0-9\]+)" \
	    $ts j d h m s] {
		return "00.00:00:00"
	}

	return [format "%02d.%02d:%02d:%02d" $d $h $m $s]
}

proc sconvert { sn v } {
#
# Convert sensor values
#
	switch $sn {

		0 {
			return [expr $v * 1.47]
		}

		1 {
			return [expr -39.62 + 0.01 * $v]
		}

		2 {
			set v [expr -4.0 + 0.0405 * $v - 0.0000028 * $v * $v]
			if { $v < 0.0 } {
				set v 0.0
			} elseif { $v > 100.0 } {
				set v 100.0
			}
			return $v
		}

		3 {
			set v [expr $v * 0.09246 - 40.1]
			if { $v < 0.0 } {
				set v 0.0
			} elseif { $v > 100.0 } {
				set v 100.0
			}
			return $v
		}
	}

	return $v
}

proc show_sensors { col ts vl } {

	set sn 0
	puts "Collector $col at [tstamp $ts]"
	foreach v $vl {
		set cv [sconvert $sn $v]
		puts "  S${sn} : [format {%4d -> %7.2f} $v $cv]"
		incr sn
	}
	puts ""
}

proc dump_sensors { col slo ts vl } {

	global ST

	set ln "TM: [tstamp $ts], COL: [format %4d $col] --"

	set sn 0
	foreach v $vl {
		set cv [sconvert $sn $v]
		append ln "  S${sn} = <[format {%4d, %7.2f} $v $cv]>"
		incr sn
	}

	catch { puts $ST(DUM) $ln }
	if !$ST(ECH) {
		puts -nonewline "\r$slo"
		flush stdout
	}
}

proc itmout { } {

	global ST

	set ST(LLI) ""
}

proc issue { cmd pat ret del } {
#
# Issues a command to the aggregator
#
	global ST

	set ST(EXP) $pat
	set ST(LLI) "+"

	while { $ret > 0 } {
		if $ST(ECH) {
			puts "<- $cmd"
		}
		catch { puts $ST(SFD) $cmd }
		set tm [after [expr $del * 1000] itmout]
		vwait ST(LLI)
		catch { after cancel $tm }
		if { $ST(LLI) != "" } {
			set ST(EXP) ""
			return 0
		}
		incr ret -1
	}

	set ST(EXP) ""
	puts "command timeout!!!"
	return 1
}

proc doit_start { arg } {
#
# Start collection to aggregator
#
	global ST PM

	if ![regexp "\[0-9\]+" $arg intv] {
		puts "sampling interval required"
		return
	}

	if { [catch { expr $intv } intv] || $intv < 30 || $intv > 32767 } {
		puts "illegal interval, should be between 30 and 32767 seconds"
		return
	}

	# check if the continue flag is set
	set cont [regexp -nocase "cont" $arg]

	if $cont {
		puts "continuing previous collection every $intv seconds ..."
	} else {
		puts "starting new collection every $intv seconds ..."
	}

	if $cont {
		puts "resetting the master ..."
		if [issue "q" "^1001.*Find collector:" 4 30] {
			return
		}
	} else {
		puts "resetting the master and erasing EEPROM (be patient) ..."
		if [issue "Q" "^1001.*Find collector:" 4 60] {
			return
		}
	}

	# sync
	puts "setting paramaters ..."
	if [issue "Y $intv" "^0003 Synced to $intv" 6 10] {
		return
	}

	if [issue "m" "^0003 (Became|Sent) Master" 6 10] {
		return
	}

	# set the time stamp to current time
	set ts [clock format [clock seconds] -format "%H %M %S"]

	if [issue "T $ts" "^1009 At time" 6 10] {
		return
	}

	# obtain the number of stored entries
	if [issue "a -1 -1 -1 3" "^1005 Stats.*tored entries \[0-9\]" 6 10] {
		return
	}

	set eu 0
	regexp "tored entries (\[0-9\]+)" $ST(LLI) junk eu
	if { $eu >= $PM(ESI) } {
		puts "warning: no EEPROM left, samples will not be stored"
	} else {
		puts "[expr $PM(ESI) - $eu] EEPROM entries available"
	}

	puts "sampling started"
}

proc doit_stop { arg } {
#
#
	puts "resetting node ..."
	issue "q" "^1001.*Find collector:" 4 30
	issue "a -1 -1 -1 0" "^1005" 4 5
	puts "done"
}

proc doit_echo { arg } {

	global ST

	if [regexp -nocase "off" $arg] {
		set ST(ECH) 0
	} else {
		set ST(ECH) 1
	}
	puts "OK"
}

proc doit_show { arg } {

	global ST

	if [regexp -nocase "off" $arg] {
		set ST(SSE) 0
	} else {
		set ST(SSE) 1
	}
	puts "OK"
}

proc doit_extract { arg } {

	global ST PM

	set arg [string trim $arg]

	if ![regexp "^\[^ \]+" $arg fn] {
		puts "file name required"
		return
	}

	set arg [string range $arg [string length $fn] end]

	if { ![regexp "\[0-9\]+" $arg col] || [catch { expr $col } col] } {
		set col ""
	}

	# open the file
	if [catch { open $fn "w" } fd] {
		puts "cannot open file '$fn': $fd"
		return
	}

	set ST(DUM) $fd
	set cmd "D 0 $PM(ESI)"
	if { $col != "" } {
		append cmd " $col"
	}

	puts "dumping samples to file ..."

	if [issue $cmd "^1007" 4 10] {
		catch { close $fd }
		set ST(DUM) ""
		return
	}

	while 1 {
		set tm [after 30000 itmout]
		vwait ST(LLI)
		catch { after cancel $tm }
		if { $ST(LLI) == "=" } {
			# done
			puts "\ndone"
			break
		}
		if { $ST(LLI) == "" } {
			puts "\ntimeout, operation aborted"
			break
		}
	}

	catch { close $fd }
	set ST(DUM) ""
}

proc doit_quit { arg } {

	exit
}

proc sget { } {
#
# STDIN becomes readable
#
	global ST

	if [catch { gets stdin line } stat] {
		# ignore any errors (can they happen at all?)
		return
	}

	if { $stat < 0 } {
		# end of file
		exit 0
	}

	set line [string trim $line]
	if { $line == "" } {
		# ignore empty lines
		return
	}

	if { [string index $line 0] == "+" } {
		# to be sent directly to the device
		set line [string trimleft [string range $line 1 end]]
		catch { puts $ST(SFD) $line }
		return
	}

	if ![regexp -nocase "^(\[a-z\]+)(.*)" $line junk kwd args] {
		puts "illegal command line"
		return
	}

	set kwd [string tolower $kwd]

	if [catch { doit_$kwd $args } ] {
		show_usage
	}
}

proc show_usage { } {

	puts "Commands:"
	puts "  start sec \[continue\]      - start sampling at sec intervals"
	puts "  stop                      - stop sampling"
	puts "  show off|on               - show received values while sampling"
	puts "  echo off|on               - show dialogue with the master"
	puts "  extract filename          - extract stored samples to file"
	puts "  quit                      - exit the script"
}

######### Init ################################################################

# echo
set ST(ECH)	0
# show sensors
set ST(SSE)	0

set ST(EXP) 	""
set ST(LLI)	""
set ST(DUM)	""

# EEPROM size at aggregator
set PM(ESI)	16382

######### COM port ############################################################

set prt [lindex $argv 0]
set spd [lindex $argv 1]

if [catch { expr $spd } spd] {
	set spd 9600
}

if { $prt != "" } {

	# use the argument

	if [catch { u_start $prt $spd "" } err] {
		puts $err
		exit 99
	}

} else {

	while 1 {

		puts -nonewline "Enter COM port number: "
		flush stdout

		set prt [string trim [gets stdin]]

		if [catch { expr $prt } prt] {
			continue
		}

		if ![catch { u_start $prt $spd "" } err] {
			break
		}
		puts $err
	}
}

fconfigure stdin -buffering line -blocking 0
fileevent stdin readable sget

# u_settrace 7 dump.txt

vwait None
