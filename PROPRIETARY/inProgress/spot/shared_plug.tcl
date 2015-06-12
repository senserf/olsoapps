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
#			- argument tag
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

	{ { 0x01 wn wp wt bi bg bs bv br bq bj be } tagevent
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
			append res "\n    txpower:  $q"
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

proc sh_issue { code vals t } {

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

	sh_dmp $out "-->" $t

	sh_outln $out $t
}

proc sh_command { inp t } {

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
					if { $PLUG($t,NODEID) == "" } {
						set a 0
					} else {
						set a $PLUG($t,NODEID)
					}
					set a "$d$a"
				}
				lappend vls $a
				continue
			}

			# argument

			set arg [string trimleft $arg]
			if [catch { sh_parsnum arg } num] {
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

				if [catch { sh_parsnum arg } num] {
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
			
				if [catch { sh_parsnum inp } val] {
					return "Bad input: $inp ($val)"
				}

				if [catch { sh_valint $val 0 255 } n] {
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

	if [catch { sh_issue $cod $vls $t } err] {
		return $err
	}

	return ""
}

proc sh_rmatch { r op bs } {
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

proc sh_showresp { m t } {
#
# Shows a formatted response
#
	set arg [lindex $m 1]
	set fun [lindex $m 2]

	set res [apply $fun {*}$arg]

	sh_tout $res $t
}

###############################################################################

proc sh_init { ags t } {

	global PLUG

	set PLUG($t,DUMP) 0

	while { $ags != "" } {
		if { [string index $ags 0] == "D" } {
			incr PLUG($t,DUMP)
		}
		set ags [string range $ags 1 end]
	}

	# node Id (unknown yet)
	set PLUG($t,NODEID) ""

	# tag association list to send
	set PLUG($t,TAL) ""

	# pending command
	set PLUG($t,PENDING) ""

	# last set reply status
	set PLUG($t,LASTSRP) ""

	# number of failures
	set PLUG($t,FAILURES) 0

	# the renesas emulation thread
	set PLUG($t,RSTATE) 0
	set PLUG($t,CB,RENESAS) [after 1 "sh_renesas $t"]
	set PLUG($t,CB,TIMEOUT) ""

	# Retransmission interval (these are constants)
	set PLUG(RTIME) 100
	# Longer interval for persistent polling for Node Id
	set PLUG(LTIME) 500
	# Poll time (60 seconds)
	set PLUG(PTIME) 60000
	# Number of retries
	set PLUG(RETRIES) 3
}

proc sh_close { t } {

	global PLUG

	# kill all callbacks
	foreach c [array names PLUG "$t,CB,*"] {
		catch { after cancel $PLUG($c) }
	}
	array unset PLUG "$t,*"
}

proc sh_input { inp f t } {
#
# f - file flag, i.e., running commands from file
#
	global PLUG

	if { $f == 0 } {
		# echo
		sh_tout $inp $t
	}

	if { $f == 0 &&
	    [regexp -nocase {^f[[:alpha:]]*[[:blank:]]+(.+)} $inp mat fn] } {
		# commands from file
		if [catch { open $fn } fd] {
			sh_tout "Cannot open file $fn, $fd" $t
			return 1
		}
		if [catch { read $fd } cmds] {
			catch { close $fd }
			sh_tout "Cannot read file $fn, $cmds" $t
			return 1
		}
		foreach cmd [split cmds "\n"] {
			set cmd [string trim $cmd]
			if { $cmd == "" || [string index $cmd 0] == "#" } {
				continue
			}
			if [sh_input $cmd 1 $t] {
				# error
				sh_tout "File processing aborted!" $t
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
			sh_tout "Number expected!" $t
			return 1
		}
		set PLUG($t,DUMP) $df
		return 0
	}

	if [regexp -nocase {^t[[:alpha:]]*} $inp mat] {
		# set association list for periodic transmission to the node
		set inp [string trimleft \
			[string range $inp [string length $mat] end]]

		if { $inp == "" } {
			# reset
			set PLUG($t,TAL) ""
			return 0
		}

		if { $inp == "?" } {
			sh_tout "TAL = [join $PLUG(TAL)]" $t
			return 0
		}
			
		if [catch { sh_parsnum inp } val] {
			sh_tout "Bad index, number expected!" $t
			return 1
		}
		if [catch { sh_valint $val 0 31 } inx] {
			sh_tout "Index out of range, $val, must be between 0\
				and 31!" $t
			return 1
		}
		set tl [list $inx]
		while 1 {
			if [catch { sh_parsnum inp } val] {
				break
			}
			if [catch { sh_valint $val 2 65534 } tag] {
				sh_tout "Illegal tag Id, $val, must be between\
					2 and 65534!" $t
				return 1
			}
			if [catch { sh_parsnum inp } val] {
				sh_tout "Button mask expected!" $t
				return 1
			}
			if [catch { sh_valint $val 0 255 } bma] {
				sh_tout "Illegal button mask, $val, must be\
					between 0 and 255!"
				return 1
			}
			lappend tl $tag
			lappend tl $bma
		}

		if { [llength $tl] == 1 } {
			sh_tout "Empty tag list!" $t
			return 1
		}

		set PLUG($t,TAL) $tl
		sh_tmclear $t
		sh_tmgooff $t

		return 0
	}

	set err [sh_command $inp $t]

	if { $err != "" } {
		sh_tout "$err!" $t
		return 1
	}

	return 0
}

proc sh_output { bts t } {

	global PLUG

	set ln [llength $bts]

	if { $ln < 5 } {
		# garbage
		sh_dmp $bts "<-U" $t
		return
	}

	set opc [lindex $bts 0]
	set tln [lindex $bts 1]
	set ags [lrange $bts 2 end]

	if { $ln != [expr { $tln - 3 }] } {
		# bad length
		sh_dmp $bts "<-U" $t
		return
	}

	# decode the command
	set m ""
	foreach r $PLUG(RESP) {
		set m [sh_rmatch $r $opc $ags]
		if { $m != "" } {
			break
		}
	}

	if { $m == "" } {
		# unrecognized
		sh_dmp $bts "<-U" $t
		return 0
	}

	set tp [lindex $m 0]

	if { $PLUG($t,PENDING) == $tp } {
		set PLUG($t,PENDING) ""
		sh_dmp $bts "<-W" $t
	} else {
		sh_dmp $bts "<-R" $t
	}

	if { $tp == "nodeaddress" } {
		# the node address
		set ni [lindex [lindex $m 1] 0]
		if { $PLUG($t,NODEID) != $ni } {
			set PLUG($t,NODEID) $ni
		}
		sh_showresp $m $t
		return
	} elseif { $tp == "setreply" } {
		# report only if change
		set ni [lindex [lindex $m 1] 0]
		if { $ni != $PLUG($t,LASTSRP) } {
			# not needed ?
			set PLUG($t,LASTSRP) $ni
		}
		sh_showresp $m $t
		return
	}

	set a [lindex $r 3]
	if { $a != "" } {
		# reply
		sh_issue [lindex $a 0] [lrange $a 1 end] $t
	}

	sh_showresp $m $t
}
		
###############################################################################
###############################################################################

proc sh_tmstart { del t } {

	global PLUG

	if { $PLUG($t,CB,TIMEOUT) != "" } {
		catch { after cancel $PLUG($t,CB,TIMEOUT) }
	}

	set PLUG($t,CB,TIMEOUT) [after $del "sh_tmgooff $t"]
}

proc sh_tmgooff { t } {

	global PLUG

	set PLUG($t,PENDING) $PLUG($t,PENDING)
}

proc sh_tmclear { t } {

	global PLUG

	if { $PLUG($t,CB,TIMEOUT) != "" } {
		catch { after cancel $PLUG($t,CB,TIMEOUT) }
		set PLUG($t,CB,TIMEOUT) ""
	}
}

###############################################################################

proc sh_renesas { t } {
#
# This is a callback acting as a background process periodically polling the
# node for status
#
	global PLUG

	while 1 {

###	#######################################################################
###	#######################################################################

	if ![info exists PLUG($t,RSTATE)] {
		# a precaution
		return
	}

	switch $PLUG($t,RSTATE) {

	0 {
		# start poll for node info; TR <- number of tries
		set PLUG($t,TR) 1
		set err [sh_command "get address" $t]
		if { $err != "" } {
			sh_tout "ERROR: $err, Renesas callback terminates" $t
			return
		}
		set PLUG($t,PENDING) "nodeaddress"
		set PLUG($t,RSTATE) 1
		sh_tmstart $PLUG(RTIME) $t
		vwait PLUG($t,PENDING)
	}

	1 {
		sh_tmclear $t
		if { $PLUG($t,PENDING) != "" && $PLUG($t,TR) <
		    $PLUG(RETRIES) } {
			set err [sh_command "get address" $t]
			if { $err != "" } {
				sh_tout "ERROR: $err, Renesas callback\
					terminates" $t
			}
			incr PLUG($t,TR)
			sh_tmstart $PLUG(RTIME) $t
		} elseif { $PLUG($t,NODEID) == "" } {
			# still don't know the node Id, keep polling
			set PLUG($t,RSTATE) 0
			sh_tmstart $PLUG(LTIME) $t
		} elseif { $PLUG($t,TAL) == "" } {
			# no TAL to send
			set PLUG($t,RSTATE) 0
			sh_tmstart $PLUG(PTIME) $t
		} else {
			set PLUG($t,TR) 1
			set err [sh_command "set tal [join $PLUG(TAL)]" $t]
			if { $err != "" } {
				sh_tout "Error sending set tal, $err,\
					TAL cleared!" $t
				set PLUG($t,TAL) ""
				set PLUG($t,RSTATE) 0
				sh_tmstart $PLUG(PTIME) $t
			} else {
				# OK
				set PLUG($t,PENDING) "settalreply"
				set PLUG($t,RSTATE) 2
				sh_tmstart $PLUG(RTIME) $t
			}
		}
		vwait PLUG($t,PENDING)
	}

	2 {
		sh_tmclear $t
		if { $PLUG($t,PENDING) != "" && $PLUG($t,TR) < $PLUG(RETRIES) &&
	 	    $PLUG($t,TAL) != "" } {
			set err [sh_command "set tal [join $PLUG(TAL)]" $t]
			if { $err != "" } {
				sh_tout "Error sending set tal, $err,\
					tal cleared!" $t
				set PLUG($t,TAL) ""
				set PLUG($t,RSTATE) 0
				sh_tmstart $PLUG(PTIME) $t
			} else {
				incr PLUG($t,TR)
				sh_tmstart $PLUG(RTIME) $t
			}
		} else {
			# close the loop
			set PLUG($t,RSTATE) 0
			sh_tmstart $PLUG(PTIME) $t
		}
		vwait PLUG($t,PENDING)
	}
	}

###	#######################################################################
###	#######################################################################
	}
}

proc sh_valint { n { min "" } { max "" } } {
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

proc sh_parsnum { l } {
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

proc sh_toh { lv } {

	set res ""

	foreach v $lv {
		lappend res [format "%02x" $v]
	}

	return [join $res]
}

proc sh_dmp { lv th t { lev 1 } } {

	global PLUG

	if { $PLUG($t,DUMP) >= $lev } {
		if { $th == "" } {
			set th "-"
		}
		sh_tout "${t}: \[[sh_toh $lv]\]" $t
	}
}

###############################################################################


if [info exists PM(DPF)] {

###############################################################################
# PITER #######################################################################
###############################################################################

proc plug_init { ags } {

	sh_init $ags 0
}

proc plug_close { } {

	sh_close 0
}

proc plug_inppp_b { in } {

	upvar $in inp

	set inp [string trim $inp]

	sh_input $inp 0 0

	return 0
}

proc plug_outpp_b { in } {

	upvar $in inp

	set bts ""

	# this is a bunch of pairs of hex digits, turn them into proper numbers
	foreach v $inp {
		lappend bts [expr { "0x$v" }]
	}

	sh_output $bts 0

	return 0
}

proc sh_outln { line t } {

	pt_outln $line
}

proc sh_tout { line t } {

	pt_tout $line
}


} else {

###############################################################################
# UDAEMON #####################################################################
###############################################################################

proc vplug_init { nn hn tp t } {

	global PLUG

	if { $tp != "a321p" } {
		# this only works for Pegs
		return 0
	}

	sh_init "" $t
	# receiver state
	set PLUG($t,S) 0

	return 1
}

proc vplug_close { t } {

	sh_close $t
}

proc vplug_receive { bytes t hex } {

	global PLUG

	set chunk ""

	while { $bytes != "" } {
		set c [string index $bytes 0]
		set bytes [string range $bytes 1 end]
		scan $c %c v
		lappend chunk $v
	}

	while 1 {

		set bl [llength $chunk]
		if { $bl == 0 } {
			return ""
		}

		switch $PLUG($t,S) {

		0 {
			# waiting for STX
			for { set i 0 } { $i < $bl } { incr i } {
				set c [lindex $chunk $i]
				if { $c == 2 } {
					# STX
					set PLUG($t,S) 1
					set PLUG($t,E) 0
					set PLUG($t,B) ""
					set PLUG($t,L) 0
					# initialize the parity byte
					set PLUG($t,P) 5
					break
				}
			}
			if { $i == $bl } {
				return ""
			}
			incr i
			set chunk [lrange $chunk $i end]
		}

		1 {
			# parse for escapes
			for { set i 0 } { $i < $bl } { incr i } {
				set c [lindex $chunk $i]
				if !$PLUG($t,E) {
					if { $c == 3 } {
						# that's it
						set PLUG($t,S) 0
						set chunk [lrange $chunk \
							[expr { $i + 1 }] end]
						if $PLUG($t,P) {
							sh_tout "Parity error!"\
								$t
							break
						}
						# remove the parity byte and
						# process the message
						sh_output [lrange \
						    $PLUG($t,B) 0 end-1] $t
						break
					}
					if { $c == 2 } {
						# reset
						set PLUG($t,S) 0
						set chunk [lrange $chunk $i end]
						break
					}
					if { $c == 16 } {
						# escape
						set PLUG($t,E) 1
						continue
					}
				} else {
					set PLUG($t,E) 0
				}
				lappend PLUG($t,B) $c
				set PLUG($t,P) \
					[expr { ($PLUG($t,P) + $c) & 0xFF }]

				incr PLUG($t,L)
			}
			if $PLUG($t,S) {
				set chunk ""
			}
		}

		}
	}
}

proc vplug_input { in t } {

	upvar $in inp

	set inp [string trim $inp]

	sh_input $inp 0 $t

	return 1
}

proc sh_outln { m t } {

	set r [list 0x02]
	set h [expr { (-(0x02 + 0x03)) & 0xFF } ]

	foreach v $m {

		if { $v == 0x02 || $v == 0x03 || $v == 0x10 } {
			lappend r 0x10
		}

		lappend r $v

		set h [expr { ($h - $v) & 0xFF } ]
	}

	if { $h == 0x02 || $h == 0x03 || $h == 0x10 } {
		lappend r 0x10
	}

	lappend r $h
	lappend r 0x03

	set out ""

	foreach v $r {
		append out [binary format c $v]
	}

	uart_send $out $t
}

proc sh_tout { line t } {

	term_output "$line\n" $t
}

###############################################################################
# END PITER OR UDAEMON ########################################################
###############################################################################
} 

}
