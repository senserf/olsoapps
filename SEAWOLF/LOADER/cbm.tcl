#!/bin/sh
###########################\
exec tclsh "$0" "$@"
#
# Converts from 24 bpp Windows bitmap to Olsonet's bitmap format for the
# Nokia 6100 LCD
#
# BMP file format
#
# Bytes 0, 1 == 'B''M'
# Bytes 2-5 LE = total file size in bytes
#       6-9 zero
#       10-13 LE = offset to image data from byte 0
#
# Header starts at byte 14
#
#       14-17 LE = header size in bytes
#       18-21 LE = x width in pixels
#       22-25 LE = y height in pixels
#       26-27 LE = number of planes (0001)
#	28-29 LE = bits per pixels (24) 1, 4, 8, 16, 24, 32
#	30-33    = compression type (0)
#	34-37    = size of image data in bytes (= file size - offset im data)
#	38-45    = conversion to metric units (ignore)
#	46-49    = number of colors
#	50-53    = some number of colors (both zeros)
#
# Olsonet format (all numbers little endian):
#
#        0-3     = 'o''l''s''o'
#	 4-5     = 0 (12bpp), 1 (8bpp)
#        6-7     = X (width) in pixels
#        8-9     = Y (height) in pixels
#       10-11    = label length in bytes
#       ....     = that many bytes of anything (used as a label)
#
#       This is followed by the list of pixels. With 8bpp, every pixel takes
#       exactly one byte (and is coded according to the 8bpp coding for the
#       Nokia LCD). With 12bpp, two pixels take 3 bytes. If the last pixel is
#       odd, it takes two bytes. In any case, the total number of pixels is
#       determined as X * Y. They are placed by rows.
#
#       

proc put1 { v } {

	global SIO

	append SIO(OB) [binary format c $v]
}

proc put2 { v } {

	global SIO

	append SIO(OB) [binary format s $v]
}

proc put4 { v } {

	global SIO

	append SIO(OB) [binary format i $v]
}

proc get1 { } {

	global SIO IDX

	set B [string index $SIO(IB) $IDX]

	if { $B == "" } {
		# just in case
		return 0
	}

	incr IDX

	binary scan $B c b
	return [expr $b & 0xff]
}

proc get2 { } {

	global SIO IDX

	set B [string index $SIO(IB) $IDX]
	if { $B == "" } {
		return 0
	}
	incr IDX
	set C [string index $SIO(IB) $IDX]
	if { $C == "" } {
		return 0
	}
	incr IDX

	binary scan $B$C s w
	return [expr $w & 0xffff]
}

proc get4 { } {

	global SIO IDX

	set B [string index $SIO(IB) $IDX]
	if { $B == "" } {
		return 0
	}
	incr IDX
	set C [string index $SIO(IB) $IDX]
	if { $C == "" } {
		return 0
	}
	incr IDX
	set D [string index $SIO(IB) $IDX]
	if { $D == "" } {
		return 0
	}
	incr IDX
	set E [string index $SIO(IB) $IDX]
	if { $E == "" } {
		return 0
	}
	incr IDX

	binary scan $B$C$D$E i d

	return $d
}

proc init { } {

	global SIO

	set a [get1]
	set b [get1]

	if { $a != [expr 0x42] || $b != [expr 0x4d] } {
		abt "source not a bitmap file"
	}

	# skip irrelevant stuff
	get4
	get4
	set off [get4]

	get4
	set x [get4]
	set y [get4]

	if { $x == 0 || $x > 130 || $y == 0 || $y > 130 } {
		abt "the dimensions, <$x,$y>, are larger than we can handle"
	}

	set SIO(X) $x
	set SIO(Y) $y

	puts "image dimensions: <$x,$y>"

	# bpp (only 24 bits supported for now)
	get2
	set bpp [get2]

	if { $bpp != 24 } {
		abt "there are $bpp bits per pixel,\
				we only know how to handle 24"
	}

	# we are basically set; this is how many bytes remain to be skipped
	incr off -30

	while { $off } {
		get1
		incr off -1
	}

	set SIO(OB) "olso"

	if $SIO(EBP) {
		# bits per pixel
		put2 1
	} else {
		put2 0
	}

	# dimensions
	put2 $SIO(X)
	put2 $SIO(Y)

	puts "Enter label or an empty line:"
	set SIO(LAB) [string trim [gets stdin]]

	# label
	set ll [string length $SIO(LAB)]
	put2 $ll
	append SIO(OB) $SIO(LAB)
}

proc reverse_y { } {

	global SIO IDX

	set SIO(IB) [string range $SIO(IB) $IDX end]
	set NEW ""

	# bytes in a row - 1
	set nused [expr $SIO(X) * 3 - 1]
	# total number of bytes in a row
	set ntotl [expr ($nused + 3) & ~3]
	# start from the last row
	set ptr [expr $ntotl * $SIO(Y)]

	for { set y 0 } { $y < $SIO(Y) } { incr y } {
		incr ptr -$ntotl
		append NEW [string range $SIO(IB) $ptr [expr $ptr + $nused]]
	}

	set IDX 0
	set SIO(IB) $NEW
}

proc do_pixels { } {

	global SIO

	# odd pixel (for 12bpp)
	set SIO(OPX) ""

	for { set y 0 } { $y < $SIO(Y) } { incr y } {
		set nc 0
		# puts "Row $y, left [string length $SIO(IB)]"
		for { set x 0 } { $x < $SIO(X) } { incr x } {
			set b [get1]
			set g [get1]
			set r [get1]

			incr nc 3

			outpixel $r $g $b

		}
	}

	flushpixel

	puts "Image written: [string length $SIO(OB)] bytes total"
}
		
proc outpixel { r g b } {

	global SIO

	if $SIO(EBP) {
		# 8bpp
		set r [expr ($r * 8) / 256]
		set g [expr ($g * 8) / 256]
		set b [expr ($b * 4) / 256]
		put1 [expr ($r << 5) | ($g << 2) | $b]
		return
	}

	# the trickier bit

	set r [expr ($r * 16) / 256]
	set g [expr ($g * 16) / 256]
	set b [expr ($b * 16) / 256]

	if { $SIO(OPX) == "" } {
		put1 [expr ($r << 4) | $g]
		set SIO(OPX) [expr $b << 4]
	} else {
		put1 [expr $SIO(OPX) | $r]
		put1 [expr ($g << 4) | $b]
		set SIO(OPX) ""
	}
}

proc flushpixel { } {

	global SIO

	if { $SIO(EBP) && $SIO(OPX) != "" } {
		put1 $SIO(OPX)
	}
}

proc abt { m } {

	puts stderr $m
	exit 99
}

set bo [lindex $argv 0]

set SIO(EBP) 0

if { $bo == "-8" || $bo == "-12" } {
	set argv [lrange $argv 1 end]
	if { $bo == "-8" } {
		set SIO(EBP) 1
	}
}

if { [llength $argv] != 2 } {
	abt "Usage: $argv0 \[-8|-12\] infile outfile"
}

set ifile [lindex $argv 0]
set ofile [lindex $argv 1]

if [catch { open $ifile "r" } ifd] {
	abt "cannot open input file $ifile: $ifd"
}

if [catch { open $ofile "w" } ofd] {
	abt "cannot open output file $ofile: $ofd"
}

fconfigure $ifd -translation binary
fconfigure $ofd -translation binary

set SIO(IB) [read $ifd]
set IDX 0
close $ifd

init

reverse_y

do_pixels

puts -nonewline $ofd $SIO(OB)

close $ofd
