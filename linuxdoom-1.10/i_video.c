#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>

#include "doomstat.h"
#include "i_system.h"
#include "v_video.h" // extern byte *screens[5]
#include "m_argv.h"
#include "d_main.h"
#include "doomdef.h" // SCREENWIDTH 320 SCREENHEIGHT 200

void I_ShutdownGraphics(void) {
    // called from I_Quit also I_Error in i_system.c
}

void I_StartFrame (void) {
    // called from d_main
}

void I_GetEvent(void) {
    // was called from I_StartTic
    // get events and post them with D_PostEvent
    // also updated mousemoved
}

void I_StartTic (void) {
    // called from d_net, d_main
    // calls I_GetEvent repeatedly
}

void I_UpdateNoBlit (void) {
    // called from d_main
}

void I_FinishUpdate (void) {
    // called from d_main
    // copied image data from screens[0] into some shm image and synced
    // optionally expanding the picture 2x 3x ...
}

void I_ReadScreen (byte* scr) {
    // called from f_wipe to read the screen
    memcpy (scr, screens[0], SCREENWIDTH*SCREENHEIGHT);
}

void I_SetPalette (byte* palette) {
    // called from m_menu, d_main, st_stuff
    // to set the palette
}

void I_InitGraphics(void) {
    // called from d_main

    // set up video backend and possibly override screen[0]
    // which is where rendering ultimately writes to
    // before FinishUpdate presents it, whatever is in screen[0]

    /*
    if (multiply == 1)
	screens[0] = (unsigned char *) (image->data);
    else
	screens[0] = (unsigned char *) malloc (SCREENWIDTH * SCREENHEIGHT);
    */
}
