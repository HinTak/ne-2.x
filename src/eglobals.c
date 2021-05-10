/*************************************************
*       The E text editor - 2nd incarnation      *
*************************************************/

/* Copyright (c) University of Cambridge, 1991 - 2004 */

/* Written by Philip Hazel, starting November 1991 */
/* This file last modified: February 2004 */


/* This file contains all the global data */


#include "ehdr.h"
#include "keyhdr.h"


/*************** Debug things *************/

FILE *crash_logfile = NULL;
FILE *debug_file = NULL;



/***************  Scalars  ****************/

FILE *cmdin_fid = NULL;
FILE *from_fid;
FILE *msgs_fid;
FILE *kbd_fid;

uschar *arg_from_name = NULL;         /* Names on command line */
uschar *arg_to_name = NULL;
uschar *arg_ver_name = NULL;
uschar *arg_with_name = NULL;
uschar *arg_zero;                     /* ARGV[0] */

bufferstr *currentbuffer;

int   cmd_bracount;
int   cmd_breakloopcount;
uschar *cmd_buffer;
BOOL  cmd_casematch = FALSE;
int   cmd_clineno;
linestr *cmd_cbufferline;
uschar *cmd_cmdline;
BOOL  cmd_eoftrap;
BOOL  cmd_faildecode;
int   cmd_ist;
BOOL  cmd_onecommand;
uschar *cmd_ptr;
BOOL  cmd_refresh;
uschar *cmd_stack[cmd_stacktop+1];
int   cmd_stackptr;

uschar  cmd_word[max_wordlen + 1];

BOOL  crash_handler_chatty = TRUE;

int cursor_row;
int cursor_col;
int cursor_max;
int cursor_rh_adjust = 0;
int cursor_offset;

linestr *cut_buffer;
linestr *cut_last;
int      cut_type;
BOOL     cut_pasted = TRUE;

int     default_filetype = 0;
int     default_rmargin = 79;

int     error_count = 0;
jmp_buf error_jmpbuf;
BOOL    error_longjmpOK = FALSE;
BOOL    error_werr = FALSE;

filewritstr *files_written = NULL;

uschar    key_codes[256];
LONGINT key_controlmap;
LONGINT key_functionmap;
LONGINT key_specialmap[4];

sestr *last_se;
sestr *last_abese;
qsstr *last_abent;
sestr *last_gse;
qsstr *last_gnt;

linestr *main_bottom;
linestr *main_current;
linestr *main_lastundelete;
linestr *main_top;
linestr *main_undelete;

bufferstr *main_bufferchain;
backstr   *main_backlist;
procstr   *main_proclist = NULL;

BOOL  main_appendswitch = FALSE;
BOOL  main_attn = TRUE;
BOOL  main_AutoAlign = FALSE;
int   main_backprevptr;
int   main_backptr;
BOOL  main_backupfiles = FALSE;
BOOL  main_binary = FALSE;
int   main_cicount = 0;
int   main_currentlinenumber = 0;
BOOL  main_detrail_output = FALSE;
BOOL  main_done = FALSE;
int   main_drawgraticules;
BOOL  main_eightbit = FALSE;
uschar *main_einit = NULL;
BOOL  main_eoftrap = FALSE;
BOOL  main_escape_pressed = FALSE;
uschar *main_filealias;
BOOL  main_filechanged;
uschar *main_filename;
uschar *main_fromlist[MAX_FROM];
int   main_hscrollamount = 10;
int   main_ilinevalue = 3;
int   main_imax;
int   main_imin;
BOOL  main_initialized = FALSE;
BOOL  main_interactive = TRUE;
BOOL  main_leave_message = FALSE;
int   main_linecount = 0;
BOOL  main_logging = FALSE;
int   main_nextbufferno;
BOOL  main_nlexit = TRUE;
BOOL  main_noinit = FALSE;
BOOL  main_nowait;
BOOL  main_oldcomment = FALSE;
BOOL  main_oneattn = FALSE;
uschar *main_opt;
int   main_optscroll = 6;
BOOL  main_overstrike = FALSE;
BOOL  main_pendnl = FALSE;
int   main_rc = 0;                  /* The final return code */
BOOL  main_readonly = FALSE;
BOOL  main_repaint;
int   main_rmargin = 79;            /* Default for line-by-line */
BOOL  main_screenmode = TRUE;
BOOL  main_screenOK = FALSE;
BOOL  main_screensuspended = FALSE;
int  *main_screentabs = NULL;
BOOL  main_selectedbuffer;
BOOL  main_shownlogo = FALSE;       /* FALSE if need to show logo on error */
LONGINT main_storetotal = 0;        /* Total store used */
int   main_stream = 0;
BOOL  main_tabflag = FALSE;
BOOL  main_tabin = FALSE;
BOOL  main_tabout = FALSE;
uschar *main_tabs = US"tabs";
int   main_undeletecount = 0;
BOOL  main_unixregexp = FALSE;
int   main_vcursorscroll = 1;
BOOL  main_verified_ptr = FALSE;
BOOL  main_verify = TRUE;
BOOL  main_warnings = TRUE;

#ifdef Windowing
BOOL  main_windowing = FALSE;       /* TRUE if running in a window system */
#endif

int   mark_col;
BOOL  mark_hold;
int   mark_type;
linestr *mark_line;

int  match_end;
BOOL match_L;
int  match_leftpos;
int  match_rightpos;
int  match_start;

sestr *par_begin = NULL;
sestr *par_end = NULL;

sestr *saved_se = NULL;

BOOL  screen_autoabove;
BOOL  screen_forcecls = FALSE;    
int   screen_max_col;               /* Applies to whole screen */
int   screen_max_row;
BOOL  screen_suspend = TRUE;        /* Suspend for * commands */
BOOL  screen_use_scroll = TRUE;     /* Use optimizing scrolls */

int   sys_openfail_reason = of_other;

int   topbit_minimum = 160;         /* minimum top-bit uschar */

uschar *version_date;                 /* Identity date */
uschar *version_string;               /* Identity of program version */

linestr **window_vector = NULL;

int  window_bottom;
int  window_depth;
int  window_top;
int  window_width;

void (HUGE *s_cls)(void);
void (HUGE *s_defwindow)(int, int, int);
void (HUGE *s_eraseright)(void);
void (HUGE *s_flush)(void);
void (HUGE *s_hscroll)(int, int, int, int, int);
void (HUGE *s_init)(int, int, BOOL);
void (HUGE *s_maxx)(void);
void (HUGE *s_maxy)(void);
void (HUGE *s_move)(int, int);
void (HUGE *s_overstrike)(int);
void (HUGE *s_printf)(uschar *, ...);
void (HUGE *s_putc)(int);
void (HUGE *s_rendition)(int);
void (HUGE *s_selwindow)(int, int, int);
void (HUGE *s_settabs)(int *);
void (HUGE *s_terminate)(void);
void (HUGE *s_vscroll)(int, int, int, BOOL);
void (HUGE *s_window)(void);
void (HUGE *s_x)(void);
void (HUGE *s_y)(void);

void  (HUGE *sys_resetterminal)(void);
void  (HUGE *sys_setupterminal)(void);


/****************  Vectors  *****************/

/* Names of keystroke actions */

keynamestr key_actnames[] = {
  { "al", ka_al },
  { "alp", ka_alp },
  { "cat", ka_join },
  { "cl", ka_cl },
  { "clb", ka_clb },
  { "co", ka_co },
  { "csd", ka_csd },
  { "csl", ka_csl },
  { "csle", ka_csle },
  { "csls", ka_csls },
  { "csnl", ka_csnl },
  { "csr", ka_csr },
  { "cssbr", ka_cssbr },
  { "cssl", ka_cssl },
  { "csstl", ka_csstl },
  { "csptb", ka_csptab },
  { "cstb", ka_cstab },
  { "cstl", ka_cstl },
  { "cstr", ka_cstr },
  { "csu", ka_csu },
  { "cswl", ka_cswl },
  { "cswr", ka_cswr },
  { "cu", ka_cu },
  { "dal", ka_dal },
  { "dar", ka_dar },
  { "dc", ka_dc },
  { "de", ka_de },
  { "dl", ka_dl },
  { "dp", ka_dp },
  { "dtwl", ka_dtwl },
  { "dtwr", ka_dtwr },  
  { "gm", ka_gm },
  { "lb", ka_lb },
  { "pa", ka_pa },
  { "rb", ka_rb },
  { "rc", ka_rc },
  { "rf", ka_reshow },
  { "rs", ka_rs },
  { "sb", ka_scbot },
  { "sd", ka_scdown },
  { "sl", ka_scleft },
  { "sp", ka_split },
  { "sr", ka_scright },
  { "st", ka_sctop },
  { "su", ka_scup },
  { "tb", ka_tb }
};

int key_actnamecount = sizeof(key_actnames)/sizeof(keynamestr);


/* Names for special keys */

keynamestr key_names[] = {
  { "up",        s_f_cup },
  { "down",      s_f_cdn },
  { "left",      s_f_clf },
  { "right",     s_f_crt },
  { "del",       s_f_del },
  { "delete",    s_f_del },
  { "bsp",       s_f_bsp },
  { "backsp",    s_f_bsp },
  { "backspace", s_f_bsp },
  { "ret",       s_f_ret },
  { "return",    s_f_ret },
  { "tab",       s_f_tab },
  { "ins",       s_f_ins },
  { "insert",    s_f_ins },
  { "home",      s_f_hom },
  { "pup",       s_f_pup },
  { "pageup",    s_f_pup },
  { "pdown",     s_f_pdn },
  { "pagedown",  s_f_pdn },
  { "pagedn",    s_f_pdn },
  { "end",       s_f_end },
  { "copy",      s_f_copykey},
  { "",          0 } 
};

/* Character type tables. Some of this could be done with standard C
functions, but it's easier just to carry over the classification used
in the previous BCPL implementation of E. In any case, there are some
private things that would need doing specially. The tables are set up
dynamically, so as to be character-set-independent, and changeable by
the "word" command. */

uschar ch_tab[256];
uschar ch_tab2[256];

/* Vector of pointers to keystrings */

uschar *main_keystrings[max_keystring+1];

/* The fixed keystrings are not changeable by the user, but are here
simply to keep them next to the previous table for convenience. */

uschar *main_fixedKstrings[max_fixedKstring+1] = {
  NULL, US"w", US"f", US"bf", US"show keys", US"2screen" };


/* The default setting for the keystroke translation table. The user can
cause the contents of this table to be changed. */

short int key_table[] = {
  0,        /* dummy  */
ka_al,      /* ctrl/a */
ka_lb,      /* ctrl/b */
ka_cl,      /* ctrl/c */
ka_reshow,  /* ctrl/d */
ka_co,      /* ctrl/e */
0,          /* ctrl/f */
ka_rc,      /* ctrl/g */
ka_scleft,  /* ctrl/h */
ka_cstab,   /* ctrl/i */
ka_scdown,  /* ctrl/j */
ka_scup,    /* ctrl/k */
ka_scright, /* ctrl/l */
ka_split,   /* ctrl/m = RETURN */
ka_gm,      /* ctrl/n */
60,         /* ctrl/o calls keystring 60 */
ka_pa,      /* ctrl/p */
ka_de,      /* ctrl/q */
ka_rb,      /* ctrl/r */
ka_rs,      /* ctrl/s */
ka_tb,      /* ctrl/t */
ka_dl,      /* ctrl/u */
ka_dar,     /* ctrl/v */
ka_cu,      /* ctrl/w */
ka_dal,     /* ctrl/x */
ka_dc,      /* ctrl/y */
ka_alp,     /* ctrl/z */
0,          /* ctrl/[ */
ka_cssl,    /* ctrl/\ */
0,          /* ctrl/] */
58,         /* ctrl/^ calls keystring 58 */
59,         /* ctrl/_ calls keystring 59 */

ka_csu, ka_scup,    ka_sctop, 0,            /* cursor up */
ka_csd, ka_scdown,  ka_scbot, 0,            /* cursor down */
ka_csl, ka_scleft,  ka_cstl,  ka_csls,      /* cursor left */
ka_csr, ka_scright, ka_cstr,  ka_csle,      /* cursor right */

ka_dp, ka_clb, ka_dal, 0,   /* del */
ka_dp, ka_clb, ka_dal, 0,   /* backspace */
ka_split, 0, 0, 0,          /* return */
ka_cstab, 0, ka_csptab, 0,  /* tab */
0, 0, 0, 0,                 /* ins */
0, 0, 0, 0,                 /* home */
0, 0, 0, 0,                 /* page up */
0, 0, 0, 0,                 /* page down */
0, 0, 0, 0,                 /* end */
ka_dc, ka_cl, ka_dar, 0,    /* copy key */

1,           /* F1 */
2,           /* F2 */
3,           /* F3 */
4,           /* F4 */
5,           /* F5 */
6,           /* F6 */
7,           /* F7 */
8,           /* F8 */
9,           /* F9 */
10,          /* F10 */

11,          /* F11 */
12,          /* F12 */
13,          /* F13 */
14,          /* F14 */
15,          /* F15 */
16,          /* F16 */
17,          /* F17 */
18,          /* F18 */
19,          /* F19 */
20,          /* F20 */

21,          /* F21 */
22,          /* F22 */
23,          /* F23 */
24,          /* F24 */
25,          /* F25 */
26,          /* F26 */
27,          /* F27 */
28,          /* F28 */
29,          /* F29 */
30};         /* F30 */


/* Table for actual control keys that are not changeable by the user. We have this
here simply to keep it next to the previous one. The only purpose of this trans-
lation is to get values into the same value space (ka_xxx) as other keys. */

short int key_fixedtable[] = {
  0,               /* s_f_user */
  ka_ret,          /* s_f_return */
  ka_wtop,         /* s_f_top */
  ka_wbot,         /* s_f_bottom */
  ka_wleft,        /* s_f_left */
  ka_wright,       /* s_f_right */
  ka_dpleft,       /* s_f_leftdel */
  ka_last,         /* s_f_lastchar */
  ka_forced,       /* s_f_forcedend */
  ka_reshow,       /* s_f_reshow */
  ka_scleft,       /* s_f_scrleft */
  ka_scright,      /* s_f_scrright */
  ka_scup,         /* s_f_scrup */
  ka_scdown,       /* s_f_scrdown */
  ka_sctop,        /* s_f_scrtop */
  ka_scbot,        /* s_f_scrbot */
  ka_dar,          /* s_f_delrt */
  ka_dal,          /* s_f_dellf */
  ka_csls,         /* s_f_startline */
  ka_csle,         /* s_f_endline */
  ka_cswl,         /* s_f_wordlf */
  ka_cswr,         /* s_f_wordrt */
  ka_csnl,         /* s_f_nextline */
  ka_csstl,        /* s_f_topleft */
  ka_cssbr,        /* s_f_botright */
  ka_rc,           /* s_f_readcom */
  -4,              /* s_f_help calls fixed keystring 4 (show keys) */
  -2,              /* s_f_find calls fixed keystring 2 (f) */
  -3,              /* s_f_bfind calls fixed keystroing 3 (bf) */
  ka_pa,           /* s_f_paste */
  ka_tb,           /* s_f_tb */
  ka_rb,           /* s_f_rb */
  ka_cu,           /* s_f_cut */
  ka_co,           /* s_f_copy */
  ka_de,           /* s_f_dmarked */
  ka_dc,           /* s_f_dc */
  ka_dp,           /* s_f_dp */
  ka_dl,           /* s_f_delline */
  -5};             /*s_f_resized calls fixed keystring 5 (2screen) */

/* Names of key actions, for printing out */

uschar *key_actionnames[] = {
  US"align line(s) with cursor",
  US"align line(s) with previous",
  US"close up spaces to right",
  US"close up spaces to left",
  US"copy text or rectangle",
  US"cursor down",
  US"cursor left",
  US"cursor to line start",
  US"cursor to line end",
  US"cursor to next line",
  US"cursor to left of text",
  US"cursor to right of text",
  US"cursor right",
  US"cursor to bottom right",
  US"cursor to left of screen",
  US"cursor to top left",
  US"cursor to next tab stop",
  US"cursor to previous tab",
  US"cursor up",
  US"cursor to previous word",
  US"cursor to next word",
  US"cut text or rectangle",
  US"delete left in line(s)",
  US"delete right in line(s)",
  US"delete character at cursor",
  US"delete text or rectangle",
  US"delete line(s)",
  US"delete previous character",
  US"delete to start word left",
  US"delete to start word right",  
  US"set global mark",
  US"concatenate lines",
  US"start bulk line operation",
  US"paste text or rectangle",
  US"start rectangular operation",
  US"refresh screen",
  US"prompt for command line",
  US"insert rectangle of spaces",
  US"scroll to end of buffer",
  US"scroll down",
  US"scroll left",
  US"scroll right",
  US"scroll to top of buffer",
  US"scroll up",
  US"split line",
  US"start text operation"
};


/* Names of special keys */

uschar *key_specialnames[] = {
  US"up     ",
  US"down   ",
  US"left   ",
  US"right  ",
  US"delete ",
  US"backsp ",
  US"return ",
  US"tab    ",
  US"insert ",
  US"home   ",
  US"pageup ",
  US"pagedn ",
  US"end    ",
  US"copy   ",
};

/* End of globals_c */
