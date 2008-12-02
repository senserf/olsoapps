#!/bin/sh
cd PICTURES
../cbm.tcl wlodek.bmp image_baca0001.nok
../cbm.tcl gerry.bmp image_baca0003.nok
../cbm.tcl pawel.bmp image_baca0002.nok
../cbm.tcl wallpaper.bmp wallpaper.nok
../cbm.tcl smiley.bmp image_baca0004.nok
../cbm.tcl olso_logo.bmp olso_logo.nok
cd ../FONTS
gcc -o mkfonts fontfile.c
./mkfonts > fonts.nok
rm mkfonts*
cd ..
./mkeeprom.tcl -i PICTURES FONTS/fonts.nok image.xml eeprom.nok
