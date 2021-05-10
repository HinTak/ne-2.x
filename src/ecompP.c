/*************************************************
*       The E text editor - 2nd incarnation      *
*************************************************/

/* Copyright (c) University of Cambridge, 1991 - 2004 */

/* Written by Philip Hazel, starting November 1991 */
/* This file last modified: February 2004 */


/* This file contains code for interfacing to the PCRE library for handling
regular expressions when compiling for that kind of regex handling. */

#include "ehdr.h"
#include <pcre.h>

/* This vector is used for holding the offsets of captured substrings, and
the scalar holds their number. It is assumed that a call to cmd_ReChange
happens after a match and before any other one. */

#define ExtractSize 30

static int Extracted[ExtractSize];
static int ExtractNumber;
static int ExtractStartAt;



/*************************************************
*           Compile a Regular Expression         *
*************************************************/

/* NE was originally written with its own regex code; we put in the calls to
PCRE with the minimal of disturbance so that two different versions of NE were
easy to maintain. This makes this a bit awkward in places. Now that only one 
version exists, this could be tidied up one day. */

BOOL cmd_makeFSM(qsstr *qs)
{
int offset;
int offset_adjust = 0;
int flags = qs->flags;
int options = ((flags & (qsef_V | qsef_FV)) == 0)? PCRE_CASELESS : 0;
const uschar *error;
uschar *temp = NULL;
uschar *temp2 = NULL;
uschar *pattern = qs->text + 1;

/* If the X flag is on, scan the pattern and encode any hexadecimal pairs.
This is 99% OK for realistic cases... */

if ((flags & qsef_X) != 0)
  {
  uschar *p;
  p = temp2 = store_Xget(strlen(pattern)*2 + 1);

  while (*pattern != 0)
    {
    if (isxdigit(*pattern))
      {
      *p++ = '\\';
      *p++ = 'x';
      *p++ = *pattern++;
      if (isxdigit(*pattern)) *p++ = *pattern++;
      }
    else if (*pattern == '\\')
      {
      if (pattern[1] != 'x')
        {
        *p++ = *pattern++;
        *p++ = *pattern++;
        }
      else pattern += 2;
      }
    else *p++ = *pattern++;
    }
  *p = 0;
  pattern = temp2;
  }

/* These really only ever need to be done once, but setting that up
probably costs more than just doing it every time! */

pcre_malloc = (void *(*)(size_t))store_Xget;
pcre_free = store_free;

/* NE flags implying "from end" matching can be handled by adding to the
beginning or end of the incoming regex. Remember that this was done. Adjust the
offset printed for errors. */

if ((flags & qsef_L) != 0 ||
    ((match_L || (flags & qsef_E) != 0) && (flags & qsef_B) == 0))
  {
  temp = store_Xget(strlen(pattern) + 6);
  offset_adjust = 3;
  sprintf(temp, ".*(%s)%s", pattern, ((flags & qsef_E) != 0)? "$" : "");
  qs->flags |= qsef_REV;
  pattern = temp;
  }
else qs->flags &= ~qsef_REV;

/* The B & H flags can be done by a PCRE flag. */

if ((flags & (qsef_B|qsef_H)) != 0) options |= PCRE_ANCHORED;

/* Do the compilation */

qs->fsm = US pcre_compile(pattern, options, (const char **)(&error), &offset, NULL);
if (temp != NULL) store_free(temp);
if (temp2 != NULL) store_free(temp2);

if (qs->fsm == NULL)
  {
  offset -= offset_adjust;
  if (offset < 0) offset = 0;
  error_moan(63, offset, error);
  return FALSE;
  }

/* The qsef_RP flag tells NE that the regex was compiled with PCRE, and there
is no need to get store for a Journal for matching. */

qs->flags |= qsef_RP;
return TRUE;
}



/*************************************************
*            Match Regular Expression            *
*************************************************/

/* The result of the match ends up in the static variable, in case needed for
replacement, but put the start and end into the standard globals. */

BOOL cmd_matchqsR(qsstr *qs, linestr *line, int USW)
{
uschar *chars = line->text;
int count = qs->count;
int flags = qs->flags;
int leftpos = match_leftpos;
int rightpos = match_rightpos;
int wleft = qs->windowleft;
int wright = qs->windowright;
BOOL yield = FALSE;
BOOL backwards = (flags & qsef_L) != 0 || ((match_L || (flags & qsef_E) != 0) &&
  (flags & qsef_B) == 0);

/* If there's no compiled expression, or if it is compiled in the wrong
direction (which can happen if the argument of an F command is reused
with a BF command or vice versa), or if the casing requirements are wrong,
then (re)compile it. */

if (qs->fsm == NULL ||
    (backwards && (flags & qsef_REV) == 0) ||
    (!backwards && (flags & qsef_REV) != 0) ||
    ((USW & qsef_U) == 0 && (flags & (qsef_U | qsef_V | qsef_FV)) == 0) ||
    ((USW & qsef_U) != 0 && (flags & qsef_FV) != 0)
   )
  {
  if ((USW & qsef_U) == 0) qs->flags |= qsef_FV; else qs->flags &= ~qsef_FV;
  if (qs->fsm != NULL) { store_free(qs->fsm); qs->fsm = NULL; }
  cmd_makeFSM(qs);
  flags = qs->flags;
  }

/* Take note of line length && sig space qualifier */

if (wright > line->len) wright = line->len;
if ((flags & qsef_S) != 0 || (USW & qsef_S) != 0)
  {
  while (wleft < wright && chars[wleft] == ' ') wleft++;
  while (wleft < wright && chars[wright-1] == ' ') wright--;
  }

/* Take note of window qualifier */

if (leftpos < wleft) leftpos = wleft;
if (rightpos > wright) rightpos = wright;
if (rightpos < leftpos) rightpos = leftpos;

/* The B and E flags can now be tested */

if (((flags & qsef_B) != 0 && leftpos != wleft) ||
  ((flags & qsef_B) == 0 && (flags & qsef_E) != 0 && rightpos != wright))
    yield = FALSE;

/* Do the match; we may have to repeat in order to handle the
count qualifier. */

else for (;;)
  {
  int i;

  ExtractNumber = pcre_exec((const pcre *)qs->fsm, NULL,
    chars + leftpos, rightpos - leftpos, 0, 0, Extracted,
      sizeof(Extracted)/sizeof(int));

  if (ExtractNumber == 0) ExtractNumber = (sizeof(Extracted)/sizeof(int))/3;
  if (ExtractNumber < 0) break;

  ExtractStartAt = ((flags & qsef_REV) != 0)? 2 : 0;

  for (i = 0; i < ExtractNumber * 2; i++) Extracted[i] += leftpos;

  match_start = Extracted[ExtractStartAt+0];
  match_end = Extracted[ExtractStartAt+1];

  /* Additional check the P qualifier */

  if ((flags & qsef_EB) == qsef_EB && match_end != rightpos) break;

  /* Additional check for W qualifier */

  yield = TRUE;
  if ((flags & qsef_W) != 0 || (USW & qsef_W) != 0)
    {
    if ((match_start != wleft &&
      (ch_tab[chars[match_start-1]] & ch_word) != 0) ||
      (match_end != wright &&
        (ch_tab[chars[match_end]] & ch_word) != 0))
    yield = FALSE;
    }

  /* Additional check for the count qualifier */

  if (yield && --count <= 0) break;

  /* Skip over this string */

  yield = FALSE;
  if ((flags & qsef_REV) != 0) rightpos = match_start;
    else leftpos = match_end;
  if (leftpos >= rightpos) break;
  }

/* The match has either succeeded or failed. If the N qualifier
is present, reverse the yield and set the pointers to include
the whole line if necessary. */

if ((flags & qsef_N) != 0)
  {
  yield = !yield;
  if (yield)
    {
    match_start = 0;
    match_end = line->len;
    }
  }

return yield;
}


/*************************************************
*         Increase buffer size                   *
*************************************************/

/* This is an auxiliary function for cmd_ReChange(). */

static uschar *incbuffer(uschar *v, int *sizep)
{
uschar *newv = store_Xget(*sizep + 1024);
memcpy(newv, v, *sizep);
*sizep += 1024;
return newv;
}


/*************************************************
*    Change Line after Regular Expression match  *
*************************************************/

/* This procedure is called to make changes to a line after it has been
matched with a regular expression. The replacement string is searched
for occurences of the '$' character, which are interpreted as follows:

$0           insert the entire matched string from the original line
$<digit>     insert the <digit>th "captured substring" from the match
$<non-digit> insert <non-digit>

If, for example, $6 is encountered and there were fewer than 6 captured
strings found, nothing is inserted. The final argument is the address
of a flag, which is always set TRUE when using pcre. */

linestr *cmd_ReChange(linestr *line, uschar *p, int len,
  BOOL hexflag, BOOL eflag, BOOL aflag, BOOL *aok)
{
int i, pp;
int n = 0;
int size = 1024;
uschar *v = store_Xget(size);

*aok = TRUE;

/* Loop to scan replacement string */

for (pp = 0; pp < len; pp++)
  {
  int cc = p[pp];

  /* Ensure at least one byte left in v */

  if (n >= size) v = incbuffer(v, &size);

  /* Deal with non-meta character (not '$') */

  if (cc != '$')
    {
    /* Deal with hexadecimal data */
    if (hexflag)
      {
      int x = 0;
      for (i = 1; i <= 2; i++)
        {
        x = x << 4;
        cc = tolower(cc);
        x += ('a' <= cc && cc <= 'f')? cc - 'a' + 10 : cc - '0';
        cc = p[pp+1];
        }
      pp++;
      v[n++] = x;
      }

    /* Deal with normal data */

    else v[n++] = cc;
    }

  /* Deal with meta-character ('$') */

  else
    {
    if (++pp < len)
      {
      cc = p[pp];
      if (!isdigit(cc)) v[n++] = cc;
      else
        {
        uschar *ppp = line->text;
        cc -= '0';

        /* Have to deal with 0 specially, since it is allowed even
        if the previous match wasn't a regex. */

        if (cc == 0)
          {
          while (n + match_end - match_start >= size) v = incbuffer(v, &size);
          for (i = match_start; i < match_end; i++) v[n++] = ppp[i];
          }
        else if (cc < ExtractNumber)
          {
          int x = Extracted[ExtractStartAt + 2*cc];
          int y = Extracted[ExtractStartAt + 2*cc+1];
          while (n + y - x >= size) v = incbuffer(v, &size);
          for (i = x; i < y; i++) v[n++] = ppp[i];
          }
        }
      }
    }
  }

/* We now have built the replacement string in v */

if (eflag)
  {
  line_deletech(line, match_start, match_end - match_start, TRUE);
  line_insertch(line, match_start, v, n, 0);
  cursor_col = match_start + n;
  }
else
  {
  line_insertch(line, (aflag? match_end:match_start), v, n, 0);
  cursor_col = match_end + n;
  }

store_free(v);
return line;
}

/* End of ecompP.c */
