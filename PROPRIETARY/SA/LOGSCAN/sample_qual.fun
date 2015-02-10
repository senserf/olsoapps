{ ts line } {

	global maxgsil maxqsil lastgts lastqts

	if [info exists lastgts] {
		# update global silence
		set dt [expr { $ts - $lastgts }]
		if { $dt > $maxgsil } {
			set maxgsil $dt
			show "NEW GLOBAL MAX SILENCE: $maxgsil"
		}
	} else {
		set maxgsil 0
		set maxqsil 0
		set lastqts 0
	}

	set lastgts $ts

	# a sample qualifier: our network and sent by master
	if ![regexp -nocase "NID=4d,.*HOC=0," $line] {
		return
	}

	# qualified silence

	if $lastqts {
		set dt [expr { $ts - $lastqts }]
		if { $dt > $maxqsil } {
			set maxqsil $dt
			show "NEW QUALIFIED MAX SILENCE: $maxqsil"
		}
		# insert the SIL field into the packet
		regsub ":" $line ", SIL=$dt\n        " line
	} else {
		# just break the line
		regsub ":" $line "\n        " line
	}

	set lastqts $ts
	show $line
}
