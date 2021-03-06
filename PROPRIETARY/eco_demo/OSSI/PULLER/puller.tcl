#!/bin/sh
#
#	Copyright 2002-2020 (C) Olsonet Communications Corporation
#	Programmed by Pawel Gburzynski & Wlodek Olesinski
#	All rights reserved
#
#	This file is part of the PICOS platform
#
#
###########################\
exec tclsh "$0" "$@"


proc msource { f } {
#
# Intelligent 'source'
#
	if ![catch { uplevel #0 source $f } er ] {
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
msource log.tcl
msource sdb.tcl
msource snippets.tcl

###############################################################################

package require xml 1.0
package require log 1.0
package require sdb 1.0
package require snippets 1.0

###############################################################################

proc abt { mes } {

	log "ABORT: $mes"
	puts stderr $mes
	exit 99
}

proc vto { to } {
#
# Validate time offset
#
	if ![catch { expr int($to) } to] {
		if { $to < 24 && $to > -24 } {
			return $to
		}
	}
	abt "configuration file error, illegal time offset: $to"
}

proc strz { v } {
#
# Strip leading zeros from a number
#
	regsub "^0+" $v "" v
	if { $v == "" } {
		set v 0
	}
	return $v
}

proc fully { v } {
#
# Full year
#
	set v [strz $v]
	if { $v < 100 } {
		set v [expr 2000 + $v]
	}
	return $v
}

proc make_offset_time { tp lval } {

	global Params PT

	set tv ""

	foreach v $lval {
		lappend tv [strz $v]
	}

	set offset 0

	if { $tp != "" } {
		if { $tp == "S" } {
			if [info exists Params(TO,S)] {
				set offset $Params(TO,S)
			} elseif [info exists Params(TO)] {
				set offset $Params(TO)
			}
		} else {
			# collector
			set nf 1
			if [info exists Params(TO,D)] {
				foreach s $Params(TO,D) {
					set p [lindex $s 0]
					if { [catch { regexp $p $tp } r] || \
					    !$r } {
						continue
					}
					set offset [lindex $s 1]
					set nf 0
					break
				}
			}
			if { $nf && [info exists Params(TO)] } {
				set offset $Params(TO)
			}
		}
	}

	set yr [fully [lindex $tv 0]]

	# ISO P-I-T format
	set ts [format $PT(TF) $yr \
					[lindex $tv 1] \
					[lindex $tv 2] \
					[lindex $tv 3] \
					[lindex $tv 4] \
					[lindex $tv 5] ]
	if { $offset == 0 } {
		return $ts
	}

	set t [expr [clock scan $ts] + $offset * 3600]

	return [clock format $t -format %Y%m%dT%H%M%S]
}

proc read_xml_config { } {
#
# Read and parse the config file 
#
	global Files Params PT

	if [catch { open $Files(XMLC) r } xfd] {
		abt "cannot open configuration file $Files(XMLC): $xfd"
	}

	if [catch { read $xfd } xc] {
		catch { close $xfd }
		abt "cannot read configuration file $Files(XMLC): $xfd"
	}

	catch { close $xfd }

	if [catch { sxml_parse xc } xc] {
		abt "configuration file error: $xc"
	}

	set xc [sxml_child $xc "puller"]

	if { $xc == "" } {
		abt "no <puller> element in configuration file"
	}

	set el [sxml_child $xc "log"]
	set Files(LOG) [sxml_txt $el]
	if { $Files(LOG) == "" } {
		set Files(LOG) "puller.log"
	}
	if [catch { expr [sxml_attr $el "size"] } mls] {
		set mls 1000000
	}

	if [catch { expr [sxml_attr $el "versions"] } lve] {
		set lve 4
	}
	log_open $Files(LOG) $mls $lve

	set el [sxml_child $xc "repository"]
	if { $el == "" } {
		abt "no <repository> element in configuration file"
	}

	# the URL
	set ul [sxml_txt $el]

	if ![regexp $PT(UR) $ul junk Params(HOS) Params(REF)] {
		abt "URL format error in configuration file"
	}

	set Params(INT) [sxml_attr $el "interval"]
	if { $Params(INT) == "" } {
		# one minute is the default
		set Params(INT) 60
	}

	set Params(POR) [sxml_attr $el "port"]
	if [catch { expr $Params(POR) } Params(POR)] {
		set Params(POR) 80
	}

	set Params(UID) [sxml_attr $el "user"]
	if { $Params(UID) == "" } {
		abt "no user attribute in <repository>"
	}

	set el [sxml_child $xc "target"]
	set Params(TAR) [sxml_txt $el]
	if { $Params(TAR) == "" } {
		set Params(TAR) "database"
	}

	set el [sxml_child $xc "timeoffsets"]
	if { $el != "" } {
		set def [sxml_txt [sxml_child $el "default"]]
		if { $def != "" } {
			set Params(TO) [vto $def]
		}
		set def [sxml_txt [sxml_child $el "satellite"]]
		if { $def != "" } {
			set Params(TO,S) [vto $def]
		}
		set def [sxml_children $el "device"]
		foreach c $def {
			set nn [sxml_attr $c "id"]
			if { $nn == "" || [catch { regexp $nn "1999" } ] } {
				abt "illegal device id pattern in <device>:\
					'$nn'"
			}
			lappend Params(TO,D) [list $nn [vto [sxml_txt $c]]]
		}
	}

	# get the conversion snippets
	set s [sxml_txt [sxml_child $xc "snippets"]]
	if { $s == "" } {
		abt "no snippets file specified in the data set"
	}

	if [catch { open $s "r" } fd] {
		abt "cannot open snippets file $s: $fd"
	}

	if [catch { read $fd } cp] {
		abt "cannot read snippets file $s: $cp"
	}

	catch { close $fd }

	set fd [snip_parse $cp]

	if { $fd != "" } {
		abt "cannot parse the snippets file: $fd"
	}
}

proc sock_in { sok } {

	global Response Timeout

	if [catch { read $sok } res] {
		# disconnection
		catch { after cancel $Timeout }
		set Timeout "e"
		return
	}

	append Response $res

	if { [regexp "<DataSet\[^<\]+/>" $Response] ||
	     [string first "</DataSet>" $Response] >= 0 } {
		# done
		catch { after cancel $Timeout }
		set Timeout ""
	}
}

proc unescape { str } {

	set os ""

	regsub -all "&amp;" $str "\\&" str
	regsub -all "&quot;" $str "\"" str
	regsub -all "&lt;" $str "<" str
	regsub -all "&gt;" $str ">" str

	return $str

}

proc parse_response { } {

	global Samples Response PT

	set msg ""
	set Samples ""

	while 1 {

		set fn [string first "<Customers" $Response]
		if { $fn < 0 } {
			break
		}
		set Response [string range $Response [expr $fn + 16] end]
		set fm [string first "</Customers" $Response]
		if { $fm < 0 } {
			break
		}

		set block [string range $Response 0 $fm]
		set Response [string range $Response [expr $fm + 10] end]

		extract_data $block
	}

	if { $Samples == "" } {
		return 0
	}

	set Samples [lsort -ascii -index 0 $Samples]

	foreach smp $Samples {
		set ts [lindex $smp 0]
		set di [string range $ts 0 7]
		set ta [lindex $smp 1]
		set df [lindex $smp 2]
		set me [lindex $smp 3]
		add_sample $di "$ta $ts $df $me"
	}

	set Samples ""
	return 1
}

proc add_sample { di mes } {

	global Params

	if [catch { db_init $Params(TAR) } er] {
		abt "cannot open database ($Params(TAR)): $er"
	}

	if [catch { db_add $di $mes } er] {
		log "couldn't write to db: $di -> $mes: $er"
	}

	db_close
}

proc extract_data { block } {

	global Samples Params PT FMODE

	# message Id
	set mid ""
	# device Id
	set did ""
	# satellite time stamp
	set sts ""
	# message time stamp
	set mts ""
	# message
	set mes ""
	#
	regexp $PT(XD) $block junk did
	regexp $PT(XM) $block junk mid
	regexp $PT(XS) $block junk sts
	regexp $PT(XT) $block junk mes

	set mes [unescape $mes]

	if { $mes == "" } {
		# no message
		log "null message: $did, $mid, $sts"
		return
	}

	# convert satellite time stamp to cannonical form
	if ![regexp $PT(ST) $sts junk yr mo dy hh mm ss] {
		log "unrecognizable satellite time stamp: $sts"
		return
	}
	set ts [make_offset_time "S" [list $yr $mo $dy $hh $mm $ss]]

	if ![regexp $PT(SV) $mes junk asl col csl yr mo dy hh mm ss vals] {
		# event message, make sure there are no newlines
		regsub -all "\[\n\r\]+" $mes " " mes
		lappend Samples [list $ts "EV$did" "N" $mes]
		return
	}

	if { [string first "*gone" $vals] >= 0 } {
		# ignore this
		return
	}

	# check if the sample's time stamp is OK

	set yr [fully $yr]
	set mo [strz $mo]

	if { $yr < 2009 || ($yr == 2009 && $mo == 1) } {
		# the date is fake
		set df "N"
		# no offset
		set of ""
	} else {
		set df "Y"
		set of $did
	}
	set cs [make_offset_time $of [list $yr $mo $dy $hh $mm $ss]]

	# create a formatted entry

	set mes "$cs $col $csl/$asl =="

	set sid 0

	while 1 {
		set vals [string trimleft $vals " ,"]
		if ![regexp "^($PT(DI))(.*)" $vals junk val vals] {
			break
		}
		append mes " $val:[snip_cnvrt $val $sid $col]"
		incr sid
	}

	# use satellite reported time as the record index
	lappend Samples [list $ts "SS$did" $df $mes]
	if $FMODE {
		puts "EXTRACTED: $ts $did $df $mes"
	}
}

proc pull_it { } {

	global Params Timeout Response

	set Response ""

	if [catch { socket $Params(HOS) $Params(POR) } sok] {
		log "connection to $Params(HOS) \[$Params(POR)\] failed: $sok"
		return
	}

	fconfigure $sok -buffering none -translation lf

	set req "GET $Params(REF)/List_Received_Messages?Access_ID=$Params(UID) HTTP/1.1\nHost: $Params(HOS)\n"
	if [catch { puts $sok $req } err] {
		log "cannot write to host $Params(HOS) \[$Params(POR)\]: $err"
		catch { close $sok }
		return
	}

	fconfigure $sok -blocking 0

	fileevent $sok readable "sock_in $sok"

	set Timeout [after 30000 "set Timeout t"]

	vwait Timeout

	catch { close $sok}

	if { $Timeout == "t" } {
		log "response timeout from $Params(HOS) \[$Params(POR)\]"
		return
	}

	if { $Timeout == "e" } {
		log "peer disconnection from $Params(HOS) \[$Params(POR)\]"
		return
	}

	# debug version, preserve those data that contain samples
	set sres $Response

	if [parse_response] {
###############################
exec echo $sres >> dump.txt
###############################
	}
}

### Patterns ##################################################################

set PT(UR)	"http://(\[^/\]+)(/.+)"

set PT(XD)	"<Mobile_ID>(\[0-9\]+)"
set PT(XM)	"<Msg_ID>(\[0-9\]+)"
set PT(XS)	"<RecvDate>(\[^<\]+)"
set PT(XT)	"<Message>(.*)</Message>"

set PT(DI)	"\[0-9\]+"

set PT(ST)	"($PT(DI))-($PT(DI))-($PT(DI)).($PT(DI)):($PT(DI)):($PT(DI))"

set PT(TF)	"%04d%02d%02dT%02d%02d%02d"

set PT(SV)	"^1002 +($PT(DI)) +($PT(DI)).($PT(DI)) "
	append PT(SV) $PT(ST)
	append PT(SV) "(.*)"

set Files(XMLC) [lindex $argv 0]

if { $Files(XMLC) == "" } {
	# default XML datafile
	set Files(XMLC) "puller.xml"
}

read_xml_config

set fn [lindex $argv 1]

if { $fn != "" } {
	# from file
	set FMODE 1
	if [catch { open $fn "r" } fd] {
		abt "cannot open source file $fn: $fd"
	}

	if [catch { read $fd } Response] {
		catch { close $fd }
		abt "cannot read source file $fn: $Response"
	}

	catch { close $fd }

	parse_response
	exit
}

set FMODE 0

while 1 {

	pull_it
	after [expr $Params(INT) * 1000]

}
