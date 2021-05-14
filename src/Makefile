# This is not called directly, but should be called from the top-level make
# file.

# As well as compiler-specific flags, CFLAGS may contain the following:
#
#   -DHAVE_TERMCAP  Should be set on systems where termcap is used. If
#                   it is not set, the use of terminfo is assumed.
#
#   -DNO_VDISCARD   Should be set for Unix systems where the VDISCARD
#                   terminal control character is not supported.

# INCLUDE contains any -I options that are necessary for compilation.

# LFLAGS contains options for the linker. This can include -L options
# that come before NE's object modules in the command.

# LIBS contains libraries, including the appropriate library for the type of
# terminal handling in use (typically either -lcurses or -ltermcap).


# Compile step for the modules

.SUFFIXES:  .o .c
.c.o:;      $(CC) -c $(CFLAGS) $(INCLUDE) $*.c

HDRS = cmdhdr.h config.h ehdr.h keyhdr.h scomhdr.h shdr.h structs.h unixhdr.h

OBJ = debug.o ecrash.o ecmdarg.o ecmdcomp.o ecmdsub.o ecompR.o ematchR.o ecutcopy.o \
  edisplay.o eerror.o ee1.o ee2.o ee3.o ee4.o efile.o eglobals.o einit.o \
  ekey.o ekeysub.o eline.o ematch.o erdseqs.o escrnrdl.o \
  escrnsub.o estore.o rdargs.o scommon.o sunix.o sysunix.o eversion.o

# Linking steps; removal of eversion.o ensures new date each time

ne:           $(OBJ)
	      $(CC) -o ne $(LFLAGS) $(OBJ) $(LIBS) -lc -lpcre
	      /bin/rm -f eversion.o
              
# Dependencies

debug.o:      Makefile ../Makefile $(HDRS) debug.c 
ecmdarg.o:    Makefile ../Makefile $(HDRS) ecmdarg.c      
ecmdcomp.o:   Makefile ../Makefile $(HDRS) ecmdcomp.c    
ecmdsub.o:    Makefile ../Makefile $(HDRS) ecmdsub.c      
ecompP.o:     Makefile ../Makefile $(HDRS) ecompP.c       
ecrash.o:     Makefile ../Makefile $(HDRS) ecrash.c       
ecutcopy.o:   Makefile ../Makefile $(HDRS) ecutcopy.c     
edisplay.o:   Makefile ../Makefile $(HDRS) edisplay.c     
ee1.o:        Makefile ../Makefile $(HDRS) ee1.c          
ee2.o:        Makefile ../Makefile $(HDRS) ee2.c          
ee3.o:        Makefile ../Makefile $(HDRS) ee3.c          
ee4.o:        Makefile ../Makefile $(HDRS) ee4.c          
eerror.o:     Makefile ../Makefile $(HDRS) eerror.c       
efile.o:      Makefile ../Makefile $(HDRS) efile.c        
eglobals.o:   Makefile ../Makefile $(HDRS) eglobals.c     
einit.o:      Makefile ../Makefile $(HDRS) einit.c        
ekey.o:       Makefile ../Makefile $(HDRS) ekey.c         
ekeysub.o:    Makefile ../Makefile $(HDRS) ekeysub.c      
eline.o:      Makefile ../Makefile $(HDRS) eline.c        
ematch.o:     Makefile ../Makefile $(HDRS) ematch.c       
erdseqs.o:    Makefile ../Makefile $(HDRS) erdseqs.c      
escrnrdl.o:   Makefile ../Makefile $(HDRS) escrnrdl.c     
escrnsub.o:   Makefile ../Makefile $(HDRS) escrnsub.c     
estore.o:     Makefile ../Makefile $(HDRS) estore.c       
rdargs.o:     Makefile ../Makefile $(HDRS) rdargs.c       
scommon.o:    Makefile ../Makefile $(HDRS) scommon.c      
sunix.o:      Makefile ../Makefile $(HDRS) sunix.c        
sysunix.o:    Makefile ../Makefile $(HDRS) sysunix.c      

# Tidying

clean:;       /bin/rm -f ne *.o

# End
