#!/bin/sh
###########################\
exec tclsh "$0" "$@"

#
# UART front for master aggregator
#

###############################################################################

proc u_conf { speed } {
#
# central fconfigure for the UART
#
	global ST

	fconfigure $ST(SFD) -mode "$speed,n,8,1" -handshake none \
		-eofchar "" -translation auto -buffering line -blocking 0
	fileevent $ST(SFD) readable u_rdline
}

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

	u_conf $speed
}

proc u_stop { } {

	global ST

	if { $ST(SFD) != "" } {
		catch { close $ST(SFD) }
	}
	set ST(SFD) ""
}

proc ecl { ln } {

	global ST

	catch { 
		puts $ST(ECH) "$ln"
		flush $ST(ECH)
	}
}

proc u_rdline { } {

	global ST EX

	if [catch { gets $ST(SFD) line } nc] {
		# puts "device has been re-opened"
		return
	}

	set line [string trim $line]
	if { $line == "" } {
		return
	}

	if { $ST(ECH) != "" } {
		# echo everything received
		ecl "-> $line"
	}

	if { $ST(EXP) != "" } {
		# expecting something
		if [regexp -nocase $ST(EXP) $line] {
			set ST(LLI) $line
		}
	}

	if { $ST(SSE) && [regexp "^1002 *Agg" $line] } {
		show_sensors $line
	}

	if { $ST(NTY) == "custodian" } {
		track_sat_status $line
	}

	if !$EX(EXT) {
		return
	}

	#######################################################################
	### sample extraction #################################################
	#######################################################################

	if { $EX(EXT) < 3 } {
		extract_aggregator_samples $EX(EXT) $line
	} else {
		extract_collector_samples $EX(EXT) $line
	}
}

proc extract_aggregator_samples { mode line } {

	global PT ST

	set nfn 1
	if { $mode < 2 } {
		if [regexp $PT(AGS) $line jk col csl asl cts ats vls] {
			dump_values $col $csl $asl $cts $ats $vls
			set ST(LLI) "."
			set nfn 0
		}
	} else {
		if [regexp $PT(AEV) $line jk col asl ats vls] {
			dump_event $col $asl $ats $vls
			set ST(LLI) "."
			set nfn 0
		}
	}
	if $nfn {
		if [regexp $PT(AEL) $line] {
			set ST(LLI) "="
		} else {
			set ST(LLI) "-"
		}
	}
}

proc track_sat_status { msg } {
#
# Display satellite status
#
	global ST PT

	if [regexp $PT(SSA) $msg jk md yr tm] {
		# got time from the satellite
		if { [catch { expr $yr } yr] || $yr < 2009 } {
			# not set
			set tm "unset"
		} else {
			set tm "$md$yr$tm"
		}
		set ST(SAS) $tm
	}

	if [regexp $PT(SSB) $msg jk ss la lo al] {
		set ST(SAZ) [list $ss $la $lo $al]
	}
}

proc extract_collector_samples { mode line } {

	global PT ST

	set nfn 1
	if { $mode < 4 } {
		if [regexp $PT(COS) $line jk typ csl cts vls] {
			dump_values $typ $csl "" $cts "" $vls
			set ST(LLI) "."
			set nfn 0
		}
	} else {
		if [regexp $PT(CEV) $line jk col asl ats vls] {
			dump_event $col $asl $ats $vls
			set ST(LLI) "."
			set nfn 0
		}
	}
	if $nfn {
		if [regexp $PT(CEL) $line] {
			set ST(LLI) "="
		} else {
			set ST(LLI) "-"
		}
	}
}

proc dump_values { col csl asl cts ats vls } {

	global EX ST

	if { $col != "" } {
		set ln "$col, "
	} else {
		set ln ""
	}

	append ln "$csl, "

	if { $asl != "" } {
		append ln "$asl, "
		set slo $asl
	} else {
		set slo $csl
	}

	append ln "[t_parse cts], "

	if { $ats != "" } {
		append ln "[t_parse ats], "
	}

	set sn 0
	while 1 {
		set v [n_parse vls]
		if { $v == "" } {
			break
		}
		set cv [sconvert $sn $v]
		append ln "$v,  [format %1.2f $cv], "
		incr sn
		if { [string index $vls 0] == "," } {
			set vls [string range $vls 1 end]
		}
	}

	catch { puts $EX(DUM) $ln }

	if { $ST(ECH) != "stdout" && $EX(DUM) != "stdout" } {
		puts -nonewline "\r$slo"
		flush stdout
	}
}

proc dump_event { evt slo tst par } {

	global EX ST

	set ln [format %10d $slo]
	append ln "  $evt [t_parse tst]"

	for { set i 0 } { $i < 3 } { incr i } {
		if [catch { expr [lindex $par $i] } v] {
			set v 0
		}
		append ln [format %6d $v]
	}

	catch { puts $EX(DUM) $ln }

	if { $EX(DUM) != "stdout" && $ST(ECH) != "stdout" } {
		puts -nonewline "\r$slo"
		flush stdout
	}
}

proc fheader { } {
#
# Generates the header line for a dump file
#
	global EX

	switch $EX(EXT) {

	1 {
	    return "Collector, Coll Slot, Agg Slot, Coll Time, Agg Time,\
		Raw Value, Converted, ..., "
	}

	2 -
	4 { return "      Slot Event     Date     Time  Plot  Intv  Mstr" }

	3 {
	    return "Status, Coll Slot, Coll Time, Raw Value, Converted, ..., "
	}

	}

	return ""
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

proc show_sensors { line } {

	t_skip line " Col "

	set cn [n_parse line 1]
	if { $cn == "" } {
		return
	}

	t_skip line ":"

	# collector's time stamp
	set ts [t_parse line]

	if { $ts == "" } {
		# looks like some garbage
		return
	}


	puts "\nCollector $cn at $ts:"
	if { [string first "gone" $line] >= 0 } {
		puts "  GONE!!!"
		return
	}

	set sn 0
	
	while 1 {
		set v [n_parse line]
		if { $v == "" } {
			break
		}
		set cv [sconvert $sn $v]
		puts "  S${sn} : [format {%4d -> %7.2f} $v $cv]"
		incr sn
		if { [string index $line 0] == "," } {
			set line [string range $line 1 end]
		}
	}
}
		
proc itmout { } {

	global ST

	set ST(LLI) ""
}

proc sendm { cmd } {

	global ST

	if { $ST(ECH) != "" } {
		ecl "<- $cmd"
	}
	catch { puts $ST(SFD) $cmd }
}

proc issue { cmd pat ret del { nowarn "" } } {
#
# Issues a command to the node
#
	global ST

	set ST(EXP) $pat
	set ST(LLI) "+"

	while { $ret > 0 } {
		sendm $cmd
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
	if { $nowarn == "" } {
		puts "command timeout!!!"
	}
	return 1
}

proc scmd { cmd } {
#
# Converts the command for satnode issue
#
	global ST

	regsub -all "\[ \t\]+" $cmd "_" cmd

	return "r $ST(SID) $cmd"
}

proc fix_intv { intv } {
#
	if { $intv == "" } {
		return ""
	}

	if { $intv == "" } {
		puts "sampling interval not specified"
		return 0
	}

	set iv $intv

	if { $iv < 60 } {
		set iv 60
	}

	set day [expr 24 * 3600]
	set daz [expr $day / 2]

	if { $iv > $daz } {
		set iv $daz
	} else {
		while { [expr $day % $iv] } {
			incr iv
		}
	}

	if { $iv != $intv } {
		puts "interval modified to $iv to meet the requirements"
	}

	return $iv
}

proc fix_plid { plid } {
#
	if { $plid == "" || $plid > 32767 } {
		puts "illegal or missing plot id, must be between 0 and 32767"
		return -1
	}

	return $plid
}

proc doit_start { arg } {
#
# Start collection to aggregator
#
	global ST

	if [node_is_not ""] {
		return
	}

	if { $ST(NTY) == "aggregator" } {
		start_aggregator $arg
	} elseif { $ST(NTY) == "custodian" } {
		start_custodian $arg
	} else {
		puts "the node must be an aggregator or a custodian"
	}
}

proc start_aggregator { arg } {

	global ST

	set intv [fix_intv [n_parse arg 1]]

	if { $intv == 0 } {
		# diagnosed by fix_intv
		return
	}

	# plot id
	set plid [fix_plid [n_parse arg 1]]
	if { $plid < 0 } {
		return
	}

	set efl [regexp -nocase "erase" $arg]

	puts "resetting the node, please wait ..."

	if [issue "q" "^(1001|1005|AT.CMGS)" 4 60] {
		return
	}

	if { [string first "1005" $ST(LLI)] >= 0 } {
		puts "clearing maintenance mode, please wait ..."
		if [issue "F" "^1001" 4 60] {
			return
		}
	} elseif { [string first "1001" $ST(LLI)] < 0 } {
		satrevert
		return
	}

	if $efl {
		puts "erase requested, erasing storage, please wait ..."
		if [issue "E" "^1001" 4 60] {
			return
		}
	}

	# sync
	puts "setting parameters ..."
	if [issue "Y $intv" "^1013" 6 4] {
		return
	}

	# make it a master
	sendm "m"
	# now this doesn't show up ...
	sendm "m"

	if [issue "P $plid" "^1012" 6 4] {
		return
	}

	# set the time stamp to current time
	set ts [clock format [clock seconds] -format "%Y-%m-%d %H:%M:%S"]

	if [issue "T $ts" "^1009" 6 4] {
		return
	}

	# stored entries
	if [issue "a -1 -1 -1 3" "^1005" 6 5] {
		return
	}

	if [issue "SA" "^1010" 4 3] {
		return
	}

	set eu 0
	regexp "tored entries (\[0-9\]+)" $ST(LLI) junk eu
	puts "$eu entries already stored at node"
	puts "sampling started"
}

proc no_satid { } {

	global ST

	if { $ST(SID) == "" } {
		puts "satellite node id is not set, use the satid command"
		return 1
	}

	return 0
}

proc start_custodian { arg } {

	global ST

	if [no_satid] {
		return
	}

	set intv [fix_intv [n_parse arg 1]]

	if { $intv == 0 } {
		# diagnosed by fix_intv
		return
	}

	# plot id
	set plid [fix_plid [n_parse arg 1]]
	if { $plid < 0 } {
		return
	}

	set efl [regexp -nocase "erase" $arg]

	puts -nonewline "resetting "

	if $efl {
		puts -nonewline "and erasing "
	}

	puts "the node, please wait ..."

	if $efl {
		# erase
		if [issue [scmd "E"] "^1001" 4 60] {
			return
		}
	} else {
		if [issue [scmd "q"] "^1001" 4 30] {
			return
		}
	}

	# sync
	puts "setting sync parameters ..."
	if [issue [scmd "Y $intv"] "^1013" 6 10] {
		return
	}

	# make it a master
	set cmd [scmd "m"]
	sendm $cmd
	sendm $cmd
	sendm $cmd
	sendm $cmd

	# plot ID
	puts "setting plot id ..."
	if [issue [scmd "P $plid"] "^1012" 4 12] {
		return
	}

###############################################################################
# do not set the time: the node will get it from GPS
if 0 {
	set ts [clock format [clock seconds] -format "%Y-%m-%d_%H:%M:%S"]

	puts "setting time ..."
	if [issue [scmd "T $ts"] "At.*uptime" 6 12] {
		return
	}
}
###############################################################################

	if [issue [scmd "a -1 -1 -1 3"] "^1005" 6 10] {
		return
	}

	if [issue [scmd "SA"] "^1010" 4 10] {
		return
	}

	puts "done, sampling started"
}

proc doit_stop { arg } {
#
# Stop sampling
#
	global ST

	if [node_is_not ""] {
		return
	}

	if { $ST(NTY) == "aggregator" } {
		stop_aggregator $arg
	} elseif { $ST(NTY) == "collector" } {
		stop_collector $arg
	} elseif { $ST(NTY) == "custodian" } {
		stop_custodian $arg
	}
}

proc stop_aggregator { arg } {

	global ST

	set maint [regexp -nocase "^m" $arg]

	puts -nonewline "resetting node"

	if $maint {
		puts -nonewline " for maintenance"
	}

	puts " ..."

	if $maint {
		if [issue "M" "^1005" 4 30] {
			return
		}
	} else {
		# in case already in maintenance mode
		if [issue "q" "^(1001|1005)" 4 30] {
			return
		}
		if { [string first "1005" $ST(LLI)] >= 0 } {
			puts "node was in maintenance mode, nothing stopped"
		} else {
			if [issue "a -1 -1 -1 0" "^1005" 4 5] {
				return
			}
		}
	}

	if [regexp "tored entries (\[0-9\]+)" $ST(LLI) junk eu] {
		puts "done, $eu stored entries"
	} else {
		puts "done"
	}
}

proc stop_custodian { arg } {

	global ST

	if [no_satid] {
		return
	}

	if [regexp -nocase "^m" $arg] {
		puts "cannot stop a satellite node into maintenance mode"
		return
	}

	puts "resetting node ..."

	if [issue [scmd "q"] "^1001" 4 60] {
		return
	}

	if [issue [scmd "a -1 -1 -1 0"] "^1005" 4 8] {
		return
	}

	puts "done"
}

proc stop_collector { arg } {

	global ST

	# collector is always reset for maintenance
	puts "resetting node for maintenance ..."

	if [issue "M" "^2005" 4 30] {
		return
	}
	if [regexp "tored reads (\[0-9\]+)" $ST(LLI) junk eu] {
		puts "done, $eu stored readings"
	} else {
		puts "done"
	}
}

proc doit_echo { arg } {

	global ST

	set fp [lindex $arg 0]

	if { $arg == "off" } {

		if { $ST(ECH) != "" && $ST(ECH) != "stdout" } {
			catch { close $ST(ECH) }
		}
		set ST(ECH) ""

	} else {

		# close the previous one
		if { $ST(ECH) != "" && $ST(ECH) != "stdout" } {
			catch { close $ST(ECH) }
		}
		set ST(ECH) ""

		if { $arg == "" || $arg == "on" || $arg == "stdout" } {
			set ST(ECH) "stdout"
			return
		}

		if { [string first "." $arg] < 0 } {
			append arg ".txt"
		}
		if [catch { open $arg "a" } fd] {
			puts "cannot open file $arg for writing"
			return
		}
		set ST(ECH) $fd
		ecl "LOG MARK --- [clock format [clock seconds]]"
	}
	puts "OK"
}

proc doit_show { arg } {

	global ST

	set ST(SSE) 0

	if [regexp -nocase "off" $arg] {
		puts "OK"
		return
	}

	if [node_is_not ""] {
		return
	}

	if { $ST(NTY) != "aggregator" && $ST(NTY) != "custodian" } {
		puts "the node must be an aggregator or a custodian"
		return
	}

	set ST(SSE) 1
	puts "OK"
}

proc doit_virgin { arg } {
#
# Zero out the node
#
	global ST

	if [node_is_not ""] {
		return
	}

	set intv [n_parse arg 1]
	if { $intv != "" } {
		set intv [fix_intv $intv]
		if { $intv == 0 } {
			return
		}
	} else {
		set intv 0
	}

	# this can be 'c' for 'custodian' reset (as opposed to satnode)
	set flag [string index $arg 0]

	if { $ST(NTY) == "collector" } {

		puts "returning the collector to factory state (be patient) ..."
		if [issue "Q" "^CC1100" 3 60] {
			return
		}
		if $intv {
			# make it store all samples
			if [issue "s $intv -1 0 -1 3" "^2005" 6 5] {
				return
			}
			if [issue "SA" "^2010" 6 5] {
				return
			}
			puts "node set for stand-alone operation"
		}

		puts "done, disconnect the node from the USB dongle to\
			prevent sampling!"
		return
	}

	if { $ST(NTY) == "aggregator" } {

		puts \
		   "returning the aggregator to factory state (be patient) ..."

		if [issue "Q" "^(1001|AT.CMGS)" 4 60] {
			return
		}

		if { [string first "1001" $ST(LLI)] < 0 } {
			satrevert
			return
		}

		if { $intv == 0 } {
			set intv 60
		}

		if [issue "Y $intv" "^1013" 6 4] {
			return
		}

		# make it a master
		sendm "m"
		sendm "m"

		if [issue "a -1 -1 -1 3" "^1005" 4 8] {
			# collect all
			return
		}
		if [issue "SA" "^1010" 4 6] {
			return
		}
		if [issue "a -1 -1 -1 0" "^1005" 4 5] {
			# do not collect any samples unless told so
			return
		}
		puts "done, node is in the stopped state"
		return
	}

	if { $ST(NTY) == "custodian" } {

		# custodian flag
		if { $flag == "c" } {
			puts "returning the custodian to factory state\
				(be patient) ..."
			if [issue "Q" "^1001" 4 60] {
				return
			}
		} else {
			puts "returning the satnode to factory state (be\
				patient) ..."

			if [issue [scmd "Q"] "^1001" 4 60] {
				return
			}
	
			if { $intv == 0 } {
				set intv 60
			}
	
			if [issue [scmd "Y $intv"] "^1013" 6 4] {
				return
			}
	
			# make it a master
			set cmd [scmd "m"]
			sendm $cmd
			sendm $cmd
			sendm $cmd
			sendm $cmd
	
			if [issue [scmd "a -1 -1 -1 3"] "^1005" 4 8] {
				# collect all
				return
			}
			if [issue "SA" "^1010" 4 6] {
				return
			}
			if [issue [scmd "a -1 -1 -1 0"] "^1005" 4 5] {
				# do not collect any samples unless told so
				return
			}
		}
		puts "done"
		return
	}
}

proc doit_extract { arg } {

	global ST EX

	set arg [string trim $arg]

	if [node_is_not ""] {
		return
	}

	if { $ST(NTY) == "custodian" } {
		puts "warning: extracting from a custodian"
	}

	set md [t_skip arg "^(e\[a-z\]*|c\[0-9\]*)"]

	if { [string first $md "events"] == 0 } {
		set md "e"
	} elseif { ![regexp "^c\[0-9\]*$" $md] } {
		puts "the first argument must identify mode: e or c\[num\]"
		return
	}

	if { $md == "" } {
		set md "c"
	}

	set upto ""
	set from [n_parse arg 1]
	if { $from != "" } {
		set upto [n_parse arg 1]
	}

	if { $from == "" } {
		set from 0
	}

	if { $upto == "" } {
		# this is a huge max, which is good for both node types
		set upto 100000000
	}

	# file name
	set fn [t_skip arg "^\[^0-9\]\[^ \t\]*"]

	if { $fn != "" } {
		# default extension is csv
		if { [string first "." $fn] < 0 } {
			if { $md == "e" } {
				append fn ".txt"
			} else {
				append fn ".csv"
			}
		}
		if [catch { open $fn "w" } fd] {
			puts "cannot open file '$fn': $fd"
			return
		}
	} else {
		set fd stdout
	}

	set EX(DUM) $fd

	if { $ST(NTY) == "collector" } {
		extract_collector $md $from $upto
	} else {
		extract_aggregator $md $from $upto
	}
	stop_extraction
}

proc stop_extraction { } {

	global EX

	set EX(EXT) 0
	if { $EX(DUM) != "" && $EX(DUM) != "stdout" } {
		catch { close  $EX(DUM) }
	}
	set EX(DUM) ""
}

proc extract_aggregator { md from upto } {

	global ST EX

	if { [string index $md 0] == "e" } {
		# extract events
		set EX(EXT) 2
		set cmd "D $from $upto 9999"
	} else {
		set cmd "D $from $upto"
		if ![catch { expr [string range $md 1 end] } col] {
			append cmd " $col"
		}
		# extract data
		set EX(EXT) 1
	}

	if { $EX(DUM) != "stdout" } {
		puts "dumping samples to file ..."
	}

	catch { puts $EX(DUM) [fheader] }

	if [issue $cmd "^10(07|08|11)" 4 10] {
		# timeout
		stop_extraction
		return
	}

	if { $ST(LLI) == "=" } {
		puts "no items found"
	} else {
		# wait for end
		while 1 {
			# removed, explicit abort only
			# set tm [after 60000 itmout]
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

	stop_extraction
}

proc extract_collector { md from upto } {

	global ST EX

	if { [string index $md 0] == "e" } {
		# extract events
		set EX(EXT) 4
	} else {
		# extract data
		set EX(EXT) 3
	}

	if { $EX(DUM) != "stdout" } {
		puts "dumping samples to file ..."
	}

	catch { puts $EX(DUM) [fheader] }

	if [issue "D $from $upto" "^20(07|08|11)" 4 10] {
		# timeout
		stop_extraction
		return
	}

	if { $ST(LLI) == "=" } {
		puts "no items found"
	} else {
		# wait for end
		while 1 {
			# set tm [after 60000 itmout]
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

	stop_extraction
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

	global ST PT

	if [node_is_not ""] {
		return
	}

	if { $ST(NTY) == "collector" } {

		if [issue "s" "^2005" 6 5] {
			return
		}
	
		if ![regexp -nocase $PT(CST) $ST(LLI) j col fre fla upt sam] {
			puts "bad response from collector"
			return
		}

		if [catch { expr 0x$fla } fla] {
			set fla 0
		}

		puts "Collector $col:"
		puts "    Sampling interval: ${fre}s"
		puts "    Uptime:            ${upt}s"
		puts "    Stored entries:    $sam"
		puts "    Writing to EEPROM: [fltos $fla]"

		return

	}

	if [issue "a" "^1005" 6 5] {
		return
	}

	if ![regexp -nocase $PT(AST) $ST(LLI) j agg fre fla upt mas sam] {
		puts "bad response from the node"
		return
	}

	if { $ST(NTY) == "aggregator" } {

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
	        puts "    Stored entries:    $sam"
	        puts "    Writing to EEPROM: [fltos $fla]"

		return
	}

	puts "Custodian $agg:"
	puts "    Uptime:            ${upt}s"
	if { $mas > 0 } {
		puts "    Master:            $mas"
	}

	if ![info exists ST(SAS)] {
		set tm "unknown"
	} else {
		set tm $ST(SAS)
	}

	puts "    Sat time:          $tm"

	if ![info exists ST(SAZ)] {
		set sq "unknown"
		set ps "unknown"
	} else {
		set sq [lindex $ST(SAZ) 0]
		set ps [lindex $ST(SAZ) 1]
		if { $ps == "n/a" } {
			set ps "unknown"
		} else {
			set ps "la = $ps, lo = [lindex $ST(SAZ) 2], al =\
				[lindex $ST(SAZ) 3]"
		}
	}

	puts "    Signal quality:    $sq"
	puts "    Position:          $ps"
}
	
proc doit_quit { arg } {

	exit
}

proc direct_command { pfx cmd } {
#
# Handle all variants of immediate command
#
	global ST

	if [node_is_not ""] {
		return ""
	}

	set md [string length $pfx]

	if { $md == 0 || $md > 3 } {
		puts "illegal direct command sequence"
		return ""
	}

	if { $md == 1 } {
		# to this node
		catch { puts $ST(SFD) $cmd }
		return "="
	}

	# must be talking to custodian
	if [node_is_not "custodian"] {
		return ""
	}

	if { $md == 2 } {
		# command to the sat node
		catch { puts $ST(SFD) [scmd $cmd] }
		return "@"
	}

	# command to the satellite
	catch { puts $ST(SFD) "s $cmd" }
	return "\$"
}

proc sget { } {
#
# STDIN becomes readable
#
	global ST CMDLIST

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
		prompt
		return
	}

	if { [string index $line 0] == "+" } {
		# to be sent directly to the device
		if [regexp "^(\\++)(.*)" $line junk pfx line] {
			# this is mandatory
			set line [string trimleft $line]
			set ifx [direct_command $pfx $line]
			if { $ifx != "" && $ST(ECH) != "" && 
			     $ST(ECH) != "stdout" } {
				ecl "<${ifx} $line"
			}
		}
		prompt
		return
	}

	if ![regexp -nocase "^(\[a-z\]+)(.*)" $line junk kwd args] {
		puts "illegal command line"
		prompt
		return
	}

	set kwd [string tolower $kwd]

	if { [lsearch -exact $CMDLIST $kwd] < 0 } {
		if { $kwd != "h" && $kwd != "help" } {
			puts "no such command"
		}
		show_usage
	} else {
		if [catch { doit_$kwd [string trim $args] } err ] {
			puts "command failed with internal error: $err"
		}
	}
	prompt
}

proc prompt { } {

	puts -nonewline ":"
	flush stdout
}

proc twod { nn } {

	regsub "^0+" $nn "" nn

	if { $nn == "" || [catch { format "%02d" [expr $nn % 100] } nn] } {
		set nn "00"
	} 

	return $nn
}

proc t_parse { ln } {

	upvar $ln line

	set res ""

	if [regexp "^\[ \t\]*(\[0-9\]+)-(\[0-9\]+)-(\[0-9\]+)" $line jk y u d] {
		set line [string range $line [string length $jk] end]
		if { $y < 2000 } {
			set y 0
		}
		append res "[twod $y]-[twod $u]-[twod $d]"
	}
	if [regexp "^\[ \t\]*(\[0-9\]+):(\[0-9\]+):(\[0-9\]+)" $line jk y u d] {
		set line [string range $line [string length $jk] end]
		if { $res != "" } {
			append res " "
		}
		append res "[twod $y]:[twod $u]:[twod $d]"
	}

	return $res
}
		
proc t_skip { ln pat } {

	upvar $ln line

	if ![regexp -indices -nocase $pat $line match] {
		set line ""
		return ""
	}

	set res [string range $line [lindex $match 0] [lindex $match 1]]

	set line [string trimleft [string range $line [expr [lindex $match 1] +\
		1] end]]

	return $res
}

proc n_parse { ln { nn 0 } { fl 0 } } {

	upvar $ln line

	set line [string trimleft $line]

	if ![regexp "^\[^ ,\t\n\r\]+" $line code] {
		return ""
	}

	if [catch { expr $code } num] {
		return ""
	}

	if !$fl {
		# integer expected
		if { [catch { expr int($num) } iv] || $iv != $num } {
			return ""
		}
		set num $iv
	}

	if { $nn && $num < 0 } {
		return ""
	}

	set line \
		[string trimleft [string range $line [string length $code] end]]

	return $num
}

proc article { nn } {

	if { $nn != "" } {
		if [regexp -nocase "^\[aeiou\]" $nn] {
			set nn "an $nn"
		} else {
			set nn "a $nn"
		}
	}
	return $nn
}

proc node_is_not { nt } {

	global ST

	if { $ST(NTY) == "" } {
		puts "not connected to a node, do 'connect' first"
		return 1
	}

	if { $nt != "" && $ST(NTY) != $nt } {
		puts "this command can only be applied to [article $nt]"
		return 1
	}

	return 0
}

proc try_port { } {
#
# Check if a node is responding at the current port and determine its type
#
	global ST

	set sat 0
	if [issue "h" "^\[12\]001 .*commands" 3 1 nowarn] {
		# check if this is a satnode
		for { set i 0 } { $i < 3 } { incr i } {
			sendm "s---"
			if ![issue "h" "^1001 .*command" 1 1 nowarn] {
				break
			}
		}
		if { $i >= 3 } {
			# failure
			return 0
		}
		set sat 1
	}

	if { [string first "Aggregator" $ST(LLI)] >= 0 } {
		set ST(NTY) "aggregator"
	} elseif { [string first "Collector" $ST(LLI)] >= 0 } {
		set ST(NTY) "collector"
	} elseif { [string first "Custodian" $ST(LLI)] >= 0 } {
		set ST(NTY) "custodian"
	} else {
		puts "\nunknown node type: $ST(LLI)"
		return 0
	}

	# check the version
	if ![regexp "\[123\]\\.\[0-9\]" $ST(LLI) ver] {
		set ver "unknown"
	}

	if { $ver != 1.2 } {
		puts "\nincompatible praxis version ($ver), 1.2 required"
		return 0
	}

	if $sat {
		puts "\nsatnode being forced to aggregator mode"
		# stop any sampling in progress
		issue "a -1 -1 -1 0" "^1005" 3 2
	}
	return 1
}

proc doit_a { arg } {
#
# Abort sample extraction
#
	global ST EX

	if { $EX(EXT) == 0 } {
		puts "no extraction in progress"
	} else {
		set ST(LLI) "="
	}
}

proc doit_satid { arg } {
#
# Satellite node Id
#
	global ST

	if [node_is_not "custodian"] {
		return
	}

	if { $arg == "" } {
		if { $ST(SID) == "" } {
			puts "satellite node id has not been set yet"
		} else {
			puts "satellite node id is $ST(SID)"
		}
		return
	}

	set snid [n_parse arg 1]

	if { $snid == "" || $snid == 0 || $snid > 32767 } {
		puts "illegal node id"
	} else {
		set ST(SID) $snid
		puts "satellite node id set to $ST(SID)"
	}
}

proc disconnect { } {

	global ST

	u_stop
	set ST(NTY) ""
}

proc satrevert { } {

	puts "satnode has been reverted to its official function"
	puts "disconnecting from the node"
	disconnect
}

proc doit_connect { arg } {
#
# Locate the node
#
	global ST

	set prt ""
	set spd ""

	if { $arg != "" } {
		if ![regexp "^(\[0-9\]+)\[^0-9\]*(\[0-9\]*)$" $arg jk prt spd] {
			show_usage
			return
		}
		if { [catch { expr int($prt) } prt] || $prt > 32 } {
			puts "port number $prt out of range, 32 is max"
			return
		}
		if { $spd != "" } {
			if { [catch { expr int($spd) } spd] || ($spd != 9600 &&\
			    $spd != 19200) } {
				puts "bit rate $spd out of range, only 9600 and\
					19200 are supported"
				return
			}
		}
	}

	# in case we are connected
	disconnect

	if { $prt == "" } {
		# scan ports
		for { set prt 0 } { $prt <= 32 } { incr prt } {
			if [catch { u_start $prt 19200 }] {
				continue
			}
			set spd 19200
			puts -nonewline "\rtrying port $prt"
			flush stdout
			if [try_port] {
				# success
				break
			}
			# change the rate and try again
			u_conf 9600
			set spd 9600
			if [try_port] {
				break
			}
			# failure: close and try again
			u_stop
		}
		if { $prt > 32 } {
			u_stop
		}
		if { $ST(SFD) == "" } {
			# no way
			puts "\nfailed to locate a responsive node"
			return
		}
		puts "\nconnected to [article $ST(NTY)] on port $prt at $spd"
	} else {
		# use the one specified port
		if { $spd != "" } {
			set s $spd
		} else {
			set s 19200
		}
		if [catch { u_start $prt $s }] {
			puts "failed to open port $prt"
			return
		}
		if ![try_port] {
			if { $spd == "" } {
				# change the rate and try again
				u_conf 9600
				set s 9600
				try_port
			}
		}
		if { $ST(NTY) == "" } {
			u_stop
			puts "failed to locate a responsive device on port $prt"
			return
		}
		set spd $s
		puts "connected to [article $ST(NTY)]"
	}
}

proc show_usage { } {

 puts "Commands:"
 puts "  connect \[port \[speed\]\]           - connect to the node"
 puts "  start sec pnum \[erase\]           - start sampling at sec intervals"
 puts "  stop \[m\]                         - stop sampling"
 puts "  echo off|on                      - show dialogue with the node"
 puts "  show off|on                      - show received values while sampling"
 puts "  extract cxx|e \[from \[to\]\] \[file\] - extract data"
 puts "  virgin \[sec\]                     - reset to factory state"
 puts "  status                           - node status"
 puts "  a                                - abort sample extraction"
 puts "  satid id                         - view/change satellite node id"
 puts "  +string                          - issue direct command to this node"
 puts "  ++string                         - issue command to sat node"
 puts "  +++string                        - send message over satellite"
 puts "  quit                             - exit the script"

}

######### Init ################################################################

# echo
set ST(ECH)	""
# show sensors
set ST(SSE)	0

set ST(EXP) 	""
set ST(LLI)	""

# node type
set ST(NTY)	""
# UART file descriptor
set ST(SFD)	""

# satellite node id
set ST(SID)	10

# extraction mode
set EX(EXT)	0
# file descriptor
set EX(DUM)	""

######### Commands ############################################################

set CMDLIST	{ start stop echo show virgin extract status quit connect a
			satid }

######### Patterns ############################################################

# time stamp
set PT(TST) 	"(\[0-9\]+-\[0-9\]+-\[0-9\]+ \[0-9\]+:\[0-9\]+:\[0-9\]+)"

# aggregator's data sample line
set PT(AGS)	"^1007 Col (\[0-9\]+) slot (\[0-9\]+).*A: (\[0-9\]+). "
	append PT(AGS) $PT(TST)
	append PT(AGS) "..A "
	append PT(AGS) $PT(TST)
	append PT(AGS) ". (.+)"

# aggregator's event
set PT(AEV)	"^1011 (\[^ \]+) (\[0-9\]+) "
	append PT(AEV) $PT(TST)
	append PT(AEV) " (.+)"

# aggregator's end of list
set PT(AEL)	"^1008 Did"

# aggregator's status
set PT(AST)	": (\[0-9\]+).: Audit freq (\[0-9\]+).*a_fl (\[0-9a-f\]+) "
	append PT(AST) "Uptime (\[0-9\]+).*aster (\[0-9\]+).*entries (\[0-9\]+)"

# collector's data sample line
set PT(COS)	"^2007 (\[^ \]+) slot (\[0-9\]+) "
	append PT(COS) $PT(TST)
	append PT(COS) ". (.+)"

# collector's event
set PT(CEV)	"^2011 (\[^ \]+) (\[0-9\]+) "
	append PT(CEV) $PT(TST)
	append PT(CEV) " (.+)"

# collector's end of list
set PT(CEL)	"^2008 Collector"

# collector's status
set PT(CST)	": (\[0-9\]+).: Maj_freq (\[0-9\]+).*c_fl (\[0-9a-f\]+) "
	append PT(CST) "Uptime (\[0-9\]+) .*reads (\[0-9\]+)"

# satellite status lines
set PT(SSA)	"^.CMGS: .System Status.,\"(..-..-)(....)(..+)\""
set PT(SSB)	"^(\[0-9\]+),(\[^ ,\]+),(\[^ ,\]+),\[^ ,\]+,\[^ ,\]+,\[^ ,\]+,"
	append PT(SSB) "(\[^ ,\]+),\[^ ,\]+,"

###############################################################################

fconfigure stdin -buffering line -blocking 0
fileevent stdin readable sget

# u_settrace 7 dump.txt

show_usage
prompt

vwait None
