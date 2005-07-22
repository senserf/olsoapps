@ECHO OFF
cd ADC
copy ..\common\app.mak .
nmake -f app.mak clean
erase /q app.mak
cd ..\MacTest
copy ..\common\app.mak . 
nmake -f app.mak clean
erase /q app.mak
cd ..\PriorityTest
copy ..\common\app.mak .
nmake -f app.mak clean
erase /q app.mak
cd ..\RemoteDisplay
copy ..\common\app.mak .
nmake -f app.mak clean
erase /q app.mak
cd ..\SerTest
copy ..\common\app.mak .
nmake -f app.mak clean
erase /q app.mak
cd ..\Sniffer
copy ..\common\app.mak .
nmake -f app.mak clean
erase /q app.mak
cd ..\Toy
copy ..\common\app.mak .
nmake -f app.mak clean
erase /q app.mak
cd ..\Radio
copy ..\common\app.mak .
nmake -f app.mak clean
erase /q app.mak
cd ..\RFPing
copy ..\common\app.mak .
nmake -f app.mak clean
erase /q app.mak
cd ..
ECHO ON
