/*************************************************
*       The E text editor - 2nd incarnation      *
*************************************************/

/* Copyright (c) University of Cambridge, 1991 - 2005 */

/* Written by Philip Hazel, starting November 1991 */
/* This file last modified: June 2005 */


/* This file contains the system-specific routines for Unix,
with the exception of the window-specific code, which lives in
its own modules. */

#include <memory.h>
#include <fcntl.h>
#include <pwd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
            
#include "ehdr.h"
#include "unixhdr.h"
#include "scomhdr.h"
#include "keyhdr.h"

/* FreeBSD needs this file for FIONREAD */

#ifndef FIONREAD
#include <sys/filio.h>
#endif

#define tc_keylistsize 1024


/* List of signals to be trapped for buffer dumping on
crashes to be effective. Names are for use in messages. */

/* SIGHUP is handled specially, so does not appear here - otherwise
problems with dedicated xterm windows. */

/* SIGXFSZ and SIGXCPU not on HP-UX */
/* SIGSYS not on Linux */

int signal_list[] = {
  SIGQUIT, SIGILL,  SIGIOT,
  SIGFPE,  SIGBUS,  SIGSEGV, SIGTERM,
#ifdef SIGSYS
  SIGSYS,
#endif
#ifdef SIGXCPU
  SIGXCPU,
#endif
#ifdef SIGXFSZ
  SIGXFSZ,
#endif
  -1 };

uschar *signal_names[] = {
  "(SIGQUIT)", "(SIGILL)",  "(SIGIOT)",
  "(SIGFPE)",  "(SIGBUS)",  "(SIGSEGV)", "(SIGTERM)",
#ifdef SIGSYS
  "(SIGSYS)",
#endif
#ifdef SIGXCPU
  "(SIGXCPU)",
#endif
#ifdef SIGXFSZ
  "(SIGXFSZ)",
#endif
  "" };



/*************************************************
*               Static data                      *
*************************************************/

#ifdef HAVE_TERMCAP
static uschar *termcap_buf = NULL;
#endif

static uschar user_init_file[256];
static uschar *term_name;
static int term_type;


/* The following keystrokes are not available and so their default
settings must be squashed. Note that we leave "ctrl/tab", because
esc tab maps to it and is suitable translated for output. For some
known terminals (e.g. Archimedes ttp) some of these keys are not
squashed, because they are built into this code. There are special
versions of the table for these. */

#define s s_f_shiftbit
#define c s_f_ctrlbit
#define sc s_f_shiftbit+s_f_ctrlbit

static uschar non_keys[] = {
  s_f_cup+s,   s_f_cup+c,     s_f_cup+sc,
  s_f_cdn+s,   s_f_cdn+c,     s_f_cdn+sc,
  s_f_clf+s,   s_f_clf+c,     s_f_clf+sc,
  s_f_crt+s,   s_f_crt+c,     s_f_crt+sc,
  s_f_copykey, s_f_copykey+s, s_f_copykey+c,
  0 };

/* This does in fact require some special configuration of xterm... */

static uschar xterm_non_keys[] = {
  s_f_cup+sc,
  s_f_cdn+sc,
  s_f_clf+sc,
  s_f_crt+sc,
  s_f_copykey, s_f_copykey+s, s_f_copykey+c,
  0 };

static uschar ttpa_non_keys[] = {
  s_f_cup+sc,
  s_f_cdn+sc,
  s_f_clf+sc,
  s_f_crt+sc,
  0 };

static uschar fawn_non_keys[] = {
  s_f_cup+c,   s_f_cup+sc,
  s_f_cdn+c,   s_f_cdn+sc,
  s_f_clf+c,   s_f_clf+sc,
  s_f_crt+c,   s_f_crt+sc,
  s_f_copykey, s_f_copykey+s, s_f_copykey+c,
  0 };

#undef s
#undef c
#undef sc




/*************************************************
*          Add to arg string if needed           *
*************************************************/

/* This function permits the system-specific code to add additional
argument definitions to the arg string which will be used to decode
the command line. */

uschar *sys_argstring(uschar *s)
{
return s;
}


/*************************************************
*      Deal with local argument settings         *
*************************************************/

/* This procedure is called when the command line has been decoded,
to enable the system-specific code to deal with its own argument
settings. The first argument points to the rdargs results structure,
while the second is the number of the first local argument. This is
called before the system-independent arguments are examined. */

void sys_takeargs(arg_result *result, int n)
{
}



/*************************************************
*     Read a termcap or terminfo string entry    *
*************************************************/

static uschar *my_tgetstr(uschar *key)
{
#ifdef HAVE_TERMCAP
uschar *yield, *p;
uschar workbuff[256];
p = workbuff;
if (tgetstr(key, &p) == 0 || workbuff[0] == 0) return NULL;
yield = (uschar *)malloc(strlen(workbuff)+1);
strcpy(yield, workbuff);
return yield;

#else
uschar *yield = (uschar *)tigetstr(key);
if (yield == NULL || yield == (uschar *)(-1) || yield[0] == 0) return NULL;
return yield;
#endif
}


/*************************************************
*      Add an escape key sequence to the list    *
*************************************************/

static void addkeystr(uschar *s, int keyvalue, int *acount, uschar **aptr)
{
uschar *ptr = *aptr;
if (s[1] == 0) tc_k_trigger[s[0]] = keyvalue; else
  {
  tc_k_trigger[s[0]] = 254;
  *acount += 1;
  *ptr = strlen(s) + 3;
  strcpy((uschar *)(ptr + 1), s);
  ptr[*ptr-1] = keyvalue;
  *aptr = ptr + *ptr;
  }
}


/*************************************************
* Read a termcap/info key entry and set up data  *
*************************************************/

static int tgetkeystr(uschar *s, int *acount, uschar **aptr, int keyvalue)
{
#ifdef HAVE_TERMCAP
uschar workbuff[256];
uschar *p = workbuff;
if (tgetstr(s, &p) == 0 || workbuff[0] == 0) return 0;

#else
uschar *workbuff = (uschar *)tigetstr(s);
if (workbuff == NULL || workbuff == (uschar *)(-1) || workbuff[0] == 0) return 0;
#endif

addkeystr(workbuff, keyvalue, acount, aptr);
return 1;   /* non-zero means OK */
}



/*************************************************
*            Check terminal type                 *
*************************************************/

/* Search the termcap database. If the terminal has sufficient
power, we can remain in screen mode; otherwise return a non-
screen terminal type.

Alternatively, search the terminfo database, depending on a
configuration option. */

/* Define alternative names for string functions */

#ifdef HAVE_TERMCAP
#define TCI_AL   "al"
#define TCI_BC   "bc"
#define TCI_CD   "ce"
#define TCI_CL   "cl"
#define TCI_CM   "cm"
#define TCI_CS   "cs"
#define TCI_DC   "dc"
#define TCI_DL   "dl"
#define TCI_DM   "dm"
#define TCI_IC   "ic"
#define TCI_IM   "im"
#define TCI_IP   "ip"
#define TCI_KE   "ke"
#define TCI_KS   "ks"
#define TCI_PC   "pc"
#define TCI_SE   "se"
#define TCI_SF   "sf"
#define TCI_SO   "so"
#define TCI_SR   "sr"
#define TCI_TE   "te"
#define TCI_TI   "ti"
#define TCI_UP   "up"

#define TCI_KU   "ku"
#define TCI_KD   "kd"
#define TCI_KL   "kl"
#define TCI_KR   "kr"

#define TCI_K0   "k0"
#define TCI_K1   "k1"
#define TCI_K2   "k2"
#define TCI_K3   "k3"
#define TCI_K4   "k4"
#define TCI_K5   "k5"
#define TCI_K6   "k6"
#define TCI_K7   "k7"
#define TCI_K8   "k8"
#define TCI_K9   "k9"
#define TCI_KK   "k;"
#define TCI_F1   "F1"
#define TCI_F2   "F2"
#define TCI_F3   "F3"
#define TCI_F4   "F4"
#define TCI_F5   "F5"
#define TCI_F6   "F6"
#define TCI_F7   "F7"
#define TCI_F8   "F8"
#define TCI_F9   "F9"
#define TCI_FA   "FA"
#define TCI_FB   "FB"
#define TCI_FC   "FC"
#define TCI_FD   "FD"
#define TCI_FE   "FE"
#define TCI_FF   "FF"
#define TCI_FG   "FG"
#define TCI_FH   "FH"
#define TCI_FI   "FI"
#define TCI_FJ   "FJ"
#define TCI_FK   "FK"

#else  /* This is for terminfo */
#define TCI_AL   "il1"
#define TCI_BC   "cub1"
#define TCI_CD   "ed"
#define TCI_CL   "clear"
#define TCI_CM   "cup"
#define TCI_CS   "csr"
#define TCI_DC   "dch1"
#define TCI_DL   "dl1"
#define TCI_DM   "smdc"
#define TCI_IC   "ich1"
#define TCI_IM   "smir"
#define TCI_IP   "ip"
#define TCI_KE   "rmkx"
#define TCI_KS   "smkx"
#define TCI_PC   "pad"
#define TCI_SE   "rmso"
#define TCI_SF   "ind"
#define TCI_SO   "smso"
#define TCI_SR   "ri"
#define TCI_TE   "rmcup"
#define TCI_TI   "smcup"
#define TCI_UP   "cuu1"

#define TCI_KU   "kcuu1"
#define TCI_KD   "kcud1"
#define TCI_KL   "kcub1"
#define TCI_KR   "kcuf1"

#define TCI_K0   "kf0"
#define TCI_K1   "kf1"
#define TCI_K2   "kf2"
#define TCI_K3   "kf3"
#define TCI_K4   "kf4"
#define TCI_K5   "kf5"
#define TCI_K6   "kf6"
#define TCI_K7   "kf7"
#define TCI_K8   "kf8"
#define TCI_K9   "kf9"
#define TCI_KK   "kf10"
#define TCI_F1   "kf11"
#define TCI_F2   "kf12"
#define TCI_F3   "kf13"
#define TCI_F4   "kf14"
#define TCI_F5   "kf15"
#define TCI_F6   "kf16"
#define TCI_F7   "kf17"
#define TCI_F8   "kf18"
#define TCI_F9   "kf19"
#define TCI_FA   "kf20"
#define TCI_FB   "kf21"
#define TCI_FC   "kf22"
#define TCI_FD   "kf23"
#define TCI_FE   "kf24"
#define TCI_FF   "kf25"
#define TCI_FG   "kf26"
#define TCI_FH   "kf27"
#define TCI_FI   "kf28"
#define TCI_FJ   "kf29"
#define TCI_FK   "kf30"
#endif




static int CheckTerminal(void)
{
uschar *p;
uschar *keyptr;
int erret;
int keycount = 0;

/* Set up a file descriptor to the terminal for use in various
ioctl calls. */

ioctl_fd = open("/dev/tty", O_RDWR);

#ifdef HAVE_TERMCAP
if (termcap_buf == NULL) termcap_buf = (uschar *)malloc(1024);
if (tgetent(termcap_buf, term_name) != 1) return term_other;
#else
if (setupterm(term_name, ioctl_fd, &erret) != OK || erret != 1)
  return term_other;
#endif

/* First, investigate terminal size. See if we can get the values
from an ioctl call. If not, we take them from termcap/info. */

tc_n_li = 0;
tc_n_co = 0;

#if HAVE_TIOCGWINSZ
  {
  struct winsize parm;
  if (ioctl(ioctl_fd, TIOCGWINSZ, &parm) == 0)
    {
    if (parm.ws_row != 0) tc_n_li = parm.ws_row;
    if (parm.ws_col != 0) tc_n_co = parm.ws_col;
    }
  }
#endif


#ifdef HAVE_TERMCAP
if (tc_n_li == 0) tc_n_li = tgetnum("li");  /* number of lines on screen */
if (tc_n_co == 0) tc_n_co = tgetnum("co");  /* number of columns on screen */
#else
if (tc_n_li == 0) tc_n_li = tigetnum("lines");  /* number of lines on screen */
if (tc_n_co == 0) tc_n_co = tigetnum("cols");  /* number of columns on screen */
#endif

if (tc_n_li <= 0 || tc_n_co <= 0) return term_other;

/* Terminal must be capable of moving the cursor to arbitrary positions */

if ((p = tc_s_cm = my_tgetstr(TCI_CM)) == 0) return term_other;

/* Have a look at the "cm" string. If it contains the characters
"%." ("%c" for TERMINFO) then cursor positions are required to be
output in binary. Set the flag to cause the positioning routines
never to generate a move to row or column zero, since binary zero is
used to mark the end of the string. (Also various bits of
communications tend to eat binary zeros.) */

NoZero = FALSE;
while (*p)
  {
#ifdef HAVE_TERMCAP
  if (*p == '%' && p[1] == '.') NoZero = TRUE;
#else
  if (*p == '%' && p[1] == 'c') NoZero = TRUE;
#endif
  p++;
  }

/* If NoZero is set, we need cursor up and cursor left movements;
they are not otherwise used. If the "bc" element is not set, cursor
left is backspace. */

if (NoZero)
  {
  if ((tc_s_up = my_tgetstr(TCI_UP)) == 0) return term_other;
  tc_s_bc = my_tgetstr(TCI_BC);
  }

/* Set the automatic margins flag */

#ifdef HAVE_TERMCAP
tc_f_am = tgetflag("am");
#else
tc_f_am = tigetflag("am");
#endif

/* Some facilities are optional - E will use them if present,
but will use alternatives if they are not. */

tc_s_al = my_tgetstr(TCI_AL);  /* add line */
tc_s_ce = my_tgetstr(TCI_CD);  /* clear EOL */
tc_s_cl = my_tgetstr(TCI_CL);  /* clear screen */
tc_s_cs = my_tgetstr(TCI_CS);  /* set scroll region */
tc_s_dl = my_tgetstr(TCI_DL);  /* delete line */
tc_s_ip = my_tgetstr(TCI_IP);  /* insert padding */
tc_s_ke = my_tgetstr(TCI_KE);  /* unsetup keypad */
tc_s_ks = my_tgetstr(TCI_KS);  /* setup keypad */
tc_s_pc = my_tgetstr(TCI_PC);  /* pad char */
tc_s_se = my_tgetstr(TCI_SE);  /* end standout */
tc_s_sf = my_tgetstr(TCI_SF);  /* scroll up */
tc_s_so = my_tgetstr(TCI_SO);  /* start standout */
tc_s_sr = my_tgetstr(TCI_SR);  /* scroll down */
tc_s_te = my_tgetstr(TCI_TE);  /* end screen management */
tc_s_ti = my_tgetstr(TCI_TI);  /* init screen management */

/* E requires either "set scroll region" with "scroll down", or "delete line"
with "add line" in order to implement scrolling. */

if ((tc_s_cs == NULL || tc_s_sr == NULL) &&
  (tc_s_dl == NULL || tc_s_al == NULL))
    return term_other;

/* If no highlighting, set to null strings */

if (tc_s_se == NULL || tc_s_so == NULL) tc_s_se = tc_s_so = "";

/* E is prepared to use "insert char" provided it does not have to
switch into an "insert mode", and similarly for "delete char". */

if ((tc_s_ic = my_tgetstr(TCI_IC)) != NULL)
  {
  uschar *tv = my_tgetstr(TCI_IM);
  if (tc_s_ic == NULL || (tv != NULL && *tv != 0)) tc_s_ic = NULL;
#ifdef HAVE_TERMCAP
  if (tv != NULL) free(tv);
#endif
  }

if ((tc_s_dc = my_tgetstr(TCI_DC)) != NULL)
  {
  uschar *tv = my_tgetstr(TCI_DM);
  if (tc_s_dc == NULL || (tv != NULL && *tv != 0)) tc_s_dc = NULL;
#ifdef HAVE_TERMCAP
  if (tv != NULL) free(tv);
#endif
  }

/* Now we must scan for the strings sent by special keys and
construct a data structure for sunix to scan. Only the
cursor keys are mandatory. Key values > 127 are always data. */

tc_k_strings = (uschar *)malloc(tc_keylistsize);
keyptr = tc_k_strings + 1;

tc_k_trigger = (uschar *)malloc(128);
memset((void *)tc_k_trigger, 255, 128);  /* all unset */

if (tgetkeystr(TCI_KU, &keycount, &keyptr, Pkey_up) == 0) return term_other;
if (tgetkeystr(TCI_KD, &keycount, &keyptr, Pkey_down) == 0) return term_other;
if (tgetkeystr(TCI_KL, &keycount, &keyptr, Pkey_left) == 0) return term_other;
if (tgetkeystr(TCI_KR, &keycount, &keyptr, Pkey_right) == 0) return term_other;

/* Some terminals have more facilities than can be described by termcap/info.
Knowledge of some of them is screwed in to this code. If termcap/info ever
expands, this can be generalized. There's a lot of history here... */

tt_special = tt_special_none;

if (strcmp(term_name, "ttpa") == 0 || strcmp(term_name, "arc-hydra") == 0)
  tt_special = tt_special_ttpa;
else if (strcmp(term_name, "termulator-bbca") == 0)
            tt_special = tt_special_ArcTermulator;
else if (strncmp(term_name, "xterm",5) == 0) tt_special = tt_special_xterm;
else if (strcmp(term_name, "fawn") == 0) tt_special = tt_special_fawn;

/* If there's screen management, and this is Xterm, we don't need a
newline at the end. */

if (tt_special == tt_special_xterm && tc_s_ti != NULL) main_nlexit = FALSE;

/* Could use some recent capabilities for some of these, but there
isn't a complete set defined yet, and the termcaps/terminfos don't
always have them in. */

if (tt_special == tt_special_fawn)
  {
  addkeystr("\033OP\033OD", Pkey_sh_left, &keycount, &keyptr);   /* #4 */
  addkeystr("\033OP\033OC", Pkey_sh_right, &keycount, &keyptr);  /* %i */
  addkeystr("\033OP\033OA", Pkey_sh_up, &keycount, &keyptr);
  addkeystr("\033OP\033OB", Pkey_sh_down, &keycount, &keyptr);
  }

if (tt_special == tt_special_xterm)
  {
  addkeystr("\033Ot", Pkey_sh_left, &keycount, &keyptr);   /* #4 */
  addkeystr("\033Ov", Pkey_sh_right, &keycount, &keyptr);  /* %i */
  addkeystr("\033Ox", Pkey_sh_up, &keycount, &keyptr);
  addkeystr("\033Or", Pkey_sh_down, &keycount, &keyptr);

  /* These allocations require suitable configuration of the xterm */
  addkeystr("\033OT", Pkey_ct_left, &keycount, &keyptr);
  addkeystr("\033OV", Pkey_ct_right, &keycount, &keyptr);
  addkeystr("\033OX", Pkey_ct_up, &keycount, &keyptr);
  addkeystr("\033OR", Pkey_ct_down, &keycount, &keyptr);
  addkeystr("\033OM", Pkey_ct_tab, &keycount, &keyptr);
  addkeystr("\033OP", Pkey_sh_del127, &keycount, &keyptr);
  addkeystr("\033ON", Pkey_ct_del127, &keycount, &keyptr);
  addkeystr("\033OQ", Pkey_sh_bsp, &keycount, &keyptr);
  addkeystr("\033OO", Pkey_ct_bsp, &keycount, &keyptr);
  }

if (tt_special == tt_special_ttpa)
  {
  addkeystr("\033F", Pkey_copy, &keycount, &keyptr);      /* &5 */
  addkeystr("\033k", Pkey_sh_copy, &keycount, &keyptr);
  addkeystr("\033K", Pkey_ct_copy, &keycount, &keyptr);
  addkeystr("\033l", Pkey_sh_left, &keycount, &keyptr);   /* #4 */
  addkeystr("\033m", Pkey_sh_right, &keycount, &keyptr);  /* %i */
  addkeystr("\033o", Pkey_sh_up, &keycount, &keyptr);
  addkeystr("\033n", Pkey_sh_down, &keycount, &keyptr);

  addkeystr("\033J", Pkey_ct_left, &keycount, &keyptr);
  addkeystr("\033I", Pkey_ct_right, &keycount, &keyptr);
  addkeystr("\033G", Pkey_ct_up, &keycount, &keyptr);
  addkeystr("\033H", Pkey_ct_down, &keycount, &keyptr);

  addkeystr("\033p", Pkey_sh_del127, &keycount, &keyptr);
  addkeystr("\033q", Pkey_ct_del127, &keycount, &keyptr);
  addkeystr("\033s", Pkey_bsp, &keycount, &keyptr);
  addkeystr("\033t", Pkey_sh_bsp, &keycount, &keyptr);
  addkeystr("\033u", Pkey_ct_bsp, &keycount, &keyptr);
  }

if (tt_special == tt_special_ArcTermulator)
  {
  addkeystr("\033[1!", Pkey_insert, &keycount, &keyptr);   /* insert key */
  addkeystr("\033[4!", Pkey_del127, &keycount, &keyptr);   /* delete key */
  addkeystr("\033[2!", Pkey_home, &keycount, &keyptr);     /* home key */
  addkeystr("\033[5!", Pkey_copy, &keycount, &keyptr);     /* copy key */
  addkeystr("\033[3!", Pkey_sh_up, &keycount, &keyptr);    /* page up */
  addkeystr("\033[6!", Pkey_sh_down, &keycount, &keyptr);  /* page down */
  addkeystr("\033[D",  Pkey_sh_left, &keycount, &keyptr);  /* shift left */
  addkeystr("\033[C",  Pkey_sh_right, &keycount, &keyptr); /* shift right */
  }

/* Termcap/info supported function keys */

tgetkeystr(TCI_K0, &keycount, &keyptr, Pkey_f0);
tgetkeystr(TCI_K1, &keycount, &keyptr, Pkey_f0+1);
tgetkeystr(TCI_K2, &keycount, &keyptr, Pkey_f0+2);
tgetkeystr(TCI_K3, &keycount, &keyptr, Pkey_f0+3);
tgetkeystr(TCI_K4, &keycount, &keyptr, Pkey_f0+4);
tgetkeystr(TCI_K5, &keycount, &keyptr, Pkey_f0+5);
tgetkeystr(TCI_K6, &keycount, &keyptr, Pkey_f0+6);
tgetkeystr(TCI_K7, &keycount, &keyptr, Pkey_f0+7);
tgetkeystr(TCI_K8, &keycount, &keyptr, Pkey_f0+8);
tgetkeystr(TCI_K9, &keycount, &keyptr, Pkey_f0+9);
tgetkeystr(TCI_KK, &keycount, &keyptr, Pkey_f0+10);
tgetkeystr(TCI_F1, &keycount, &keyptr, Pkey_f0+11);
tgetkeystr(TCI_F2, &keycount, &keyptr, Pkey_f0+12);
tgetkeystr(TCI_F3, &keycount, &keyptr, Pkey_f0+13);
tgetkeystr(TCI_F4, &keycount, &keyptr, Pkey_f0+14);
tgetkeystr(TCI_F5, &keycount, &keyptr, Pkey_f0+15);
tgetkeystr(TCI_F6, &keycount, &keyptr, Pkey_f0+16);
tgetkeystr(TCI_F7, &keycount, &keyptr, Pkey_f0+17);
tgetkeystr(TCI_F8, &keycount, &keyptr, Pkey_f0+18);
tgetkeystr(TCI_F9, &keycount, &keyptr, Pkey_f0+19);
tgetkeystr(TCI_FA, &keycount, &keyptr, Pkey_f0+20);
tgetkeystr(TCI_FB, &keycount, &keyptr, Pkey_f0+21);
tgetkeystr(TCI_FC, &keycount, &keyptr, Pkey_f0+22);
tgetkeystr(TCI_FD, &keycount, &keyptr, Pkey_f0+23);
tgetkeystr(TCI_FE, &keycount, &keyptr, Pkey_f0+24);
tgetkeystr(TCI_FF, &keycount, &keyptr, Pkey_f0+25);
tgetkeystr(TCI_FG, &keycount, &keyptr, Pkey_f0+26);
tgetkeystr(TCI_FH, &keycount, &keyptr, Pkey_f0+27);
tgetkeystr(TCI_FI, &keycount, &keyptr, Pkey_f0+28);
tgetkeystr(TCI_FJ, &keycount, &keyptr, Pkey_f0+29);
tgetkeystr(TCI_FK, &keycount, &keyptr, Pkey_f0+30);

/* Other terminals may support alternatives, or shifted versions,
or not use termcap/terminfo. */

if (tt_special == tt_special_ttpa)
  {
/*  addkeystr("\033X", Pkey_f0+10, &keycount, &keyptr)  // F10 */
/*  addkeystr("\033W", Pkey_f0+11, &keycount, &keyptr)  // F11 */
/*  addkeystr("\033V", Pkey_f0+12, &keycount, &keyptr)  // F12 */
  addkeystr("\033b", Pkey_f0+11, &keycount, &keyptr);  /* shift/f1 = f11 */
  addkeystr("\033c", Pkey_f0+12, &keycount, &keyptr);  /* shift/f2 = f12 */
  addkeystr("\033d", Pkey_f0+13, &keycount, &keyptr);  /* shift/f3 */
  addkeystr("\033e", Pkey_f0+14, &keycount, &keyptr);  /* shift/f4 */
  addkeystr("\033f", Pkey_f0+15, &keycount, &keyptr);  /* shift/f5 */
  addkeystr("\033g", Pkey_f0+16, &keycount, &keyptr);  /* shift/f6 */
  addkeystr("\033h", Pkey_f0+17, &keycount, &keyptr);  /* shift/f7 */
  addkeystr("\033i", Pkey_f0+18, &keycount, &keyptr);  /* shift/f8 */
  addkeystr("\033j", Pkey_f0+19, &keycount, &keyptr);  /* shift/f9 */
  addkeystr("\033.", Pkey_f0+20, &keycount, &keyptr);  /* shift/f10 */
  }

if (tt_special == tt_special_ArcTermulator)
  {
  addkeystr("\033[17!", Pkey_f0+1, &keycount, &keyptr);  /* function keys not*/
  addkeystr("\033[18!", Pkey_f0+2, &keycount, &keyptr);  /* in termcap/info */
  addkeystr("\033[19!", Pkey_f0+3, &keycount, &keyptr);
  addkeystr("\033[20!", Pkey_f0+4, &keycount, &keyptr);
  addkeystr("\033[21!", Pkey_f0+5, &keycount, &keyptr);
  addkeystr("\033[23!", Pkey_f0+6, &keycount, &keyptr);
  addkeystr("\033[24!", Pkey_f0+7, &keycount, &keyptr);
  addkeystr("\033[25!", Pkey_f0+8, &keycount, &keyptr);
  addkeystr("\033[26!", Pkey_f0+9, &keycount, &keyptr);
  addkeystr("\033[28!", Pkey_f0+10, &keycount, &keyptr);
  addkeystr("\033[29!", Pkey_f0+11, &keycount, &keyptr);
  addkeystr("\033[31!", Pkey_f0+12, &keycount, &keyptr);
  }

/* There are certain escape sequences that are built into NE. We
put them last so that they are only matched if those obtained
from termcap/terminfo do not contain the same sequences. */

addkeystr("\0330", Pkey_f0+10, &keycount, &keyptr);
addkeystr("\0331", Pkey_f0+1,  &keycount, &keyptr);
addkeystr("\0332", Pkey_f0+2,  &keycount, &keyptr);
addkeystr("\0333", Pkey_f0+3,  &keycount, &keyptr);
addkeystr("\0334", Pkey_f0+4,  &keycount, &keyptr);
addkeystr("\0335", Pkey_f0+5,  &keycount, &keyptr);
addkeystr("\0336", Pkey_f0+6,  &keycount, &keyptr);
addkeystr("\0337", Pkey_f0+7,  &keycount, &keyptr);
addkeystr("\0338", Pkey_f0+8,  &keycount, &keyptr);
addkeystr("\0339", Pkey_f0+9,  &keycount, &keyptr);

addkeystr("\033\0330", Pkey_f0+20, &keycount, &keyptr);
addkeystr("\033\0331", Pkey_f0+11, &keycount, &keyptr);
addkeystr("\033\0332", Pkey_f0+12, &keycount, &keyptr);
addkeystr("\033\0333", Pkey_f0+13, &keycount, &keyptr);
addkeystr("\033\0334", Pkey_f0+14, &keycount, &keyptr);
addkeystr("\033\0335", Pkey_f0+15, &keycount, &keyptr);
addkeystr("\033\0336", Pkey_f0+16, &keycount, &keyptr);
addkeystr("\033\0337", Pkey_f0+17, &keycount, &keyptr);
addkeystr("\033\0338", Pkey_f0+18, &keycount, &keyptr);
addkeystr("\033\0339", Pkey_f0+19, &keycount, &keyptr);

addkeystr("\033\033", Pkey_data, &keycount, &keyptr);
addkeystr("\033\177", Pkey_null, &keycount, &keyptr);
addkeystr("\033\015", Pkey_reshow, &keycount, &keyptr);
addkeystr("\033\t", Pkey_backtab, &keycount, &keyptr);
addkeystr("\033s", 19, &keycount, &keyptr);  /* ctrl/s */
addkeystr("\033q", 17, &keycount, &keyptr);  /* ctrl/q */

/* Set the count of strings in the first byte */

tc_k_strings[0] = keycount;

/* Remove the default actions for various shift+ctrl keys
that are not settable. This will prevent them from being
displayed. */

if (tt_special == tt_special_ttpa)
  {
  keycount = 0;
  while (ttpa_non_keys[keycount] != 0)
    key_table[ttpa_non_keys[keycount++]] = 0;
  }

else if (tt_special == tt_special_xterm)
  {
  keycount = 0;
  while (xterm_non_keys[keycount] != 0)
    key_table[xterm_non_keys[keycount++]] = 0;
  }

else if (tt_special == tt_special_fawn)
  {
  keycount = 0;
  while (fawn_non_keys[keycount] != 0)
    key_table[fawn_non_keys[keycount++]] = 0;
  }

else if (tt_special != tt_special_ttpa)
  {
  keycount = 0;
  while (non_keys[keycount] != 0) key_table[non_keys[keycount++]] = 0;
  }

/* Yield fullscreen terminal type */

return term_screen;
}


/*************************************************
*       Generate names for special keys          *
*************************************************/

/* This procedure is used to generate names for odd key
combinations that are used to represent keystrokes that
can't be generated normally. Return TRUE if the name
has been generated, else FALSE. */

BOOL sys_makespecialname(int key, uschar *buff)
{
if (key == s_f_tab + s_f_ctrlbit)
  {
  strcpy(buff, "esc tab    ");
  return TRUE;
  }
return FALSE;
}


/*************************************************
*         Additional special key text            *
*************************************************/

/* This function is called after outputting info about
special keys, to allow any system-specific comments to
be included. */

void sys_specialnotes(void)
{
error_printf("\n");
error_printf("esc-q        synonym for ctrl/q (XON)  esc-s          synonym for ctrl/s (XOFF)\n");
error_printf("esc-digit    functions 1-10            esc-esc-digit  functions 11-20\n");
error_printf("esc-return   re-display screen         esc-tab        backwards tab\n");
error_printf("esc-esc-char control char as data\n");

}


/*************************************************
*          Handle window size change             *
*************************************************/

void sigwinch_handler(int sig)
{
window_changed = TRUE;
signal(SIGWINCH, sigwinch_handler);
}



/*************************************************
*          Handle (ignore) SIGHUP                *
*************************************************/

/* SIGHUP's are usually ignored, as otherwise there are problems
with running NE in a dedicated xterm window. Ignored in the
sense of not trying to put out fancy messages, that is. 

December 1997: Not quite sure what the above comment is getting at,
but modified NE so as to do the dumping thing without trying to write 
anything to the terminal. Otherwise it was getting stuck if a connection
hung up. */

void sighup_handler(int sig)
{
crash_handler_chatty = FALSE;
crash_handler(sig);
}




/*************************************************
*              Local initialization              *
*************************************************/

/* This is called first thing in main() for doing vital system-
specific early things, especially those concerned with any
special store handling. */

void sys_init1(void)
{
uschar *tabs = getenv("NETABS");
signal(SIGHUP, sighup_handler);

#ifdef call_local_init1
local_init1();
#endif

if (tabs != NULL) main_tabs = tabs;
}


/* This is called after argument decoding is complete to allow
any system-specific over-riding to take place. Main_screenmode
will be TRUE unless -line or -with was specified. */

void sys_init2(void)
{
int i;
uschar *nercname;
uschar *filechars = US"+-*/,.:!?";       /* leave only " and ' */
struct stat statbuf;

#ifdef call_local_init2
local_init2();
#endif

term_name = getenv("TERM");

/* Look for the presence user-specific initialization file */

nercname = getenv("NERC");
if (nercname == NULL)
  {
  strcpy(user_init_file, getenv("HOME"));
  strcat(user_init_file, "/.nerc");
  }
else strcpy(user_init_file, nercname);

if (stat(user_init_file, &statbuf) == 0) main_einit = user_init_file;

/*This is the old code that used to use environment variables. It
is no longer used */

/*************
main_einit = getenv("NEINIT");
if (main_einit == NULL) main_einit = getenv("EINIT");
**************/

/* Remove legal file characters from file delimiters list */

for (i = 0; i < (int)strlen(filechars); i++)
  ch_tab[filechars[i]] &= ~ch_filedelim;

/* Set up terminal type and terminal-specific things */

if (!main_screenmode) term_type = term_other; else
  {
  term_type = CheckTerminal();
  switch (term_type)
    {
    case term_screen:
    screen_max_row = tc_n_li - 1;
    screen_max_col = tc_n_co - 1;
    scommon_select();             /* connect the common screen driver */
    #if HAVE_TIOCGWINSZ
    signal(SIGWINCH, sigwinch_handler);
    #endif
    break;

    default:
    printf("This terminal (%s) cannot support screen editing in NE;\n",
      term_name);
    printf("therefore entering line mode:\n\n");
    main_screenmode = main_screenOK = FALSE;
    break;
    }
  }
}



/*************************************************
*                Munge return code               *
*************************************************/

/* This procedure is called just before exiting, to enable a
return code to be set according to the OS's conventions. */

int sys_rc(int rc)
{
return rc;
}



/*************************************************
*                   Beep                         *
*************************************************/

/* Called only when interactive. Makes the terminal beep.

Arguments:   none
Returns:     nothing
*/

void
sys_beep(void)
{
uschar buff[1];
int tty = open("/dev/tty", O_WRONLY);
buff[0] = 7;
write(tty, buff, 1);
close(tty);
}



/*************************************************
*        Decode ~ at start of file name          *
*************************************************/

/* Called both when opening a file, and when completing a file name.

Arguments:
  name         the current name
  len          length of same 
  buff         a buffer in which to return the expansion
  
Returns:       the buffer address if changed
               the name pointer if unchanged
*/

uschar *
sort_twiddle(uschar *name, int len, uschar *buff)
{
int i;
uschar logname[20];
struct passwd *pw;

/* For ~/thing, convert by reading the HOME variable */

if (name[1] == '/')
  {
  strcpy(buff, getenv("HOME"));
  strncat(buff, name+1, len-1);
  return buff; 
  }

/* Otherwise we must get the home directory from the
password file. */

for (i = 2;; i++) if (i >= len || name[i] == '/') break;
strncpy(logname, name+1, i-1);
logname[i-1] = 0;
pw = getpwnam(logname);
if (pw == NULL) strncpy(buff, name, len); else
  {
  strcpy(buff, pw->pw_dir);
  strncat(buff, name + i, len - i);
  } 
return buff; 
}



/*************************************************
*            Complete a file name                *
*************************************************/

/* This function is called when TAB is pressed in a command line while
interactively screen editing. It tries to complete the file name.

Arguments:
  cmd_buffer    the command buffer
  p             the current offset in the command buffer
  pmaxptr       pointer to the current high-water mark in the command buffer
  
Returns:        possibly updated value of p
                pmax may be updated via the pointer
*/

int
sys_fcomplete(uschar *cmd_buffer, int p, int *pmaxptr)
{
int pb = p - 1;
int pe = p;
int pmax = *pmaxptr;
int len, endlen;
uschar buffer[256];
uschar insert[256];
uschar leafname[64];
BOOL insert_found = FALSE;
BOOL beep = TRUE;
uschar *s;
DIR *dir;
struct dirent *dent;

if (p < 1 || cmd_buffer[pb] == ' ' || p > pmax) goto RETURN;
while (pb > 0 && cmd_buffer[pb - 1] != ' ') pb--;

/* One day we may implement completing in the middle of names, but for
the moment, this hack up just completes ends. */

/* while (pe < pmax && cmd_buffer[pe] != ' ') pe++; */
if (pe < pmax && cmd_buffer[pe] != ' ') goto RETURN;

len = pe - pb;

if (cmd_buffer[pb] == '~') 
  (void)sort_twiddle(cmd_buffer + pb, len, buffer);
else if (cmd_buffer[pb] != '/')
  {
  strcpy(buffer, "./");
  strncat(buffer, cmd_buffer + pb, len); 
  }   
else
  {
  strncpy(buffer, cmd_buffer + pb, len);
  buffer[len] = 0;
  }  

len = strlen(buffer);

/* There must be at least one '/' in the string, because of the checking
done above. */

s = buffer + len - 1;
while (s > buffer && *s != '/') s--;

*s = 0;
endlen = len - (s - buffer) - 1;
dir = opendir(buffer);
if (dir == NULL) goto RETURN;

while ((dent = readdir(dir)) != NULL)
  {
  if (strcmp(dent->d_name, ".") == 0 || strcmp(dent->d_name, "..") == 0)
    continue; 
  if (strncmp(dent->d_name, s + 1, endlen) == 0)
    {
    if (!insert_found)
      { 
      strcpy(insert, dent->d_name + endlen);  
      strcpy(leafname, dent->d_name); 
      insert_found = TRUE; 
      beep = FALSE; 
      }
    else
      {
      int i;
      beep = TRUE; 
      for (i = 0; i < strlen(insert); i++)
        {
        if (insert[i] != dent->d_name[endlen + i]) break;
        }   
      insert[i] = 0;
      }      
    }  
  }
closedir(dir);  

if (insert_found && insert[0] != 0)
  {
  int inslen;
  
  if (!beep)
    { 
    struct stat statbuf;
    strcat(buffer, "/"); 
    strcat(buffer, leafname); 
    if (stat(buffer, &statbuf) == 0 && S_ISDIR(statbuf.st_mode))
      strcat(insert, "/");
     }  
 
  inslen = strlen(insert);
  memmove(cmd_buffer + p + inslen, cmd_buffer + p, strlen(cmd_buffer + p));
  memcpy(cmd_buffer + p, insert, inslen);
  *pmaxptr = pmax + inslen;  
  p += inslen;
  } 

RETURN:
if (beep) sys_beep();
return p;
}


/*************************************************
*             Generate crash file name           *
*************************************************/

uschar *sys_crashfilename(int which)
{
return which? "NEcrash" : "NEcrashlog";
}


/*************************************************
*              Open a file                       *
*************************************************/

/* Interpretation of file names beginning with ~ is done here.
As Unix is not a file-typed system, the filetype argument is
ignored. */

FILE *sys_fopen(uschar *name, uschar *type, int *filetype)
{
uschar buff[256];
if (name[0] == '~') name = sort_twiddle(name, strlen(name), buff);

/* Handle optional automatic backup for output files. We add "~"
to the name, as is common on Unix. */

if (main_backupfiles && strcmp(type, "w") == 0 && !file_written(name))
  {
  uschar bakname[80];
  strcpy(bakname, name);
  strcat(bakname, "~");
  remove(bakname);
  rename(name, bakname);
  file_setwritten(name);
  }

return fopen(name, type);
}


/*************************************************
*              Check file name                   *
*************************************************/

uschar *sys_checkfilename(uschar *s, BOOL reading)
{
uschar *p = (uschar *)s;

while (*p != 0)
  {
  if (*p == ' ')
    {
    while (*(++p) != 0)
      {
      if (*p != ' ') return "(contains a space)";
      }
    break;
    }
  if (*p < ' ' || *p > '~') return "(contains a non-printing character)";
  p++;
  }

return NULL;
}


/*************************************************
*            Set a file's type                   *
*************************************************/

/* Unix does not use file types. */

void sys_setfiletype(uschar *name, int type)
{
}



/*************************************************
*         Give number of characters for newline  *
*************************************************/

int sys_crlfcount(void)
{
return 1;
}



/*************************************************
*            Write to message stream             *
*************************************************/

/* Output to msgs_fid is passed through here, so that it can
be handled system-specifically if necessary. */

void sys_mprintf(FILE *f, char *format, ...)
{
va_list ap;
va_start(ap, format);
vfprintf(f, format, ap);
va_end(ap);
}



/*************************************************
*            Give reason for key refusal         *
*************************************************/

/* This procedure gives reasons for the setting of bits
in the maps that forbid certain keys being set. */

uschar *sys_keyreason(int key)
{
switch (key & ~(s_f_shiftbit+s_f_ctrlbit))
  {
  case s_f_bsp: return "backspace is equivalent to ctrl/h";
  case s_f_ret: return "return is equivalent to ctrl/m";
  default:      return "not available on keyboard";
  }
}



/*************************************************
*           System-specific help info            *
*************************************************/

BOOL sys_help(uschar *s)
{
s = s;
return FALSE;
}



/*************************************************
*           System-specific interrupt check      *
*************************************************/

/* This is called from main_interrupted(). This in turn is called only
when the main part of NE is in control, not during screen editing operations.
We have to check for ^C by steam, as we are running in raw terminal mode.

It turns out that the ioctl can be quite slow, so we don't actually want
to do it for every line in, e.g., an m command. NE now maintains a count of
calls to main_interrupted(), reset when a command line is read, and it also
passes over the kind of processing that is taking place:

  ci_move     m, f, bf, or g command
  ci_type     t or tl command
  ci_read     reading command line or data for i command
  ci_cmd      about to obey a command
  ci_delete   deleting all lines of a buffer
  ci_scan     scanning lines (e.g. for show wordcount)
  ci_loop     about to obey the body of a loop

These are successive integer values, starting from zero. */


/* This vector of masks is used to mask the counter; if the result is
zero, the ioctl is done. Thereby we have different intervals for different
types of activity with very little cost. The numbers are pulled out of the 
air... */

static int ci_masks[] = {
    1023,    /* ci_move - every 1024 lines*/
    0,       /* ci_type - every time */
    0,       /* ci_read - every time */
    15,      /* ci_cmd  - every 16 commands */
    127,     /* ci_delete - every 128 lines */
    1023,    /* ci_scan - every 1024 lines */
    15       /* ci_loop - every 16 commands */
};

void sys_checkinterrupt(int type)
{
if (main_screenOK && !main_escape_pressed &&
    (main_cicount & ci_masks[type]) == 0)
  {
  int c = 0;
  ioctl(ioctl_fd, FIONREAD, &c);
  while (c-- > 0)
    {
    if (getchar() == tc_int_ch) main_escape_pressed = TRUE;
    }
  }
}



/*************************************************
*           Tidy up after interrupted fgets      *
*************************************************/

/* No tidying needed for Unix */

void sys_after_interrupted_fgets(BOOL buffergiven, uschar *buffer)
{
}



/*************************************************
*       Display hourglass during commands        *
*************************************************/

/* This function is called while obeying command lines from
an interactive session. It can be used to display an hourglass
or any other indication of busy-ness. */

void sys_hourglass(BOOL on)
{
}


/* End of sysunix.c */
