#!/bin/sh
#
#	Copyright 2002-2020 (C) Olsonet Communications Corporation
#	Programmed by Pawel Gburzynski & Wlodek Olesinski
#	All rights reserved
#
#	This file is part of the PICOS platform
#
#
###################\
exec tclsh "$0" "$@"

proc bad_use { } {

	puts stderr "Usage: rssi inFiles outFile"
	exit 1
}

proc get_input { } {
	global argv argc infiles qu_f db_f qu_b db_b

	if { $argc < 2 } {
		bad_use
	}
	
#	CR happens, mess happens
    set outfile [string trimright [lindex $argv end]]
	set infiles [lreplace $argv end end]

	if [catch { open [concat ${outfile}_db_f] a+ } db_f] {
		err "Cannot open a+ [concat ${outfile}_db_f]"
	}
	
	if [catch { open [concat ${outfile}_db_b] a+ } db_b] {
		err "Cannot open a+ [concat ${outfile}_db_b]"
	}
	
	if [catch { open [concat ${outfile}_qu_f] a+ } qu_f] {
		err "Cannot open a+ [concat ${outfile}_qu_f]"
	}
	
	if [catch { open [concat ${outfile}_qu_b] a+ } qu_b] {
		err "Cannot open a+ [concat ${outfile}_qu_b]"
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

	global count body_b body_f
	
	set ref  0
	while { [gets $fdin line] >= 0 } {
		dbg $line

#		e.g. "8312: odr #8310 [1.0.9]:"
		if [regexp {odr (#[0-9]+).*([0-9])\]:$} $line nulik ref hop] {	;# prepend # to ref, for #0 != 0
			dbg "odr $ref $hop"
			if { $hop != 1 } { 	;# not interested
				set ref 0
				puts "?multihop? $line"
			}
			continue
		}
	
		if { $ref != 0 } {
		
#			e.g. " 0:  (1285 0 166)<"
			if [regexp {^ 0: +\(([0-9]+) ([0-9]+) ([0-9]+)} $line nulik lh nulik rssb] {
				dbg "0: $lh $rssb"
			} elseif [regexp {^ 1: +\(([0-9]+) ([0-9]+) ([0-9]+)} $line nulik id rssf nulik] {
				if [info exist count($lh$ref)] {			;# lh needed e.g. when #0 come from diff hosts
					incr count($lh$ref)
					dbg "append $lh$ref $id $rssb $rssf"
				} else {
					set count($lh$ref) 1
					dbg "do $lh$ref $id $rssb $rssf"
				}
				append body_b($lh$ref) " $id $rssb"
				append body_f($lh$ref) " $id $rssf"
				set ref 0
			}
		}
	}
}

proc write_stuff { } {

	global body_b body_f count db_f db_b qu_f qu_b

	if [array exists count] {
		foreach {ref val} [array get body_b] {
			if ![regexp {^([0-9]+)#} $ref nulek id] {
				puts "dict_b error: $ref $val"
			} else {
				puts $db_b "[expr $id / 256] [expr $id % 256] $id 0x80000000 $count($ref)$val"
				puts $qu_b "l $id 0 $count($ref)$val"				
				dbg "file_b [expr $id / 256] [expr $id % 256] $id 0x80000000 $count($ref)$val"
			}
		}
		foreach {ref val} [array get body_f] {
			if ![regexp {^([0-9]+)#} $ref nulek id] {
				puts "dict_f error: $ref $val"
			} else {
				puts $db_f "[expr $id / 256] [expr $id % 256] $id 0x80000000 $count($ref)$val"
				puts $qu_f "l $id 0 $count($ref)$val"				
				dbg "file_f [expr $id / 256] [expr $id % 256] $id 0x80000000 $count($ref)$val"
			}
		}
	} else {
		err "no data"
	}
}

# main
global body_b body_f count db_f db_b qu_f qu_b infiles

get_input
puts $db_b "DBVersion 1"
puts $db_f "DBVersion 1"

foreach fil $infiles {

	if [catch { open $fil r } fdin] {
		err "Cannot open r $fil"
	}

	proc_file $fdin
	close $fdin
}

write_stuff
close $db_b
close $db_f
close $qu_b
close $qu_f
exit 0

