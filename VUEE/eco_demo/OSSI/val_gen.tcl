#!/bin/sh
###########################\
exec tclsh "$0" "$@"

set	Files(SMAP)		"sensors.xml"

set	Agent(DPORT)		4443
set	Agent(DEV)		"localhost:4443"

###############################################################################

#
# Mini XML parser
#

proc sxml_string { s } {
#
# Extract a possibly quoted string
#
	upvar $s str

	if { [sxml_space str] != "" } {
		error "illegal white space"
	}

	set c [string index $str 0]
	if { $c == "" } {
		error "empty string illegal"
	}

	if { $c != "\"" } {
		# no quote; this is formally illegal in XML, but let's be
		# pragmatic
		regexp "^\[^ \t\n\r\>\]+" $str val
		set str [string range $str [string length $val] end]
		return [sxml_unescape $val]
	}

	# the tricky way
	if ![regexp "^.(\[^\"\]*)\"" $str match val] {
		error "missing \" in string"
	}
	set str [string range $str [string length $match] end]

	return [sxml_unescape $val]
}

proc sxml_unescape { str } {
#
# Remove escapes from text
#
	regsub -all "&amp;" $str "\\&" str
	regsub -all "&quot;" $str "\"" str
	regsub -all "&lt;" $str "<" str
	regsub -all "&gt;" $str ">" str
	regsub -all "&nbsp;" $str " " str

	return $str
}

proc sxml_space { s } {
#
# Skip white space
#
	upvar $s str

	if [regexp -indices "^\[ \t\r\n\]+" $str ix] {
		set ix [lindex $ix 1]
		set match [string range $str 0 $ix]
		set str [string range $str [expr $ix + 1] end]
		return $match
	}

	return ""
}

proc sxml_ftag { s } {
#
# Find and extract the first tag in the string
#
	upvar $s str

	set front ""

	while 1 {
		# locate the first tag
		set ix [string first "<" $str]
		if { $ix < 0 } {
			set str "$front$str"
			return ""
		}
		append front [string range $str 0 [expr $ix - 1]]
		set str [string range $str $ix end]
		# check for a comment
		if { [string range $str 0 3] == "<!--" } {
			# skip it
			set ix [string first "-->" $str]
			if { $ix < 0 } {
				error "unterminated comment: [string range \
					$str 0 15]"
			}
			incr ix 3
			set str [string range $str $ix end]
			continue
		}
		set et ""
		if [regexp -nocase "^<(/)?\[a-z_\]" $str ix et] {
			# this is a tag
			break
		}
		# skip the thing and keep going
		append front "<"
		set str [string range $str 1 end]
	}


	if { $et != "" } {
		set tm 1
	} else {
		set tm 0
	}

	if { $et != "" } {
		# terminator, skip the '/', so the text is positioned at the
		# beginning of keyword
		set ix 2
	} else {
		set ix 1
	}

	# starting at the keyword
	set str [string range $str $ix end]

	if ![regexp -nocase "^(\[a-z0-9_\]+)(.*)" $str ix kwd str] {
		# error
		error "illegal tag: [string range $str 0 15]"
	}

	set kwd [string tolower $kwd]

	# decode any attributes
	set attr ""
	array unset atts

	while 1 {
		sxml_space str
		if { $str == "" } {
			error "unterminated tag: <$et$kwd"
		}
		set c [string index $str 0]
		if { $c == ">" } {
			# done
			set str [string range $str 1 end]
			# term preceding_text keyword attributes
			return [list $tm $front $kwd $attr]
		}
		# this must be a keyword
		if ![regexp -nocase "^(\[a-z\]\[a-z0-9_\]*)=" $str match atr] {
			error "illegal attribute: <$et$kwd ... [string range \
				$str 0 15]"
		}
		set atr [string tolower $atr]
		if [info exists atts($attr)] {
			error "duplicate attribute: <$et$kwd ... $atr"
		}
		set atts($atr) ""
		set str [string range $str [string length $match] end]
		if [catch { sxml_string str } val] {
			error "illegal attribute value: \
				<$et$kwd ... $atr=[string range $str 0 15]"
		}
		lappend attr [list $atr $val]
	}
}

proc sxml_advance { s kwd } {
#
# Returns the text + the list of children for the current tag
#
	upvar $s str

	set txt ""
	set chd ""

	while 1 {
		# locate the nearest tag
		set tag [sxml_ftag str]
		if { $tag == "" } {
			# no more
			if { $kwd != "" } {
				error "unterminated tag: <$kwd ...>"
			}
			return [list "$txt$str" $chd]
		}

		set md [lindex $tag 0]
		set fr [lindex $tag 1]
		set kw [lindex $tag 2]
		set at [lindex $tag 3]

		append txt $fr

		if { $md == 0 } {
			# opening
			set cl [sxml_advance str $kw]
			set tc [list $kw [lindex $cl 0] $at [lindex $cl 1]]
			lappend chd $tc
		} else {
			# closing (must be ours)
			if { $kw != $kwd } {
				error "mismatched tag: <$kwd ...> </$kw>"
			}
			# we are done
			return [list $txt $chd]
		}
	}
}

proc sxml_parse { s } {
#
# Builds the XML tree from the provided string
#
	upvar $s str

	set v [sxml_advance str ""]

	return [list root [lindex $v 0] "" [lindex $v 1]]
}

proc sxml_name { s } {

	return [lindex $s 0]
}

proc sxml_txt { s } {

	return [string trim [lindex $s 1]]
}

proc sxml_attr { s n } {

	set al [lindex $s 2]
	set n [string tolower $n]
	foreach a $al {
		if { [lindex $a 0] == $n } {
			return [lindex $a 1]
		}
	}
	return ""
}

proc sxml_children { s { n "" } } {

	set cl [lindex $s 3]

	if { $n == "" } {
		return $cl
	}

	set res ""

	foreach c $cl {
		if { [lindex $c 0] == $n } {
			lappend res $c
		}
	}

	return $res
}

proc sxml_child { s n } {

	set cl [lindex $s 3]

	foreach c $cl {
		if { [lindex $c 0] == $n } {
			return $c
		}
	}

	return ""
}

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

	error $m
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

	abt "connection failed: timeout"
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
		agent_tmout
	}

	# wait for a reply
	fileevent $sok readable "agent_sokin"

	agent_stmout 10000 agent_tmout

	vwait Agent(READY)
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

		if ![regexp "(\[0-9\]+)\[^0-9\]+(\[0-9\]+)" $de junk lo hi] {
			set lo 0
			set hi 32768
		}

		if ![info exists NID($no)] {
			continue
		}

		set ci $NID($no)

		lappend SBN($ci) [list $lo $hi]
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
		set pv [lindex $se 2]

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

		lappend re [list $lo $hi $pv]
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
		agent_write "S $ix [expr int([lindex $sn 2])]"
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
