/* src/config.h.  Generated automatically by configure.  */

/* This file, config.in, is converted by configure into src/config.h. NE is
written in Standard C, but there are a few non-standard things it can cope
with, allowing it to run on SunOS4 and other "close to standard" systems. */

/* The names of header files vary from system to system */

#define HAVE_SYS_FCNTL_H    1
#define HAVE_TERMIO_H       1
#define HAVE_TERMIOS_H      1

/* Other features that not all systems may not have */

#define HAVE_TIOCGWINSZ     1   /* Assume most have this these days. */

/* The following two definitions are mainly for the benefit of SunOS4, which
doesn't have the strerror() or memmove() functions that should be present in
all Standard C libraries. The macros HAVE_STRERROR and HAVE_MEMMOVE should
normally be defined with the value 1 for other systems, but unfortunately we
can't make this the default because "configure" files generated by autoconf
will only change 0 to 1; they won't change 1 to 0 if the functions are not
found. */

#define HAVE_STRERROR 1
#define HAVE_MEMMOVE  1

/* There are some non-Unix systems that don't even have bcopy(). If this macro
is false, an emulation is used. If HAVE_MEMMOVE is set to 1, the value of
HAVE_BCOPY is not relevant. */

#define HAVE_BCOPY    1

/* End */
