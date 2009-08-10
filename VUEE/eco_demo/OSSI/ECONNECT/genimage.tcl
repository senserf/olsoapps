#!/bin/sh
##########################\
exec tclsh "$0" "$@"

############################################################################
# Generates copies of an ihex file with modified node ID                   #
#                                                                          #
# Copyright (C) Olsonet Communications, 2008 All Rights Reserved           #
############################################################################

set PM(HOM)	"econnect"
set PM(VER)	1.3

proc verify_pnm { s } {
#
# Integer >= 1
#
	if ![regexp "^\[0-9\]+$" $s] {
		return ""
	}

	if [catch { expr $s } v] {
		return ""
	}

	if { $v < 1  || $v > 65535 } {
		return ""
	}

	return $v
}

proc verify_idn { s } {
#
# A string of bytes represented as hex digits in little endian
#
	set ln [string length $s]

	if { $ln != 8 } {
		return ""
	}

	set res ""

	while { $ln } {
		set b [string toupper [string range $s 0 1]]
		set s [string range $s 2 end]
		if ![regexp "\[0-9A-F\]\[0-9A-F\]" $b] {
			return ""
		}
		if { $res == "" } {
			set res $b
		} else {
			set res "$b $res"
		}
		incr ln -2
	}
	return $res
}

proc increment { b } {
#
# Increments the hex string
#
	set r ""

	set more 1

	foreach v $b {

		if $more {
			set v [expr 0x$v + 1]
			if { $v < 256 } {
				set more 0
			} else {
				set v 0
			}
			set v [format "%02X" $v]
		}
		lappend r $v
	}

	return $r
}

proc checksum { s } {

	if { [expr [string length $s] & 1] != 0 } {
		return ""
	}

	set sum 0

	while { $s != "" } {
		set b [string range $s 0 1]
		set s [string range $s 2 end]
		if [catch { expr 0x$b + $sum } sum] {
			return ""
		}
	}

	return [format %02X [expr (0x100 - ( $sum & 0xff )) & 0xff]]
}

proc ferror { n s } {

	error "Image format error, line $n: $s!"
}

proc cerror { n s } {

	error "Image checksum error, line $n: $s!"
}

proc read_source { fn } {
#
# Reads the source image file
#
	set chunks ""
	set chunk ""
	set caddr -1

	if [catch { open $fn "r" } fd] {
		error "Cannot open image file $fn: $fd"
	}

	if [catch { read $fd } fc] {
		catch { close $fd }
		error "Cannot read image file $fn: $fc"
	}

	catch { close $fd }

	# preprocess into chunks
	set fc [split $fc "\n"]
	set linenum 1
	foreach ln $fc {
		set ln [string toupper [string trim $ln]]
		if ![regexp "^:(..)(....)(..)(.*)(..)" $ln jk bc ad rt rec ch] {
			incr linenum
			continue
		}
		set chk [checksum "$bc$ad$rt$rec"]
		if { $chk == "" } {
			ferror $linenum $ln
		}
		if { [catch { expr 0x$rt } rt] || \
		     [catch { expr 0x$bc } bc] || \
		     [catch { expr 0x$ad } ad] || \
		     [catch { expr 0x$ch } ch]  } {
			    ferror $linenum $ln
		}
		set ll [string length $rec]
		if { $ll != [expr $bc * 2] } {
			ferror $linenum $ln
		}
		if { $ch != [expr 0x$chk] } {
			cerror $linenum $ln
		}
		if { $rt != 0 } {
			# not a data record
			if { $chunk != "" } {
				# pending data record
				lappend chunks [list $start 0 $chunk]
				set chunk ""
				set caddr -1
			}
			lappend chunks [list $ad $rt $rec]
		} else {
			# data record
			if { $ad != $caddr } {
				# start a new chunk
				if { $chunk != "" } {
					lappend chunks \
						[list $start 0 $chunk]
				}
				set start $ad
				set chunk $rec
				set caddr [expr $ad + $bc]
			} else {
				append chunk $rec
				set caddr [expr $caddr + $bc]
			}
		}
		incr linenum
	}

	if { $chunk != "" } {
		lappend chunks [list $start 0 $chunk]
	}

	return $chunks
}

proc locate_id { im idn } {
#
# Locates the tag in the image
#
	set cn -1
	set ix -1
	foreach ch $im {
		incr ix
		if { [lindex $ch 1] != 0 } {
			# not a data record
			continue
		}
		set chunk [lindex $ch 2]
		set loc [string first $idn $chunk] 
		if { $loc < 0 } {
			lappend new $ch
			continue
		}
		if { $cn >= 0 || [string last $idn $chunk] != $loc } {
			error "Image error, multiple occurrences of the cookie!"
		}
		set cn $ix
		set po $loc
	}

	if { $cn < 0 } {
		error "Cookie not found in the image file!"
	}

	return [list $cn $po]
}

proc modify_image { im org s } {
#
# Substitute the new string in the image
#
	set new ""

	set wh [lindex $org 0]
	set ix [lindex $org 1]

	set ch [lindex $im $wh]
	set ad [lindex $ch 0]
	set rc [lindex $ch 2]

	set rl [string length $s]

	set rc "[string range $rc 0 [expr $ix - 1]]$s[string range $rc \
		[expr $ix + $rl] end]"

	set im [lreplace $im $wh $wh [list $ad 0 $rc]]

	return $im
}

proc mkfname { ofn s } {
#
# New file name
#
	set t ""
	foreach q $s {
		set t "${q}$t"
	}
	set rfn "[file rootname $ofn]"

	regsub -nocase "_\[a-z0-9.\]*$" $rfn "" rfn

	append t "_[format %05u [expr 0x$t & 0x0000ffff]]"

	if { $rfn == $ofn } {
		return "${rfn}_$t"
	}

	return "${rfn}_$t[file extension $ofn]"
}

proc write_image { im fn } {

	if [catch { open $fn "w" } fd] {
		error "Cannot open $fn for writing: $fd!"
	}

	foreach ch $im {
		set ad [lindex $ch 0]
		set rt [lindex $ch 1]
		set rc [lindex $ch 2]
		while 1 {
			set ln [string length $rc]
			if { $ln > 32 } {
				set ln 32
			}
			set bc [expr $ln / 2]
			set bk [format "%02X%04X%02X" $bc $ad $rt]
			append bk [string range $rc 0 [expr $ln - 1]]
			append bk [checksum $bk]
			if [catch {
				puts $fd ":$bk"
			} er] {
				error "Cannot write to $fn:$er!"
			}
			set rc [string range $rc $ln end]
			if { $rc == "" } {
				break
			}
			set ad [expr $ad + $bc]
		}
	}
	catch { close $fd }
}
			
###############################################################################

proc find_ifn { { fn "" } } {
#
# Tries to find a candidate input file in the current directory
#

	if { $fn == "" } {
		set fn "Image.a43"
	}

	if [file isfile $fn] {
		return $fn
	}

	if [catch { exec ls } flst] {
		# cannot list current directory
		return ""
	}

	foreach fn $flst {
		if { [string tolower [file extension $fn]] == ".a43" } {
			return $fn
		}
	}

	return ""
}

proc defhome { } {
#
# Return the default home directory, like the Desktop
#
	global env

	if ![info exists env(HOMEPATH)] {
		return [pwd]
	}

	set dfn [file join $env(HOMEPATH) Desktop]

	if [file isdirectory $dfn] {
		return $dfn
	}

	set dfn [file join $env(HOMEPATH) Documents]

	if [file isdirectory $dfn] {
		return $dfn
	}

	if [file isdirectory $env(HOMEPATH)] {
		return $env(HOMEPATH)
	}

	return [pwd]
}

proc setdefs { } {
#
# Set defaults from previous run
#
	global PM env DEFS IFN TFN

	array unset DEFS

	while (1) {
		if ![info exists env(APPDATA)] {
			# impossible
			break
		}
		set dfn [file join $env(APPDATA) $PM(HOM) "genidata.dat"]
		if [catch { open $dfn "r" } dfd] {
			# no data file
			break
		}
		if [catch { read -nonewline $dfd } defs] {
			# cannot read
			catch { close $dfd }
			break
		}
		catch { close $dfd }
		foreach d $defs {
			set key [lindex $d 0]
			if { $key != "" } {
				set DEFS($key) [lindex $d 1]
			}
		}
		break
	}

	# fill in those that may be missing
	if { ![info exists DEFS(SRD)] || $DEFS(SRD) == "" } {
		# source directory
		set DEFS(SRD) [defhome]
	}
	set DEFS(SRD) [file normalize $DEFS(SRD)]

	if { ![info exists DEFS(TRD)] || $DEFS(TRD) == "" } {
		# target directory
		set DEFS(TRD) [defhome]
	}
	set DEFS(TRD) [file normalize $DEFS(TRD)]

	if { ![info exists DEFS(IFN)] || $DEFS(IFN) == "" } {
		# source file name
		set DEFS(IFN) "Image.a43"
	}

	if { ![info exists DEFS(TFN)] || $DEFS(TFN) == "" } {
		# target file name
		set DEFS(TFN) "Image_nnnn.a43"
	}

	if { ![info exists DEFS(IDN)] || [verify_idn $DEFS(IDN)] == "" } {
		set DEFS(IDN) "BACADEAD"
	}

	if { ![info exists DEFS(FRM)] || [verify_pnm $DEFS(FRM)] == "" } {
		set DEFS(FRM) 1
	}

	if { ![info exists DEFS(CNT)] || [verify_pnm $DEFS(CNT)] == "" } {
		set DEFS(CNT) 1
	}

	# check if the source exists
	set IFN [file join $DEFS(SRD) $DEFS(IFN)]
	if ![file isfile $IFN] {
		set DEFS(SRD) ""
		set DEFS(IFN) ""
		set IFN ""
	}
	set TFN [file join $DEFS(TRD) $DEFS(TFN)]
}

proc savdefs { } {
#
# Save the defaults
#
	global PM env DEFS

	if ![info exists env(APPDATA)] {
		# impossible
		return
	}
	set dfn [file join $env(APPDATA) $PM(HOM) "genidata.dat"]
	if [catch { open $dfn "w" } dfd] {
		# cannot write
		return
	}
	set w ""
	foreach d [array names DEFS] {
		lappend w [list $d $DEFS($d)]
	}

	catch { puts -nonewline $dfd $w }
	catch { close $dfd }
}

###############################################################################

proc disable_go { } {
	global GOBUT
	$GOBUT configure -state disabled
}

proc enable_go { } {
	global GOBUT
	$GOBUT configure -state normal
}

proc alert { msg } {

	tk_dialog .alert "Attention!" $msg "" 0 "OK"
}

proc mk_mess_window { w h } {
#
# Progress message
#
	set wn ".msg"

	toplevel $wn

	wm title $wn "Progress report"

	label $wn.t -width $w -height $h -borderwidth 2 -state normal
	pack $wn.t -side top -fill x -fill y
	button $wn.c -text "Cancel" -command cancel_gen
	pack $wn.c -side top
	bind $wn <Destroy> cancel_gen
	$wn.t configure -text ""
	raise $wn
}

proc cancelled { } {

	return ![winfo exists ".msg"]
}

proc outpgs { msg } {
#
# Progress message, detect cancellation
#
	if [cancelled] {
		return 1
	}

	catch { .msg.t configure -text $msg }

	return 0
}

proc cancel_gen { } {

	global EWAIT

	catch { destroy .msg }
	incr EWAIT
}

proc blink { } {

	global EWAIT

	incr EWAIT
}

proc change_ifn { } {
#
# Input name selection
#
	global DEFS IFN

	if { $DEFS(SRD) == "" } {
		set dd [defhome]
	} else {
		set dd $DEFS(SRD)
	}

	set fn [tk_getOpenFile \
			-defaultextension ".a43" \
			-initialdir $dd \
			-filetypes {{ "Intel Hex" {*.a43}} { "All" {*.*} }} \
			-title "Source Image File"]

	if { $fn == "" } {
		# no change
		return
	}

	set DEFS(SRD) [file dirname $fn]
	set DEFS(IFN) [file tail $fn]
	set IFN $fn
}

proc change_trd { } {
#
# Target directory selection
#
	global DEFS TFN

	set fn [tk_getSaveFile \
		-initialdir $DEFS(TRD) \
		-initialfile $DEFS(TFN) \
		-title "Target files"]

	if { $fn == "" } {
		# no change
		return
	}

	set DEFS(TRD) [file dirname $fn]
	set DEFS(TFN) [file tail $fn]
	set TFN $fn
}

proc generate { } {
#
# Do it
# 
	global TFN IFN DEFS EWAIT

	if { $IFN == "" } {
		alert "Input file has not been specified!"
		return
	}

	if { $TFN == "" } {
		alert "Destination file(s) unknown!"
		return
	}

	set str [verify_pnm $DEFS(FRM)]
	if { $str == "" } {
		alert "The FROM number is illegal, must be something integer\
			greater than zero and less than 65536!"
		return
	}

	set cnt [verify_pnm $DEFS(CNT)]
	if { $cnt == "" } {
		alert "The COUNT is illegal, must be something integer\
			greater than zero and less than 65536!"
		return
	}

	if { [expr $str + $cnt] > 65536 } {
		alert "FROM + COUNT is greater than 65536, which is illegal,\
			as the maximum node number is 65535!"
		return
	}

	set idn [verify_idn $DEFS(IDN)]
	if { $idn == "" } {
		alert "The cookie is illegal, must be an 8-digit hex number!"
		return
	}

	if [catch { read_source $IFN } image] {
		alert $image
		return
	}

	# transform str into hex string
	set frm [verify_idn "[string range $DEFS(IDN) 0 3][format %04X $str]"]
	if { $frm == "" } {
		# impossible
		alert "Internal error converting node number!"
		return
	}

	if [catch { locate_id $image [join $idn ""] } org] {
		alert $org
		return
	}

	disable_go
	mk_mess_window 32 1

	while { $cnt } {

		if [outpgs "Doing image $str, $cnt to go ..."] {
			# cancelled
			break
		}

		# wait for the message window to update
		after 100 blink
		tkwait variable EWAIT
		if [cancelled] {
			# in case
			break
		}

		set image [modify_image $image $org [join $frm ""]]

		set nfn [mkfname $TFN $frm]
		if [catch { write_image $image $nfn } err] {
			cancel_gen
			alert "Cannot write to $nfn: $err!"
			enable_go
			return
		}
		set frm [increment $frm]
		incr cnt -1
		incr str
	}
	cancel_gen
	enable_go

	if { $cnt == 0 } {
		alert "All done!"
		savdefs
	}
}


###############################################################################

catch { close stdin }
catch { close stdout }
catch { close stderr }

set EWAIT 0

setdefs

wm title . "Flash Image Generator $PM(VER)"

labelframe .fls -text "Files" -padx 4 -pady 4
pack .fls -side top -expand 1 -fill x

##

set w ".fls"

label $w.ifl -text "Input file:"
grid $w.ifl -column 0 -row 0 -sticky w -padx 4

entry $w.ifn -width 58 -textvariable IFN
grid $w.ifn -column 1 -row 0 -sticky we -padx 4 

button $w.ifb -text "Change" -command "change_ifn"
grid $w.ifb -column 2 -row 0 -sticky w -padx 4

##

label $w.odl -text "Output file(s): "
grid $w.odl -column 0 -row 1 -sticky w -padx 4

entry $w.odn -width 58 -textvariable TFN
grid $w.odn -column 1 -row 1 -sticky we -padx 4

button $w.odb -text "Change" -command "change_trd"
grid $w.odb -column 2 -row 1 -sticky w -padx 4

##

grid columnconfigure $w 1 -weight 10

##

frame .sel
pack .sel -side top -fill x

##

labelframe .sel.nn -text "Node numbers" -padx 4 -pady 4
pack .sel.nn -side left -expand 1 -fill x

##

set w ".sel.nn"

label $w.frl -text "From: "
pack $w.frl -side left -expand 0 -padx 4

entry $w.frn -width 6 -textvariable DEFS(FRM)
pack $w.frn -side left -expand 0 -padx 4

##

label $w.pad -text "  "
pack $w.pad -side left -expand 1 -fill x

##

label $w.cnl -text "How many: "
pack $w.cnl -side left -expand 0 -padx 4

entry $w.cnn -width 6 -textvariable DEFS(CNT)
pack $w.cnn -side left -expand 0 -padx 4

##

labelframe .sel.co -text "Cookie" -padx 4 -pady 4
pack .sel.co -side left -expand 0 -fill x -fill y

##

set w ".sel.co"

entry $w.co -width 10 -textvariable DEFS(IDN)
pack $w.co -side top -padx 4

##

frame .act -pady 4 -padx 4 -bd 2
pack .act -side top -expand 1 -fill x

##

set w ".act"

button $w.run -text "Generate" -command "generate"
pack $w.run -side left -padx 4

set GOBUT "$w.run"

button $w.q -text "Exit" -command "exit"
pack $w.q -side right -padx 4

bind . <Destroy> { exit }
