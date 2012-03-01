#
# Piter I/O preprocessor for aggregator and collector
#

###############################################################################
# SNIPPETS (in econnect format, i.e., directly copied from there ##############
###############################################################################

set plug_snippets {

{SHT_Temp {if { $value == 0 } {
        set value -1
}
if { $value != -1 } {
        set value [expr -39.62 + 0.01 * $value]
}} C {2 {1 2 5}} {4 5} {6 5}} {IR_Motion {set value [expr $value]} N {3 3}} {Light {set value [expr $value * 0.5]} L {2 3}} {Chronos_Temp {if [expr $value & 0x2000] {
        set value [expr (~$value & 0x1fff) + 1]
        set value [expr -$value]
}
set value [expr $value / 20.0]} C {3 4}} {SHT_Humid {if { $value == 0 } {
        set value -1
}
if { $value != -1 } {
set value [expr -4.0 + 0.0405 * $value - 0.0000028 * $value * $value]
if { $value < 0.0 } {
        set value 0.0
} elseif { $value > 100.0 } {
        set value 100.0
}
}} % {3 {1 2 5}} {5 5} {7 5}} {Chip_Temp {set value [expr $value * 0.1032 - 277.75]} C {0 all}} {Chronos_Acc {set value [expr $value]} N {2 4}} {Battery {set value [expr $value * 0.001221]} V {1 all}}

}

###############################################################################
###############################################################################
###############################################################################

package provide snippets 1.0
#
# Conversion snippets for sensors
#

namespace eval SNIPPETS {

# snippet cache
variable SC

# the snippets themselves
variable SN

# parameters
variable PM

# max snippet name length
set PM(SNL)	32

# number of sensor assignment columns in the snippet map (restricted to fit
# a window, but hardly hard)
set PM(NAS)	4

# maximum number of sensors per node
set PM(SPN)	10

proc snip_getparam { pm } {

	variable PM

	if ![info exists PM($pm)] {
		return ""
	} else {
		return $PM($pm)
	}
}

proc snip_cnvrt { v s c { units "" } { name "" } } {
#
# Convert a raw sensor value
#
	variable SN
	variable SC

	if ![info exists SC($c,$s)] {
		# build the cache entry
		set fn 0
		foreach sn [array names SN] {
			foreach a [lrange $SN($sn) 2 end] {
				if { [lindex $a 0] != $s } {
					# not this sensor
					continue
				}
				# scan the collector range
				set rl [lindex $a 1]
				if { $rl == "" } {
					# all
					set fn 1
					break
				}
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
			# the snippet and units
			set SC($c,$s) [lrange $SN($sn) 0 1]
			lappend SC($c,$s) $sn
		} else {
			set SC($c,$s) ""
		}
	}

	set snip [lindex $SC($c,$s) 0]

	if { $units != "" } {
		upvar $units un
		set un [lindex $SC($c,$s) 1]
	}

	if { $name != "" } {
		upvar $name na
		set na [lindex $SC($c,$s) 2]
	}

	if { $snip != "" && ![catch { snip_eval $snip $v } r] } {
		return [format %1.2f $r]
	}
	return ""
}

proc snip_icache { } {
#
# Invalidate snippet cache
#
	variable SC

	array unset SC
}

proc ucs { cl } {
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

		set a [vnum $a 0 65536]
		if { $a == "" } {
			return ""
		}

		if { $b == "" } {
			# single value
			lappend v $a
		} else {
			set b [vnum $b 0 65536]
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

proc vnum { n { min "" } { max "" } } {
#
# Verify integer number
#
	if [catch { expr int($n) } n] {
		return ""
	}

	if { ($min != "" && $n < $min) || ($max != "" && $n >= $max) } {
		return ""
	}

	return $n
}

proc pcs { lv } {
#
# Convert collector set from list to text
#
	if { $lv == "" } {
		return "all"
	}

	set tx ""

	foreach s $lv {
	
		set a [lindex $s 0]
		set b [lindex $s 1]

		if { $tx != "" } {
			append tx " "
		}

		append tx $a

		if { $b != "" } {
			append tx "-$b"
		}
	}

	return $tx
}

proc snip_names { } {

	variable SN

	return [lsort [array names SN]]
}

proc snip_vsn { nm } {
#
# Validate snippet name
#
	if { $nm == "" } {
		return 1
	}

	if { [string length $nm] > [snip_getparam SNL] } {
		return 1
	}

	if ![regexp -nocase "^\[a-z\]\[0-9a-z_\]*$" $nm] {
		return 1
	}

	return 0
}

proc snip_assignments { nm } {

	variable SN

	if ![info exists SN($nm)] {
		return ""
	}

	set asl [lrange $SN($nm) 2 end]

	set res ""

	foreach as $asl {
		lappend res [list [lindex $as 0] [pcs [lindex $as 1]]]
	}

	return $res
}

proc snip_units { nm } {

	variable SN

	if ![info exists SN($nm)] {
		return ""
	}

	return [lindex $SN($nm) 1]
}

proc snip_script { nm } {

	variable SN

	if ![info exists SN($nm)] {
		return ""
	}

	return [lindex $SN($nm) 0]
}

proc snip_exists { nm } {

	variable SN

	return [info exists SN($nm)]
}

proc snip_delete { nm } {

	variable SN

	if [info exists SN($nm)] {
		unset SN($nm)
	}
}

proc snip_set { nm scr units asl } {

	variable SN

	if [snip_vsn $nm] {
		return "the name is invalid, must be no more than\
			[snip_getparam SNL] alphanumeric characters starting\
			with a letter (no spaces)"
	}

	if [catch { snip_eval $scr 1000 } err] {
		return "execution fails, $err"
	}

	set ssl ""
	set i 0
	foreach as $asl {
		set asg [lindex $as 0]
		set cli [ucs [lindex $as 1]]
		incr i
		if { $cli == "" } {
			return "node set number $i is invalid"
		}
		lappend ssl [list $asg $cli]
	}

	set SN($nm) [concat [list $scr] [list $units] $ssl]

	return ""
}

## version dependent code #####################################################

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

proc snip_parse { cf } {
#
# Parse conversion snippets, e.g., read from a file
#
	variable SN

	# invalidate the cache
	snip_icache

	set ix 0

	set snl [snip_getparam SNL]
	set nas [snip_getparam NAS]

	foreach snip $cf {

		# we start from 1 and end up with the correct count
		incr ix

		# snippet name
		set nm [lindex $snip 0]

		# the code
		set ex [lindex $snip 1]

		# the units
		set un [lindex $snip 2]

		if [snip_vsn $nm] {
			return "illegal name '$nm' of snippet number $ix,\
				must be no more than $snl alphanumeric\
				characters starting with a letter (no spaces)"
		}
		if [info exists SN($nm)] {
			return "duplicate snippet name '$nm', snippet number\
				$ix"
		}

		# this is the list of up to PM(NAS) assignments
		set asgs [lrange $snip 3 end]

		if { [llength $asgs] > $nas } {
			return "too many (> $nas) assignments in snippet\
				'$nm' (number $ix)"
		}

		if [catch { snip_eval $ex 1000 } er] {
			return "cannot evaluate snippet $nm (number $ix), $er"
		}

		# we produce an internal representation, which is a name-indexed
		# bunch of lists
		
		set SN($nm) [list $ex]
		lappend SN($nm) $un
		set spn [snip_getparam SPN]

		set iy 0
		foreach as $asgs {

			incr iy

			# this is a sensor number (small)
			set ss [lindex $as 0]
			set se [vnum $ss 0 $spn]

			if { $se == "" } {
				return "illegal sensor number '$ss' in\
					assignment $iy of snippet '$nm'"
			}

			# this is a set of collectors which consists of
			# individual numbers and/or ranges
			set ss [lindex $as 1]
			set sv [ucs $ss]

			if { $sv == "" } {
				return "illegal node range '$ss' in\
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

proc snip_scmp { a b } {
#
# Compares two strings in a flimsy sort of way
#
	set a [string trim $a]
	regexp -all "\[ \t\n\r\]" $a " " a
	set b [string trim $b]
	regexp -all "\[ \t\n\r\]" $b " " b

	return [string compare $a $b]
}

proc snip_encode { } {
#
# Encode conversion snippets to be written back to the file
#
	variable SN

	set res ""

	foreach nm [array names SN] {

		set curr ""

		set ex [lindex $SN($nm) 0]
		set un [lindex $SN($nm) 1]
		set al [lrange $SN($nm) 2 end]

		lappend curr $nm
		lappend curr $ex
		lappend curr $un

		foreach a $al {

			set se [lindex $a 0]
			set cs [lindex $a 1]

			lappend curr [list $se [pcs $cs]]
		}

		lappend res $curr
	}

	return $res
}

namespace export snip_*

}

namespace import ::SNIPPETS::*

###############################################################################
###############################################################################
###############################################################################

proc plug_init { } {
#
# Preprocess snippets (this is 
#
	global plug_snippets

	set er [snip_parse $plug_snippets]

	if { $er != "" } {
		error $er
	}
}

proc plug_outpp_t { ln } {

	upvar $ln line

	if [regexp "^1002 *(Agg.*:\[0-9\]+ *)(.*)" $line mat hdr vals] {
		# write the header, the sensors will go into separate lines
		pt_tout "[string trim $hdr]:"
		plug_outsensors $vals
		return 0
	}

	if [regexp "^1007 *(Col.*\\)) *(.*)" $line mat hdr vals] {
		pt_tout "[string trim $hdr]:"
		plug_outsensors $vals
		return 0
	}

	if [regexp "^2007 *(.*:\[0-9\]+ *)(.*)" $line mat hdr vals] {
		pt_tout "[string trim $hdr]:"
		plug_outsensors $vals
		return 0
	}

	return 2
}

proc plug_outsensors { vals } {

	set vl ""
	while 1 {
		set vals [string trim $vals]
		if { $vals == "" } {
			break
		}
		if ![regexp "^\[-0-9\]+" $vals val] {
			break
		}
		set vals [string range $vals [string length $val] end]
		if [catch { expr $val } val] {
			break
		}
		lappend vl $val
	}

	set is [lindex $vl end]
	set vl [lreplace $vl end end]

	set inx 0
	foreach val $vl {
		set val [snip_cnvrt $val $inx $is un na]
		if { $val == "" } {
			continue
		}
		append val $un
		set len [string length $na]
		set val [plug_trims $val [expr 24 - $len]]
		pt_tout "    Sensor $inx === $na$val"
		incr inx
	}
	pt_tout ""
}

proc plug_trims { t n } {
#
# Make sure t is at least n chars long
#
	set ln [string length $t]

	set p ""

	while { $ln < $n } {
		append p " "
		incr ln
	}

	return "$p$t"
}
