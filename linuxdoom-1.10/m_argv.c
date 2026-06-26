// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id:$
//
// Copyright (C) 1993-1996 by id Software, Inc.
//
// This source is available for distribution and/or modification
// only under the terms of the DOOM Source Code License as
// published by id Software. All rights reserved.
//
// The source is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// FITNESS FOR A PARTICULAR PURPOSE. See the DOOM Source Code License
// for more details.
//
// $Log:$
//
// DESCRIPTION:
//
//-----------------------------------------------------------------------------

#include <strings.h>
#include <stdlib.h>

int		myargc;
char**		myargv;




//
// M_CheckParm
// Checks for the given parameter
// in the program's command line arguments.
// Returns the argument number (1 to argc-1)
// or 0 if not present
int M_CheckParm (char *check)
{
    int		i;

    for (i = 1;i<myargc;i++)
    {
	if ( !strcasecmp(check, myargv[i]) )
	    return i;
    }

    return 0;
}


char *M_GetParm(char *check) {
    int p = M_CheckParm(check);
    if (!p || p >= myargc - 1) return NULL;
    return myargv[p + 1];
}


char **M_GetParmArgs(char *check, int *numout) {
    int p = M_CheckParm(check);
    if (!p) return NULL;
    *numout = 0;
    for (int count = 1; p + count < myargc; count++) {
        if (myargv[p + count][0] == '-') break;
        *numout = count;
    }
    return *numout > 0 ? &myargv[p + 1] : &myargv[p];
}

// zero ambiguous
int M_GetParmInt(char *check) {
    char *str = M_GetParm(check);
    return str ? atoi(str) : 0;
}

int M_GetParmInts(char *check, int *out1, int *out2) {
    int count;
    char **strings = M_GetParmArgs(check, &count);
    if (!strings) return 0;
    if (out1) *out1 = 0;
    if (out2) *out2 = 0;
    if (out1 && count >= 1) *out1 = atoi(strings[0]);
    if (out2 && count >= 2) *out2 = atoi(strings[1]);
    return count;
}
