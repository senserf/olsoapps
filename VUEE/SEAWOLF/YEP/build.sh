#!/bin/sh
cd PICTURES
../cbm.tcl wlodek1.bmp wlodek1.nok
../cbm.tcl gerry3.bmp gerry3.nok
../cbm.tcl pawel2.bmp pawel2.nok
../cbm.tcl wallpaper.bmp wallpaper.nok
../cbm.tcl olso_logo4.bmp olso_logo4.nok
../cbm.tcl combatLogo5.bmp combatLogo5.nok
../cbm.tcl rob6.bmp rob6.nok
../cbm.tcl kitty7.bmp kitty7.nok
cd ../FONTS
gcc -o mkfonts fontfile.c
./mkfonts > fonts.nok
rm mkfonts*
cd ..
./mkeeprom.tcl -i PICTURES FONTS/fonts.nok noombuzz.xml eeprom.nok
