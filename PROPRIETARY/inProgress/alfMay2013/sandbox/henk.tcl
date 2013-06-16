#!/bin/sh
###################\
exec tclsh "$0" "$@"

###########################
# Simple & dirty script to multiply node data to include in a model's .xml
###########################

set DEBUG 0

set inpF pizda.xml
set outF out.xml

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

if { 0} {
proc pline { x_ptr y_ptr nu } {
	upvar $x_ptr x
	upvar $y_ptr y
	set y [expr $y + 50 * $nu]
}
}

if [catch { open $inpF r } fdin] {
	err "Cannot open file '$fname': $fd"
}

if [catch { open $outF a+ } fdout] {
	err "Cannot open file '$fname': $fd"
}

set inno  0
while { [gets $fdin line] >= 0 } {
    dbg $line
	append body($inno) $line
	incr inno
}
close $fdin

set MAX 25

set i 1
while { $i < $MAX } {

  set j 0
  while { $j < $inno } {

	set done 0
	if [regexp {(^.*<location>)([0-9]+)(\.[0-9]* +)([0-9]+)(\..*$)} $body($j) nulek a b c d e] {
#		pline $b $d $i
		set b [expr $b + 50 * $i]
		puts $fdout $a$b$c$d$e
		set done 1
	}
	
	if { $done == 0 && [regexp {(^.*hid=")(.*)(".*$)} $body($j) nu a b c] } {
		if [regexp {0x} $b] {
			scan $b "%x" id
			incr id $i
			set b [format "%x" $id]
		} else {
			incr $b $i
		}
		puts $fdout $a$b$c
		set done 1
	}
	
	if { $done == 0 } {
		puts $fdout $body($j)
	}
	
	incr j
  }
  
  incr i
}
	
close $fdout
exit 0

