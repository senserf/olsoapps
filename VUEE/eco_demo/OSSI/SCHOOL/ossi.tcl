#!/bin/sh
###########################\
exec tclsh "$0" "$@"

##################################################################
# The OSS module for ARTURO's SCHOOL                             #
#                                                                #
# Copyright (C) Olsonet Communications, 2009 All Rights Reserved #
##################################################################

##################################################################
##################################################################

proc msource { f } {
#
# Intelligent 'source'
#
	if ![catch { uplevel #0 source $f } ] {
		# found it right here
		return
	}

	set dir "Scripts"
	set lst ""

	for { set i 0 } { $i < 10 } { incr i } {
		set dir "../$dir"
		set dno [file normalize $dir]
		if { $dno == $lst } {
			# no progress
			break
		}
		if ![catch { uplevel #0 source [file join $dir $f] } ] {
			# found it
			return
		}
	}

	# failed
	puts stderr "Cannot locate file $f 'sourced' by the script."
	exit 99
}

msource xml.tcl
msource log.tcl

###############################################################################

package require xml 1.0
package require log 1.0

if [catch { package require mysqltcl } ] {
	set SQL_present 0
} else {
	set SQL_present 1
}

###############################################################################

proc abt { m } {

	msg "aborted: $m"
	exit 99
}

proc msg { m } {

	puts $m
	catch { log $m }
}

proc uart_init { rfun } {

	global Uart Turn

	set Uart(RF) $rfun

	if { [info exists Uart(FD)] && $Uart(FD) != "" } {
		catch { close $Uart(FD) }
		unset Uart(FD)
	}

	# list of devices to try
	set c [string index $Uart(DEV) 0]
	set d $Uart(DEV)
	set dl [list $d]
	if { $c != "/" && $c != "\\" } {
		# not an absolute path
		lappend dl "\\\\/\\$d"
		lappend dl "/dev/$d"
		lappend dl "/dev/tty$d"
	}

	while 1 {

		msg "connecting to UART $Uart(DEV), encoding $Uart(PAR) ..."
		set c 1
		foreach d $dl {
			if ![catch { open $d RDWR } ser] {
				set c 0
				break
			}
		}

		if $c {
puts "FAIL 0 $dl $ser"
			after 20000
			continue
		}

		if [catch { fconfigure $ser -mode $Uart(PAR) -handshake none \
			-blocking 0 -translation binary } err] {
puts "FAIL 1 $dl"

			catch { close $ser }
			after 20000
			continue
		}

		break
	}

	set Uart(FD) $ser
	set Uart(BF) ""
	fileevent $Uart(FD) readable "uart_read"
	incr Turn
}

proc uart_ctmout { } {

	global Uart

	if [info exists Uart(TO)] {
		after cancel $Uart(TO)
		unset Uart(TO)
	}
}

proc uart_stmout { del fun } {

	global Uart

	if [info exists Uart(TO)] {
		# cancel the previous timeout
		uart_ctmout
	}

	set Uart(TO) [after $del $fun]
}

proc uart_erase { } {

	global Uart

	unset Uart(TO)
	set Uart(BF) ""
}

proc uart_write { w } {

	global Uart

	msg "-> $w"

	catch {
		puts -nonewline $Uart(FD) "$w\r\n" 
		flush $Uart(FD)
	}
}

proc uart_read { } {

	global Uart

	if [catch { read $Uart(FD) } chunk] {
		# ignore errors
		return
	}
		
	if { $chunk == "" } {
		# nothing available
		return
	}

	uart_ctmout

	append Uart(BF) $chunk

	while 1 {
		set el [string first "\n" $Uart(BF)]
		if { $el < 0 } {
			break
		}
		set ln [string range $Uart(BF) 0 $el]
		incr el
		set Uart(BF) [string range $Uart(BF) $el end]
		$Uart(RF) $ln
	}

	# if the buffer is nonempty, erase it on timeout

	if { $Uart(BF) != "" } {
		uart_stmout 1000 uart_erase
	}
}

proc vnum { n min max } {
#
# Verify integer number
#
	if [catch { expr int($n) } n] {
		return ""
	}

	if { $n < $min || $n >= $max } {
		return ""
	}

	return $n
}

proc input_line { inp } {
#
# Handle line input from UART
#
	set inp [string trim $inp]

	# write the line to the log (and standard output)
	msg "<- $inp"

	if ![regexp "^(\[0-9\]\[0-9\]\[0-9\]\[0-9\]) (.*)" $inp jnk tp inp] {
		# every line of significance to us must begin with a four
		# digit identifier
		return
	}

	# for now, we only care about a single line
	switch $tp {

	1002 { show_sensors $inp }

	}
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
#
# Parse a number
#
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

	set line [string range $line [string length $code] end]

	regsub "^\[ ,\t\n\r\]+" $line "" line

	return $num
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

	if ![regexp "^\[ \t\]*(\[0-9\]+)-(\[0-9\]+)-(\[0-9\]+)" $line j y u d] {
		return ""
	}
	set line [string range $line [string length $j] end]
	if { $y < 2000 } {
		set y 2000
	}
	set res "20[twod $y][twod $u][twod $d]T"

	if ![regexp "^\[ \t\]*(\[0-9\]+):(\[0-9\]+):(\[0-9\]+)" $line j y u d] {
		return ""
	}
	set line [string range $line [string length $j] end]
	append res "[twod $y][twod $u][twod $d]"
	return $res
}
	
proc show_sensors { line } {
#
# Extract sensor values and time stamps from aggregator's line
#
	t_skip line " Col "

	set cn [n_parse line 1]
	if { $cn == "" } {
		return
	}

	t_skip line ":"

	# collector's time stamp
	set ts [t_parse line]

	if { $ts == "" } {
		# looks like garbage
		return
	}

	if { [string first "gone" $line] >= 0 } {
		return
	}

	set time [clock seconds]

	if { [catch { clock scan $ts } stm] || [expr $time - $stm] > 86400 } {
		# uninitialized
		set ts [clock format $time -format "%Y%m%dT%H%M%S!"]
	} else {
		append ts "+"
	}

	set rv ""
	set cv ""
	set sen 0
	while 1 {
		set v [n_parse line]
		if { $v == "" } {
			break
		}
		set vv [snip_cnvrt $v $sen $cn]
		lappend rv $v
		lappend cv $vv
		incr sen
	}

	send_db $ts $cn $rv $cv
	send_file $ts $cn $rv $cv
}

proc send_db { ts col raw cooked } {

	global PM SQL_present

	if { $PM(DAB) == "" } {
		return
	}

	set nam [lindex $PM(DAB) 0]
	set hos [lindex $PM(DAB) 1]
	set por [lindex $PM(DAB) 2]
	set usr [lindex $PM(DAB) 3]
	set pwd [lindex $PM(DAB) 4]

	if { $hos == "" || $usr == "" || $nam == "" } {
		return
	}

	if { $por == "" } {
		set por 7777
	}

	if !$SQL_present {
		msg "SQL unavailable but DB functionality requested"
		return
	}

	if [catch { mysql::connect -user $usr -db $nam -host $hos -port $por \
		-password $pwd } con] {

		msg "SQL connection failed: $con"
		return
	}

	# this is what Andrew expects
	set ts "[string range $ts 0 7][string range $ts 9 14]"

	set sn 0

	foreach r $raw c $cooked {
		if { $c != "" } {
			# ignore those that are not converted
			if [catch { mysql::exec $con "INSERT INTO readings\
			 (node_id, sensor_id, time, converted_value, raw_value)\
			  VALUES ($col, $sn, $ts, $c, '$r')" } nr] {
				msg "SQL query failed: $nr"
			}
		}
		incr sn
	}

	catch { mysql::close $con }
}

proc send_file { ts col raw cooked } {

	global PM

	if { $PM(REP) == "" } {
		# no value logging
		return
	}

	if ![file isdirectory $PM(REP)] {
		if [catch { file mkdir $PM(REP) } er] {
			msg "cannot create directory $PM(REP): $er"
			return
		}
	}

	# obtain filename from time stamp
	set fn [string range $ts 0 7]

	set ln "$ts +$col+"

	set sn 0
	foreach a $raw b $cooked {
		if { $b != "" } {
			append ln " $sn=<$a $b>"
		}
		incr sn
	}

	set fn [file join $PM(REP) $fn]

	catch { exec echo $ln >> $fn }
}

###############################################################################

proc xml_read { } {

	global PM

	if [catch { open $PM(XML) r } fd] {
		abt "cannot open $PM(XML)"
	}

	if [catch { read $fd } sm] {
		abt "cannot read $PM(XML): $sm"
	}

	catch { close $fd }

	if [catch { sxml_parse sm } sm] {
		abt "error in $PM(XML): $sm"
	}

	set sm [sxml_child $sm "params"]

	if { $sm == "" } {
		abt "params tag not found in $PM(XML)"
	}

	set it [sxml_child $sm "database"]
	if { $it == "" } {
		# no database
		set PM(DAB) ""
	} else {
		set PM(DAB) [list [sxml_attr $it "name"] \
				  [sxml_txt [sxml_child $it "host"]] \
				  [sxml_txt [sxml_child $it "port"]] \
				  [sxml_txt [sxml_child $it "user"]] \
				  [sxml_txt [sxml_child $it "password"]] ]
	}

	set it [sxml_child $sm "log"]
	if { $it == "" } {
		set PM(LOG) [list "log.txt"]
	} else {
		if [catch { expr [sxml_attr $it "size"] } mls] {
			set mls 1000000
		}

		if [catch { expr [sxml_attr $it "versions"] } lve] {
			set lve 4
		}

		set PM(LOG) [list [sxml_txt $it] $mls $lve]
	}

	set PM(REP) [sxml_txt [sxml_child $sm "repository"]]

	set it [sxml_txt [sxml_child $sm "init"]]
	set PM(AGG) ""
	if { $it != "" } {
		set it [split $it "\n"]
		foreach ln $it {
			lappend PM(AGG) [string trim $ln]
		}
	}
}

###############################################################################

proc snip_cnvrt { v s c } {
#
# Convert a raw sensor value
#
	global SN SC

	if ![info exists SC($c,$s)] {
		# build the cache entry
		set fn 0
		foreach sn [array names SN] {
			foreach a [lrange $SN($sn) 1 end] {
				if { [lindex $a 0] != $s } {
					# not this sensor
					continue
				}
				# scan the collector range
				foreach r [lindex $a 1] {
					set x [lindex $r 0]
					if { $c == $x } {
						set fn 1
						break
					}
					set y [lindex $r 1]
					if { $y != "" && $c > $x && $c <= $y } {
						set fn 1
						break
					}
				}
				if $fn {
					break
				}
			}
			if $fn {
				break
			}
		}
		if $fn {
			set SC($c,$s) [lindex $SN($sn) 0]
		} else {
			set SC($c,$s) ""
		}
	}

	set snip $SC($c,$s)

	if { $snip != "" && ![catch { snip_eval $snip $v } r] } {
		return [format %1.2f $r]
	}
	return ""
}

proc snip_vsn { nm } {
#
# Validate snippet name
#
	global PM

	if { $nm == "" } {
		return 1
	}

	if { [string length $nm] > $PM(SNL) } {
		return 1
	}

	if ![regexp -nocase "^\[a-z\]\[0-9a-z_\]*$" $nm] {
		return 1
	}

	return 0
}

if { [info tclversion] < 8.5 } {

	proc snip_eval { sn val } {
	#
	# Safely evaluates a snippet for a given value
	#
		set in [interp create -safe]

		if [catch {
			# build the script
			set s "set value $val\n$sn\n"
			append s { return $value }
			set s [interp eval $in $s]
		} err] {
			# make sure to clean up
			interp delete $in
			error $err
		}
		interp delete $in
		return $s
	}

} else {

	proc snip_eval { sn val } {
	#
	# Safely evaluates a snippet for a given value
	#
		set in [interp create -safe]
	
		if [catch {
			# make sure we get out of loops
			interp limit $in commands -value 512

			# build the script
			set s "set value $val\n$sn\n"
			append s { return $value }
			set s [interp eval $in $s]
		} err] {
			# make sure to clean up
			interp delete $in
			error $err
		}

		interp delete $in
		return $s
	}

}

proc snip_ucs { cl } {
#
# Unpack and validate a collector set
#
	set v ""

	while 1 {

		set cl [string trimleft $cl]

		if { $cl == "" } {
			# empty is OK, it stands for all
			break
		}

		if [regexp -nocase "^all" $cl] {
			# overrides everything
			return "all"
		}

		if { [string index $cl 0] == "," } {
			# ignore commas
			set cl [string range $cl 1 end]
			continue
		}

		# expect a number or range
		set a ""
		set b ""
		if ![regexp "^(\[0-9\]+) *- *(\[0-9\]+)" $cl ma a b] {
			regexp "^(\[0-9\]+)" $cl ma a
		}

		if { $a == "" } {
			# error
			return ""
		}

		set a [vnum $a 100 65536]
		if { $a == "" } {
			return ""
		}

		if { $b == "" } {
			# single value
			lappend v $a
		} else {
			set b [vnum $b 100 65536]
			if { $b == "" || $b < $a } {
				return ""
			}
			# range
			lappend v "$a $b"
		}

		set cl [string range $cl [string length $ma] end]
	}

	if { $v == "" } {
		return "all"
	} else {
		return $v
	}
}

proc snip_parse { cf } {
#
# Parse conversion snippets read from a file
#
	global SN PM

	set ix 0
	foreach snip $cf {

		# we start from 1 and end up with the correct count
		incr ix

		# snippet name
		set nm [lindex $snip 0]

		# the code
		set ex [lindex $snip 1]

		if [snip_vsn $nm] {
			return "illegal name '$nm' of snippet number $ix,\
				must be no more than $PM(SNL) alphanumeric\
				characters starting with a letter (no spaces)"
		}
		if [info exists SN($nm)] {
			return "duplicate snippet name '$nm', snippet number\
				$ix"
		}

		# this is the list of up to PM(NAS) assignments
		set asgs [lrange $snip 2 end]

		if { [llength $asgs] > $PM(NAS) } {
			return "too many (> $PM(NAS)) assignments in snippet\
				'$nm' (number $ix)"
		}

		if [catch { snip_eval $ex 1000 } er] {
			return "cannot evaluate snippet $nm (number $ix), $er"
		}

		# we produce an internal representation, which is a name-indexed
		# bunch of lists
		
		set SN($nm) [list $ex]

		set iy 0
		foreach as $asgs {

			incr iy

			# this is a sensor number (small)
			set ss [lindex $as 0]
			set se [vnum $ss 0 $PM(SPN)]

			if { $se == "" } {
				return "illegal sensor number '$ss' in\
					assignment $iy of snippet '$nm'"
			}

			# this is a set of collectors which consists of
			# individual numbers and/or ranges
			set ss [lindex $as 1]
			set sv [snip_ucs $ss]

			if { $sv == "" } {
				return "illegal collector range '$ss' in\
					 assignment $iy of snippet '$nm'"
			}
			if { $sv == "all" } {
				set sv ""
			}
			lappend SN($nm) [list $se $sv]
		}
	}
	return ""
}

proc snip_icache { } {
#
# Invalidate snippet cache
#
	global SC

	array unset SC
}

proc snip_read { } {
#
# Read snippets from file
#
	global PM

	# make sure there is no cache
	snip_icache

	set fn $PM(COB)

	if [catch { open $fn "r" } fd] {
		abt "cannot open the snippets file $fn"
	}

	if [catch { read $fd } cf] {
		catch { close $fd }
		abt "cannot read from $fn: $cf"
	}

	catch { close $fd }

	set er [snip_parse $cf]

	if { $er != "" } {
		abt "cannot parse snippets: $er"
	}
}

###############################################################################

proc init_aggregator { } {

	global PM

	set apm $PM(AGG)

	if { $apm == "" } {
		return
	}

	foreach ln $apm {
		if { [string first "%T" $ln] >= 0 } {
			set ts [clock format [clock seconds] -format \
				"%Y-%m-%d %H:%M:%S"]
			regsub -all "%T" $ln $ts ln
		}
		uart_write $ln
	}
}

proc loop { } {
#
# The main loop
#
	global Uart Turn

	while 1 {

		init_aggregator

		after 60000 { incr Turn }
		vwait Turn
	}
}

###############################################################################

set PM(COB) "snippets.dat"
set PM(XML) "params.xml"
set PM(SNL) 32
set PM(NAS) 4
set PM(SPN) 10

###############################################################################

xml_read
log_open [lindex $PM(LOG) 0] [lindex $PM(LOG) 1] [lindex $PM(LOG) 2]
snip_read

set Uart(DEV) [lindex $argv 0]
set Uart(PAR) "19200,n,8,1"

set Turn 0

uart_init input_line

loop
