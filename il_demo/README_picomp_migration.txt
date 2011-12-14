I have partitioned this into subdirectories to show that it can be done, but
there is no implied meaning in that (you can do it whichever way suits you).

Read the comments (mostly) in app_peg.cc. The relevant ones are tagged with
PiComp.

====

1. Make sure you have the newest versions of SIDE, VUEE, and (of course)
   PICOS (you do, if you are reading this)

2. VUEE should be set up the normal way. The vuee compiler in your personal
   BIN directory MUST be named vuee (as of now). Rename or create a symbolic
   link if it isn't.

3. Make sure to use the new mkmk, i.e., copy mkmk from Scripts to your BIN
   directory. You will be able to use the new version with old PicOS as well.

4. Additionally, copy picomp from Scripts to BIN and make sure it is
   executable.

5. Execute mkmk (e.g., mkmk WARSAW_ILS) and see how things compile.

6. Execute picomp (just picomp) and see how things compile.

====
