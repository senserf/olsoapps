#!/bin/sh
###################\
exec tclsh "$0" "$@"

proc bad_use { } {

	puts stderr "Usage: preandro inFiles outFile"
	exit 1
}

proc get_input { } {
	global argv argc infiles out_andro outtakes

	if { $argc < 2 } {
		bad_use
	}
	
#	CR happens, mess happens
    set outfile [string trimright [lindex $argv end]]
	set infiles [lreplace $argv end end]

	if [catch { open [concat ${outfile}_andro] a+ } out_andro] {
		err "Cannot open a+ [concat ${outfile}_andro]"
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

proc proc_file { fdin } {

	global g_count out_andro outtakes
	
	set count(wri) 0
	set count(rea) 0
	set skip 1
	set num 0
	
	while { [gets $fdin line] >= 0 } {
		incr count(rea)
		
		if [regexp {REMOVE_LAST} $line] {
			if { $skip == 0 } {
				err "REMOVE_LAST"
			}
			set del $num	;# let's not unwind the numbering
			while { $del > 0 && ![info exist wri($del)] } {
				incr del -1
			}
			if { $del > 0 } {
				unset wri($del)
				puts "Removed $del"
			} else {
				puts "Removed nothing"
			}
			continue
		}
			
		if [regexp {START READ} $line] {
			if { $skip == 0 } {
				err "START READ"
			}
			set skip 0
			incr num
			continue
		}
			
		if [regexp {END READ} $line] {
			if { $skip != 0 } {
				err "END READ"
			}
			set skip 1
			continue
		}

		if { $skip != 0 } {
			continue
		}
		
		if ![regexp {^[0-9]+ (.*)$} $line nulik lin] {
			puts $outtakes $line
			continue
		}
		
# OK, kludge alert: in further processing, we may want to combine WIFI lines with our 'infrastructure',
# ignore them, or have them independent. The last scenario can be even reduced to the situation where
# there is nothing but WIFI. For the latter, we must have a bundling reference and node id without
# dereferencing the (nonexisting) dogle. In any case: here, we just output the complete WIFI line.
		if [regexp {WIFI:} $line] {
			lappend wri($num) $line
		} else {
			lappend wri($num) $lin
		}
	}
	
	if { $skip == 0 } {
		err "Incomplete data"
	}
	
	foreach { n l } [array get wri] {
		puts "Writing $n..."
		foreach li $l {
			puts $out_andro $li
			incr count(wri)
		}
	}
	
	puts "Read $count(rea) written $count(wri)"
	incr g_count(rea) $count(rea)
	incr g_count(wri) $count(wri)
}



# main & all globals
global g_count out_andro outtakes infiles

get_input

set he "#\n#preandro 1.0\n#[clock format [clock seconds] -format "%y/%m/%d at %H:%M:%S"]\n#"
puts $out_andro $he
puts $outtakes $he

set g_count(wri) 0
set g_count(rea) 0
				
foreach fil $infiles {

	puts "Processing $fil ..."
	if [catch { open $fil r } fdin] {
		err "Cannot open r $fil"
	}

	proc_file $fdin
	close $fdin
}

puts "Done: $g_count(rea) read, $g_count(wri) written"
close $outtakes
close $out_andro
exit 0

