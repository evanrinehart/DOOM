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

#define _POSIX_C_SOURCE 199309L

#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include <stdarg.h>
#include <unistd.h>
#include <errno.h>

#include <sys/stat.h>

#include "doomdef.h"
#include "m_misc.h"
#include "i_video.h"
#include "i_sound.h"

#include "d_net.h"
#include "g_game.h"

#ifdef __GNUG__
#pragma implementation "i_system.h"
#endif
#include "i_system.h"

extern boolean verbose;

void I_Nanosleep(long sec, long nsec) {
    struct timespec ts = {sec, nsec};
    nanosleep(&ts, NULL);
}

void I_Sleep(float delta_t) {
    long sec = floor(delta_t);
    long nsec = (delta_t - sec) * 1e9;
    I_Nanosleep(sec, nsec);
}



int	mb_used = 16;


ticcmd_t	emptycmd;
ticcmd_t*	I_BaseTiccmd(void)
{
    return &emptycmd;
}


int  I_GetHeapSize (void)
{
    return mb_used*1024*1024;
}

byte* I_ZoneBase (int*	size)
{
    *size = mb_used*1024*1024;
    return (byte *) malloc (*size);
}


static const long BILLION = 1000L*1000L*1000L;
static struct timespec timebase;
static bool timebase_set = false;

// Return time in 1/35 second ticks with subtick precision
double I_GetMonotime(void) {

    if (!timebase_set) {
        int e = clock_gettime(CLOCK_MONOTONIC, &timebase);
        if (e < 0) I_Error("clock_gettime: %s\n", strerror(errno));
        timebase_set = true;
    }

    struct timespec now;
    int e = clock_gettime(CLOCK_MONOTONIC, &now);
    if (e < 0) I_Error("clock_gettime: %s\n", strerror(errno));

    double t0 = timebase.tv_sec + (double)timebase.tv_nsec / BILLION;
    double t1 = now.tv_sec + (double)now.tv_nsec / BILLION;
    double diff = t1 - t0;
    return TICRATE * diff;

}

long I_GetMonotimeLong(float *frac) {

    double t = I_GetMonotime();

    if (frac == NULL) {
        return (long)t;
    }
    else {
        double dwhole;
        *frac = modf(t, &dwhole);
        return (long)dwhole;
    }

}



//
// I_GetTime
// returns time in 1/35th second tics
//
int  I_GetTime (void)
{
    return I_GetMonotimeLong(NULL);
}



//
// I_Init
//
void I_Init (void)
{
    I_InitSound();
    I_InitMusic();
}

//
// I_Quit
//
void I_Quit (void)
{
    D_QuitNetGame ();
    I_ShutdownGraphics();
    I_ShutdownMusic();
    I_ShutdownSound();
    M_SaveDefaults ();
    exit(0);
}


void I_WaitVBL(int count)
{
#ifdef SGI
    sginap(1);                                           
#else
#ifdef SUN
    sleep(0);
#else
    if (verbose) printf("WaitVBL(%d)\n", count);
    I_Sleep((float)count / 70);
#endif
#endif
}

void I_BeginRead(void)
{
}

void I_EndRead(void)
{
}

byte*	I_AllocLow(int length)
{
    byte*	mem;
        
    mem = (byte *)malloc (length);
    memset (mem,0,length);
    return mem;
}


//
// I_Error
//
extern boolean demorecording;

_Noreturn void I_Error (char *error, ...)
{
    va_list	argptr;

    // Message first.
    va_start (argptr,error);
    fprintf (stderr, "Error: ");
    vfprintf (stderr,error,argptr);
    fprintf (stderr, "\n");
    va_end (argptr);

    fflush( stderr );

    // Shutdown. Here might be other errors.
    if (demorecording)
	G_CheckDemoStatus();

    D_QuitNetGame ();
    I_ShutdownGraphics();
    
    exit(-1);
}


char *GetDoomWadDir() {
#ifdef NORMALUNIX
    char *dir = getenv("DOOMWADDIR");
    if (!dir) return ".";
    return dir;
#else
    return ".";
#endif
}

char *GetHomeDir() {
    char *home = getenv("HOME");
    if (!home) I_Error("Please set $HOME to your home directory");
    return home;
}

#define APP_NAME "linuxdoom"

char *GetDataHome() {
    char *base = getenv("XDG_DATA_HOME");
    if (base && base[0] == '/') {
        char *path = malloc(strlen(base) + 1 + strlen(APP_NAME) + 1);
        sprintf(path, "%s/%s", base, APP_NAME);
        return path;
    }
    else {
        char *home = GetHomeDir();
        char *localshare = ".local/share";
        char *path = malloc(strlen(home) + 1 + strlen(localshare) + 1 + strlen(APP_NAME) + 1);
        sprintf(path, "%s/%s/%s", home, localshare, APP_NAME);
        return path;
    }
}

char *GetDataPath(const char *dirname, const char *filename) {
    char *datahome = GetDataHome(); // e.g. HOME/.local/share/linuxdoom
    char *path;
    if (filename) {
        path = malloc(strlen(datahome) + 1 + strlen(dirname) + 1 + strlen(filename) + 1);
        sprintf(path, "%s/%s/%s", datahome, dirname, filename); // DATAHOME/<dirname>/filename
    }
    else {
        path = malloc(strlen(datahome) + 1 + strlen(dirname) + 1);
        sprintf(path, "%s/%s", datahome, dirname); // DATAHOME/<dirname>
    }
    free(datahome);
    return path;
}

void EstablishDataHomeDir(void) {
    char *datahome = GetDataHome();
    int e = mkdir(datahome, 0700);
    if (e < 0 && errno != EEXIST) I_Error("mkdir %s: %s\n", datahome, strerror(errno));
    free(datahome);
}

void EstablishSavesDir(void) {
    EstablishDataHomeDir();
    char *saves = GetDataPath("saves", NULL);
    int e = mkdir(saves, 0700);
    if (e < 0 && errno != EEXIST) I_Error("mkdir %s: %s\n", saves, strerror(errno));
    free(saves);
}

void EstablishWadsDir(void) {
    EstablishDataHomeDir();
    char *saves = GetDataPath("wads", NULL);
    int e = mkdir(saves, 0700);
    if (e < 0 && errno != EEXIST) I_Error("mkdir %s: %s\n", saves, strerror(errno));
    free(saves);
}
