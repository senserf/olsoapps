#!/bin/sh
###################\
exec tclsh "$0" "$@"

proc bad_use { } {

	puts stderr "Usage: 2loca inFiles outFile"
	exit 1
}

set WIFIDAT(max)		25	;# anything WIFI level > -25 is as RSSI 255
set WIFIDAT(ncount) 	1000 ;# WIFI node ids (likely, we'll have to write them out in a _wifi file)
set WIFIDAT(combine)	1	;# if nonzero, tag's lines combine CC and WIFI; otherwise they're separated

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

proc get_input { } {
	global argv argc infiles qu_pf qu_tf db_pf db_tf qu_pb qu_tb db_pb db_tb outtakes slr samples

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
		
	if [catch { open [concat ${outfile}_slr] a+ } slr] {
		err "Cannot open a+ [concat ${outfile}_slr]"
	}

	if [catch { open [concat ${outfile}_samples] a+ } samples] {
		err "Cannot open a+ [concat ${outfile}_samples]"
	}
}

proc rogue { id } {
	set dont { 777 666 } ;# rogue nodes
	if { [lsearch $dont $id] == -1 } {
		return 0
	}
	return 1
}

proc proc_file { fdin } {

	global count_t count_p list_t list_p list_ch peg_b peg_f tag_b tag_f outtakes WIFIDAT
			
	set ref   0
	set coolh 0
	
	while { [gets $fdin line] >= 0 } {
		dbg $line

		if [regexp {^#} $line] {
			continue
		}

#		e.g. "1366995548763 Coords: 384.0cm X 291.0cm"
		if [regexp {Coords: +([0-9]+)[\.| ].*[x|X] *([0-9]+)[\.| |$]} $line nulik myX myY] {
			set coolh [expr ($myX / 10) * 10000 + $myY / 10]	;# it is xxxxyyyy in *deci*meters
			puts "Coords $coolh: $myX $myY"	;# I'm not sure if we want to output this...
		}
		
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

#		Another kludge,this time to combine WIFI with ad-hoc
		if { $coolh != 0 } { ;# WIFI is useless without coords

#           e.g. 1366995550236 WIFI: SSID: CombatVoice, BSSID: 00:16:ca:3a:e6:c0, 
#                capabilities: [WPA-PSK-TKIP+CCMP][WPA2-PSK-TKIP+CCMP][ESS], level: -54, frequency: 2437
			if [regexp {^([0-9]+) WIFI:.*SSID: (.+), BSSID: ([0-9a-f:]+),.*level: +-([0-9]+),} \
				$line nulik ts nam ss lev] {

#			when we want to combine, WIFI still can be out of ref, hopefully all entries for a single scan
			if { !$WIFIDAT(combine) || $ref == 0 } {
					set wifiref "#[expr ($ts / 1000) % 10000]" ;# ca. 3 hrs of unique refs
				} else {
					set wifiref $ref ;# we want to combine WIFI with our RF
				}
				dbg "wifi $wifiref $ss $lev"
				
				if ![info exist WIFIDAT($ss)] {
					incr WIFIDAT(ncount)
					set WIFIDAT($ss) $WIFIDAT(ncount)
					puts $outtakes "wifi map $nam $ss $WIFIDAT(ncount)"
				}
				
				set lev [expr 255 - ($lev - $WIFIDAT(max)) * 2]
				if { $lev > 255} {
					set lev 255
				}
				if { $lev < 0 } {
					set lev 0
				}

				if [info exist count_t($coolh$wifiref)] {
					if { [lsearch $list_t($coolh$wifiref) $WIFIDAT($ss)] == -1 } {
						incr count_t($coolh$wifiref)
						lappend list_t($coolh$wifiref) $WIFIDAT($ss)
						append tag_b($coolh$wifiref) " $WIFIDAT($ss) $lev"
#						tag_f formally should not be updated; however, the counters would have to be
#						separated or combined wifi and cc lines would be faulty
						append tag_f($coolh$wifiref) " $WIFIDAT($ss) $lev"
						dbg "append wifi $coolh$wifiref $WIFIDAT($ss) $lev"
					}
				} else {
					set count_t($coolh$wifiref) 1
					lappend list_t($coolh$wifiref) $WIFIDAT($ss)
					append tag_b($coolh$wifiref) " $WIFIDAT($ss) $lev"
					append tag_f($coolh$wifiref) " $WIFIDAT($ss) $lev"
					dbg "do wifi $coolh$wifiref $WIFIDAT($ss) $lev"
				}
				lappend list_ch($WIFIDAT($ss)#$coolh) $lev
			}
		}
				
		if { $ref != 0 } {
		
#			e.g. " 0:  (1285 0 166)<"
			if [regexp {^ 0: +\(([0-9]+) ([0-9]+) ([0-9]+)\)<} $line nulik lh nulik rss0b] {
				dbg "0: $lh $rss0b"
				set lin 1
				
#				kludge with multiple coords for a single lh
				set truelh	$lh
				if { $coolh != 0 } { ;# assumed cm, after our android frontend
					set lh $coolh
				}
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
				lappend list_ch($lh#$id1) $rss1f
				lappend list_ch($id1#$lh) $rss0b
				set lin 2
				if { !$WIFIDAT(combine) && $hop < 2 } {
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
				if { $id2 != $truelh } {	;# don't do the ping-pong entries
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
				lappend list_ch($id1#$id2) $rss2f
				lappend list_ch($id2#$id1) $rss1b
				set ref 0
			}
		}
	}
}

proc load_coord { } {
	global cX cY
	
	set cnt 0
	
	if [catch { open _coord r } coord] {
		err "Cannot open r _coord"
	}

	while { [gets $coord line] >= 0 } {
		dbg $line

		if [regexp {^#} $line] {
			continue
		}
		
#		id X Y
		if [regexp {[ \t]*([0-9]+)[ \t]*([.0-9]+)[ \t]*([.0-9]+)} $line nulik id X Y] {
		
			if { [info exist cX($id)] || [info exist cY($id)] } {
				err "duplicate $id"
			}
			set cX($id) $X
			set cY($id) $Y
			incr cnt
			dbg "$line $cnt: $id $X $Y"
		}
	}
	close $coord
	puts "loaded $cnt coord"
}
	
proc coord { id x_ptr y_ptr } {
	global cX cY
	upvar $x_ptr x
	upvar $y_ptr y
	set x 0
	set y 0
	
	if { $id > 10000 } { ;# kludge for moving ids replaced with current coords
		set x [format "%.1f" [expr ($id / 10000) / 10.0]]
		set y [format "%.1f" [expr ($id % 10000) / 10.0]]
		return 0
	}
	
# this below can barf (and get caught)
	set x $cX($id)
	set y $cY($id)
	return 0
}

proc write_samples { } {

	global list_ch slr samples outtakes

	foreach { ids lrss } [array get list_ch] {
		if ![regexp {^([0-9]+)#([0-9]+)$} $ids nulek s d] {
			puts $outtakes "samples err $ids $rss"
		} else {
			foreach rss $list_ch($ids) { ;# $lrss <-> $list_ch($ids)
				if [catch { coord $s sx sy }] {
					puts $outtakes "coord $s $sx $sy"
					continue
				}
				if [catch { coord $d dx dy }] {
					puts $outtakes "coord $d $dx $dy"
					continue
				}
				puts $samples "$sx $sy $dx $dy $rss"
				
				# let's try to construct slr				
				if [catch { expr { sqrt ( [expr ($sx - $dx) * ($sx - $dx) + ($sy - $dy) * ($sy - $dy)] ) \
								 } } dist] {
					puts $outtakes "math $s $d: $sx $sy $dx $dy"
					continue
				}
				set dist [format "%.2f" $dist]
				
				# one for all SLR
				if [info exist dtab($dist)] {
					set dtab($dist) [expr { $dtab($dist) + $rss }]
					incr dtabc($dist)
				} else {
					set dtab($dist) $rss
					set dtabc($dist) 1
					set dtabn($dist) 0
				}

				#per node SLR
				set key [expr { $dist + $s * 1000.0 }] 
				if [info exist dtab($key)] {
					set dtab($key) [expr { $dtab($key) + $rss }]
					incr dtabc($key)
				} else {
					set dtab($key) $rss
					set dtabc($key) 1
					set dtabn($key) $s
				}
			}
		}
	}

	foreach { d } [lsort -real [array names dtab]] {
		puts $slr " $dtabn($d) [format "%.2f" [expr { $d - $dtabn($d) * 1000.0 }]] \
								[expr { $dtab($d) / $dtabc($d) }] $dtabc($d)"
	}	
}
	
proc write_stuff { } {

	global peg_b tag_b peg_f tag_f count_t count_p \
			db_pf db_pb db_tf db_tb qu_pf qu_pb qu_tf qu_tb outtakes

# perhaps we can read in or include this stuff... later
# let's move limits 1 down from perfect
	set lim(db_t) 10
	set lim(db_p) 9
#	set lim(db_t) 9
#	set lim(db_p) 8
	set lim(qu_t) 3
	set lim(qu_p) 3
	
	if [array exists count_t] {
		foreach {ref val} [array get tag_b] {
			if ![regexp {^([0-9]+)#} $ref nulek id] {
				puts $outtakes "dict_tb error: $ref $val"
			} else {
			
#				queries don't need coords, do them first
				if { $count_t($ref) < $lim(qu_t) } {
					puts $outtakes "qu_tb below $lim(qu_t) $id:l $id 0 $count_t($ref)$val"
				} else {
					puts $qu_tb "l $id 0 $count_t($ref)$val"				
				}
				
				if [catch { coord $id x y }] {
					puts $outtakes "coord $id $x $y"
					continue
				}
				if { $count_t($ref) < $lim(db_t) } {
					puts $outtakes "db_tb below $lim(db_t) $id:$x $y $id 0x00000000 $count_t($ref)$val"
				} else {
					puts $db_tb "$x $y $id 0x00000000 $count_t($ref)$val"
				}

				dbg "file_tb $x $y $id 0x00000000 $count_t($ref)$val"
			}
		}
		foreach {ref val} [array get tag_f] {
			if ![regexp {^([0-9]+)#} $ref nulek id] {
				puts $outtakes "dict_tf error: $ref $val"
			} else {
			
#				queries don't need coords, do them first
				if { $count_t($ref) < $lim(qu_t) } {
					puts $outtakes "qu_tf below $lim(qu_t) $id:l $id 0 $count_t($ref)$val"
				} else {
					puts $qu_tf "l $id 0 $count_t($ref)$val"				
				}
				
				if [catch { coord $id x y }] {
					puts $outtakes "coord $id $x $y"
					continue
				}
				if { $count_t($ref) < $lim(db_t) } {
					puts $outtakes "db_tf below $lim(db_t) $id:$x $y $id 0x00000000 $count_t($ref)$val"
				} else {
					puts $db_tf "$x $y $id 0x00000000 $count_t($ref)$val"
				}

				dbg "file_tf $x $y $id 0x00000000 $count_t($ref)$val"
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
			
#				queries don't need coords, do them first
				if { $count_p($ref) < $lim(qu_p) } {
					puts $outtakes "qu_pb below $lim(qu_p) $id:l $id 0 $count_p($ref)$val"
				} else {
					puts $qu_pb "l $id 0 $count_p($ref)$val"				
				}
				
				if [catch { coord $id x y }] {
					puts $outtakes "coord $id $x $y"
					continue
				}
				if { $count_p($ref) < $lim(db_p) } {
					puts $outtakes "db_pb below $lim(db_p) $id:$x $y $id 0x80000000 $count_p($ref)$val"
				} else {
					puts $db_pb "$x $y $id 0x80000000 $count_p($ref)$val"
				}

				dbg "file_pb $x $y $id 0x80000000 $count_p($ref)$val"
			}
		}
		foreach {ref val} [array get peg_f] {
			if ![regexp {^([0-9]+)#} $ref nulek id] {
				puts $outtakes "dict_pf error: $ref $val"
			} else {

#				queries don't need coords, do them first
				if { $count_p($ref) < $lim(qu_p) } {
					puts $outtakes "qu_pf below $lim(qu_p) $id:l $id 0 $count_p($ref)$val"
				} else {
					puts $qu_pf "l $id 0 $count_p($ref)$val"				
				}
				
				if [catch { coord $id x y }] {
					puts $outtakes "coord $id $x $y"
					continue
				}
				if { $count_p($ref) < $lim(db_p) } {
					puts $outtakes "db_pf below $lim(db_p) $id:$x $y $id 0x80000000 $count_p($ref)$val"
				} else {
					puts $db_pf "$x $y $id 0x80000000 $count_p($ref)$val"
				}

				dbg "file_pf $x $y $id 0x80000000 $count_p($ref)$val"
			}
		}
	} else {
		puts "no peg data"
	}
	
}

# main & all globals
global peg_b peg_f tag_b tag_f count_t count_p list_t list_p list_ch \
       db_pf db_pb qu_pf qu_pb db_tf db_tb qu_tf qu_tb slr samples outtakes infiles WIFIDAT
		
load_coord
get_input

set he "#\n#2loca 1.0\n#[clock format [clock seconds] -format "%y/%m/%d at %H:%M:%S"]\n#"

puts $db_pb "DBVersion 1"
puts $db_pf "DBVersion 1"

puts $outtakes "$he\n#Patterns\n#?multihop? line"
puts $outtakes "#out lin: line"
puts $outtakes "#rogue id:lin line"
puts $outtakes "#coord id x y"
puts $outtakes "#math s d: sx sy dx dy"

puts $outtakes "#dict_tb error: ref val"
puts $outtakes "#dict_tf error: ref val"
puts $outtakes "#dict_pb error: ref val"
puts $outtakes "#dict_pf error: ref val"

puts $outtakes "#db_tb below lim(db_t): count_t(ref)"
puts $outtakes "#qu_tb below lim(qu_t): count_t(ref)"
puts $outtakes "#db_tf below lim(db_t): count_t(ref)"
puts $outtakes "#qu_tf below lim(qu_t): count_t(ref)"
puts $outtakes "#db_pb below lim(db_p): count_p(ref)"
puts $outtakes "#qu_pb below lim(qu_p): count_p(ref)"
puts $outtakes "#db_pf below lim(db_p): count_p(ref)"
puts $outtakes "#qu_pf below lim(qu_p): count_p(ref)\n#========="

puts $samples $he
puts $slr $he

foreach fil $infiles {

	if [catch { open $fil r } fdin] {
		err "Cannot open r $fil"
	}

	proc_file $fdin
	close $fdin
}

write_stuff
write_samples

close $db_pb
close $db_pf
close $db_tb
close $db_tf

puts $qu_pb q
close $qu_pb

puts $qu_pf q
close $qu_pf

puts $qu_tb q
close $qu_tb

puts $qu_tf q
close $qu_tf

close $outtakes
close $samples
close $slr
exit 0

