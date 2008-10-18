#!/bin/sh
###########################\
exec tclsh "$0" "$@"

set	Files(SMAP)		"sensors.xml"

set	Agent(DPORT)		4443
set	Agent(DEV)		"localhost:4443"

###############################################################################

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

###############################################################################

package require xml 1.0

###############################################################################

#
# Error detectors and handlers
#

proc nannin { n } {
#
# Not A NonNegative Integer Number
#
	upvar $n num

	if [catch { expr $num } num] {
		return 1
	}
	set t $num
	if [catch { incr t }] {
		return 1
	}
	if { $num < 0 } {
		return 1
	}
	return 0
}

proc napin { n } {
#
# Not A Positive Integer Number
#
	upvar $n num

	if [catch { expr $num } num] {
		return 1
	}
	set t $num
	if [catch { incr t }] {
		return 1
	}
	if { $num <= 0 } {
		return 1
	}
	return 0
}

proc nan { n } {
#
# Not a number
#
	if [catch { expr $n }] {
		return 1
	}
	return 0
}

###############################################################################

proc msg { m } {

	puts $m
}

proc abt { m } {

	global Agent

	set Agent(READY) 0

	error $m
}

proc bgerror { m} {

	error "in background: $m"
}

#
# Agent functions
#

proc abinS { s h } {
#
# append one short int to string s (in network order)
#
	upvar $s str
	append str [binary format S $h]
}

proc abinI { s l } {
#
# append one 32-bit int to string s (in network order)
#
	upvar $s str
	append str [binary format I $l]
}

proc dbinB { s } {
#
# decode one binary byte from string s
#
	upvar $s str
	if { $str == "" } {
		return -1
	}
	binary scan $str c val
	set str [string range $str 1 end]
	return [expr ($val & 0x000000ff)]
}

proc agent_tmout { } {

	global Agent

	set Agent(READY) 0
	
}

proc agent_ctmout { } {

	global Agent

	if [info exists Agent(TO)] {
		after cancel $Agent(TO)
		unset Agent(TO)
	}
}

proc agent_stmout { del fun } {

	global Agent

	if [info exists Agent(TO)] {
		# cancel the previous timeout
		agent_ctmout
	}

	set Agent(TO) [after $del $fun]
}

proc agent_sokin { } {
#
# Initial read: VUEE handshake
#
	global Agent

	agent_ctmout

	if [catch { read $Agent(FD) 1 } res] {
		# disconnection
		abt "connection failed: $res"
	}

	if { $res == "" } {
		abt "connection closed by VUEE"
	}

	set code [dbinB res]

	if { $code != 129 } {
		abt "connection rejected by VUEE, code $code"
	}

	# so far, so good

	set Agent(READY) 1
}

proc agent_vuee { node ser pp } {
#
# Connect to VUEE
#
	global Agent
	
	msg "Connecting to a VUEE model: node $node, host $ser, port $pp ..."

	if [catch { socket -async $ser $pp } sok] {
		abt "connection failed: $sok"
	}

	if [catch { fconfigure $sok -blocking 0 -buffering none -translation \
	    binary -encoding binary } err] {
		abt "connection failed: $err"
	}

	set Agent(FD) $sok

	# send the request
	set rqs ""
	abinS rqs 0xBAB4

	abinS rqs 7
	abinI rqs $node

	if [catch { puts -nonewline $sok $rqs } err] {
		abt "connection failed: $err"
	}

	# wait for it to be accepted

	for { set i 0 } { $i < 10 } { incr i } {
		if [catch { flush $sok } err] {
			abt "connection failed: $err"
		}
		if ![fblocked $sok] {
			break
		}
		after 1000
	}

	if { $i == 10 } {
		abt "connection failed: timeout"
	}

	# wait for a reply
	fileevent $sok readable "agent_sokin"

	agent_stmout 10000 agent_tmout

	vwait Agent(READY)

	if { $Agent(READY) == 0 } {
		catch { close $sok }
		abt "connection failed: timeout"
	}
}

proc agent_init { rfun node } {

	global Agent

	if [info exists Agent(FD)] {
		catch { close $Agent(FD) }
		unset Agent(FD)
	}

	# socket connection
	set port $Agent(DPORT)
	if ![regexp "(.+):(.+)" $Agent(DEV) j server port] {
		set server $Agent(DEV)
	}

	agent_vuee $node $server $port

	set Agent(READY) 1
	set Agent(BF) ""
	# OK, start reading
	fileevent $Agent(FD) readable "agent_read"
	set Agent(RF) $rfun
}

proc agent_read { } {

	global Agent

	if $Agent(MODE) {

		# socket

		if [catch { read $Agent(FD) } chunk] {
			# disconnection
			msg "connection broken by VUEE: $chunk"
			catch { close $Agent(FD) }
			set Agent(READY) 0
			return
		}

		if [eof $Agent(FD)] {
			msg "connection closed by VUEE"
			catch { close $Agent(FD) }
			set Agent(READY) 0
			return
		}

	} else {

		# regular UART

		if [catch { read $Agent(FD) } chunk] {
			# ignore errors
			return
		}
	}
		
	if { $chunk == "" } {
		# nothing available
		return
	}

	agent_ctmout

	append Agent(BF) $chunk

	while 1 {
		set el [string first "\n" $Agent(BF)]
		if { $el < 0 } {
			break
		}
		set ln [string range $Agent(BF) 0 $el]
		incr el
		set Agent(BF) [string range $Agent(BF) $el end]
		$Agent(RF) $ln
	}

	# if the buffer is nonempty, erase it on timeout

	if { $Agent(BF) != "" } {
		agent_stmout 1000 agent_erase
	}
}

proc agent_erase { } {

	global Agent

	unset Agent(TO)
	set Agent(BF) ""
}

proc agent_write { w } {

	global Agent

	if [catch {
		puts -nonewline $Agent(FD) "$w\r\n" 
		flush $Agent(FD)
	} err] {
		if $Agent(MODE) {
			msg "connection closed by VUEE"
			catch { close $Agent(FD) }
			set Agent(READY) 0
		}
	}
}

proc agent_close { } {

	global Agent

	catch { close $Agent(FD) }
	unset Agent(FD)
}

###############################################################################

proc read_map { } {
#
# Read the map file, build a list of nodes and their sensors
#
	global Files SBN NNodes AggOff

	if [catch { open $Files(SMAP) r } fd] {
		abt "cannot open map file $Files(SMAP): $fd"
		return 0
	}

	if [catch { read $fd } sm] {
		abt "cannot read map file $Files(SMAP): $sm"
		return 0
	}

	catch { close $fd }

	if [catch { sxml_parse sm } sm] {
		abt "map file error, $sm"
		return 0
	}

	set sm [sxml_child $sm "map"]
	if { $sm == "" } {
		abt "map tag not found in the map file"
		return 0
	}

	set cv [sxml_children [sxml_child $sm "aggregators"] "node"]

	# assumes aggregators precede collectors in VUEE node numbering
	set AggOff [llength $cv]

	set colls ""		;# collectors

	# collect node info: collectors #######################################

	set cv [sxml_children [sxml_child $sm "collectors"] "node"]
	if { $cv == "" } {
		abt "no collectors in map file"
		return 0
	}

	set ix 0
	foreach c $cv {

		set ci $ix
		incr ix

		set sn [sxml_attr $c "id"]
		if [napin sn] {
			abt "illegal or missing id in collector $ix"
			return 0
		}

		set SBN($ci) ""
		set NID($sn) $ci
	}

	set NNodes $ix

	# collect sensor info #################################################

	set cv [sxml_children [sxml_child $sm "sensors"] "sensor"]

	foreach c $cv {

		set no [sxml_attr $c "node"]
		set de [sxml_attr $c "emu"]
		set lo 0
		set hi 32768
		set ix ""

		if [regexp "^(\[0-9\])+/" $de junk ix] {
			set ix [expr $ix]
			set de [string range $de [string length $junk] end]
		}

		regexp "(\[0-9\]+)\[^0-9\]+(\[0-9\]+)" $de junk lo hi

		if ![info exists NID($no)] {
			continue
		}

		set ci $NID($no)

		lappend SBN($ci) [list $lo $hi $ix]
	}
}

proc genvalues { nn } {

	global SBN

	set no $SBN($nn) 

	set re ""

	foreach se $no {

		set r [expr rand()]

		set lo [lindex $se 0]
		set hi [lindex $se 1]
		set ix [lindex $se 2]
		set pv [lindex $se 3]

		if { $pv == "" } {
			# first time around
			set pv [expr $lo + ($hi - $lo) * $r]
		} else {
			set delta [expr ($hi - $lo) * 0.05]
			if { $pv >= $hi } {
				set delta [expr -$delta]
			} elseif { $pv > $lo } {
				if { [expr rand()] < 0.5 } {
					set delta [expr -$delta]
				}
			}
			set pv [expr $pv + $delta * $r]
			if { $pv > $hi } {
				set pv $hi
			} elseif { $pv < $lo } {
				set pv $lo
			}
		}

		lappend re [list $lo $hi $ix $pv]
	}

	set SBN($nn) $re
}

proc agent_input { ln } {

}

proc update { nn } {

	global SBN AggOff

	set vn [expr $nn + $AggOff]

	if [catch { agent_init agent_input $vn } err] {
		puts "connection to $nn failed: $err"
		return
	}

	set ix 0 

	foreach sn $SBN($nn) {
		set iy [lindex $sn 2]
		if { $iy == "" } {
			set iy $ix
		}
		agent_write "S $iy [expr int([lindex $sn 3])]"
		incr ix
	}

	puts "updated node $vn"
}

read_map

expr srand([clock seconds])

if { $NNodes < 1 } {
	abt "No nodes"
}

for { set i 0 } { $i < $NNodes } { incr i } {

	genvalues $i
	update $i
}

set i 0

while 1 {

	after 10000

	genvalues $i
	update $i

	incr i

	if { $i == $NNodes } {
		set i 0
	}
}
