# Copyright (c) 2003 Olsonet Communication Corporation
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to
# deal in the Software without restriction, including without limitation the
# rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
# sell copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
# IN THE SOFTWARE.
#
CC=msp430-gcc
LD=msp430-ld
LC=$(CC)
AS=msp430-as
GCC_FLAGS=-mmcu=msp430x148 -mendup-at=main
OPT_FLAGS=-O2 -g
RP=../..
CP=$(RP)/PicOS
PP=$(CP)/MSP430
SP=$(CP)/kernel
LP=$(RP)/Libs/Lib
KP=./KTMP

CC_FLAGS=$(GCC_FLAGS) -I $(KP) -I $(LP)
CL_FLAGS=-o Image
DEL_CMD=rm -f

MAIN_INCLUDES=$(PP)/arch.h $(PP)/mach.h $(SP)/kernel.h $(SP)/scheduler.h $(CP)/sysio.h options.sys
TCV_INCLUDES=
RADIO_INCLUDES=
LIB_INCLUDES=$(LP)/ser.h $(LP)/form.h
OBJECT_FILES=$(KP)/main.o $(KP)/kernel.o $(KP)/__inserial.o $(KP)/__outserial.o $(KP)/form.o $(KP)/scan.o $(KP)/ser_in.o $(KP)/ser_inf.o $(KP)/ser_out.o $(KP)/ser_outf.o $(KP)/ser_select.o $(KP)/vform.o $(KP)/vscan.o app.o

Image : $(OBJECT_FILES)
	$(LC) $(CL_FLAGS) $(CC_FLAGS) $(OBJECT_FILES)

$(KP)/main.o : $(PP)/main.c $(MAIN_INCLUDES) $(RADIO_INCLUDES) $(LIB_INCLUDES)
	mkdir -p $(KP)
	cp $(PP)/main.c $(MAIN_INCLUDES) $(RADIO_INCLUDES) $(LIB_INCLUDES) $(KP)
	$(CC) $(CC_FLAGS) $(OPT_FLAGS) -c $(KP)/main.c 
	mv main.o $(KP)

$(KP)/kernel.o : $(SP)/kernel.c $(MAIN_INCLUDES)
	mkdir -p $(KP)
	cp $(SP)/kernel.c $(MAIN_INCLUDES) $(KP)
	$(CC) $(CC_FLAGS) $(OPT_FLAGS) -c $(KP)/kernel.c
	mv kernel.o $(KP)

$(KP)/__inserial.o : $(LP)/__inserial.c
	mkdir -p $(KP)
	cp $(LP)/__inserial.c $(MAIN_INLCUDES) $(LIB_INCLUDES) $(KP)
	$(CC) $(CC_FLAGS) $(OPT_FLAGS) -c $(KP)/__inserial.c
	mv __inserial.o $(KP)

$(KP)/__outserial.o : $(LP)/__outserial.c
	mkdir -p $(KP)
	cp $(LP)/__outserial.c $(MAIN_INLCUDES) $(LIB_INCLUDES) $(KP)
	$(CC) $(CC_FLAGS) $(OPT_FLAGS) -c $(KP)/__outserial.c
	mv __outserial.o $(KP)

$(KP)/form.o : $(LP)/form.c
	mkdir -p $(KP)
	cp $(LP)/form.c $(MAIN_INLCUDES) $(LIB_INCLUDES) $(KP)
	$(CC) $(CC_FLAGS) $(OPT_FLAGS) -c $(KP)/form.c
	mv form.o $(KP)

$(KP)/scan.o : $(LP)/scan.c
	mkdir -p $(KP)
	cp $(LP)/scan.c $(MAIN_INLCUDES) $(LIB_INCLUDES) $(KP)
	$(CC) $(CC_FLAGS) $(OPT_FLAGS) -c $(KP)/scan.c
	mv scan.o $(KP)

$(KP)/ser_in.o : $(LP)/ser_in.c
	mkdir -p $(KP)
	cp $(LP)/ser_in.c $(MAIN_INLCUDES) $(LIB_INCLUDES) $(KP)
	$(CC) $(CC_FLAGS) $(OPT_FLAGS) -c $(KP)/ser_in.c
	mv ser_in.o $(KP)

$(KP)/ser_inf.o : $(LP)/ser_inf.c
	mkdir -p $(KP)
	cp $(LP)/ser_inf.c $(MAIN_INLCUDES) $(LIB_INCLUDES) $(KP)
	$(CC) $(CC_FLAGS) $(OPT_FLAGS) -c $(KP)/ser_inf.c
	mv ser_inf.o $(KP)

$(KP)/ser_out.o : $(LP)/ser_out.c
	mkdir -p $(KP)
	cp $(LP)/ser_out.c $(MAIN_INLCUDES) $(LIB_INCLUDES) $(KP)
	$(CC) $(CC_FLAGS) $(OPT_FLAGS) -c $(KP)/ser_out.c
	mv ser_out.o $(KP)

$(KP)/ser_outf.o : $(LP)/ser_outf.c
	mkdir -p $(KP)
	cp $(LP)/ser_outf.c $(MAIN_INLCUDES) $(LIB_INCLUDES) $(KP)
	$(CC) $(CC_FLAGS) $(OPT_FLAGS) -c $(KP)/ser_outf.c
	mv ser_outf.o $(KP)

$(KP)/ser_select.o : $(LP)/ser_select.c
	mkdir -p $(KP)
	cp $(LP)/ser_select.c $(MAIN_INLCUDES) $(LIB_INCLUDES) $(KP)
	$(CC) $(CC_FLAGS) $(OPT_FLAGS) -c $(KP)/ser_select.c
	mv ser_select.o $(KP)

$(KP)/vform.o : $(LP)/vform.c
	mkdir -p $(KP)
	cp $(LP)/vform.c $(MAIN_INLCUDES) $(LIB_INCLUDES) $(KP)
	$(CC) $(CC_FLAGS) $(OPT_FLAGS) -c $(KP)/vform.c
	$(CC) $(CC_FLAGS) $(OPT_FLAGS) -S $(KP)/vform.c
	mv vform.o $(KP)

$(KP)/vscan.o : $(LP)/vscan.c
	mkdir -p $(KP)
	cp $(LP)/vscan.c $(MAIN_INLCUDES) $(LIB_INCLUDES) $(KP)
	$(CC) $(CC_FLAGS) $(OPT_FLAGS) -c $(KP)/vscan.c
	$(CC) $(CC_FLAGS) $(OPT_FLAGS) -S $(KP)/vscan.c
	mv vscan.o $(KP)

app.o : app.c $(MAIN_INCLUDES) $(TCV_INCLUDES) $(LIB_INCLUDES)
	mkdir -p $(KP)
	cp $(MAIN_INCLUDES) $(TCV_INCLUDES) $(LIB_INCLUDES) $(KP)
	$(CC) $(CC_FLAGS) $(OPT_FLAGS) -c app.c

clean :
	-$(DEL_CMD) *.o
	rm -rf KTMP
