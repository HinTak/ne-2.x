/*************************************************
*       The E text editor - 2nd incarnation      *
*************************************************/

/* Copyright (c) University of Cambridge, 1991 - 2004 */

/* Written by Philip Hazel, starting November 1991 */
/* This file last modified: February 2004 */

/* This file is specific to the support modules for UNIX */


#include <sys/types.h>
#include <unistd.h>

#if HAVE_SYS_FCNTL_H
#include <sys/fcntl.h>
#endif

#ifndef HAVE_TERMCAP
#include <curses.h>
#endif

#if HAVE_TERMIO_H
#include <termio.h>
#else          
  #if HAVE_TERMIOS_H
  #include <termios.h>
  #endif
#endif
          
#if GWINSZ_IN_SYS_IOCTL
#include <sys/ioctl.h>
#endif

#ifndef NO_TERM_H
  #ifndef TERM_H
  #define TERM_H <term.h>
  #endif
  #include TERM_H
#endif


#define term_screen   1
#define term_other    2

/* ASCII code for ESC */

#define escch        27      

/* Special terminal types */

#define tt_special_none           0
#define tt_special_ArcTermulator  1
#define tt_special_ttpa           2
#define tt_special_xterm          3
#define tt_special_fawn           4

/* Standard file descriptors */

#define stdin_fd   0  /* standard input */
#define stdout_fd  1  /* standard output */
#define stderr_fd  2  /* standard error */

/* Definitions for special keys */

#define Pkey_base        128
#define Pkey_up          128
#define Pkey_down        129
#define Pkey_left        130
#define Pkey_right       131
#define Pkey_sh_up       132
#define Pkey_sh_down     133
#define Pkey_sh_left     134
#define Pkey_sh_right    135
#define Pkey_ct_up       136
#define Pkey_ct_down     137
#define Pkey_ct_left     138
#define Pkey_ct_right    139
#define Pkey_reshow      140
#define Pkey_del127      141
#define Pkey_sh_del127   142
#define Pkey_ct_del127   143
#define Pkey_bsp         144
#define Pkey_sh_bsp      145
#define Pkey_ct_bsp      146
#define Pkey_copy        147
#define Pkey_sh_copy     148
#define Pkey_ct_copy     149
#define Pkey_home        150
#define Pkey_backtab     151
#define Pkey_ct_tab      152
#define Pkey_insert      153
#define Pkey_data        154  /* "next key is data" */
#define Pkey_null        155  /* ignore */
#define Pkey_max         155

#define Pkey_f0          160  /* other fkeys contiguous */




/*************************************************
*             Shared routines                    *
*************************************************/

extern void local_init1();
extern void local_init2();

extern void tc_connect(void);



/*************************************************
*             Global variables                   *
*************************************************/

extern int tc_n_co;        /* columns on screen */
extern int tc_n_li;        /* lines on screen */

extern uschar *tc_s_al;    /* add new line */
extern uschar *tc_s_bc;    /* cursor left - used only if NoZero */
extern uschar *tc_s_ce;    /* clear to end of line */
extern uschar *tc_s_cl;    /* clear screen */
extern uschar *tc_s_cm;    /* cursor move */
extern uschar *tc_s_cs;    /* set scrolling region */
extern uschar *tc_s_dc;    /* delete uschar */
extern uschar *tc_s_dl;    /* delete line */
extern uschar *tc_s_ic;    /* insert character */
extern uschar *tc_s_ip;    /* insert padding */
extern uschar *tc_s_ke;    /* reset terminal */
extern uschar *tc_s_ks;    /* set up terminal */
extern uschar *tc_s_pc;    /* pad character */
extern uschar *tc_s_se;    /* end standout */
extern uschar *tc_s_sf;    /* scroll text up */
extern uschar *tc_s_so;    /* start standout */
extern uschar *tc_s_sr;    /* scroll text down */
extern uschar *tc_s_te;    /* end screen management */
extern uschar *tc_s_ti;    /* start screen management */
extern uschar *tc_s_up;    /* cursor up - user only if NoZero */

extern int tc_f_am;        /* automatic margin flag */

extern uschar *tc_k_trigger;  /* trigger char table for special keys */
extern uschar *tc_k_strings;  /* strings for keys 0-n and specials */

extern int tt_special;     /* terminal special types */
extern int tc_int_ch;      /* interrupt char */
extern int tc_stop_ch;     /* stop char */
extern int NoZero;         /* never generate zero row or col */
extern int ioctl_fd;       /* FD to use for ioctl calls */
extern int window_changed; /* SIGWINSZ received */

/* End of unixhdr.h */
