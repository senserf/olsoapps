{

###############################################################################
#
# A single list element is a list of these items:
#
#	- command keyword
#	- list
#		- subcommand keyword
#		- list
#			- opcode
#			- argument tage
#			- ...
#
#	w - word, b - byte (default is byte)
#	% - provided by user
#      *n - repeat the tail up to n times, possibly zero, depending on user
#	    data availability
#
###############################################################################

set PLUG(CMDS) {

	{ "get"   { "address" { 0x11 w0 w1 } }
		  { "master"  { 0x11 w0 w2 } }
		  { "fwd"     { 0x11 w0 w3 } }
		  { "det"     { 0x11 w0 w4 } }
		  { "tal"     { 0x14 w0 b% } }
	}

	{ "set"   { "address" { 0x12 w0 w1 w% } }
		  { "master"  { 0x12 w0 w2 w% } }
		  { "tal"     { 0x13 wn b% *20 w% b% } }
	}

	{ "reset" { "tal"     { 0x15 wn } }
	}
}

###############################################################################
#
#	- list
#		- opcode
#		- argument tag
#		- ...
#	- symbolic name of the reponse
#	- function body to format the output
#	- optional ACK or NAK, if the message is to be acked
#
#	argument tag w or b means word or byte to skip
#	followed by a value -> expected value
#	followed by a variable name (e.g., wx) -> to be passed to the function
#
###############################################################################

set PLUG(RESP) {

	{ { 0x91 w w1 wx } nodeaddress { return [format "Node Id = %1d" $x] } }

	{ { 0x91 w w2 wx } masterid { return [format "Master Id = %1d" $x] } }

	{ { 0x91 w w3 wx } fwdoffstatus
		{
			if $x {
				set x "OFF"
			} else {
				set x "ON"
			}
			return "Forwarding = $x"
		}
	}

	{ { 0x91 w w4 wx ws ba bh } dethrone
		{
			if { $x == 0 } {
				return "No dethroning attempts"
			} else {
				return "$a attempts, $s sec ago, $h hops away"
			}
		}
	}

	{ { 0x92 w bx  } setreply
		{
			if { $x == 6 } {
				return "SET OK"
			} else {
				return "SET FAILED"
			}
		}
	}

	{ { 0x01 wn wp wt bi bg bs bv br bp bj be } tagevent
		{
			set res "Event, peg = $p, tag = $t, adr = $j, tst = $s:"
			set retr [expr { ($g >> 4) & 7 }]
			if { $g & 0x80 } {
				set noack " ?"
			} else {
				set noack ""
			}
			if { $g & 0x0f } {
				set g "G"
			} else {
				set g "L"
			}
			append res "\n    button:   $i ($g $retr$noack)"
			set v [expr { ((($v << 3) + 1000.0) / 4095.0) * 5.0 }]
			append res "\n    voltage:  [format %4.2f $v]"
			append res "\n    txpower:  $p"
			append res "\n    age:      $e"
			return $res
		}
		{ 0x81 w1 b6 }
	}

	{ { 0x94 w bx * wt bb } taldata
		{
			if { $args == "" } {
				return "Empty TAL"
			}
			set res "TAL = <$x>"
			foreach { t b } $args {
				append res " <$t-$b>"
			}
			return $res
		}
	}

	{ { 0x93 w bx  } settalreply
		{
			if { $x == 6 } {
				return "SET TAL OK"
			} else {
				return "SET TAL FAILED"
			}
		}
	}			

	{ { 0x95 w bx  } resettalreply
		{
			if { $x == 6 } {
				return "RESET TAL OK"
			} else {
				return "RESET TAL FAILED"
			}
		}
	}			
}

###############################################################################

proc issue_command { code vals } {

	set out ""
	foreach a $vals {

		if ![regexp -nocase {^([wb])(.+)} $a j d a] {
			set d b
		}

		if [catch { expr { int($a) } } val] {
			error "illegal numerical value $a"
		}

		lappend out [expr { $val & 0xff }]

		if { $d == "w" } {
			lappend out [expr { ($val >> 8) & 0xff }]
		}
	}

	set lf [expr { [llength $out] + 5 }]

	set out [concat [list $code] [list $lf] $out]

	plug_dmp $out "-->"

	pt_outln $out
}

proc run_command { inp } {

	global PLUG

	set vls ""

	if [regexp {^([[:alpha:]]+)[[:blank:]]+([[:alpha:]]+)(.*)} \
		$inp jnk com sub arg] {

		set com [string tolower $com]
		set sub [string tolower $sub]

		set bad 1
		foreach c $PLUG(CMDS) {
			if { [string first $com [lindex $c 0]] == 0 } {
				set bad 0
				set com [lindex $c 0]
				break
			}
		}

		if $bad {
			return "Illegal command: $com"
		}

		set bad 1
		foreach a [lrange $c 1 end] {
			if { [string first $sub [lindex $a 0]] == 0 } {
				set bad 0
				set sub [lindex $a 0]
				break
			}
		}

		if $bad {
			return "Illegal subcommand for $com: $sub"
		}

		set arp [lindex $a 1]
		set cod [lindex $arp 0]
		set arp [lrange $arp 1 end]
		set rep 0
		set inx 0

		foreach a $arp {

			incr inx

			if [regexp {^\*([[:digit:]]*)} $a jnk rep] {
				# repeat count
				set arp [lrange $arp $inx end]
				if { $rep == 0 } {
					set rep 999
				}
				break
			}

			set d [string index $a 0]
			set r [string range $a 1 end]

			if { $r != "%" } {
				# must be a number
				if { $r == "n" } {
					# node Id
					if { $PLUG(NODEID) == "" } {
						set a 0
					} else {
						set a $PLUG(NODEID)
					}
					set a "$d$a"
				}
				lappend vls $a
				continue
			}

			# argument

			set arg [string trimleft $arg]
			if [catch { plug_parse_number arg } num] {
				return "Illegal argument, $arg, $num"
			}

			lappend vls "$d$num"
		}

		while { $rep } {

			set arg [string trimleft $arg]

			if { $arg == "" } {
				break
			}

			foreach a $arp {

				set d [string index $a 0]
				set r [string range $a 1 end]

				if { $r != "%" } {
					# must be a number
					lappend vls $a
					continue
				}

				# argument

				if [catch { plug_parse_number arg } num] {
					return "Illegal argument, $arg, $num
				}

				lappend vls "$d$num"
			}

			incr rep -1
		}

	} else {

		if [regexp -nocase {^x} $inp] {

			# line in hex

			set inp [string range $inp 1 end]

			while 1 {

				set inp [string trimleft $inp]
				if { $inp == "" } {
					break
				}

				set dd [string range $inp 0 1]
				set inp [string range $inp 2 end]

				if [catch { expr 0x$dd } val] {
					return "Bad input: $dd is not a pair\
						of hex digits"
				}
				lappend vls $val
			}

		} else {

			# free style numbers/expressions
			while 1 {
				set inp [string trimleft $inp]
				if { $inp == "" } {
					break
				}
			
				if [catch { plug_parse_number inp } val] {
					return "Bad input: $inp ($val)"
				}

				if [catch { plug_validate_int $val 0 255 } n] {
					return "Illegal byte value: $val"
				}
				lappend vls $n
			}
		}

		if { [llength $vls] < 2 } {
			return "Need at least two bytes"
		}

		set cod [lindex $vls 0]
		set vls [lrange $vls 1 end]
	}

	if [catch { issue_command $cod $vls } err] {
		return $err
	}

	return ""
}

proc response_match { r op bs } {
#
# Checks if the response pattern matches the received node response
#
	set cp [lindex $r 0]

	if { [lindex $cp 0] != $op } {
		# opcode mismatch
		return ""
	}

	set vls ""
	set fal ""

	set inx 0
	set rep 0
	set cp [lrange $cp 1 end]

	foreach arg $cp {

		incr inx

		set d [string index $arg 0]
		set arg [string range $arg 1 end]

		if { $d == "*" } {
			# repeat
			set cp [lrange $cp $inx end]
			set rep 1
			break
		}

		if { $bs == "" } {
			# no match
			return ""
		}

		set v [lindex $bs 0]
		set bs [lrange $bs 1 end]

		if { $d == "w" } {
			if { $bs == "" } {
				return ""
			}
			set v [expr { $v | ([lindex $bs 0] << 8) }]
			set bs [lrange $bs 1 end]
		}

		if { $arg == "" } {
			# just skip
			continue
		}

		if [catch { expr { int($arg) } } var] {
			# not an expression, assume variable name; add the name
			# to the function's list
			lappend fal $arg
			lappend vls $v
			continue
		}

		# an expression, the values must much
		if { $v != $var } {
			return ""
		}
	}

	# first round done, check if there's a repeat
	if $rep {
		# the subsequent values will form a list
		lappend fal "args"

		while { $bs != "" } {

			foreach arg $cp {

				set d [string index $arg 0]
				set arg [string range $arg 1 end]

				if { $bs == "" } {
					# no match
					return ""
				}

				set v [lindex $bs 0]
				set bs [lrange $bs 1 end]

				if { $d == "w" } {
					if { $bs == "" } {
						return ""
					}
					set v \
					  [expr { $v | ([lindex $bs 0] << 8) }]
					set bs [lrange $bs 1 end]
				}

				if { $arg == "" } {
					# just skip
					continue
				}

				if [catch { expr { int($arg) } } var] {
					lappend vls $v
					continue
				}

				# an expression, the values must much
				if { $v != $var } {
					return ""
				}
			}
		}
	}

	# match, return a list:
	#	- id
	#	- list of values
	#	- function (for apply) which is a list: arglist procbody
	#

	return [list [lindex $r 1] $vls [list $fal [lindex $r 2]]]
}

proc show_response { m } {
#
# Shows a formatted response
#
	set arg [lindex $m 1]
	set fun [lindex $m 2]

	set res [apply $fun {*}$arg]

	pt_tout $res
}

###############################################################################

proc plug_init { ags } {

	global PLUG

	set PLUG(DUMP) 0

	while { $ags != "" } {
		if { [string index $ags 0] == "D" } {
			incr PLUG(DUMP)
		}
		set ags [string range $ags 1 end]
	}

	# node Id (unknown yet)
	set PLUG(NODEID) ""

	# tag association list to send
	set PLUG(TAL) ""

	# pending command
	set PLUG(PENDING) ""

	# last set reply status
	set PLUG(LASTSRP) ""

	# number of failures
	set PLUG(FAILURES) 0

	# the renesas thread
	set PLUG(RSTATE) 0
	set PLUG(CB,RENESAS) [after 1 plug_renesas]
	set PLUG(CB,TIMEOUT) ""

	# Retransmission interval
	set PLUG(RTIME)	100
	# Longer interval for persistent polling for Node Id
	set PLUG(LTIME) 500
	# Poll time (60 seconds)
	set PLUG(PTIME) 60000
	# Number of retries
	set PLUG(RETRIES) 3
}

proc plug_close { } {

	global PLUG

	# kill all callbacks
	foreach c [array names PLUG "CB,*"] {
		catch { after cancel $PLUG($c) }
	}
	array unset PLUG
}

proc plug_inppp_b { in } {

	upvar $in inp

	set inp [string trim $inp]

	process_input $inp 0
	return 0
}

proc process_input { inp f } {
#
# f - file flag, i.e., running commands from file
#
	global PLUG

	if { $f == 0 } {
		# echo
		pt_tout $inp
	}

	if { $f == 0 &&
	    [regexp -nocase {^f[[:alpha:]]*[[:blank:]]+(.+)} $inp mat fn] } {
		# commands from file
		if [catch { open $fn } fd] {
			pt_tout "Cannot open file $fn, $fd"
			return 1
		}
		if [catch { read $fd } cmds] {
			catch { close $fd }
			pt_tout "Cannot read file $fn, $cmds"
			return 1
		}
		foreach cmd [split cmds "\n"] {
			set cmd [string trim $cmd]
			if { $cmd == "" || [string index $cmd 0] == "#" } {
				continue
			}
			if [process_input $cmd 1] {
				# error
				pt_tout "File processing aborted!"
				break
			}
		}
		return 0
	}

	if [regexp -nocase {^d[[:alpha:]]*} $inp mat] {
		# dump
		set inp [string trimleft \
			[string range $inp [string length $mat] end]]
		if [catch { expr { int($inp) } } df] {
			pt_tout "Number expected!"
			return 1
		}
		set PLUG(DUMP) $df
		return 0
	}

	if [regexp -nocase {^t[[:alpha:]]*} $inp mat] {
		# set association list for periodic transmission to the node
		set inp [string trimleft \
			[string range $inp [string length $mat] end]]

		if { $inp == "" } {
			# reset
			set PLUG(TAL) ""
			return 0
		}

		if { $inp == "?" } {
			pt_tout "TAL = [join $PLUG(TAL)]"
			return 0
		}
			
		if [catch { plug_parse_number inp } val] {
			pt_tout "Bad index, number expected!"
			return 1
		}
		if [catch { plug_validate_int $val 0 31 } inx] {
			pt_tout "Index out of range, $val, must be between 0\
				and 31!"
			return 1
		}
		set tl [list $inx]
		while 1 {
			if [catch { plug_parse_number inp } val] {
				break
			}
			if [catch { plug_validate_int $val 2 65534 } tag] {
				pt_tout "Illegal tag Id, $val, must be between\
					2 and 65534!"
				return 1
			}
			if [catch { plug_parse_number inp } val] {
				pt_tout "Button mask expected!"
				return 1
			}
			if [catch { plug_validate_int $val 0 255 } bma] {
				pt_tout "Illegal button mask, $val, must be\
					between 0 and 255!"
				return 1
			}
			lappend tl $tag
			lappend tl $bma
		}

		if { [llength $tl] == 1 } {
			pt_tout "Empty tag list!"
			return 1
		}

		set PLUG(TAL) $tl
		plug_timeout_clear
		plug_timeout_gooff

		return 0
	}

	set err [run_command $inp]

	if { $err != "" } {
		pt_tout "$err!"
		return 1
	}

	return 0
}

proc plug_outpp_b { in } {

	upvar $in inp
	global PLUG

	set bts ""
	set ln 0
	foreach v $inp {
		lappend bts "0x$v"
		incr ln
	}

	if { $ln < 5 } {
		# garbage
		plug_dmp $bts "<-U"
		return 0
	}

	set opc [lindex $bts 0]
	set tln [lindex $bts 1]
	set ags [lrange $bts 2 end]

	if { $ln != [expr { $tln - 3 }] } {
		# bad length
		plug_dmp $bts "<-U"
		return 0
	}

	# decode the command
	set m ""
	foreach r $PLUG(RESP) {
		set m [response_match $r $opc $ags]
		if { $m != "" } {
			break
		}
	}

	if { $m == "" } {
		# unrecognized
		plug_dmp $bts "<-U"
		return 0
	}

	set tp [lindex $m 0]

	if { $PLUG(PENDING) == $tp } {
		set PLUG(PENDING) ""
		plug_dmp $bts "<-W"
	} else {
		plug_dmp $bts "<-R"
	}

	if { $tp == "nodeaddress" } {
		# the node address
		set ni [lindex [lindex $m 1] 0]
		if { $PLUG(NODEID) != $ni } {
			set PLUG(NODEID) $ni
		}
		show_response $m
		return 0
	} elseif { $tp == "setreply" } {
		# report only if change
		set ni [lindex [lindex $m 1] 0]
		if { $ni != $PLUG(LASTSRP) } {
			# not needed ?
			set PLUG(LASTSRP) $ni
		}
		show_response $m
		return 0
	}

	set a [lindex $r 3]
	if { $a != "" } {
		# reply
		issue_command [lindex $a 0] [lrange $a 1 end]
	}
		
	show_response $m
	return 0
}
		
###############################################################################
###############################################################################

proc plug_timeout_start { del } {

	global PLUG

	if { $PLUG(CB,TIMEOUT) != "" } {
		catch { after cancel $PLUG(CB,TIMEOUT) }
	}

	set PLUG(CB,TIMEOUT) [after $del plug_timeout_gooff]
}

proc plug_timeout_gooff { } {

	global PLUG

	set PLUG(PENDING) $PLUG(PENDING)
}

proc plug_timeout_clear { } {

	global PLUG

	if { $PLUG(CB,TIMEOUT) != "" } {
		catch { after cancel $PLUG(CB,TIMEOUT) }
		set PLUG(CB,TIMEOUT) ""
	}
}

###############################################################################

proc plug_renesas { } {
#
# This is a callback acting as a background process periodically polling the
# node for status
#
	global PLUG

	while 1 {

###	#######################################################################
###	#######################################################################

	switch $PLUG(RSTATE) {

	0 {
		# start poll for node info; TR <- number of tries
		set PLUG(TR) 1
		set err [run_command "get address"]
		if { $err != "" } {
			pt_tout "ERROR: $err, Renesas callback terminates"
			return
		}
		set PLUG(PENDING) "nodeaddress"
		set PLUG(RSTATE) 1
		plug_timeout_start $PLUG(RTIME)
		vwait PLUG(PENDING)
	}

	1 {
		plug_timeout_clear
		if { $PLUG(PENDING) != "" && $PLUG(TR) < $PLUG(RETRIES) } {
			set err [run_command "get address"]
			if { $err != "" } {
				pt_tout "ERROR: $err, Renesas callback\
					terminates"
			}
			incr PLUG(TR)
			plug_timeout_start $PLUG(RTIME)
		} elseif { $PLUG(NODEID) == "" } {
			# still don't know the node Id, keep polling
			set PLUG(RSTATE) 0
			plug_timeout_start $PLUG(LTIME)
		} elseif { $PLUG(TAL) == "" } {
			# no TAL to send
			set PLUG(RSTATE) 0
			plug_timeout_start $PLUG(PTIME)
		} else {
			set PLUG(TR) 1
			set err [run_command "set tal [join $PLUG(TAL)]"]
			if { $err != "" } {
				pt_tout "Error sending set tal, $err,\
					TAL cleared!"
				set PLUG(TAL) ""
				set PLUG(RSTATE) 0
				plug_timeout_start $PLUG(PTIME)
			} else {
				# OK
				set PLUG(PENDING) "settalreply"
				set PLUG(RSTATE) 2
				plug_timeout_start $PLUG(RTIME)
			}
		}
		vwait PLUG(PENDING)
	}

	2 {
		plug_timeout_clear
		if { $PLUG(PENDING) != "" && $PLUG(TR) < $PLUG(RETRIES) &&
	 	    $PLUG(TAL) != "" } {
			set err [run_command "set tal [join $PLUG(TAL)]"]
			if { $err != "" } {
				pt_tout "Error sending set tal, $err,\
					tal cleared!"
				set PLUG(TAL) ""
				set PLUG(RSTATE) 0
				plug_timeout_start $PLUG(PTIME)
			} else {
				incr PLUG(TR)
				plug_timeout_start $PLUG(RTIME)
			}
		} else {
			# close the loop
			set PLUG(RSTATE) 0
			plug_timeout_start $PLUG(PTIME)
		}
		vwait PLUG(PENDING)
	}
	}

###	#######################################################################
###	#######################################################################
	}
}
			
proc plug_validate_int { n { min "" } { max "" } } {
#
# Validate an integer value
#
	set n [string tolower [string trim $n]]
	if { $n == "" } {
		error "empty string"
	}

	if { [string first "." $n] >= 0 || [string first "e" $n] >= 0 } {
		error "not an integer number"
	}

	if [catch { expr { int($n) } } n] {
		error "not a number"
	}

	if { $min != "" && $n < $min } {
		error "must not be less than $min"
	}

	if { $max != "" && $n > $max } {
		error "must not be greater than $max"
	}

	return $n
}

proc plug_parse_number { l } {
#
# Parses something that should amount to a number, which can be an expression
#
	upvar $l line

	if [regexp {^[[:space:]]*"} $line] {
		# a special check for a string which fares fine as an
		# expression, but we don't want it; if it doesn't open the
		# expression, but occurs inside, that's fine
		error "illegal \" at the beginning of expression"
	}

	set ll [string length $line]
	set ix $ll

	while 1 {

		incr ix -1
		if { $ix < 0 } {
			# failure
			error "illegal number"
		}

		if ![catch { expr { int([string range $line 0 $ix]) } } res] {
			# found, remove the match
			set line [string range $line [expr $ix + 1] end]
			return $res
		}
	}
}

proc plug_toh { lv } {

	set res ""

	foreach v $lv {
		lappend res [format "%02x" $v]
	}

	return [join $res]
}

proc plug_dmp { lv t { lev 1 } } {

	global PLUG

	if { $PLUG(DUMP) >= $lev } {
		if { $t == "" } {
			set t "-"
		}
		pt_tout "${t}: \[[plug_toh $lv]\]"
	}
}

###############################################################################

}
