#!/bin/sh
cd PICTURES
../cbm.tcl wlodek1.bmp image_baca0001.nok
../cbm.tcl gerry3.bmp image_baca0003.nok
../cbm.tcl pawel2.bmp image_baca0002.nok
../cbm.tcl wallpaper.bmp wallpaper.nok
../cbm.tcl olso_logo4.bmp image_baba0004.nok
../cbm.tcl combatLogo5.bmp image_baba0005.nok
../cbm.tcl rob6.bmp image_baca0006.nok
../cbm.tcl kitty7.bmp image_baca0007.nok
cd ../FONTS
gcc -o mkfonts fontfile.c
./mkfonts > fonts.nok
rm mkfonts*
cd ..
./mkeeprom.tcl -i PICTURES FONTS/fonts.nok noombuzz.xml eeprom.nok
