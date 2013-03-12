#!/bin/sh
###################\
exec tclsh "$0" "$@"

proc bad_use { } {

	puts stderr "Usage: rssi inFiles outFile"
	exit 1
}

proc get_input { } {
	global argv argc infiles qu_pf qu_tf db_pf db_tf qu_pb qu_tb db_pb db_tb outtakes

	if { $argc < 2 } {
		bad_use
	}
	
#	CR happens, mess happens
    set outfile [string trimright [lindex $argv end]]
	set infiles [lreplace $argv end end]

	if [catch { open [concat ${outfile}_db_pf] a+ } db_pf] {
		err "Cannot open a+ [concat ${outfile}_db_pf]"
	}
	
	if [catch { open [concat ${outfile}_db_pb] a+ } db_pb] {
		err "Cannot open a+ [concat ${outfile}_db_pb]"
	}
	
	if [catch { open [concat ${outfile}_qu_pf] a+ } qu_pf] {
		err "Cannot open a+ [concat ${outfile}_qu_pf]"
	}
	
	if [catch { open [concat ${outfile}_qu_pb] a+ } qu_pb] {
		err "Cannot open a+ [concat ${outfile}_qu_pb]"
	}
	
	if [catch { open [concat ${outfile}_db_tf] a+ } db_tf] {
		err "Cannot open a+ [concat ${outfile}_db_tf]"
	}
	
	if [catch { open [concat ${outfile}_db_tb] a+ } db_tb] {
		err "Cannot open a+ [concat ${outfile}_db_tb]"
	}
	
	if [catch { open [concat ${outfile}_qu_tf] a+ } qu_tf] {
		err "Cannot open a+ [concat ${outfile}_qu_tf]"
	}
	
	if [catch { open [concat ${outfile}_qu_tb] a+ } qu_tb] {
		err "Cannot open a+ [concat ${outfile}_qu_tb]"
	}
	
	if [catch { open [concat ${outfile}_outtakes] a+ } outtakes] {
		err "Cannot open a+ [concat ${outfile}_outtakes]"
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

proc rogue { id } {
	set dont { 150 } ;# rogue nodes
	if { [lsearch $dont $id] == -1 } {
		return 0
	}
	return 1
}

proc proc_file { fdin } {

	global count_t count_p list_t list_p peg_b peg_f tag_b tag_f outtakes
		
	set ref  0
	while { [gets $fdin line] >= 0 } {
		dbg $line

#		e.g. "8312: odr #8310 [1.0.9]:"
		if [regexp {odr (#[0-9]+).*([0-9])\]:$} $line nulik ref hop] {	;# prepend # to ref, for #0 != 0
			dbg "odr $ref $hop"
			set lin 0
			if { $hop > 2 } { 	;# not interested
				set ref 0
				puts $outtakes "?multihop? $line"
			}
			continue
		}
	
		if { $ref != 0 } {
		
#			e.g. " 0:  (1285 0 166)<"
			if [regexp {^ 0: +\(([0-9]+) ([0-9]+) ([0-9]+)\)<} $line nulik lh nulik rss0b] {
				dbg "0: $lh $rss0b"
				set lin 1
			} 
			
			if [regexp {^ 1: +\(([0-9]+) ([0-9]+) ([0-9]+)} $line nulik id1 rss1f rss1b] {
				if { $lin != 1 } {
					puts $outtakes "out $lin: $line"
					set ref 0
					continue
				}
				if [rogue $id1] {
					puts $outtakes "rogue $id1:$lin $line"
					set ref 0
					continue
				}
				if [info exist count_t($lh$ref)] {			;# lh needed e.g. when #0 come from diff hosts
					if { [lsearch $list_t($lh$ref) $id1] == -1 } {
						incr count_t($lh$ref)
						lappend list_t($lh$ref) $id1
						append tag_b($lh$ref) " $id1 $rss0b"
						append tag_f($lh$ref) " $id1 $rss1f"
						dbg "append $lh$ref $id1 $rss0b $rss1f"
					}
				} else {
					set count_t($lh$ref) 1
					lappend list_t($lh$ref) $id1
					dbg "do $lh$ref $id1 $rss0b $rss1f"
					append tag_b($lh$ref) " $id1 $rss0b"
					append tag_f($lh$ref) " $id1 $rss1f"
				}
				set lin 2
				if { $hop < 2 } {
					set ref 0
				}
			}
			
			if [regexp {^ 2: +\(([0-9]+) ([0-9]+) ([0-9]+)} $line nulik id2 rss2f nulik] {
				if { $lin != 2 } {
					puts $outtakes "out $lin: $line"
					set ref 0
					continue
				}
				if [rogue $id2] {
					puts $outtakes "rogue $id2:$lin $line"
					set ref 0
					continue
				}
				if { $id2 != $lh } {	;# don't do the ping-pong entries
					if [info exist count_p($id1$ref)] {
						if { [lsearch $list_p($id1$ref) $id2] == -1 } {
							incr count_p($id1$ref)
							lappend list_p($id1$ref) $id2
							append peg_b($id1$ref) " $id2 $rss1b"
							append peg_f($id1$ref) " $id2 $rss2f"
							dbg "append $id1$ref $id2 $rss1b $rss2f"
						}
					} else {
						set count_p($id1$ref) 1
						lappend list_p($id1$ref) $id2
						append peg_b($id1$ref) " $id2 $rss1b"
						append peg_f($id1$ref) " $id2 $rss2f"
						dbg "do $id1$ref $id2 $rss1b $rss2f"
					}
				}
				set ref 0
			}
		}
	}
}

proc coord { id } {
# combat10.xml
	set xy(1285) "5 5"
	set xy(3845) "15 5"
	set xy(6405) "25 5"
	set xy(8965) "35 5"
	set xy(8975) "35 15"
	set xy(6415) "25 15"
	set xy(3855) "15 15"
	set xy(1295) "5 15"
	set xy(1305) "5 25"
	set xy(3865) "15 25"
	set xy(10) "10 10"
#combReal.xml
	set xy(100) "0 5.1"
	set xy(101) "10.0 4.8"
	set xy(102) "24.5 4.6"
	set xy(103) "32.4 4.3"
	set xy(104) "1.4 13.1"
	set xy(105) "8.8 13.5"
	set xy(106) "21.8 14.3"
	set xy(107) "30.6 13.3"
	set xy(108) "24.7 21.7"
	set xy(109) "32.2 21.5"
	set xy(77) "23.3 11.9"
	set xy(150) "33.1 12.5"

	if [info exists xy($id)] {
		return $xy($id)
	}
	return "0.0 0.0"
}

proc write_stuff { } {

	global peg_b tag_b peg_f tag_f count_t count_p db_pf db_pb db_tf db_tb qu_pf qu_pb qu_tf qu_tb outtakes

# perhaps we can read in or include this stuff... later
	set lim(db_t) 10
	set lim(db_p) 9
	set lim(qu_t) 3
	set lim(qu_p) 3
	
	if [array exists count_t] {
		foreach {ref val} [array get tag_b] {
			if ![regexp {^([0-9]+)#} $ref nulek id] {
				puts $outtakes "dict_tb error: $ref $val"
			} else {
				if { $count_t($ref) < $lim(db_t) } {
					puts $outtakes "db_tb below $lim(db_t) $id: $count_t($ref)$val"
				} else {
					puts $db_tb "[coord $id] $id 0x00000000 $count_t($ref)$val"
				}
				if { $count_t($ref) < $lim(qu_t) } {
					puts $outtakes "qu_tb below $lim(qu_t) $id: $count_t($ref)$val"
				} else {
					puts $qu_tb "l $id 0 $count_t($ref)$val"				
				}
				dbg "file_tb [coord $id] $id 0x00000000 $count_t($ref)$val"
			}
		}
		foreach {ref val} [array get tag_f] {
			if ![regexp {^([0-9]+)#} $ref nulek id] {
				puts $outtakes "dict_tf error: $ref $val"
			} else {
				if { $count_t($ref) < $lim(db_t) } {
					puts $outtakes "db_tf below $lim(db_t) $id: $count_t($ref)$val"
				} else {
					puts $db_tf "[coord $id] $id 0x00000000 $count_t($ref)$val"
				}
				if { $count_t($ref) < $lim(qu_t) } {
					puts $outtakes "qu_tf below $lim(qu_t) $id: $count_t($ref)$val"
				} else {
					puts $qu_tf "l $id 0 $count_t($ref)$val"				
				}
				dbg "file_tf [coord $id] $id 0x00000000 $count_t($ref)$val"
			}
		}
	} else {
		puts "no tag data"
	}
	
	if [array exists count_p] {
		foreach {ref val} [array get peg_b] {
			if ![regexp {^([0-9]+)#} $ref nulek id] {
				puts $outtakes "dict_pb error: $ref $val"
			} else {
				if { $count_p($ref) < $lim(db_p) } {
					puts $outtakes "db_pb below $lim(db_p) $id: $count_p($ref)$val"
				} else {
					puts $db_pb "[coord $id] $id 0x80000000 $count_p($ref)$val"
				}
				if { $count_p($ref) < $lim(qu_p) } {
					puts $outtakes "qu_pb below $lim(qu_p) $id: $count_p($ref)$val"
				} else {
					puts $qu_pb "l $id 0 $count_p($ref)$val"				
				}
				dbg "file_pb [coord $id] $id 0x80000000 $count_p($ref)$val"
			}
		}
		foreach {ref val} [array get peg_f] {
			if ![regexp {^([0-9]+)#} $ref nulek id] {
				puts $outtakes "dict_pf error: $ref $val"
			} else {
				if { $count_p($ref) < $lim(db_p) } {
					puts $outtakes "db_pf below $lim(db_p) $id: $count_p($ref)$val"
				} else {
					puts $db_pf "[coord $id] $id 0x80000000 $count_p($ref)$val"
				}
				if { $count_p($ref) < $lim(qu_p) } {
					puts $outtakes "qu_pf below $lim(qu_p) $id: $count_p($ref)$val"
				} else {
					puts $qu_pf "l $id 0 $count_p($ref)$val"				
				}
				dbg "file_pf [coord $id] $id 0x80000000 $count_p($ref)$val"
			}
		}
	} else {
		err "no peg data"
	}
	
}

# main & all globals
global peg_b peg_f tag_b tag_f count_t count_p list_t list_p \
       db_pf db_pb qu_pf qu_pb db_tf db_tb qu_tf qu_tb outtakes infiles

get_input
puts $db_pb "DBVersion 1"
puts $db_pf "DBVersion 1"
puts $db_tb "DBVersion 1"
puts $db_tf "DBVersion 1"

puts $outtakes "Patterns\n?multihop? line"
puts $outtakes "out lin: line"
puts $outtakes "rogue id:lin line"

puts $outtakes "dict_tb error: ref val"
puts $outtakes "dict_tf error: ref val"
puts $outtakes "dict_pb error: ref val"
puts $outtakes "dict_pf error: ref val"

puts $outtakes "db_tb below lim(db_t): count_t(ref)"
puts $outtakes "qu_tb below lim(qu_t): count_t(ref)"
puts $outtakes "db_tf below lim(db_t): count_t(ref)"
puts $outtakes "qu_tf below lim(qu_t): count_t(ref)"
puts $outtakes "db_pb below lim(db_p): count_p(ref)"
puts $outtakes "qu_pb below lim(qu_p): count_p(ref)"
puts $outtakes "db_pf below lim(db_p): count_p(ref)"
puts $outtakes "qu_pf below lim(qu_p): count_p(ref)\n========="

foreach fil $infiles {

	if [catch { open $fil r } fdin] {
		err "Cannot open r $fil"
	}

	proc_file $fdin
	close $fdin
}

write_stuff
close $db_pb
close $db_pf
close $qu_pb
close $qu_pf
close $db_tb
close $db_tf
close $qu_tb
close $qu_tf
close $outtakes
exit 0

