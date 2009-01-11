#!/bin/sh
###########################\
exec tclsh "$0" "$@"
#
# Creates EEPROM memory image from XML description
#

# set to 1 for absolute EEPROM image
set ABSOLUTE		1

set OEP_CHUNKLEN	54
set LCDG_IM_CHUNKLEN	[expr $OEP_CHUNKLEN + 2]
set LCDG_IM_PAGESIZE	8192
set LCDG_IM_PIXPERCHUNK	[expr ($OEP_CHUNKLEN * 8) / 12]
set LCDG_IM_CHUNKSPP	[expr $LCDG_IM_PAGESIZE / $LCDG_IM_CHUNKLEN]
set EEPROM_SIZE		[expr 512 * 1024]

set SEA_FONTSIZE	4096
set SEA_NRECORDS	128	;# maximum number of records stored
set SEA_FIMPAGE		4	;# first page of the picture area
set SEA_RECSIZE		20	;# record size
set SEA_NCATS		16	;# number of categories (of each type)

set LM(COL)		12	;# number of colors (from lcdg)

set BN(PGS)	$LCDG_IM_PAGESIZE

#
# Record layout:
#
#	ID (4), ECats (2), MCats (2), Class (2), MeterE (2), MeterM (2),
#	Note (2), Image (2), Name (2)
#
#	20 bytes total
#

# The starting locations of the respective chunks
set BN(FNT)	0
set BN(OFF)	[expr $BN(FNT) + $SEA_FONTSIZE]		;# offsets (6 bytes)
set BN(REN)	[expr $BN(OFF) + 3 * 2]			;# N records
set BN(REO)	[expr $BN(REN) + 2]			;# Org records
set BN(TXT)	[expr $BN(REO) + $SEA_RECSIZE * $SEA_NRECORDS]
# This is LWA+1 of the text area
set BN(ETX)	[expr $SEA_FIMPAGE * $LCDG_IM_PAGESIZE]

###############################################################################

# default menu parameters; width/height of zero means max based on the
# font size; we have: X, Y, fnt, bg, fg, w (chars), h (chars)
set DP(EC)	{ 0 0 0 0 1 0 0 }
set DP(MC)	$DP(EC)
set DP(PE)	$DP(EC)

# default text area parameters: X, Y, fnt, bg, fg, w (chars), h (chars, opt);
# if Y is negative, its is 129-Y and the text extends u; if h is defined
# (present ns nonzero), it means fixed and truncated, otherwise up to max
# X, Y 
set DP(ME)	{ 0 0 3 0 1 16 1}
set DP(MM)	{ 65 0 3 0 1 16 1}

set DP(NO)	{ 0 0 0 0 1 0 0}
set DP(NA)	$DP(NO)
set DP(NI)	$DP(NO)

###############################################################################

proc msource { f } {
#
# Intelligent 'source'
#
	if ![catch { uplevel #0 source $f } ] {
		# found it right here
		return
	}

	set dir "Scripts"
	set lst ""

	for { set i 0 } { $i < 10 } { incr i } {
		set dir "../$dir"
		set dno [file normalize $dir]
		if { $dno == $lst } {
			# no progress
			break
		}
		if ![catch { uplevel #0 source [file join $dir $f] } ] {
			# found it
			return
		}
	}

	# failed
	puts stderr "Cannot locate file $f 'sourced' by the script."
	exit 99
}

msource xml.tcl

###############################################################################

proc abt { m } {
	puts stderr $m
	exit 99
}

proc bad_usage { } {
	global argv0
	abt "Usage: $argv0 \[-i imagedir\] fontfile xmldatafile outputfile"
}

proc appendbc { wh v } {
#
# Append binary byte
#
	upvar $wh where
	append where [binary format c $v]
}

proc appendbs { wh v } {
#
# Append binary short
#
	upvar $wh where
	append where [binary format s $v]
}

proc appendbi { wh v } {
#
# Append binary int (4 bytes)
#
	upvar $wh where
	append where [binary format i $v]
}

###############################################################################

proc extract_font_sizes { ff } {
#
# Extract font sizes
#
	global FST

	set fl [string length $ff]

	if { [binary scan $ff ss mag nf] < 2 } {
		return "file too short"
	}

	if { [expr $mag & 0x00ffff] != [expr 0x07f01] } {
		return "bad magic code"
	}

	if { $nf <= 0 || $nf > 14 } {
		return "bad file header"
	}

	for { set i 0 } { $i < $nf } { incr i } {
		set ix [expr 4 + 2 * $i]
		binary scan [string range $ff $ix [expr $ix + 1]] s off
		set off [expr $off & 0x00ffff]
		if { $off + 16 > $fl } {
			return "corrupted file"
		}
		set hdr [string range $ff $off [expr $off + 1]]
		binary scan $hdr cc col row
		set col [expr $col & 0xff]
		set row [expr $row & 0xff]
		puts "Font $i ($col x $row)"
		set FST($i) [list $col $row]
	}

	puts "$nf fonts in the font file"
}

###############################################################################

proc image_id { id } {
#
# Transforms record ID into picture file ID
#
	set id [string tolower $id]
	regexp "^0x(.*)" $id junk id
	return "image_${id}.nok"
}

proc output_block { org data } {
#
# Writes a chunk to the output file
#
	global OFD ABSOLUTE

	set len [string length $data]

	if $ABSOLUTE {
		# create absolute image
		global EIM
		if { $org >= $EIM(LEN) } {
			while { $org > $EIM(LEN) } {
				append EIM(MEM) \xFF
				incr EIM(LEN)
			}
			append EIM(MEM) $data
			incr EIM(LEN) $len
			return
		}
		# overlap size
		set lwa [expr $org + $len]
		set EIM(MEM) [string replace $EIM(MEM) $org \
			[expr $org + $len - 1] $data]

		if { $EIM(LEN) < $lwa } {
			set EIM(LEN) $lwa
		}
		return
	}

	set out ""

	# a magic for consistency checks
	appendbi out [expr 0xcaca1112]
	appendbi out $org
	appendbi out $len

	append out $data

	puts "Chunk: [format %08x $org] \[[format %08x $len]\]"

	if [catch { puts -nonewline $OFD $out } err] {
		global ifile
		abt "cannot write to output file $ifile: $err"
	}
}

proc do_display_params { } {
#
# Process display parameters: just store them to be used for constructing the
# menus and stuff
#
	global DF DP BN

	# menus
	foreach tp { "eventcategories" "mycategories" "people" } \
	    ix { "EC" "MC" "PE" } {
		set pm [sxml_txt [sxml_child [sxml_child $DF $tp] "menu"]]
		parse_menu_params $pm $ix $tp
	}

	# meters
	foreach tp { "eventcategories" "mycategories" } ix { "ME" "MM" } {
		set pm [sxml_txt [sxml_child [sxml_child $DF $tp] "meter"]]
		parse_text_params $pm $ix $tp \
			[regexp -nocase "extend\[^a-z\]*up" $pm]
	}

	# record text attributes
	set pe [sxml_child $DF "people"]
	foreach tp { "note" "name" "nickname" } ix { "NO" "NA" "NI" } {
		set pm [sxml_txt [sxml_child $pe $tp]]
		parse_text_params $pm $ix $tp \
			[regexp -nocase "extend\[^a-z\]*up" $pm]
	}
}

proc numbers { str } {
#
# Extract the list of all (nonnegative) numbers found in the string
#
	set res ""

	while 1 {
		if ![regexp "\[0-9\]+" $str mat] {
			return $res
		}
		set str [string range $str [expr \
			[string first $mat $str] + [string length $mat]] \
				end]
		if [catch { expr $mat } mat] {
			error "llegal number"
		}

		lappend res $mat
	}
	return $res
}

proc cvalue { la lb ix } {

	set res [lindex $la $ix]
	if { $res == "" } {
		set res [lindex $lb $ix]
	}
	return $res
}

proc dserr { msg what } { abt "error in display parameters ($what): $msg" }

proc parse_menu_params { str ix what } {
#
# Parse categories parameters
#
	global FST DP LM

	if [catch { numbers $str } NL] { dserr $NL $what }

	set def $DP($ix)

	# font (index == 2)
	set F [cvalue $NL $def 2]

	if ![info exists FST($F)] {
		dserr "no font number $F" $what
	}

	set fw [lindex $FST($F) 0]
	set fh [lindex $FST($F) 1]

	# X, Y (corner coordinates)
	set X [cvalue $NL $def 0]
	set Y [cvalue $NL $def 1]

	# start the list
	set out [list $X $Y $F]

	set cb [cvalue $NL $def 3]
	if { $cb >= $LM(COL) } {
		dserr "bad background color $cb" $what
	}
	lappend out $cb

	set cb [cvalue $NL $def 4]
	if { $cb >= $LM(COL) } {
		dserr "bad foreground color $cb" $what
	}
	lappend out $cb

	# width and height
	set cb [cvalue $NL $def 5]
	# maximum width
	set mw [expr (130 - $X) / $fw]
	if { $mw < 2 } {
		set X [expr 130 - 2 * $fw]
		set mw 2
	}
	if { $cb == 0 || $cb > $mw } {
		set cb $mw
	}
	lappend out $cb

	set cb [cvalue $NL $def 6]
	# maximum height
	set mw [expr (130 - $Y) / $fh]
	if { $mw < 2 } {
		set Y [expr 130 - 2 * $fh]
		set mw 2
	}
	if { $cb == 0 || $cb > $mw } {
		set cb $mw
	}
	lappend out $cb

	set DP($ix) $out
}

proc parse_text_params { str ix what eup } {

	global DP LM FST

	if [catch { numbers $str } NL] { dserr $NL $what }

	set def $DP($ix)

	# font (index == 2)
	set F [cvalue $NL $def 2]

	if ![info exists FST($F)] {
		dserr "no font number $F" $what
	}
	
	set fw [lindex $FST($F) 0]
	set fh [lindex $FST($F) 1]

	# X, Y
	set X [cvalue $NL $def 0]
	set Y [cvalue $NL $def 1]

	# requested height
	set rh [cvalue $NL $def 6]

	# make sure Y is sane
	set nh [expr 130 - $fh]
	if { $Y > $nh } {
		set Y $nh
	}

	# determine maximum height
	if $eup {
		# coordinates refer to the bottom line
		# this is how many lines we can accommodate
		set nh [expr ($Y / $fh) + 1]
	} else {
		# coordinates refer to the top line
		set nh [expr (130 - $Y) / $fh]
	}

	if { $rh != 0 && $rh > $nh } {
		set rh $nh
	}

	# rh == 0 -> extensible, $nh == max number of lines, eup == direction
	#####################################################################

	# the maximum width
	set mw [expr (130 - $X) / $fw]
	if { $mw < 2 } {
		set X [expr 130 - 2 * $fw]
		set mw 2
	}

	# requested width
	set nw [cvalue $NL $def 5]
	if { $nw < 2 || $nw > $mw } {
		set nw $mw
	}

	set out [list $X $Y $F]

	# the colors
	set cb [cvalue $NL $def 3]
	if { $cb >= $LM(COL) } {
		dserr "bad background color $cb" $what
	}
	lappend out $cb

	set cb [cvalue $NL $def 4]
	if { $cb >= $LM(COL) } {
		dserr "bad foreground color $cb" $what
	}
	lappend out $cb

	lappend out $nw
	lappend out $rh

	lappend out $eup
	lappend out $nh

	# X Y F bg fg w h ext maxh (note: h can be zero -> extensible)

	set DP($ix) $out
}

proc alloc_menu { att lines } {
#
# Builds a menu
#
#    XLb YLb XHb YHb BLKw BLKw TPb Fn BGb FGb LINESw NLw Wb Hb FLw SHw SEw
#    ------- ------- ---- ---- ------ ------- ------ --- ----- --- --- ---
#        0      1      2    3     4      5       6    7    8    9   10  11
#
	global FST MEM

	set OFF(LI) 6
	set OFF(LE) 12

	set bk ""

	set NL [llength $lines]

	# coordinates
	set X [lindex $att 0]
	set Y [lindex $att 1]
	appendbc bk $X
	appendbc bk $Y

	set F [lindex $att 2]

	set fw [lindex $FST($F) 0]
	set fh [lindex $FST($F) 1]

	set w [lindex $att 5]
	set h [lindex $att 6]

	# bounding rectangle
	appendbc bk [expr $X + $fw * $w - 1]
	appendbc bk [expr $Y + $fh * $h - 1]

	# next + extras
	appendbs bk 0
	appendbs bk 0

	# type == menu
	appendbc bk 1

	# font
	appendbc bk $F

	# colors
	appendbc bk [lindex $att 3]
	appendbc bk [lindex $att 4]

	# Lines (right behind the menu structure - to be relocated)
	appendbs bk [expr $OFF(LE) << 1]

	# number of lines
	appendbs bk $NL

	# width/height
	appendbc bk $w
	appendbc bk $h

	# first line, shift, selection
	appendbs bk 0
	appendbs bk 0
	appendbs bk 0

	# OK, we have basically filled the record, now take care of the lines

	# the block of UNIX-formatted lines
	set lbk ""

	# the initial offset: skip the record + the Lines array
	set lof [expr ($OFF(LE) + $NL) << 1]

	foreach ln $lines {
		append ln "\x00"
		set len [string length $ln]
		appendbs bk $lof
		append lbk $ln
		incr lof $len
	}

	# we have the Lines array at the end; now, put everything together
	set bk "$bk$lbk"

	# turn it into a complete object
	set len [string length $bk]

	# this doesn't have to be aligned
	set lbk ""
	# prepend the length header
	appendbs lbk $len
	set bk "$lbk$bk"

	# follow it by the list of relocation points
	set lbk ""
	# the number of relocation points == 1 + NL
	appendbs lbk [expr $NL + 1]
	# the Lines array: 6
	appendbs lbk $OFF(LI)

	set lof $OFF(LE)
	for { set i 0 } { $i < $NL } { incr i } {
		# string relocation
		appendbs lbk [expr $lof | 0x8000]
		incr lof
	}

	# and this is the complete chunk
	set bk "$bk$lbk"

	set loc [string length $MEM(TXT)]

	append MEM(TXT) $bk

	return $loc
}

proc alloc_text { att line } {
#
# Allocates a text structure
#
#    XLb YLb XHb YHb BLKw BLKw TPb Fn BGb FGb LINEw Wb Hb
#    ------- ------- ---- ---- ------ ------- ----- -----
#       0       1      2    3     4      5      6     7
#
	global FST MEM

	set OFF(LI) 6
	# in words
	set OFF(LE) 8

	set bk ""

	# coordinates
	set X [lindex $att 0]
	set Y [lindex $att 1]

	# the font
	set F [lindex $att 2]

	set fw [lindex $FST($F) 0]
	set fh [lindex $FST($F) 1]

	# extensibility and stuff:
	#	actual width
	#	fixed height or 0, if extensible
	#	interpretation of the Y-coordinate (up direction if nonzero)
	#	maximum height

	set w [lindex $att 5]
	set h [lindex $att 6]
	set e [lindex $att 7]
	set m [lindex $att 8]

	# first, split the line based on the width, which is one known
	# parameter, and see how it works from there

	# count the total number of (broken) lines
	set nl 1
	# the processed string
	set out ""
	set t $line
	# length of current line
	set cl 0

	while 1 {
		set t [string trimleft $t]
		if { $t == "" } {
			break
		}
		# look at the next word to handle
		if ![regexp "^(\[^ \t\n\]+)(.*)" $t junk wd t] {
			# impossible
			abt "text line '$txt' contains illegal characters"
		}

		# length of the word
		set wl [string length $wd]

		if { $cl > 0 } {
			# some characters already present in the current line;
			# after adding the word the length will be ...
			set ul [expr $cl + $wl + 1]
			if { $ul > $w } {
				# this would exceed the bound, so must move to
				# new line
				while { $cl < $w } {
					# fill the current one to the end with
					# blanks
					append out " "
					incr cl
				}
				incr nl
				set cl 0
			} else {
				# the new word fits into the present line
				append out " $wd"
				set cl $ul
			}
		}

		# we will fall through this condition, if we must put the new
		# word into a new line
		if { $cl == 0 } {
			# at the beginning of a new line
			append out $wd
			while { $wl > $w } {
				# if the word itself is longer than w, then
				# it must be broken, anyway; note that this
				# happens automatically
				set wl [expr $wl - $w]
				incr nl
			}
			# and this is the final length of the last filled line
			set cl $wl
		}
	}

	if $h {
		# we have a fixed height
		if { $nl > $h } {
			set out [string range $out 0 [expr $h * $w - 1]]
			set nl $m
		} elseif { $nl < $h } {
			# fill it up with spaces (just for now - there should
			# be a smarter way, if we ever need this option,
			# probably not)
			set cl [string length $out]
			set lf [expr $cl % $w] 
			if { $lf != 0 } {
				while { $lf < $w } {
					append out " "
					incr lf
				}
			}
			while 1 {
				# force it to the next line
				append out " "
				incr nl
				if { $nl == $h } {
					break
				}
				for { set i 1 } { $i < $w } { incr i } {
					append out " "
				}
			}
		}
	} elseif { $nl > $m } {
		# more than maximum height, trim it down
		set out [string range $out 0 [expr $m * $w - 1]]
		set nl $m
	}
				
	if $e {
		# extend up
		set Y [expr $Y - $nl * $fh]
	}
			
	appendbc bk $X
	appendbc bk $Y
	appendbc bk [expr $X + $fw * $w - 1]
	appendbc bk [expr $Y + $fh * $nl - 1]

	# next + extras
	appendbs bk 0
	appendbs bk 0

	# type == text
	appendbc bk 2

	# font
	appendbc bk $F

	# colors
	appendbc bk [lindex $att 3]
	appendbc bk [lindex $att 4]

	# Line (right behind the text structure - to be relocated)
	appendbs bk [expr $OFF(LE) << 1]

	# width and height
	appendbc bk $w
	appendbc bk $nl

	# the line
	append bk $out
	append bk "\x00"

	# that's it
	set len [string length $bk]

	set lbk ""
	appendbs lbk $len
	set bk "$lbk$bk"

	# there is a single relocation point
	appendbs bk 1
	appendbs bk [expr 0x8000 + $OFF(LI)]

	# that's it
	set loc [string length $MEM(TXT)]
	append MEM(TXT) $bk
	return $loc
}

proc alloc_image { handle x y } {
#
# Allocates an image structure
#
	global MEM

	set xd [expr (130 - $x) / 2]
	set yd [expr (130 - $x) / 2]

	set bk ""

	appendbc bk $xd
	appendbc bk $yd

	appendbc bk [expr $xd + $x - 1]
	appendbc bk [expr $yd + $y - 1]

	appendbs bk 0
	appendbs bk 0

	# type == 0
	appendbc bk 0
	# alignment
	appendbc bk 0

	appendbs bk $handle

	set lw ""
	appendbs lw [string length $bk]
	# there is no relocation
	appendbs bk 0
	set loc [string length $MEM(TXT)]
	append MEM(TXT) "$lw$bk"

	return $loc
}

proc meter_string { wc } {
#
# Create a meter string from flags
#
	set out ""
	for { set i 0 } { $i < 16 } { incr i } {
		set bit [expr ($wc >> $i) & 1]
		if $bit {
			append out "a"
		} else {
			append out "b"
		}
	}
	return $out
}
	
proc do_categories { } {

	global BN DF SEA_NCATS DP

	# the offsets (or pointers)
	set off ""

	foreach ct { "eventcategories" "mycategories" } ix { "EC" "MC" } {
		set ec [sxml_child $DF $ct]
		if { $ec == "" } {
			abt "<$ct> not found"
		}
		set cl [sxml_children $ec "category"]
		# extract the texts
		set tel ""
		# counter
		set ni 0
		foreach c $cl {
			if { $ni == $SEA_NCATS } {
				abt "too many categories in <$ct>, $SEA_NCATS\
					is the max"
			}
			# get the string
			set st [sxml_txt $c]
			if { $st == "" } {
				abt "category string in item $ni of <$ct> is\
					empty"
			}
			# prefix
			set pc "[string toupper [string index $ct 0]]$ni"
			set st "$pc.$st"
			lappend tel $st
			incr ni
		}

		appendbs off [alloc_menu $DP($ix) $tel]
	}
	output_block $BN(OFF) $off
}

proc numlist { str } {
#
# Converts a list of numbers into flags
#
	global SEA_NCATS

	set flg 0

	while 1 { 
		if ![regexp "\[0-9\]+" $str mat] {
			return $flg
		}
		set str [string range $str [expr \
			[string first $mat $str] + [string length $mat]] \
				end]
		if [catch { expr $mat } mat] {
			error "llegal number"
		}

		if { $mat >= $SEA_NCATS } {
			error "number too big"
		}

		set mat [expr 1 << $mat]

		if [expr $flg & $mat] {
			error "duplicate number"
		}

		set flg [expr $flg | $mat]
	}
	return $flg
}

proc handle_image { id } {
#
# Load image
#
	global MEM LCDG_IM_PIXPERCHUNK LCDG_IM_CHUNKSPP OEP_CHUNKLEN
	global LCDG_IM_CHUNKLEN LCDG_IM_PAGESIZE EEPROM_SIZE EIM
	

	if { $EIM(DIR) != "" } {
		set id [file join $EIM(DIR) $id]
	}

	# for now, this is treated as a file name
	if [catch { open $id "r" } ifd] {
		# no image: WNONE
		return ""
	}

	# read the image
	fconfigure $ifd -encoding binary -translation binary

	if [catch { read $ifd } IMG] {
		abt "cannot read image file $id: $IMG"
	}

	catch { close $ifd }

	set ll [string length $IMG]
	if { $ll < 1024 } {
		# basic sanity check
		abt "image file format error: $id $ll"
	}

	# decode the header
	binary scan $IMG scc w x y

	set w [expr $w & 0xffff]
	set x [expr $x & 0xff]
	set y [expr $y & 0xff]

	# more sanity checks
	if { $w != [expr 0x77ac] ||
	    $x < 8 || $x > 130 || $y < 8 || $y > 130 } {
		abt "bad image file header: $id"
	}

	# initialize the output chunk
	set res ""

	####
	appendbs res [expr 0x7f00]

	# the total number of chunks - the header chunk
	set nc [expr (($x * $y) + \
		$LCDG_IM_PIXPERCHUNK - 1) / $LCDG_IM_PIXPERCHUNK]

	# the total number of pages
	set np [expr ($nc + $LCDG_IM_CHUNKSPP) / $LCDG_IM_CHUNKSPP]

	# file length test
	if { $ll < [expr ($x * $y * 12 + 7) / 8 + 42] } {
		abt "image file $id too short"
	}

	####
	appendbs res $np

	# starting page number
	set pn $MEM(IMP)
	for { set i 0 } { $i < $np } { incr i } {
		####
		appendbs res $pn
		incr pn
	}

	while { $i < 5 } {
		####
		appendbs res 0
		incr i
	}

	####
	appendbs res $nc

	####
	appendbc res $x
	####
	appendbc res $y

	# the label
	append res [string range $IMG 4 41]

	# we are done with the header
	set IMG [string range $IMG 42 end]

	# fill count of the first page
	set pfill $LCDG_IM_CHUNKLEN
	# maximum fill to accommodate a chunk
	set mfill [expr $LCDG_IM_PAGESIZE - $LCDG_IM_CHUNKLEN]

	set cp 0
	set ce [expr $OEP_CHUNKLEN - 1]

	for { set i 0 } { $i < $nc } { incr i } {

		# chunk from file
		set chk [string range $IMG $cp $ce]
		incr cp $OEP_CHUNKLEN
		incr ce $OEP_CHUNKLEN

		# page boundary
		if { $pfill > $mfill } {
			# zero out the tail
			while { $pfill < $LCDG_IM_PAGESIZE } {
				appendbc res 0
				incr pfill
			}
			set pfill 0
		}

		# write the chunk
		appendbs res $i
		append res $chk
		incr pfill $LCDG_IM_CHUNKLEN
	}

	# image handle
	set ih $MEM(IMP)

	# update the next free page
	incr MEM(IMP) $np

	# check for overflow
	if { [expr $MEM(IMP) * $LCDG_IM_PAGESIZE] > $EEPROM_SIZE } {
		abt "too many images"
	}

	output_block [expr $ih * $LCDG_IM_PAGESIZE] $res
	return [list $ih $x $y]
}
		
proc do_people { } {

	global DF BN SEA_NCATS SEA_NRECORDS DP

	set ec [sxml_children [sxml_child $DF "people"] "record"]

	set data ""

	set ni 0
	set mlines ""

	foreach rc $ec {

		if { $ni >= $SEA_NRECORDS } {
			abt "too many records, $SEA_NRECORDS is the max"
		}

		# class (a single character) ##################################
		set it [string tolower [sxml_attr $rc "class"]]
		if { $it == "" } {
			# the default class i "+"
			set it "+"
		} elseif { [string length $it] != 1 } {
			abt "class in record $ni ($it) is not a single\
				character"
		}
		# the numerical code
		scan $it %c CL
		# just in case
		set CL [expr $CL & 0x00ff]

		# ID (a lword) ################################################
		set it [sxml_child $rc "id"]
		if { $it == "" } {
			abt "id element missing at record $ni"
		}
		set it [sxml_txt $it]
		if [catch { expr $it } ID] {
			abt "illegal value of id ($it) at record $ni"
		}
		# this is supposed to produce the file name of the image
		set II [image_id $it]

		# name ########################################################
		set nm [sxml_txt [sxml_child $rc "name"]]
		if { $nm == "" } {
			# name required
			abt "name missing at record $ni"
		}

		set NM [alloc_text $DP(NA) $nm]

		# nickname (prefix with ".. " #################################
		set it [sxml_txt [sxml_child $rc "nickname"]]
		if { $it == "" } {
			set it $nm
		}
		set it ".. $it"

		# used for the menu
		lappend mlines $it

		#	set NI [alloc_text $it]
		# do we need to store it separately

		# note (have to format it) ####################################
		set it [sxml_txt [sxml_child $rc "note"]]
		if { $it == "" } {
			set NO -1
		} else {
			set NO [alloc_text $DP(NO) $it]
		}

		# EC / MC bits ################################################
		set it [sxml_txt [sxml_child $rc "why"]]
		if { $it == "" } {
			abt "no <why> list at record $ni"
		} else {
			set p [string first ";" $it]
			if { $p < 0 } {
				set el $it
				set ml ""
			} else {
				set el [string trimright [string range $it 0 \
					[expr $p - 1]]]
				set ml [string trimleft [string range $it \
					[expr $p + 1] end]]
			}
			if [catch { expr [numlist $el] } EC] {
				abt "$EC in event categories of record $ni"
			}
			if [catch { expr [numlist $ml] } MC] {
				abt "$MC in my categories of record $ni"
			}
			if { $EC == 0 && $MC == 0 } {
				abt "the <why> list at record $ni is empty"
			}
		}

		# build the meters

		set it [meter_string $EC]
		set ME [alloc_text $DP(ME) $it]
		set it [meter_string $MC]
		set MM [alloc_text $DP(MM) $it]

		# image #######################################################
		set IM [handle_image $II]
		if { $IM != "" } {
			set IM [alloc_image [lindex $IM 0] [lindex $IM 1] \
				[lindex $IM 2]]
		} else {
			set IM -1
		}

		# append to data ##############################################
		appendbi data $ID
		appendbs data $EC
		appendbs data $MC
		appendbs data $CL
		appendbs data $ME
		appendbs data $MM
		appendbs data $NO
		appendbs data $IM
		# appendbs data $NI
		appendbs data $NM
		incr ni
	}

	# create the menu
	set mp [alloc_menu $DP(PE) $mlines]
	set bk ""
	appendbs bk $mp
	output_block [expr $BN(OFF) + 4] $bk

	set it ""
	appendbs it $ni
	output_block $BN(REN) "$it$data"
}

###############################################################################

# image directory
set EIM(DIR) ""

if { [lindex $argv 0] == "-i" } {
	set EIM(DIR) [lindex $argv 1]
	if { $EIM(DIR) == "" } {
		bad_usage
	}
	set argv [lrange $argv 2 end]
}

set ffile [lindex $argv 0]
set dfile [lindex $argv 1]
set ifile [lindex $argv 2]

if { $ffile == "" || $ifile == "" || $dfile == "" } {
	bad_usage
}

if [catch { open $dfile "r" } dfd] {
	abt "cannot open data file $dfile: $dfd"
}

if [catch { read $dfd } DF] {
	abt "cannot read data file $dfile: $DF"
}

catch { close $dfd }

if [catch { sxml_parse DF } DF] {
	abt "XML error in data file $dfile: $DF"
}

set DF [sxml_child $DF "sealist"]
if { $DF == "" } {
	abt "sealist tag not found in the XML data file"
}

#### take care of the fonts file, which goes first

if [catch { open $ffile "r" } dfd] {
	abt "cannot open font file $ffile: $dfd"
}

fconfigure $dfd -encoding binary -translation binary

if [catch { read $dfd } ff] {
	abt "cannot read font file $ffile: $ff"
}

catch { close $dfd }

set dfd [string length $ff]

if { $dfd > $SEA_FONTSIZE } {
	abt "font file ($ffname) too long ($dfd), $SEA_FONTSIZE is the max"
} elseif { $dfd < 128 } {
	abt "font file too short (corrupted?)"
}

#### start the output file

if [catch { open $ifile "w" } OFD] {
	abt "cannot open output file $ifile for writing: $OFD"
}

fconfigure $OFD -encoding binary -translation binary

# memory image
set EIM(MEM) ""
set EIM(LEN) 0

output_block $BN(FNT) $ff

# we shall need these to properly pre-scale things in the script
extract_font_sizes $ff

unset ff

###############################################################################

puts "Offsets:"
puts "            Fonts:      [format %04x $BN(FNT)] = $BN(FNT)"
puts "            Pointers:   [format %04x $BN(OFF)] = $BN(OFF)"
puts "            Records:    [format %04x $BN(REN)] = $BN(REN)"
puts "            Texts:      [format %04x $BN(TXT)] = $BN(TXT)"
puts "            Images:     [format %04x $BN(ETX)] = $BN(ETX)"

# initialize the text (menu) area, which will go last
set MEM(TXT) ""

# first free image page
set MEM(IMP) $SEA_FIMPAGE

# list of image blocks
set MEM(IML) ""

# display parameters
do_display_params

do_categories

# this must be the first image, if it exists at all
handle_image "wallpaper.nok"

do_people

# check for text overflow
if { [string length $MEM(TXT)] > [expr $BN(ETX) - $BN(TXT)] } {
	abt "text area overflow"
}

# write the text block
output_block $BN(TXT) $MEM(TXT)

# that's it
if $ABSOLUTE {
	if [catch { puts -nonewline $OFD $EIM(MEM) } err] {
		abt "cannot write to output file $ifile: $err"
	}
}
