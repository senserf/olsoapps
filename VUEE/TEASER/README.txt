This is an intro to the forthcoming VUEE/PICOS compiler. Do this:

1. Make sure that VUEE is set up the normal way. The vuee compiler in your
   personal BIN directory MUST be named vuee (as of now). Create a peritnent
   symbolic link if it isn't.

2. Make sure to use the new mkmk, i.e., copy mkmk from Scripts to your BIN
   directory. You will be able to use the new version with old PicOS as well.

3. Additionally, copy picomp from Scripts to BIN and make sure it is
   executable.

4. Look at the files app_rcv.cc and app_snd.cc. Simplicity, no VUEE kludges, no
   nonsense. Note the new keywords. Praxes consisting of multiple files (with
   header files) are possible.

5. Execute mkmk (e.g., mkmk WARSAW) and see how things compile.

6. Execute picomp and see how things compile.

Note that this is just a teaser. A few loose ends have to be tied up, but we
are slowly getting there.
