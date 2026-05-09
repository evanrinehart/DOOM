#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>

#include "raylib.h"

#include "common.h"
#include "doomkeys.h"

#include "i_system.h"
#include "m_argv.h"
#include "d_main.h"

extern byte *screens[5];

static Color current_palette[256];
static Texture screen_tex;
static Image screen_img;

static int window_w;
static int window_h;
static int offset_x;

static long frame_num = 0;
static long tic = 0;

void I_ShutdownGraphics(void) {
    // called from I_Quit also I_Error in i_system.c

    UnloadTexture(screen_tex);
    UnloadImage(screen_img);
    CloseWindow();
}

void I_StartFrame (void) {
    // called from d_main

    if (WindowShouldClose()) I_Quit();
}


int translate_key(int rkey) {
    switch (rkey) {
        case KEY_LEFT: return DOOM_KEY_LEFTARROW;
        case KEY_RIGHT: return DOOM_KEY_RIGHTARROW;
        case KEY_UP: return DOOM_KEY_UPARROW;
        case KEY_DOWN: return DOOM_KEY_DOWNARROW;
        case KEY_ESCAPE: return DOOM_KEY_ESCAPE;
        case KEY_ENTER: return DOOM_KEY_ENTER;
        case KEY_TAB: return DOOM_KEY_TAB;
        case KEY_F1: return DOOM_KEY_F1;
        case KEY_F2: return DOOM_KEY_F2;
        case KEY_F3: return DOOM_KEY_F3;
        case KEY_F4: return DOOM_KEY_F4;
        case KEY_F5: return DOOM_KEY_F5;
        case KEY_F6: return DOOM_KEY_F6;
        case KEY_F7: return DOOM_KEY_F7;
        case KEY_F8: return DOOM_KEY_F8;
        case KEY_F9: return DOOM_KEY_F9;
        case KEY_F10: return DOOM_KEY_F10;
        case KEY_F11: return DOOM_KEY_F11;
        case KEY_F12: return DOOM_KEY_F12;
        case KEY_BACKSPACE: return DOOM_KEY_BACKSPACE;
        case KEY_DELETE: return DOOM_KEY_BACKSPACE;
        case KEY_PAUSE: return DOOM_KEY_PAUSE;
        case KEY_EQUAL: return DOOM_KEY_EQUALS;
        case KEY_KP_EQUAL: return DOOM_KEY_EQUALS;
        case KEY_MINUS: return DOOM_KEY_MINUS;
        case KEY_KP_SUBTRACT: return DOOM_KEY_MINUS;
        case KEY_LEFT_SHIFT: return DOOM_KEY_RSHIFT;
        case KEY_RIGHT_SHIFT: return DOOM_KEY_RSHIFT;
        case KEY_LEFT_CONTROL: return DOOM_KEY_RCTRL;
        case KEY_RIGHT_CONTROL: return DOOM_KEY_RCTRL;
        case KEY_LEFT_ALT: return DOOM_KEY_RALT;
        case KEY_RIGHT_ALT: return DOOM_KEY_RALT;
        default:
            if ('A' <= rkey && rkey <= 'Z') return rkey - 'A' + 'a';
            if (KEY_SPACE <= rkey && rkey <= KEY_GRAVE) return rkey;
            return 0;
    }
}

static bool down_keys[500];

void I_GetEvent(void) {
    // was called from I_StartTic
    // get events and post them with D_PostEvent
    // also updated mousemoved

    event_t ev;

    for (int key = GetKeyPressed(); key; key = GetKeyPressed()) {
        ev.type = ev_keydown;
        ev.data1 = translate_key(key);
        D_PostEvent(&ev);
        down_keys[key] = 1;
    }

    for (int k = 0; k < 500; k++) {
        if (down_keys[k] && IsKeyReleased(k)) {
                ev.type = ev_keyup;
                ev.data1 = translate_key(k);
                D_PostEvent(&ev);
                down_keys[k] = 0;
        }
    }

    Vector2 delta = GetMouseDelta();
    int motion = delta.x || delta.y;
    int button =
        IsMouseButtonPressed(0) ||
        IsMouseButtonPressed(1) ||
        IsMouseButtonPressed(2) ||
        IsMouseButtonReleased(0) ||
        IsMouseButtonReleased(1) ||
        IsMouseButtonReleased(2);

    if (button || motion) {
        ev.type = ev_mouse;
        ev.data1 = IsMouseButtonDown(0) | (IsMouseButtonDown(1) ? 2 : 0) | (IsMouseButtonDown(2) ? 4 : 0);
        ev.data2 = delta.x;
        ev.data3 = -delta.y;
        D_PostEvent(&ev);
    }

}

void I_StartTic (void) {
    // called from d_net, d_main
    // calls I_GetEvent repeatedly

    I_GetEvent();
}

void I_UpdateNoBlit (void) {
    // called from d_main
}

void I_FinishUpdate (void) {
    // called from d_main
    // copied image data from screens[0] into some shm image and synced
    // optionally expanding the picture 2x 3x ...

    Color *writing = screen_img.data;
    for (int i = 0; i < SCREENWIDTH*SCREENHEIGHT; i++) {
        *writing++ = current_palette[screens[0][i]];
    }

    BeginDrawing();

    ClearBackground(BLACK);
    UpdateTexture(screen_tex, screen_img.data);

    Rectangle src = {0, 0, screen_tex.width, screen_tex.height};
    Rectangle dst = {offset_x, 0, window_w, window_h};
    Vector2 zero = {0,0};
    DrawTexturePro(screen_tex, src, dst, zero, 0.0f, WHITE);

    EndDrawing();

    frame_num++;
}

void I_ReadScreen (byte* scr) {
    // called from f_wipe to read the screen
    memcpy (scr, screens[0], SCREENWIDTH*SCREENHEIGHT);
}

void I_SetPalette (byte* palette) {
    // called from m_menu, d_main, st_stuff
    // to set the palette

    for (int i = 0; i < 256; i++) {
        current_palette[i].r = palette[3*i + 0];
        current_palette[i].g = palette[3*i + 1];
        current_palette[i].b = palette[3*i + 2];
        current_palette[i].a = 255;
    }

}

void I_InitGraphics(void) {
    // called from d_main

    // set up video backend and possibly override screen[0]
    // which is where rendering ultimately writes to
    // before FinishUpdate presents it, whatever is in screen[0]

    InitWindow(320, 240, "DOOM");

    if (M_CheckParm("-fullscreen")) {
        int monitor = GetCurrentMonitor();
        window_h = GetMonitorHeight(monitor);
        window_w = window_h * 4 / 3;
        offset_x = GetMonitorWidth(monitor)/2 - window_w/2;
        SetWindowSize(window_w, window_h);
        ToggleBorderlessWindowed();
    }
    else {
        int multiply = 1;

        if (M_CheckParm("-2")) multiply = 2;
        if (M_CheckParm("-3")) multiply = 3;
        if (M_CheckParm("-4")) multiply = 4;

        window_w = 320 * multiply;
        window_h = 240 * multiply;
        offset_x = 0;

        SetWindowSize(window_w, window_h);
    }

    screen_img = GenImageColor(SCREENWIDTH, SCREENHEIGHT, GREEN);
    screen_tex = LoadTextureFromImage(screen_img);

    SetExitKey(KEY_HOME);

    DisableCursor();

}
