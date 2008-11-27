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
set SEA_RECSIZE		18	;# record size
set SEA_NCATS		16	;# number of categories (of each type)

set BN(PGS)	$LCDG_IM_PAGESIZE

#
# Record layout:
#
#	ID (4), ECats (2), MCats (2), Flags (2), Nick (2), Name (2), Note (2),
#	Image (2)
#
#	18 bytes total
#

# The starting locations of the respective chunks
set BN(FNT)	0
set BN(CAT)	[expr $BN(FNT) + $SEA_FONTSIZE]
set BN(REC)	[expr $BN(CAT) + 4 * $SEA_NCATS]
set BN(TXT)	[expr $BN(REC) + $SEA_RECSIZE * $SEA_NRECORDS]
# This is LWA+1 of the text area
set BN(ETX)	[expr $SEA_FIMPAGE * $LCDG_IM_PAGESIZE]

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

proc image_id { id } {
#
# Transforms record ID into picture file ID
#
	set id [string tolower $id]
	regexp "^0x(.*)" $id junk id
	return "image_${id}.nok"
}

proc alloc_string { st } {
#
# Allocate memory for string
#
	global MEM

	set len [string length $st]

	if { $len > 255 } {
		abt "string length > 255 characters: $st"
	}

	set st "[binary format c $len]$st"

	set loc [string first $st $MEM(TXT)]

	if { $loc >= 0 } {
		# already present
		return $loc
	}

	# allocate a new string
	set loc [string length $MEM(TXT)]

	append MEM(TXT) $st

	return $loc
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

proc do_categories { } {

	global BN DF SEA_NCATS

	set org $BN(CAT)

	foreach ct { "eventcategories" "mycategories" } {
		set ec [sxml_child $DF $ct]
		if { $ec == "" } {
			puts "Warning: <$ct> not found"
		}
		set cl [sxml_children $ec "category"]
		for { set i 0 } { $i < $SEA_NCATS } { incr i } {
			set cc($i) ""
		}
		set ni 0
		foreach c $cl {
			incr ni
			# ordinal present ?
			set od [sxml_attr $c "ord"]
			if { $od != "" } {
				if [catch { expr $od } ov] {
					abt "Illegal 'ord=\"$od\"', item $ni\
						of <$ct>"
					set od ""
				} else {
					set od $ov
				}
			}
			if { $od != "" } {
				if ![info exists cc($od)] {
					abt "Attribute 'ord=\"$od\"', item $ni\
						of <$ct> out of range"
				}
				if { $cc($od) != "" } {
					abt "Duplicate ord ($od), item $ni\
						of <$ct>"
				}
			} else {
				# find first free
				for { set od 0 } { $od < $SEA_NCATS }\
				    { incr od } {
					if { $cc($od) == "" } {
						break
					}
				}
				if { $od == $SEA_NCATS } {
					abt "too many items in <$ct>"
				}
			}
			# get the string
			set st [sxml_txt $c]
			if { $st == "" } {
				abt "category string in item $ni of <$ct> is\
					empty"
			}
			set cc($od) [alloc_string $st]
		}

		set ch ""
		set cs 0
		set end 0
		for { set i 0 } { $i < $SEA_NCATS } { incr i } {
			if { $cc($i) == "" } {
				# WNONE
				appendbs ch -1
				set end 1
			} else {
				if $end {
					abt \
					   "category list <$ct> contains a hole"
				}
				# note that these locations are relative to
				# the origin of the TXT area, so zero is
				# legit; also, WNONE (all ones) is a preferred
				# way to indicate 'nothing', as this is the
				# default contents of unprogrammed EEPROM
				appendbs ch $cc($i)
				# update chunk size
				set cs [expr ($i + 1) * 2]
			}
		}

		incr cs -1
		output_block $org [string range $ch 0 $cs]

		incr org [expr $SEA_NCATS * 2]
	}
}

proc numlist { str } {
#
# Extracts a list of nonnegative numbers from string
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
		return -1
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
	return $ih
}
		
proc do_people { } {

	global DF BN SEA_NCATS SEA_RECSIZE SEA_NRECORDS

	set ec [sxml_children [sxml_child $DF "people"] "record"]

	set data ""

	set ni 0
	foreach rc $ec {
		incr ni

		if { $ni > $SEA_NRECORDS } {
			abt "too many records"
		}
		set it [string tolower [sxml_attr $rc "class"]]
		if { $it == "" || $it == "yes" } {
			# default == yes
			set CL 1
		} elseif { $it == "no" } {
			set CL 2
		} elseif { $it == "ignore" } {
			set CL 0
		} else {
			abt "illegal class ($cl) at record $ni"
		}
		# the ID element is mandatory
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

		# collect other items #########################################

		set it [sxml_txt [sxml_child $rc "name"]]
		if { $it == "" } {
			if $CL {
				# name required if not ignore
				abt "name missing at record $ni"
			}
			# wnone
			set NM -1
		} else {
			set NM [alloc_string $it]
		}

		###############################################################

		set it [sxml_txt [sxml_child $rc "nickname"]]
		if { $it == "" } {
			# use name by default
			set NI $NM
		} else {
			set NI [alloc_string $it]
		}

		###############################################################

		set it [sxml_txt [sxml_child $rc "note"]]
		if { $it == "" } {
			set NO -1
		} else {
			set NO [alloc_string $it]
		}

		###############################################################

		set it [sxml_txt [sxml_child $rc "why"]]
		if { $it == "" } {
			if $CL {
				abt "no why list at record $ni"
			}
			set EC 0
			set MC 0
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
				abr "the why list is empty at record $ni"
			}
		}

		# image #######################################################

		set IM [handle_image $II]

		# append to data ##############################################

		appendbi data $ID
		appendbs data $EC
		appendbs data $MC
		appendbs data $CL
		appendbs data $NI
		appendbs data $NM
		appendbs data $NO
		appendbs data $IM
	}

	# check if no overflow

	output_block $BN(REC) $data
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
unset ff

###############################################################################

# initialize the text area, which will go last
set MEM(TXT) ""

# first free image page
set MEM(IMP) $SEA_FIMPAGE

# list of image blocks
set MEM(IML) ""

do_categories

# this must be the first image, if exists at all
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
