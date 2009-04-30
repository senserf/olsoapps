#!/usr/bin/tclsh

##########################################################
#                                                        #
# EMS data extractor                                     #
#                                                        #
# Copyright (C) 2009 Olsonet Communications Corporation  #
#                                                        #
##########################################################

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
			# skip it
			set ix [string first "-->" $str]
			if { $ix < 0 } {
				error "unterminated comment: [string range \
					$str 0 15]"
			}
			incr ix 3
			set str [string range $str $ix end]
			continue
		}
		set et ""
		if [regexp -nocase "^<(/)?\[a-z_\]" $str ix et] {
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

	if ![regexp -nocase "^(\[a-z0-9_\]+)(.*)" $str ix kwd str] {
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
			# opening
			set cl [xadv str $kw]
			set tc [list $kw [lindex $cl 0] $at [lindex $cl 1]]
			lappend chd $tc
		} else {
			# closing (must be ours)
			if { $kw != $kwd } {
				error "mismatched tag: <$kwd ...> </$kw>"
			}
			# we are done
			return [list $txt $chd]
		}
	}
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

###############################################################################

package provide sdb 1.0
#
# Berkeley DB access functions
#
namespace eval SDB {

variable DB

set DB(DP)	""
set DB(CU)	""
set DB(HR)	[expr 24 * 3600]
set DB(YC)	""

variable PT

# database index
set PT(DI)	"2\[0-9\]\[0-9\]\[0-9\]\[0-9\]\[0-9\]\[0-9\]\[0-9\]$"

proc db_init { dnam } {
#
# The argument is a directory name that must exist
#
	variable DB

	if ![file isdirectory $dnam] {
		error "db_init: argument not a valid directory path"
	}

	set DB(DP) $dnam
}

proc db_close { } {
#
	db_stop
	set DB(DP) ""
}

proc db_add { dt data } {
#
# Add an entry to the current list of entries
#
	variable DB

	if { $DB(DP) == "" } {
		error "db_add: database not opened"
	}

	if [catch { open [file join $DB(DP) $dt] "a" } fd] {
		error "db_add: cannot open daily record: $fd"
	}

	if [catch { puts $fd $data } er] {
		catch { close $fd }
		error "db_add: cannot write: $fd"
	}

	catch { close $fd }
}

proc db_stop { } {
#
# Stop reading daily record
#
	variable DB

	if { $DB(CU) != "" } {
		catch { close $DB(CU) }
		set DB(CU) ""
	}
}

proc db_ttods { tm } {

	if [catch { clock format $tm -format %Y%m%d } res] {
		error "db_ttods: illegal number of seconds"
	}
	return $res
}

proc db_dstot { ds } {

	if [catch { clock scan "${ds}T000000" } sec] {
		error "db_dstot: bad day index: $ds"
	}

	return $sec
}

proc fyear { ni max } {
#
# Find the nearest next record by year
#
	variable DB
	variable PT

	# the year
	set yy [string range $ni 0 3]

	while { "${yy}0101" <= $max } {

		if { $DB(YC) == "" || $DB(YC,Y) != $yy } {
			# year not in the cache
			set DB(YC) ""
			set DB(YC,Y) $yy
			if [catch { glob -directory $DB(DP) -nocomplain -tails \
			    ${yy}* } yc] {
				# not found
				set yc ""
			}
			foreach y $yc {
				# cleanup the list
				if ![regexp $PT(DI) $y y] {
					continue
				}
				lappend DB(YC) $y
			}
			set DB(YC) [lsort $DB(YC)]
		}
		foreach y $DB(YC) {
			if { $y >= $ni } {
				if { $y > $max } {
					return ""
				} else {
					return $y
				}
			}
		}
		# not found, increment the year
		incr yy
	}

	return ""
}

proc db_start { dt { next "" } { max 20300101 }} {
#
# Start reading daily record
#
	variable DB

	db_stop

	if { $next == "+" } {
		# we are asked to increment
		set ni [db_ttods [expr [db_dstot $dt] + $DB(HR)]]
		if { $ni > $max } {
			# no more
			return ""
		}

		# naively try to open (expecting success in normal circumstances
		if ![catch { open [file join $DB(DP) $ni] "r" } fd] {
			# success
			set DB(CU) $fd
			return $ni
		}

		# we have failed, there is a hole, try by the year
		set ni [fyear $ni $max]

		if { $ni == "" } {
			return ""
		}

		# this must succeed
		if ![catch { open [file join $DB(DP) $ni] "r" } fd] {
			# success
			set DB(CU) $fd
			return $ni
		}

		error "db_start: cannot open record $ni: $fd"
	}

	if [catch { open [file join $DB(DP) $dt] "r" } fd] {
		# assume it doesn't exist
		return ""
	}

	set DB(CU) $fd
	return $dt
}

proc db_get { } {
#
# Next record at cursor
#
	variable DB

	if { $DB(CU) == "" } {
		return ""
	}

	while 1 {
		if [catch { gets $DB(CU) line } cnt] {
			error "db_get: cannot read: $cnt"
		}
		if { $cnt < 0 } {
			db_stop
			return ""
		}
		set line [string trim $line]
		if { $line != "" } {
			return $line
		}
	}
}

namespace export db_*

### end of SDB namespace ######################################################
}

namespace import ::SDB::db_*

###############################################################################

proc wf_getvals { } {
#
# This function extracts values from the form and stores them in a single
# binary string.
#
  global Values

  array unset Values

  fconfigure stdin -translation binary
  if [catch { gets stdin line } stat] {
    abortSession $stat
  }
  #
  # Determine the encoding type
  #
  if { $stat < 0 } {
    # empty message -- this shouldn't happen
    return
  }

  while 1 {
    if { [string first = $line] >= 0 } {
      # this looks like a valid line with some values
      set line [split $line &]
      foreach entry $line {
        if [regexp -- "(.*)=(\[^\n\r\]*)" $entry junk param value] {
          # we have a new parameter
          # convert '+' to ' '
          regsub -all {\+} $value " " value
          # convert newlines
          regsub -all {%0[dD]%0[aA]} $value "\n" value
          # eliminate escapes
          set oval ""
          while { [regexp -- {%[0-9a-fA-F][0-9a-fA-F]} $value escape] } {
            set ix [string first $escape $value]
            set oval ${oval}[string range $value 0 [expr $ix - 1]][format\
               %c 0x[string range $escape 1 2]]
            set value [string range $value [expr $ix + 3] end]
          }
          set value ${oval}${value}
          set Values($param) $value
        }
      }
    }
    # try next line
    if [catch { gets stdin line } stat] {
      # there was an input error
      error "read error from client: $stat"
    }
    if { $stat < 0 } { return }
  }
}

proc cstime { ts } {
#
# Converts date/time to ISO PIT
#
	if [catch { clock scan $ts } tv] {
		# illegal format
		return ""
	}
	return [clock format $tv -format %Y%m%dT%H%M%S]
}

proc incrdi { di } {
#
# Increments the day index
#
	set sec [clock scan "${di}T000000"]
	return [clock format [expr $sec + 24 * 3600] -format %Y%m%d]
}

proc extract { } {

	global Values DBPATH

	if { ![info exists Values(EXF_from)] || $Values(EXF_from) == "" ||
	     ![info exists Values(EXF_upto)] || $Values(EXF_upto) == "" } {
		errpage "From and Upto arguments are mandatory and at least\
			one of them is not specified"
	}

	if { ![info exists Values(EXF_rig)] || $Values(EXF_rig) == "" } {
		errpage "The rig number is mandatory and it has not been\
			specified"
	}

	if ![info exists Values(EXF_coll)] {
		set coll ""
	} else {
		set coll $Values(EXF_coll)
	}

	# process time bounds
	set fr [cstime $Values(EXF_from)]
	set up [cstime $Values(EXF_upto)]

	if { $fr == "" } {
		errpage "From time format is not recognizable"
	}
	if { $up == "" } {
		errpage "Upto time format is not recognizable"
	}

	if { [string compare $fr $up] > 0 } {
		errpage "Upto time precedes From"
	}

	set fsu ".csv"
	if { $coll != "" } {
		if [regexp -nocase "^e" $coll] {
			# used as event flag
			set coll "e"
			set fsu ".txt"
		} else {
			if { [catch { expr int($coll) } coll] || $coll <= 0 } {
				errpage "Illegal collector Id, must be > 0 or\
					event"
			}
		}
	}

	set ri $Values(EXF_rig)
	if { [catch { expr int($ri) } ri] || $ri <= 0 } {
		errpage "Illegal rig Id, must be > 0"
	}

	if [catch { db_init $DBPATH } er] {
		errpage "Cannot open the database: $er"
	}

	# day file index
	set di [string range $fr 0 7]
	set li [string range $up 0 7]

	# initialize extraction
	set du $di
	set txt [dayload du "" $li $fr $up $ri $coll]

	while { $di != "" } {
		append txt [dayload di "+" $li $fr $up $ri $coll]
	}

	db_close

	if { $txt == "" } {
		errpage "Nothing found"
	}

	output "Content-type: text/ascii"
	output "Content-disposition: attachment; filename=extract$fsu"
	output ""
	output $txt 1
}

proc output { txt { nf 0 } } {

	catch {
		if $nf { puts -nonewline $txt } else { puts $txt }
	}
}

proc errpage { msg } {

	if [catch { open "errpage.html" "r" } fd] {
		exit 0
	}

	if [catch { read $fd } ep] {
		catch { close $fd }
		exit 0
	}

	catch { close $fd }

	regsub "@@@" $ep [escapefm $msg] ep

	output "Content-type: text/html\n"

	output $ep 1

	exit 0
}

proc normts { ts } {

	if [catch { clock scan $ts } ts] {
		return ""
	}

	return [clock format $ts -format "%y-%m-%d %H:%M:%S"]
}

proc dayload { di adv max fr up ri coll } {

	upvar $di ni

	set ni [db_start $ni $adv $max]

	if { $ni == "" } {
		# no record
		return ""
	}

	set txt ""

	while 1 {
		if [catch { db_get } rec] {
			db_stop
			errpage "Cannot read database: $rec"
		}
		if { $rec == "" } {
			db_stop
			break
		}
		if { $coll == "e" } {
			if ![regexp "^EV$ri .(...............)" $rec jk ts] {
				continue
			}
			if { [string compare $fr $ts] > 0 } {
				continue
			}
			if { [string compare $up $ts] < 0 } {
				continue
			}
			# we want this line
			if ![regexp "(\[NY\])(...............) (.*)" $rec jk \
			    df ts ms] {
				continue
			}
			set ts [normts $ts]
			if { $ts == "" } {
				continue
			}
			append txt "$df $ts $ms\n"
		}
		# data sample
		if ![regexp "^SS$ri (\[NY\])(...............) (\[0-9\]+) (.*)" \
		    $rec jk df ts co rec] {
			continue
		}
		if { $coll != "" && $coll != $co } {
			continue
		}
		if { [string compare $fr $ts] > 0 } {
			continue
		}
		if { [string compare $up $ts] < 0 } {
			continue
		}

		set ts [normts $ts]
		if { $ts == "" } {
			continue
		}

		set lin "$co, $df, $ts"

		while 1 {
			if ![regexp "(\[0-9\]+):(\[^ \]+)(.*)"\
			    $rec jk val cnv rec] {
				break
			}
			if { $cnv == "?" } {
				set cv $val
			} elseif [catch { format %1.2f $cnv } cv] {
				set cv $val
			}
			append lin ", $val, $cv"
		}

		append txt "$lin\n"
	}
	return $txt
}

proc escapefm { str } {

	global FEscape

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
			append os $ch
		}
	}

	return $os
}

set DBPATH "database"
#set DBPATH "/home/pawel/EMSDB/database"

fconfigure stdout -translation crlf

wf_getvals

extract
