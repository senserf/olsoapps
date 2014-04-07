{

###############################################################################

proc plug_init { ags } {

	global PLUG

	set PLUG(DUMP) 0
	if { [string first "D" $ags] >= 0 } {
		set PLUG(DUMP) 1
	}
	if { [string first "DD" $ags] >= 0 } {
		set PLUG(DUMP) 2
	}

	if [regexp {[[:<:]]-m[[:blank:]]([[:digit:]]+)[[:>:]]} $ags jnk mn] {
		set PLUG(MA) [expr $mn]
	} else {
		# the default master node
		set PLUG(MA) 1
	}

	# unacknowledged count for poll messages
	set PLUG(UC) 0

	# node Id (unknown yet)
	set PLUG(NI) ""

	# time to failure report
	set PLUG(TF,R) 10000
	set PLUG(TF) $PLUG(TF,R)

	plug_periodic_query
}

proc plug_close { } {

	global PLUG

	catch { after cancel $PLUG(PQ) }
	array unset PLUG
}

proc plug_inppp_b { in } {

	upvar $in inp
	global PLUG

	set inp [string trim $inp]

	if [regexp {^m[[:alpha:]]*} $inp mat] {
		# master [n]
		set inp [string trimleft \
			[string range $inp [string length $mat] end]]
		if [catch { expr $inp } no] {
			set no $PLUG(NI)
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

		set PLUG(MA) $no
		pt_tout "Master node Id = [format %04X $no] ($no)"
		return 0
	}

	# direct

	set lv ""

	while 1 {
		set inp [string trimleft $inp]
		if { $inp == "" } {
			break
		}

		if [catch { plug_parse_number inp } val] {
			pt_tout "Illegal expression: $inp, input ignored!"
			return 0
		}

		if [catch { plug_validate_int $val 0 255 } n] {
			pt_tout "Illegal byte value: $val, input ignored!"
			return 0
		}
		lappend lv $n
	}

	if { $lv == "" } {
		pt_tout "Empty sequence ignored!"
		return 0
	}

	set res "[lindex $lv 0] [expr [llength $lv] + 4]\
		[join [lrange $lv 1 end]]"

	if $PLUG(DUMP) {
		pt_tout "-->: \[$res\]"
	}
	pt_outln $res

	return 0
}

proc plug_outpp_b { in } {

	upvar $in inp
	global PLUG

	set ln [llength $inp]

	if { $ln < 5 } {
		# garbage
		if $PLUG(DUMP) {
			pt_tout "<-U: \[$inp\]"
		}
		return
	}

	set org $inp

	set tp [expr 0x[lindex $inp 0]]
	set tl [expr 0x[lindex $inp 1]]
	set nl [expr 0x[lindex $inp 2]]
	set nh [expr 0x[lindex $inp 3]]
	set no [expr $nl | ($nh << 8)]
	set inp [lrange $inp 4 end]

	if { $tp == 0x91 && $ln >= 8 } {
		# response to "get node info"
		if { $PLUG(DUMP) > 1 } {
			pt_tout "<-P: \[$org\]"
		}
		set ii [expr 0x[lindex $inp 0] | (0x[lindex $inp 1] << 8)]
		if { $ii == 1 } {
			set ni [expr 0x[lindex $inp 2] | \
				(0x[lindex $inp 3] << 8)]
			if { $PLUG(NI) == "" } {
				set PLUG(TF) $PLUG(TF,R)
				set PLUG(UC) 0
				pt_tout "Node Id = [format %04X $ni] ($ni)"
				set PLUG(NI) $ni
			} elseif { $PLUG(NI) != $ni } {
				pt_tout "Node Id change:\
					[format %04X $PLUG(NI)] ($PLUG(NI)) -->\
					[format %04X $ni] ($ni)"
				set PLUG(NI) $ni
			}
		}
		# more?
		return 0
	}

	if { $tp == 0x92 } {
		# ACK for "set node info"
		if { $PLUG(DUMP) > 1 } {
			pt_tout "<-A: \[$org\]"
		}
		set ac [expr 0x[lindex $inp 0]]
		if { $ac == 0x06 } {
			if { $PLUG(UC) > 0 } {
				incr PLUG(UC) -1
			}
		}
		return 0
	}

	if { $tp == 0x01 && $ln >= 16 } {
		if $PLUG(DUMP) {
			pt_tout "<-E: \[$org\]"
		}
		set pi [expr 0x[lindex $inp  0] | (0x[lindex $inp 1] << 8)]
		set ti [expr 0x[lindex $inp  2] | (0x[lindex $inp 3] << 8)]
		set ts [expr 0x[lindex $inp  6]]
		if { $pi == $PLUG(NI) } {
			# this peg
			if { [info exists PLUG(EV,$ti)] && $PLUG(EV,$ti) ==
			     $ts } {
				# duplicate
				set out "0x81 0x08 $nl $nh 0x06"
				if { $PLUG(DUMP) > 1 } {
					pt_tout "D->: \[$out\]"
				}
				pt_outln $out
				return 0
			}
			set PLUG(EV,$ti) $ts
		}
		set bu [expr 0x[lindex $inp  4]]
		set it [expr 0x[lindex $inp  5]]
		set vo [expr 0x[lindex $inp  7]]
		set rs [expr 0x[lindex $inp  8]]
		set tx [expr 0x[lindex $inp  9]]
		set ad [expr 0x[lindex $inp 10]]
		set ag [expr 0x[lindex $inp 11]]
		pt_tout "Event, peg = [format %04X $pi] ($pi),\
			tag = [format %04X $ti] ($ti), ad = $ad, sn = $ts:"
		if $it {
			set it "G"
		} else {
			set it "L"
		}
		pt_tout "    button:   $bu ($it)"
		set vo [expr ((($vo << 3) + 1000.0) / 4095.0) * 5.0]
		pt_tout "    voltage:  [format %4.2f $vo]"
		pt_tout "    txpower:  $tx"
		pt_tout "    age:      $ag"
		# send ACK
		set out "0x81 0x08 $nl $nh 0x06"
		if $PLUG(DUMP) {
			pt_tout "A->: \[$out\]"
		}
		pt_outln $out
		return 0
	}

	if $PLUG(DUMP) {
		pt_tout "<-U: \[$org\]"
	}
	return 0
}
		
###############################################################################
###############################################################################

proc plug_periodic_query { } {

	global PLUG

	# low and high bytes of master node Id
	set ml [expr $PLUG(MA) % 256]
	set mh [expr $PLUG(MA) / 256]

	set out "0x11 0x09 0x00 0x00 $ml $mh"
	if { $PLUG(DUMP) > 1 } {
		pt_tout "P->: \[$out\]"
	}
	pt_outln $out

	if { $PLUG(NI) == "" } {
		if { $PLUG(TF) <= 0 } {
			pt_tout "Node Id still unknown!"
			set PLUG(TF) $PLUG(TF,R)
		} else {
			incr PLUG(TF) -500
		}
		set PLUG(PQ) [after 500 plug_periodic_query]
		return
	}

	if { $PLUG(UC) >= 10 } {
		if { $PLUG(TF) <= 0 } {
			pt_tout "$PLUG(UC) unacked queries"
			set PLUG(TF) $PLUG(TF,R)
		}
	}
	set out "0x12 0x0B 0x00 0x00 0x02 0x00 $ml $mh"
	if { $PLUG(DUMP) > 1 } {
		pt_tout "P->: \[$out\]"
	}
	pt_outln $out
	set PLUG(PQ) [after 2000 plug_periodic_query]

	if { $PLUG(TF) >= 0 } {
		incr PLUG(TF) -2000
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
		error "illegal expression"
	}

	set ll [string length $line]
	set ix $ll

	while 1 {

		incr ix -1
		if { $ix < 0 } {
			# failure
			return 1
		}

		if ![catch { expr [string range $line 0 $ix] } res] {
			# found, remove the match
			set line [string range $line [expr $ix + 1] end]
			return $res
		}
	}
}

###############################################################################

}
