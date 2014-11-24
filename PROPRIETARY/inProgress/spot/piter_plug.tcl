{

###############################################################################

proc plug_init { ags } {

	global PLUG

	set PLUG(DUMP) 0

	if [regexp {[[:<:]]-m[[:blank:]]([[:digit:]]+)[[:>:]]} $ags jnk mn] {
		set PLUG(MASTER) [expr $mn]
	} else {
		# the default master node
		set PLUG(MASTER) 1
	}

	while { $ags != "" } {
		if { [string index $ags 0] == "D" } {
			incr PLUG(DUMP)
		}
		set ags [string range $ags 1 end]
	}

	# node Id (unknown yet)
	set PLUG(NODEID) ""

	# pending command
	set PLUG(PENDING) ""

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
	# Poll time
	set PLUG(PTIME) 2000

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
	global PLUG

	set inp [string trim $inp]

	if [regexp -nocase {^m[[:alpha:]]*} $inp mat] {
		# master [n]
		set inp [string trimleft \
			[string range $inp [string length $mat] end]]
		if [catch { expr $inp } no] {
			set no $PLUG(NODEID)
			if { $no == "" } {
				pt_tout "Node Id still unknown,\
					command ignored!"
				return 0
			}
		}

		if { $no > 0xFFFE || $no < 0 } {
			pt_tout "Illegal master node id $no, command ignored!"
			return 0
		}

		set PLUG(MASTER) $no
		pt_tout "Master node Id = [format %04X $no] ($no)"
		return 0
	}

	if [regexp -nocase {^d[[:alpha:]]*} $inp mat] {
		# dump
		set inp [string trimleft \
			[string range $inp [string length $mat] end]]
		if [catch { expr $inp } df] {
			pt_tout "Number expected!"
			return 0
		}
		set PLUG(DUMP) $df
		return 0
	}

	# direct

	set lv ""

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
				pt_tout "Bad input: $dd is not a pair of hex\
					digits, ignored!"
				return 0
			}
			lappend lv $val
		}
	} else {
		# free style numbers/expressions
		while 1 {
			set inp [string trimleft $inp]
			if { $inp == "" } {
				break
			}
		
			if [catch { plug_parse_number inp } val] {
				pt_tout "Bad input: $inp ($val), ignored!"
				return 0
			}

			if [catch { plug_validate_int $val 0 255 } n] {
				pt_tout "Illegal byte value: $val,\
					input ignored!"
				return 0
			}
			lappend lv $n
		}
	}

	if { [llength $lv ] < 2 } {
		pt_tout "Need at least two bytes!"
		return 0
	}

	set lf [expr [llength $lv] + 3]
	set ls [lindex $lv 1]

	if { $ls != $lf } {
		pt_tout "Fixing length $ls to $lf"
	}

	set lv [lreplace $lv 1 1 $lf]

	plug_dmp $lv "-->"
	pt_outln $lv

	return 0
}

proc plug_outpp_b { in } {

	upvar $in inp
	global PLUG

	set org ""
	set ln 0
	foreach v $inp {
		lappend org "0x$v"
		incr ln
	}

	if { $ln < 5 } {
		# garbage
		plug_dmp $org "<-U"
		return
	}

	lassign $org tp tl nl nh no
	set inp [lrange $org 4 end]

	if { $tp == 0x91 && $ln >= 8 } {
		# response to "get node info"
		if { $PLUG(PENDING) == $tp } {
			# awaited
			set PLUG(PENDING) ""
			plug_dmp $org "<-W" 2
		} else {
			# spurious
			plug_dmp $org "<-S" 1
		}
		set ii [expr [lindex $inp 0] | ([lindex $inp 1] << 8)]
		if { $ii == 1 } {
			set ni [expr [lindex $inp 2] | \
				([lindex $inp 3] << 8)]
			if { $PLUG(NODEID) == "" } {
				pt_tout "Node Id = [format %04X $ni] ($ni)"
				set PLUG(NODEID) $ni
			} elseif { $PLUG(NODEID) != $ni } {
				pt_tout "Node Id change:\
					[format %04X $PLUG(NODEID)]\
					($PLUG(NODEID)) -->\
					[format %04X $ni] ($ni)"
				set PLUG(NODEID) $ni
			}
		}
		# more?
		return 0
	}

	if { $tp == 0x92 } {
		# ACK for "set node info"
		if { $PLUG(PENDING) == $tp } {
			# awaited
			set PLUG(PENDING) ""
			plug_dmp $org "<-W" 2
		} else {
			# spurious
			plug_dmp $org "<-S" 1
		}
		return 0
	}

	if { $tp == 0x01 && $ln >= 16 } {
		plug_dmp $org "<-E"
		set pi [expr [lindex $inp  0] | ([lindex $inp 1] << 8)]
		set ti [expr [lindex $inp  2] | ([lindex $inp 3] << 8)]
		set ts [expr [lindex $inp  6]]
		if { $pi == $PLUG(NODEID) } {
			set PLUG(EV,$ti) $ts
		}
		set bu [expr [lindex $inp  4]]
		set it [expr [lindex $inp  5]]
		set vo [expr [lindex $inp  7]]
		set rs [expr [lindex $inp  8]]
		set tx [expr [lindex $inp  9]]
		set ad [expr [lindex $inp 10]]
		set ag [expr [lindex $inp 11]]
		pt_tout "Event, peg = [format %04X $pi] ($pi),\
			tag = [format %04X $ti] ($ti), ad = $ad, sn = $ts:"
		# A copy of Wlodek's hack from uplug
		set retr [expr ($it >> 4) & 7]
		if { $it & 0x80 } {
			set noack " ?"
		} else {
			set noack ""
		}
		if { $it & 0x0f } {
			set it "G"
		} else {
			set it "L"
		}
		pt_tout "    button:   $bu ($it $retr$noack)"
		set vo [expr ((($vo << 3) + 1000.0) / 4095.0) * 5.0]
		pt_tout "    voltage:  [format %4.2f $vo]"
		pt_tout "    txpower:  $tx"
		pt_tout "    age:      $ag"
		# send ACK
		set out "0x81 0x08 $nl $nh 0x06"
		plug_dmp $out "A->"
		pt_outln $out
		return 0
	}

	plug_dmp $org "<-U"
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
	set PLUG(CB,TO) ""
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

	global PLUG

	while 1 {

###	#######################################################################
###	#######################################################################

	if ![info exists PLUG(MASTER)] {
		return
	}

	set ml [expr {  $PLUG(MASTER)       & 0xFF }]
	set mh [expr { ($PLUG(MASTER) << 8) & 0xFF }]

	switch $PLUG(RSTATE) {

	0 {
		# start for get node info
		set PLUG(TR) 1
		set out "0x11 0x09 0x00 0x00 $ml $mh"
		plug_dmp $out "P->" 2
		pt_outln $out
		set PLUG(PENDING) 0x91
		set PLUG(RSTATE) 1
		plug_timeout_start $PLUG(RTIME)
		vwait PLUG(PENDING)
	}

	1 {
		plug_timeout_clear
		if { $PLUG(PENDING) != "" && $PLUG(TR) < $PLUG(RETRIES) } {
			set out "0x11 0x09 0x00 0x00 $ml $mh"
			plug_dmp $out "P->" 3
			pt_outln $out
			incr PLUG(RETRIES)
			plug_timeout_start $PLUG(RTIME)
		} elseif { $PLUG(NODEID) == "" } {
			# still don't know the node Id, keep polling
			set PLUG(RSTATE) 0
			plug_timeout_start $PLUG(LTIME)
		} else {
			set PLUG(TR) 1
			set out "0x12 0x0B 0x00 0x00 0x02 0x00 $ml $mh"
			plug_dmp $out "P->" 2
			pt_outln $out
			set PLUG(PENDING) 0x92
			set PLUG(RSTATE) 2
			plug_timeout_start $PLUG(RTIME)
		}
		vwait PLUG(PENDING)
	}

	2 {
		plug_timeout_clear
		if { $PLUG(PENDING) != "" && $PLUG(TR) < $PLUG(RETRIES) } {
			set out "0x12 0x0B 0x00 0x00 0x02 0x00 $ml $mh"
			plug_dmp $out "P->" 3
			pt_outln $out
			incr PLUG(RETRIES)
			plug_timeout_start $PLUG(RTIME)
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

	if [catch { expr $n } n] {
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

		if ![catch { expr [string range $line 0 $ix] } res] {
			# found, remove the match
			set line [string range $line [expr $ix + 1] end]
			return $res
		}
		# check for the special case of illegal octal number
		set so [string range $line $ix [expr $ix + 1]]
		if [regexp {[0-7][8-9]} $so] {
			error "illegal octal digit in '$so'"
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
