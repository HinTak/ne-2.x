/*************************************************
*       The E text editor - 2nd incarnation      *
*************************************************/

/* Copyright (c) University of Cambridge, 1991 - 2004 */

/* Written by Philip Hazel, starting November 1991 */

/* This file contains one function, which sets up the current version string.
Note the fudgery needed to prevent the compiler optimizing the copyright string
out of the binary. */

#define this_version  "2.01"

#include "ehdr.h"

void version_init(void)
{
int i = 0;
uschar *copyright = US"Copyright (c) University of Cambridge 2005";
uschar today[20];

version_string = malloc(strlen(copyright)+1);
strcpy(version_string, copyright);
strcpy(version_string, this_version);

strcpy(today, __DATE__);
if (today[4] == ' ') i = 1;
today[3] = today[6] = '-';

version_date = malloc(20);
strcpy(version_date, "(");
strncat(version_date, today+4+i, 3-i);
strncat(version_date, today, 4);
strncat(version_date, today+7, 4);
strcat(version_date, ")");
}

/* End of c.eversion */
