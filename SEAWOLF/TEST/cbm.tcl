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
#        0-1     = 0x77AC
#	 2-3     = X and Y in pixels
#        4-41    = label
#        ....    = chunks (a multiple of 54 bytes)
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

	set SIO(OB) ""

	put2 [expr 0x77AC]

	# dimensions
	put1 $SIO(X)
	put1 $SIO(Y)

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

	if { $SIO(OPX) != "" } {
		put1 $SIO(OPX)
	}
}

proc abt { m } {

	puts stderr $m
	exit 99
}

proc usage { } {
	global argv0
	abt "Usage: $argv0 \[-l\[n\] label\] infile outfile"
}

proc numlab { str } {
#
# Build a numerical label
#
	global SIO

	set SIO(OB) ""
	set str [string tolower $str]

	while 1 {
		set mx ""
		set mi ""
		regexp "0x\[0-9a-f\]+" $str mx
		regexp "\[+-\]?\[0-9\]+" $str mi
		if { $mx != "" } {
			set mxp [string first $mx $str]
		} elseif { $mi == "" } {
			break
		} else {
			set mxp [expr 0x7fffffff]
		}
		if { $mi != "" } {
			set mip [string first $mi $str]
		} else {
			set mip [expr 0x7fffffff]
		}
		if { $mip >= $mxp } {
			set mi $mx
			set mip $mxp
		}
		if [catch { expr $mi } miv] {
			usage
		}
		set str \
		    [string range $str [expr $mip + [string length $mi]] end]
		put4 $miv
	}

	return $SIO(OB)
}

set nl ""
if [regexp "^-l(n)?$" [lindex $argv 0] junk nl] {
	# label
	set SIO(LAB) [lindex $argv 1]
	if { $SIO(LAB) == "" } {
		usage
	}
	set argv [lrange $argv 2 end]
	if { $nl != "" } {
		set SIO(LAB) [numlab $SIO(LAB)]
	}
	if { [string length $SIO(LAB)] == 0 } {
		usage
	}
} else {
	set SIO(LAB) ""
}
	
if { [llength $argv] < 2 } {
	usage
}

set ifile [lindex $argv 0]
set ofile [lindex $argv 1]

if { $SIO(LAB) == "" } {
	# use the file name (sans suffix) as the default label
	set SIO(LAB) [string tolower [file rootname [file tail $ofile]]]
}
puts "LABEL '$SIO(LAB)'"

set ll [string length $SIO(LAB)]
	
if { $ll > 38 } {
	set SIO(LAB) [string range $SIO(LAB) 0 37]
} else {
	while { $ll < 38 } {
		append SIO(LAB) \x00
		incr ll
	}
}

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
