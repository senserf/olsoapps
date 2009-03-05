#!/bin/sh
###########################\
exec tclsh "$0" "$@"

#
# UART front for master aggregator
#

###############################################################################

proc u_start { udev speed } {
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

	if { $ST(DUM) == "" } {
		return
	}

	#######################################################################
	### sample extraction #################################################
	#######################################################################

	if { $ST(DUN) != 1 } {
		# aggregator
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
	} else {
		# collector
		if { [regexp \
		  "^2007 COLLECTED slot (\[0-9\]+).*time (\[^ \]+).*S0:" \
		    $line j slo tst] && ![catch { expr $slo } slo] } {
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
			dump_sensors "" $slo $tst $sv
			set ST(LLI) "."
		} elseif [regexp "^2008 Col" $line] {
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

	set ln "[tstamp $ts]"

	if { $col != "" } {
		append ln "    [format %4d $col]"
	}

	set sn 0
	foreach v $vl {
		set cv [sconvert $sn $v]
		append ln "    [format {%4d %7.2f} $v $cv]"
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

	set nt [ntype]
	if { $nt == 0 } {
		return
	}

	if { $nt == 1 } {
		puts "can't do this to a collector"
		return
	}

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
		if [issue "E" "^1001.*Find collector:" 4 60] {
			return
		}
	}

	# sync
	puts "setting parameters ..."
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
	set nt [ntype]
	if { $nt == 0 } {
		puts "node doesn't respond"
		return
	}

	puts "resetting node ..."

	if { $nt == 1 } {
		# collector
		if [issue "q" "^CC1100" 4 30] {
			return
		}
		if [issue "s -1 -1 -1 -1 0" "^2005" 4 5] {
			return
		}
	} else {
		# aggregator
		if [issue "q" "^1001.*Find collector:" 4 30] {
			return
		}
		if [issue "a -1 -1 -1 0" "^1005" 4 5] {
			return
		}
		puts "done"
	}
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

	set nt [ntype]
	if { $nt == 0 } {
		return
	}

	if { $nt == 1 } {
		puts "can't do this to a collector"
		return
	}

	if [regexp -nocase "off" $arg] {
		set ST(SSE) 0
	} else {
		set ST(SSE) 1
	}
	puts "OK"
}

proc doit_virgin { arg } {
#
# Zero out the node
#
	global ST

	set nt [ntype]

	if { $nt == 0 } {
		return
	}

	if [regexp "\[0-9\]+" $arg intv] {
		# interval specified, validate
		if { [catch { expr $intv } intv] || $intv < 30 || \
		    $intv > 32767 } {
			puts "illegal interval, should be between 30 and 32767\
				seconds"
			return
		}
	} else {
		set intv 0
	}

	if { $nt == 1 } {
		# collector
		puts "returning the collector to factory state (be patient) ..."
		if [issue "Q" "^PicOS" 3 60] {
			return
		}
		if $intv {
			# make it store all samples
			if [issue "s $intv -1 -1 -1 3" "^2005 Stats" 6 5] {
				return
			}
			if [issue "SA" "^2010 Flash" 6 5] {
				return
			}
			puts "node set for stand-alone operation"
		}

		puts "done, disconnect the node ASAP from the USB dongle to\
			prevent sampling!"
		return
	}

	# aggregator
	puts "returning the aggregator to factory state (be patient) ..."
	if [issue "Q" "^1001.*Find collector:" 4 60] {
		return
	}

	if { $intv == 0 } {
		set intv 60
	}

	if [issue "Y $intv" "^0003 Synced to" 6 4] {
		return
	}
	if [issue "m" "^0003 (Became|Sent) Master" 6 4] {
		return
	}
	if [issue "a -1 -1 -1 3" "^1005" 4 5] {
		# collect all
		return
	}
	if [issue "SA" "^1010 Flash" 4 6] {
		return
	}
	if [issue "a -1 -1 -1 0" "^1005" 4 5] {
		# do not collect any samples unless told so
		return
	}
	puts "done, node is in the stopped state"
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

	# collector or aggregator?
	set ST(DUN) [ntype]
	if { $ST(DUN) == 0 } {
		# no response
		return
	}

	if { $ST(DUN) == 1 } {
		# collector
		set lim $PM(ESC)
	} else {
		set lim $PM(ESI)
	}

	set ST(DUM) $fd

	set cmd "D 0 $lim"

	if { $col != "" && $ST(DUN) != 1 } {
		append cmd " $col"
	}

	puts "dumping samples to file ..."

	if [issue $cmd "^\[12\]00\[78\]" 4 10] {
		catch { close $fd }
		set ST(DUM) ""
		return
	}

	if { $ST(LLI) == "=" } {
		puts "there are no samples to dump"
	} else {
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
	}
	catch { close $fd }
	set ST(DUM) ""
}

proc fltos { fl } {

	switch [expr $fl & 0x3] {

	0 { return "nothing" }
	1 { return "not confirmed" }
	2 { return "confirmed" }

	}

	return "all"
}

proc doit_status { arg } {

	global ST PM

	set nt [ntype]

	if { $nt == 0 } {
		return
	}

	set d "(\[0-9\]+)"
	set h "(\[0-9a-f\]+)"

	if { $nt == 1 } {
		# collector
		if [issue "s" "^2005 Stats" 6 5] {
			return
		}
		if ![regexp -nocase \
		  "$d.: Maj_freq $d.*c_fl $h Uptime $d.*reads $d" $ST(LLI) \
		    j col fre fla upt sam] {
			puts "bad response from collector"
			return
		}

		if [catch { expr 0x$fla } fla] {
			set fla 0
		}

		puts "Collector $col:"
		puts "    Sampling interval: ${fre}s"
		puts "    Uptime:            ${upt}s"
		puts "    Stored samples:    $sam / $PM(ESC)"
		puts "    Writing to EEPROM: [fltos $fla]"

		return

	}

	# aggregator
	if [issue "a" "^1005 Stats" 6 5] {
		return
	}

	if ![regexp\
	  "$d.: Audit freq $d.*a_fl $h Uptime $d.*Master $d.*entries $d" \
	    $ST(LLI) j agg fre fla upt mas sam] {
		puts "nad response from aggregator"
		return
	}

	if [catch { expr 0x$fla } fla] {
		set fla 0
	}

	puts "Aggregator $agg:"
	puts "    Audit interval:    ${fre}s"
	puts "    Uptime:            ${upt}s"
	if { $mas > 0 } {
		if { $mas == $agg } {
			puts "    Master:            this node"
		} else {
			puts "    Master:            $mas"
		}
	}
	puts "    Stored samples:    $sam / $PM(ESI)"
	puts "    Writing to EEPROM: [fltos $fla]"
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
	puts "  echo off|on               - show dialogue with the node"
	puts "  extract filename          - extract stored samples to file"
	puts "  virgin \[sec\]              - reset to factory state"
	puts "  status                    - node status"
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
# EEPROM size at the collector
set PM(ESC)	32766

######### COM port ############################################################

set prt [lindex $argv 0]
set spd [lindex $argv 1]

if [catch { expr $spd } spd] {
	set spd 9600
}

if { $prt != "" } {

	# use the argument

	if [catch { u_start $prt $spd } err] {
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

		if ![catch { u_start $prt $spd } err] {
			break
		}
		puts $err
	}
}

proc ntype { } {
#
# Determine the node type we are talking to
#
	global ST

	if [issue "a" "^\[01\]00\[15\]" 4 3] {
		# failure
		return 0
	}

	if { [string index $ST(LLI) 3] == 1 } {
		# collector
		return 1
	}

	# aggregator
	return 2
}

fconfigure stdin -buffering line -blocking 0
fileevent stdin readable sget

# u_settrace 7 dump.txt

vwait None
