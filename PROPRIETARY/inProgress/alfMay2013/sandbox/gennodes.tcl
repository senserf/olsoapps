#!/bin/sh
###################\
exec tclsh "$0" "$@"

######################################################################
# Multiplies node data according to the provided single-node pattern #
######################################################################

#
# Usage: gennodes.tcl source target (files default to stdin stdout)
#
# Source examples:
#
#	<node number="10,300" type="simps" start="on" hid="0xBACA0065">
#		<location>10.0 12.0 60.0 70.0</location>
#		...
#	</node>
#
# Generates 300 nodes, starting number 10, hid starts at BACA0065, location
# is random within the rectangle: XL, YL, XH, YH.
#
#	<node number="210" type="simps" start="on">
#		<location>5 10.0 12.0</location>
#		...
#	</node>
#
# Generates 210 nodes, starting number 0, location is grid with 5 rows,
# horizontal distance 10.0, vertical distance 12.0.
#

proc valnum { n { min "" } { max "" } } {

	set n [string tolower [string trim $n]]
	if { $n == "" } {
		error "empty string"
	}

	if { [string first "." $n] >= 0 || 
	   (![regexp "^0x" $n] && [string first "e" $n] >= 0) } {
		error "string is not an integer number"
	}

	if [catch { expr $n } n] {
		error "string is not a number"
	}

	if { $min != "" && $n < $min } {
		error "number must not be less than $min"
	}

	if { $max != "" && $n > $max } {
		error "number must not be greater than $max"
	}

	return $n
}

proc valfnum { v { min "" } { max "" } } {

	set v [string tolower [string trim $v]]

	if { $v == "" } {
		error "empty string"
	}

	if [catch { expr double($v) } v] {
		error "string is not an fp number"
	}

	if { $min != "" && $v < $min } {
		error "fp number must not be less than $min"
	}

	if { $max != "" && $v > $max } {
		error "fp number must not be greater than $max"
	}

	return $v
}

proc bad_usage { } {

	global argv0

	puts stderr "Usage: $argv0 \[source \[target\]\]"
	exit 1
}

proc abt { m } {

	puts stderr $m
	exit 1
}

proc process_arguments { } {

	global argv P

	if { [llength $argv] > 2 } {
		bad_usage
	}

	set sfn [lindex $argv 0]

	if { $sfn == "" } {
		set ifd "stdin"
	} else {
		if [catch { open $sfn "r" } ifd] {
			abt "Cannot open input file: $ifd"
		}
	}

	set ofn [lindex $argv 1]

	if { $ofn == "" } {
		set ofd "stdout"
	} else {
		if [catch { open $ofn "w" } ofd] {
			abt "Cannot open outpu file: $ofd"
		}
	}

	set P(IF) $ifd
	set P(OF) $ofd
}

proc remrange { s r } {

	return "[string range $s 0 [expr [lindex $r 0] - 1]][string range $s \
		[expr [lindex $r 1] + 1] end]"
}

proc remitem { s r { ix "" } { it "" } } {

	if [regexp -indices "($r)\[ \t\r\n\]*" $s m u] {
		if { $it != "" } {
			upvar $it uu
			set uu [string range $s [lindex $u 0] [lindex $u 1]]
		}
		set s [remrange $s $m]
		if { $ix != "" } {
			upvar $ix xx
			set xx [lindex $m 0]
		}
	}

	return $s
}

###############################################################################

proc prepare_data { } {

	global P SOURCE D

	if ![regexp "<node.*</node>" $SOURCE nd] {
		abt "<node> data not found in source file"
	}

	if [regexp "<node.*<node" $SOURCE] {
		abt "More than one <node> item in source file"
	}

	set D(PFX) "<node "
	set nd [string range $nd 5 end]

	# locate the numbering attribute, which is mandatory
	set ix ""
	set nd [remitem $nd "number=\"\[^\"\]+\"" ix nu]

	if { $ix == "" } {
		abt "No valid \"number\" attribute in source file"
	}

	# decode the numbering parameters
	regexp "\"(.+)\"" $nu ju nu
	set nu [string trim $nu]
	regsub -all "\[ \t\r\n,\]+" $nu "," nu
	if ![regexp "^(\[0-9\]+),(\[0-9\]+)$" $nu ju st nu] {
		set st 0
	}

	if [catch { valnum $st 0 } st] {
		abt "Invalid starting node number, $st"
	}

	if [catch { valnum $nu 1 10000 } nu] {
		abt "Invalid number of nodes, $nu"
	}

	set P(FNODE) $st
	set P(NODES) $nu

	# now for the (initial) hid attribute
	set ix ""
	set nd [remitem $nd "hid=\"\[^\"\]+\"" ix hi]
	if { $ix != "" } {
		regexp "\"(.+)\"" $hi ju hi
		set hi [string trim $hi]
		if [catch { valnum $hi } hi] {
			abt "Illegal hid attribute, $hi"
		}
		set P(HID) $hi
	} else {
		set P(HID) ""
	}

	# now for the location item, which is mandatory
	set ix ""
	set nd [remitem $nd "<location>.*</location>" ix lo]
	if { $ix == "" } {
		abt "No <location> item in the source file"
	}

	set D(HDR) [string trim [string range $nd 0 [expr $ix - 1]]]
	set D(TAI) [string trim [string range $nd $ix end]]

	# determine the location distribution
	regexp ">(.*)<" $lo ju lo
	regsub -all "\[ \t\r\n,\]+" $lo " " lo
	set lo [string trim $lo]
	set lo [split $lo " "]
	set ll [llength $lo]

	if { $ll == 4 } {
		# four numbers == bounding rectangle for random distribution
		set P(D) "r"
		foreach n $lo mo { "XL" "YL" "XH" "YH" } {
			if [catch { valfnum $n 0.0 } n] {
				abt "Illegal value in <location>, $n"
			}
			set P($mo) $n
		}
		if { $P(XL) > $P(XH) } {
			abt "In <location>,\
				XL = $P(XL) is greater than XH = $P(XH)"
		}
		if { $P(YL) > $P(YH) } {
			abt "In <location>,\
				YL = $P(YL) is greater than YH = $P(YH)"
		}
	} elseif { $ll == 3 }  {
		# number of rows + 2 grid distances
		set P(D) "g"
		set n [lindex $lo 0]
		# this is the number of rows, which must be <= the
		# number of nodes
		if [catch { valnum $n 1 $P(NODES) } n] {
			abt "Illegal number of rows in <location>, $n"
		}
		set P(RO) $n
		# two more FP numbers expected
		foreach n [lrange $lo 1 end] mo { "DX" "DY" } {
			if [catch { valfnum $n 0.0 } n] {
				abt "Illegal grid parameter in <location>, $n"
			}
			set P($mo) $n
		}
	} else {
		abt "Illegal contents of <location>,\
			must be XL, YL, XH, YH, or NR, DX, DY"
	}
}

proc clean_node { nod } {

	regsub -all "\[ \t\n\r\]+<" $nod "<" nod
	regsub -all ">\[ \t\n\r\]+" $nod ">" nod
	regsub -all "\"\[ \t\n\r\]+>" $nod "\">" nod
	return $nod
}

proc generate_node { loc } {

	global D OUTPUT P

	set chunk ""

	append chunk "\n$D(PFX)"

	append chunk "number=\"$P(FNODE)\" "
	incr P(FNODE)

	if { $P(HID) != "" } {
		append chunk "hid=\"[format %08X $P(HID)]\" "
		incr P(HID)
	}

	append chunk $D(HDR)
	append chunk "<location>$loc</location>\n"
	append chunk $D(TAI)

	set chunk [clean_node $chunk]

	append OUTPUT $chunk
	append OUTPUT "\n"

}

proc grand { min max } {

	return [expr $min + (rand () * ($max - $min))]
}

proc generate_nodes { } {

	global P

	if { $P(D) == "g" } {
		# number of columns
		set nc [expr ($P(NODES) + $P(RO) - 1) / $P(RO)]
		set nn 0
		set yy $P(DY)
		for { set r 0 } { $r < $P(RO) } { incr r } {
			set xx $P(DX)
			for { set c 0 } { $c < $nc } { incr c } {
				if { $nn >= $P(NODES) } {
					return
				}
				generate_node [format "%1.2f %1.2f" $xx $yy]
				set xx [expr $xx + $P(DX)]
				incr nn
			}
			set yy [expr $yy + $P(DY)]
		}
	} else {
		expr srand ([clock seconds])
		for { set nn 0 } { $nn < $P(NODES) } { incr nn } {
			set xx [grand $P(XL) $P(XH)]
			set yy [grand $P(YL) $P(YH)]
			generate_node [format "%1.2f %1.2f" $xx $yy]
		}
	}
}

proc main { } {

	global P SOURCE OUTPUT argv0

	process_arguments

	if [catch { read $P(IF) } SOURCE] {
		abt "Cannot read input file: $SOURCE"
	}

	prepare_data

	set OUTPUT "<!-- Generated automatically by $argv0 -->\n"

	generate_nodes

	append OUTPUT "<!-- End of automatically generated data -->\n"

	if [catch { puts $P(OF) $OUTPUT } err] {
		abt "Cannot write to output file: $err"
	}
}

main
