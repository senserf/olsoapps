NOTE: some Tcl script(s) in this directory may need 'standard' package(s)
located in the Scripts directory up the PicOS tree.

To make such a script independent of the PicOS tree (and executable from any
place), you should run it through 'isource' (see PICOS/Scripts), i.e.,

	isource scriptfile outfile

which takes the original script file (scriptfile) and produces its expanded
version (outfile) with the required packages physically inserted into its
code. There is no harm in using isource on any Tcl script. If the script
does not depend on library packages, isource will notify you about this fact
and do nothing.
