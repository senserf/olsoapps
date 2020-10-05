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

###########################
# Simple & dirty script to process data from RSSI catching on an always-on master
# from arbitrary placed (moving) nodes. The input is supposed to be annotated with ^dist lines.
#
# Likely a rare experiment. If needed, we should write a nicer script.
###########################

set DEBUG 0

set inpF rssiIN
set outF rssiOUT

if $DEBUG {
	proc dbg { t } {
		puts $t
	}
} else {
	proc dbg { t } { }
}

proc err { m } {

	puts stderr $m
	exit 1
}

if [catch { open $inpF r } fdin] {
	err "Cannot open file '$fname': $fd"
}

if [catch { open $outF a+ } fdout] {
	err "Cannot open file '$fname': $fd"
}

set ref  empty
while { [gets $fdin line] >= 0 } {
    dbg $line

	if [regexp {^dist (.+)} $line nulik ref] {
		dbg "ref $ref"
	}
	
	if { $ref != "empty" } {
#		e.g. " 0:  (1285 0 166)<"
		if [regexp {^ 0: +\(([0-9]+) ([0-9]+) ([0-9]+)} $line nulik lh nulik rssb] {
			dbg "0: $lh $rssb"
		} elseif [regexp {^ 1: +\(([0-9]+) ([0-9]+) ([0-9]+)} $line nulik id rssf nulik] {
			set kij $ref#$lh.$id
			append body($kij) " <$rssf $rssb>"
			if [info exist count($kij)] {
				incr count($kij) 2
				incr tot($kij) [expr $rssf + $rssb]
			} else {
				set count($kij) 2
				set tot($kij) [expr $rssf + $rssb]
			}
		}
	}
}

close $fdin

if [array exists body] {
	set bundle empty
	foreach ref [lsort [array names body]] {
		if ![regexp {^(.+)#(.*)} $ref nulek dist hosts] {
			puts "***dict error: $ref $body($ref)"
		} else {
			if {$bundle != $dist} {
				set bundle $dist
				puts $fdout "dist $dist"
				dbg "file dist $dist"
			}
			puts $fdout "$hosts $body($ref)"
			append avg($hosts) "[expr round($tot($ref) / ($count($ref) + 0.0))] "
			dbg "file $hosts $body($ref) avg [expr round($tot($ref) / ($count($ref) + 0.0))] on $count($ref)"
		}
	}
} else {
	puts "no data"
	exit 1
}

foreach {h a} [array get avg] {
	puts $fdout "$h $a"
}

close $fdout
