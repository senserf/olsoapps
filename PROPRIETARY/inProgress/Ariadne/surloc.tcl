#!/bin/sh
###################\
exec tclsh "$0" "$@"

proc bad_use { } {

	puts stderr "Usage: rssi inFiles outFile"
	exit 1
}

proc get_input { } {
	global argv argc infiles fdout

	if { $argc < 2 } {
		bad_use
	}
	
#	CR happens, mess happens
    set outfile [string trimright [lindex $argv end]]
	set infiles [lreplace $argv end end]

	if [catch { open $outfile a+ } fdout] {
		err "Cannot open a+ $outfile"
	}
}

set DEBUG 0
if $DEBUG {
	proc dbg { t } {
		puts $t
	}
} else {
	proc dbg { t } { }
}

proc err { m } {
	puts stderr "***$m"
	exit 1
}


proc proc_file { fdin } {

	global count body
	
	set ref  0
	while { [gets $fdin line] >= 0 } {
		dbg $line

#		e.g. "-- 7 1 0		15 15 110		5 5 134		28 509"
		if [regexp {^-- .....[ \t]+([0-9]+) ([0-9]+) ([0-9]+)[ \t]+([0-9]+) ([0-9]+) ([0-9]+)} \
				$line nulek s1 s2 seq t1 t2 rss] {

#			let's operate on ids even if they code coordinates here
			set s [expr $s1 * 256 + $s2]
			set t [expr $t1 * 256 + $t2]
			set k $s#$seq
			if ![info exist count($k)] {
				set count($k) 1
				append body($k) " $t $rss"
			} elseif { ![regexp "$t $rss" $body($k)] } {
				incr count($k)
				append body($k) " $t $rss"
			}
		}
	}
}

proc write_stuff { } {

	global body count fdout

	if [array exists count] {
		foreach {ref val} [array get body] {
			if {$count($ref) > 2} {
				if ![regexp {^([0-9]+)#} $ref nulek id] {
					puts "***dict error: $ref $val"
				} else {
				puts $fdout "[expr $id / 256] [expr $id % 256] $id 0x80000000 $count($ref) $val"
				dbg "file [expr $id / 256] [expr $id % 256] $id 0x80000000 $count($ref) $val"
				}
			} else {
				puts "?low count: $ref $val $count($ref)"
			}
		}
	} else {
		puts "no data"
	}
}

# main
global body count fdout infiles

get_input
puts $fdout "DBVersion 1"

foreach fil $infiles {

	if [catch { open $fil r } fdin] {
		err "Cannot open r $fil"
	}

	proc_file $fdin
	close $fdin
}

write_stuff
close $fdout
exit 0

