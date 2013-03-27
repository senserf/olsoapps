#!/bin/sh
###################\
exec tclsh "$0" "$@"

proc bad_use { } {

	puts stderr "Usage: verloca inFiles outFile"
	exit 1
}

proc get_input { } {
	global argv argc infiles out_ver outtakes

	if { $argc < 2 } {
		bad_use
	}
	
#	CR happens, mess happens
    set outfile [string trimright [lindex $argv end]]
	set infiles [lreplace $argv end end]

	if [catch { open [concat ${outfile}_ver] a+ } out_ver] {
		err "Cannot open a+ [concat ${outfile}_ver]"
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
# can barf and get caught:
	set x $cX($id)
	set y $cY($id)
	return 0
}

proc proc_file { fdin } {

	global g_count g_dist g_dx g_dy g_distM g_dxM g_dyM out_ver outtakes
	
	while { [gets $fdin line] >= 0 } {

#		e.g. "Command: Location 150: [  30.97   11.46] (9)"
		if [regexp {[^0-9]+([.0-9]+)[^.0-9]+([.0-9]+)[^.0-9]+([.0-9]+)[^.0-9]+([0-9]+)} \
				$line nulik id X Y n] {
			if [catch { coord $id x y }] {
				puts $outtakes "coord $id $x $y"
				continue
			}
			if [catch { expr { sqrt ( [expr ($X - $x) * ($X - $x) + ($Y - $y) * ($Y - $y)] ) } } dist] {
				puts $outtakes "math $id: $x $y $X $Y"
				continue
			}
			set dx [expr { abs([expr {$x - $X}]) }]
			set dy [expr { abs([expr {$y - $Y}]) }]
			puts $out_ver "$id [format "%.2f" $dist] $dx $dy $x $y $X $Y"
			if [info exist g_count($id)] {
				incr g_count($id)
				
				set g_dist($id)  [expr { $g_dist($id) + $dist }]
				if { $g_distM($id) < $dist } {
					set g_distM($id) $dist
				}
				
				set g_dx($id) [expr { $g_dx($id) + $dx }]
				if { $g_dxM($id) < $dx } {
					set g_dxM($id) $dx
				}
				set g_dy($id) [expr { $g_dy($id) + $dy }]
				if { $g_dyM($id) < $dy } {
					set g_dyM($id) $dy
				}
			} else {
				set g_count($id) 1
				set g_dist($id) $dist
				set g_distM($id) $dist
				set g_dx($id) $dx
				set g_dxM($id) $dx
				set g_dy($id) $dy
				set g_dyM($id) $dy
			}
			incr g_count(all)
			set g_dist(all)  [expr { $g_dist(all) + $dist }]
			set g_dx(all) [expr { $g_dx(all) + $dx }]
			set g_dy(all) [expr { $g_dy(all) + $dy }]

			if { $g_distM(all) < $dist } {
					set g_distM(all) $dist
			}

			if { $g_dxM(all) < $dx } {
					set g_dxM(all) $dx
			}

			if { $g_dyM(all) < $dy } {
				set g_dyM(all) $dy
			}							
		} else {
			puts $outtakes "unparsed: $line"
		}
	}
}



proc write_sum { } {

	global g_count g_dist g_dx g_dy g_distM g_dxM g_dyM out_ver outtakes

	puts $out_ver "Summaries:" 
	foreach { id num } [array get g_count] {
		set line "$id\t$num\t[format "Dist: %.2f %.2f dx: %.2f %.2f dy: %.2f %.2f" \
			[expr { $g_dist($id) / $num }] $g_distM($id) \
			[expr { $g_dx($id) / $num }] $g_dxM($id) \
			[expr { $g_dy($id) / $num }] $g_dyM($id)]"
		puts $out_ver $line
		puts $line
	}
}


# main & all globals
global g_count g_dist g_dx g_dy g_distM g_dxM g_dyM out_ver outtakes infiles

get_input
load_coord

set he "#\n#verloca 0.1\n#[clock format [clock seconds] -format "%y/%m/%d at %H:%M:%S"]\n#"
puts $out_ver $he
puts $outtakes $he
puts $outtakes "#Patterns\n#unparsed: line\n#math id: X x Y x\n#coord id x y\n"

#init counters(all)
set g_count(all) 0
set g_dist(all) 0
set g_distM(all) 0
set g_dx(all) 0
set g_dxM(all) 0
set g_dy(all) 0
set g_dyM(all) 0
				
foreach fil $infiles {

	if [catch { open $fil r } fdin] {
		err "Cannot open r $fil"
	}

	proc_file $fdin
	close $fdin
}

write_sum
close $outtakes
close $out_ver
exit 0

