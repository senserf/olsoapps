#!/bin/sh
###########################\
exec tclsh "$0" "$@"

set	Files(VP)	""		;# path to the OSS directory
set	Files(SVAL)	"values"
set	Files(SMAP)	"sensors.xml"

###############################################################################

package provide xml 1.0
#
# Mini XML parser
#

namespace eval XML {

proc xstring { s } {
#
# Extract a possibly quoted string
#
	upvar $s str

	if { [xspace str] != "" } {
		error "illegal white space"
	}

	set c [string index $str 0]
	if { $c == "" } {
		error "empty string illegal"
	}

	if { $c != "\"" } {
		# no quote; this is formally illegal in XML, but let's be
		# pragmatic
		regexp "^\[^ \t\n\r\>\]+" $str val
		set str [string range $str [string length $val] end]
		return [xunesc $val]
	}

	# the tricky way
	if ![regexp "^.(\[^\"\]*)\"" $str match val] {
		error "missing \" in string"
	}
	set str [string range $str [string length $match] end]

	return [xunesc $val]
}

proc xunesc { str } {
#
# Remove escapes from text
#
	regsub -all "&amp;" $str "\\&" str
	regsub -all "&quot;" $str "\"" str
	regsub -all "&lt;" $str "<" str
	regsub -all "&gt;" $str ">" str
	regsub -all "&nbsp;" $str " " str

	return $str
}

proc xspace { s } {
#
# Skip white space
#
	upvar $s str

	if [regexp -indices "^\[ \t\r\n\]+" $str ix] {
		set ix [lindex $ix 1]
		set match [string range $str 0 $ix]
		set str [string range $str [expr $ix + 1] end]
		return $match
	}

	return ""
}

proc xcmnt { s } {
#
# Skip a comment
#
	upvar $s str

	set sav $str

	set str [string range $str 4 end]
	set cnt 1

	while 1 {
		set ix [string first "-->" $str]
		set iy [string first "<!--" $str]
		if { $ix < 0 } {
			error "unterminated comment: [string range $sav 0 15]"
		}
		if { $iy > 0 && $iy < $ix } {
			incr cnt
			set str [string range $str [expr $iy + 4] end]
		} else {
			set str [string range $str [expr $ix + 3] end]
			incr cnt -1
			if { $cnt == 0 } {
				return
			}
		}
	}
}

proc xftag { s } {
#
# Find and extract the first tag in the string
#
	upvar $s str

	set front ""

	while 1 {
		# locate the first tag
		set ix [string first "<" $str]
		if { $ix < 0 } {
			set str "$front$str"
			return ""
		}
		append front [string range $str 0 [expr $ix - 1]]
		set str [string range $str $ix end]
		# check for a comment
		if { [string range $str 0 3] == "<!--" } {
			# skip the comment
			xcmnt str
			continue
		}
		set et ""
		if [regexp -nocase "^<(/)?\[a-z:_\]" $str ix et] {
			# this is a tag
			break
		}
		# skip the thing and keep going
		append front "<"
		set str [string range $str 1 end]
	}

	if { $et != "" } {
		set tm 1
	} else {
		set tm 0
	}

	if { $et != "" } {
		# terminator, skip the '/', so the text is positioned at the
		# beginning of keyword
		set ix 2
	} else {
		set ix 1
	}

	# starting at the keyword
	set str [string range $str $ix end]

	if ![regexp -nocase "^(\[a-z0-9:_\]+)(.*)" $str ix kwd str] {
		# error
		error "illegal tag: [string range $str 0 15]"
	}

	set kwd [string tolower $kwd]

	# decode any attributes
	set attr ""
	array unset atts

	while 1 {
		xspace str
		if { $str == "" } {
			error "unterminated tag: <$et$kwd"
		}
		set c [string index $str 0]
		if { $c == "/" } {
			# self-terminating
			if { $tm != 0 || [string index $str 1] != ">" } {
				error "broken self-terminating tag:\
					<$et$kwd ... [string range $str 0 15]"
			}
			set str [string range $str 2 end]
			return [list 2 $front $kwd $attr]
		}
		if { $c == ">" } {
			# done
			set str [string range $str 1 end]
			# term preceding_text keyword attributes
			return [list $tm $front $kwd $attr]
		}
		# this must be a keyword
		if ![regexp -nocase "^(\[a-z\]\[a-z0-9_\]*)=" $str match atr] {
			error "illegal attribute: <$et$kwd ... [string range \
				$str 0 15]"
		}
		set atr [string tolower $atr]
		if [info exists atts($attr)] {
			error "duplicate attribute: <$et$kwd ... $atr"
		}
		set atts($atr) ""
		set str [string range $str [string length $match] end]
		if [catch { xstring str } val] {
			error "illegal attribute value: \
				<$et$kwd ... $atr=[string range $str 0 15]"
		}
		lappend attr [list $atr $val]
	}
}

proc xadv { s kwd } {
#
# Returns the text + the list of children for the current tag
#
	upvar $s str

	set txt ""
	set chd ""

	while 1 {
		# locate the nearest tag
		set tag [xftag str]
		if { $tag == "" } {
			# no more
			if { $kwd != "" } {
				error "unterminated tag: <$kwd ...>"
			}
			return [list "$txt$str" $chd]
		}

		set md [lindex $tag 0]
		set fr [lindex $tag 1]
		set kw [lindex $tag 2]
		set at [lindex $tag 3]

		append txt $fr

		if { $md == 0 } {
			# opening, not self-closing
			set cl [xadv str $kw]
			# inclusion ?
			set tc [list $kw [lindex $cl 0] $at [lindex $cl 1]]
			if ![xincl str $tc] {
				lappend chd $tc
			}
		} elseif { $md == 2 } {
			# opening, self-closing
			set tc [list $kw "" $at ""]
			if ![xincl str $tc] {
				lappend chd $tc
			}
		} else {
			# closing
			if { $kw != $kwd } {
				error "mismatched tag: <$kwd ...> </$kw>"
			}
			# we are done with the tag - check for file
			# inclusion
			return [list $txt $chd]
		}
	}
}

proc xincl { s tag } {
#
# Process an include tag
#
	set kw [lindex $tag 0]

	if { $kw != "include" && $kw != "xi:include" } {
		return 0
	}

	set fn [sxml_attr $tag "href"]

	if { $fn == "" } {
		error "href attribute of <$kw ...> is empty"
	}

	if [catch { open $fn "r" } fd] {
		error "cannot open include file $fn: $fd"
	}

	if [catch { read $fd } fi] {
		catch { close $fd }
		error "cannot read include file $fn: $fi"
	}

	# merge it
	upvar $s str

	set str $fi$str

	return 1
}

proc sxml_parse { s } {
#
# Builds the XML tree from the provided string
#
	upvar $s str

	set v [xadv str ""]

	return [list root [lindex $v 0] "" [lindex $v 1]]
}

proc sxml_name { s } {

	return [lindex $s 0]
}

proc sxml_txt { s } {

	return [string trim [lindex $s 1]]
}

proc sxml_attr { s n } {

	set al [lindex $s 2]
	set n [string tolower $n]
	foreach a $al {
		if { [lindex $a 0] == $n } {
			return [lindex $a 1]
		}
	}
	return ""
}

proc sxml_children { s { n "" } } {

	set cl [lindex $s 3]

	if { $n == "" } {
		return $cl
	}

	set res ""

	foreach c $cl {
		if { [lindex $c 0] == $n } {
			lappend res $c
		}
	}

	return $res
}

proc sxml_child { s n } {

	set cl [lindex $s 3]

	foreach c $cl {
		if { [lindex $c 0] == $n } {
			return $c
		}
	}

	return ""
}

namespace export sxml_*

### end of XML namespace ######################################################

}

namespace import ::XML::*

###############################################################################

#
# HTML form processor
#

proc getValues { } {

	global Values

	while 1 {

		if [catch { gets stdin line } stat] {
			error $stat
		}

		if { $stat < 0 } {
			return
		}
		if { [string first = $line] >= 0 } {
			# this looks like a valid line with some values
			set line [split $line &]
			foreach entry $line {
				if [regexp -- "(.*)=(\[^\n\r\]*)" $entry \
				    junk param value] {
					lappend Values($param) \
						[string trim [unescape $value]]
				}
			}
		}
	}
}

proc unescape { str } {

	set os ""

	regsub -all "\\+" $str " " str

	while 1 {

		set ix [string first "%" $str]
		if { $ix < 0 } {
			append os $str
			return $os
		}

		append os [string range $str 0 [expr $ix - 1]]
		set str [string range $str [expr $ix + 1] end]

		if ![regexp {^[0-9a-fA-F][0-9a-fA-F]} $str match] {
			append os "%"
			continue
		}

		append os [format %c 0x$match]
		set str [string range $str 2 end]
	}
}

proc escapefm { str } {

	variable FEscape

	if ![array exists FEscape] {
		# initialize
		set FEscape(\&) "&amp;"
		set FEscape(\") "&quot;"
		set FEscape(\<) "&lt;"
		set FEscape(\>) "&gt;"
	}

	set os ""
	set nc [string length $str]
	for { set i 0 } { $i < $nc } { incr i } {
		set ch [string index $str $i]
		if [info exists FEscape($ch)] {
			append os $FEscape($ch)
		} else {
			# sanitize non-printable characters, in case the
			# reviewer's browser plays tricks on us
			scan $ch %c v
			if { $v < 32 || $v > 127 } {
				if { $ch != "\n" && $ch != "\r" } {
					set ch " "
				}
			}
			append os $ch
		}
	}

	return $os
}

###############################################################################

proc nannin { n } {
#
# Not A NonNegative Integer Number
#
	upvar $n num

	if [catch { expr $num } num] {
		return 1
	}
	set t $num
	if [catch { incr t }] {
		return 1
	}
	if { $num < 0 } {
		return 1
	}
	return 0
}

###############################################################################

proc mksel { } {

	global Files

	if [catch { open [file join $Files(VP) $Files(SMAP)] r } fd] {
		return "<p>Network map file not present!!!"
	}

	if [catch { read $fd } map] {
		return "<p>Network map file not readable!!!"
	}

	catch { close $fd }

	if [catch { sxml_child [sxml_parse map] "map" } map] {
		return "<p>Network map file error: $map!!!"
	}

	# the list of collector nodes
	set cv [sxml_children [sxml_child $map "collectors"] "node"]

	set Nodes ""
	set res "\n"

	foreach c $cv {
		set id [sxml_attr $c "id"]
		if { $id == "" } {
			# just in case
			continue
		}
		set nm [sxml_txt $c]
		if { $nm == "" } {
			set nm "Node $id"
		}
		set Node($id) ""
		set Names($id) $nm
		lappend Nodes $id
	}

	set cv [sxml_children [sxml_child $map "sensors"] "sensor"]

	set ix -1
	foreach c $cv {
		incr ix
		set nm [sxml_txt $c]
		if { $nm == "" } {
			set nm [sxml_attr $c "name"]
			if { $nm == "" } {
				# just in case
				continue
			}
		}
		set co [sxml_attr $c "node"]
		if ![info exists Node($co)] {
			continue
		}
		lappend Node($co) [list $ix $nm]
	}

	set NR 8

	append res "<table>\n"
	set ix 0

	foreach co $Nodes {

		if { $Node($co) == "" } {
			# no sensors
			continue
		}

		if { [expr $ix % $NR] == 0 } {
			# a new row in the outer table
			if { $ix != 0 } {
				append res "</tr>\n"
			}
			append res "<tr>\n"
		}

		append res "<td><table border=\"1\">\n<tr>\n"
		append res "<td colspan=\"2\">[escapefm $Names($co)]</td>"
		append res "</tr>\n"

		foreach s $Node($co) {
			append res "<tr><td>[escapefm [lindex $s 1]]</td>"
			append res "<td><input type=\"checkbox\" name=\""
			append res "sslist\" value=\"[lindex $s 0]\"></td>"
			append res "</tr>\n"
		}
		append res "</table></td>\n"
		incr ix
	}

	if { $ix > 0 } {

		while { [expr $ix % $NR] != 0 } {
			append res "<td></td>"
			incr ix
		}

		append res "</tr>\n"
	}

	append res "</table>\n"

	return $res
}

proc outhdr { } {
	puts "Content-type: text/html\n"
        puts "<html>\n<body>"
}

proc outend { } {
	puts "</body>\n</html>"
}


proc outform { } {

	outhdr

	puts \
	    "<form method=\"post\" action=\"sensors.cgi\" target=\"s_display\">"
 
        puts "<h2>ECO SENSORS</h2>"

	puts "<p>[mksel]\n<br><br>\n"

	puts "<p>"
	puts "<input type=\"submit\" value=\"Display\" name=\"Display\">"

	puts "</form>"

	outend
}

proc outrel { sl } {
#
# Output the 'reload' meta header
#
	puts "Content-type: text/html\n"
        puts "<html>\n<head>"
	set mt "<meta http-equiv=\"refresh\" content=\"10; url=sensors.cgi?S"

	foreach s $sl {
		append mt "&$s"
	}

	append mt "\">"
	puts $mt

	puts "</head>\n<body>"
}

proc svals { slist } {

	global Files

	if [catch { open [file join $Files(VP) $Files(SVAL)] r } vd] {
		return "Cannot access sensor values!!!"
	}

	if [catch { open [file join $Files(VP) $Files(SMAP)] r } fd] {
		return "<p>Network map file not present!!!"
	}

	if [catch { read $fd } map] {
		return "<p>Network map file not readable!!!"
	}

	catch { close $fd }

	if [catch { sxml_child [sxml_parse map] "map" } map] {
		return "<p>Network map file error: $map!!!"
	}

	set cv [sxml_children [sxml_child $map "sensors"] "sensor"]

	foreach sn $slist {

		seek $vd [expr $sn << 5]
		if [catch { read $vd 32 } rv] {
			set v "---"
			set u "---"
		} else {
			set v [string trim [string range $rv 0 15]]
			set u [string trim [string range $rv 16 end]]
			if { [catch {
					set v [format %1.1f $v]
					set u [clock format $u -format %H:%M:%S]
				    } ] } {
				set v "---"
				set u "---"
			}
		}

		set c [lindex $cv $sn]

		set nm [sxml_txt $c]
		if { $nm == "" } {
			set nm [sxml_attr $c "name"]
			if { $nm == "" } {
				set nm "---"
			}
		}

		lappend sli [list $nm $v $u]
	}

	catch { close $vd }
	unset map

	set res "<table border=\"1\">\n<tr><td>Sensor:</td>"

	foreach c $sli {
		append res "<td align=\"center\">[escapefm [lindex $c 0]]</td>"
	}

	append res "</tr>\n<tr><td>Value:</td>"

	foreach c $sli {
		append res "<td align=\"center\">[lindex $c 1]</td>"
	}

	append res "</tr>\n<tr><td>Time:</td>"

	foreach c $sli {
		append res "<td align=\"center\">[lindex $c 2]</td>"
	}

	append res "</tr>\n</table>\n"

	return $res
}

proc outslist { sl } {
#
# Sanitize the list of sensors to display
#

	foreach s $sl {
		if [nannin s] {
			continue
		}
		# make them unique
		set se($s) ""
	}

	set sl ""

	foreach s [array names se] {
		lappend sl $s
	}

	if { $sl == "" } {
		outhdr
		puts "<p>No sensors selected!!!!"
		outend
		return
	}

	set sl [lsort -integer $sl]

	# the reload meta
	outrel $sl

	# the list of values
	puts [svals $sl]

	# the trailer
	outend
}

if { $argv == "" } {

	# form submission
	getValues

	# find the sensor list
	if ![info exists Values(sslist)] {
		set sl ""
	} else {
		set sl $Values(sslist)
	}

	outslist $sl
	exit
}

if { [string first "F" $argv] >= 0 } {

	# request to send the selection form

	outform
	exit
}

# list of sensors to display

regsub -all "\[^0-9\]+" $argv " " sl

outslist [split $sl]
