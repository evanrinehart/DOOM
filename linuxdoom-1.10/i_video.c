#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>

#include <unistd.h>

#include "raylib.h"

#include "common.h"
#include "doomkeys.h"

#include "i_system.h"
#include "m_argv.h"
#include "d_main.h"

static bool video_initialized = false;

extern byte *screens[5];

static Color current_palette[256];
static Texture screen_tex;
static Image screen_img;

static int window_w;
static int window_h;
static int offset_x;

static long frame_num = 0;

static bool mouse_captured = 0;

void I_ShutdownGraphics(void) {
    if (!video_initialized) return;

    // called from I_Quit also I_Error in i_system.c

    UnloadTexture(screen_tex);
    UnloadImage(screen_img);
    CloseWindow();
}

void I_StartFrame (void) {
    // called from d_main

    if (WindowShouldClose()) I_Quit();
}

struct joystick_state {
    int axis1;
    int axis2;
    int buttons;
};

struct joystick_state poll_joystick() {
    struct joystick_state js = {0,0,0};
    if (!IsGamepadAvailable(0)) return js;
    float a1 = GetGamepadAxisMovement(0, 0);
    js.axis1 = a1 < -0.1 ? -1 : (a1 > 0.1 ? 1 : 0);
    float a2 = GetGamepadAxisMovement(0, 1);
    js.axis2 = a2 < -0.1 ? -1 : (a2 > 0.1 ? 1 : 0);
    js.buttons |= IsGamepadButtonDown(0, GAMEPAD_BUTTON_RIGHT_FACE_LEFT) ? 1 : 0;
    js.buttons |= IsGamepadButtonDown(0, GAMEPAD_BUTTON_RIGHT_FACE_DOWN) ? 2 : 0;
    js.buttons |= IsGamepadButtonDown(0, GAMEPAD_BUTTON_RIGHT_FACE_UP) ? 4 : 0;
    js.buttons |= IsGamepadButtonDown(0, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT) ? 8 : 0;
    js.buttons |= IsGamepadButtonDown(0, GAMEPAD_BUTTON_LEFT_TRIGGER_1) ? 16 : 0;
    js.buttons |= IsGamepadButtonDown(0, GAMEPAD_BUTTON_RIGHT_TRIGGER_1) ? 32 : 0;
    return js;
}

struct joystick_state joystate = {0,0,0};

struct single_key {
    bool state;
    int rlkey;
    int doomkey;
};

struct single_key keyboard[256];
int num_single_keys = 0;

struct multi_key {
    bool state;
    int rlkey[2];
    int doomkey;
};

struct multi_key mkctrl  = {0, {KEY_LEFT_CONTROL, KEY_RIGHT_CONTROL}, DOOM_KEY_RCTRL};
struct multi_key mkshift = {0, {KEY_LEFT_SHIFT,   KEY_RIGHT_SHIFT},   DOOM_KEY_RSHIFT};
struct multi_key mkalt   = {0, {KEY_LEFT_ALT,     KEY_RIGHT_ALT},     DOOM_KEY_RALT};
struct multi_key mksub   = {0, {KEY_MINUS,        KEY_KP_SUBTRACT},   DOOM_KEY_MINUS};
struct multi_key mkdel   = {0, {KEY_BACKSPACE,    KEY_DELETE},        DOOM_KEY_BACKSPACE};
struct multi_key mkequal = {0, {KEY_EQUAL,        KEY_KP_EQUAL},      DOOM_KEY_EQUALS};

void add_key(int rlkey, int doomkey) {
    if (num_single_keys >= 256) I_Error("I_InitVideo: too many keyboard keys\n");
    struct single_key *sk = &keyboard[num_single_keys++];
    sk->state = 0;
    sk->rlkey = rlkey;
    sk->doomkey = doomkey;
}

void poll_single_key(bool *state, int rlkey, int doomkey) {
    event_t ev;
    if (IsKeyDown(rlkey) && !*state) {
        *state = true;
        ev.type = ev_keydown;
        ev.data1 = doomkey;
        D_PostEvent(&ev);
    }
    else if (!IsKeyDown(rlkey) && *state) {
        *state = false;
        ev.type = ev_keyup;
        ev.data1 = doomkey;
        D_PostEvent(&ev);
    }
}

void poll_multi_key(struct multi_key *mk) {
    event_t ev;
    if (!mk->state && (IsKeyDown(mk->rlkey[0]) || IsKeyDown(mk->rlkey[1]))) {
        mk->state = true;
        ev.type = ev_keydown;
        ev.data1 = mk->doomkey;
        D_PostEvent(&ev);
    }
    else if (mk->state && !IsKeyDown(mk->rlkey[0]) && !IsKeyDown(mk->rlkey[1])) {
        mk->state = false;
        ev.type = ev_keyup;
        ev.data1 = mk->doomkey;
        D_PostEvent(&ev);
    }
}

void I_GetEvent(void) {
    // was called from I_StartTic
    // get events and post them with D_PostEvent
    // also updated mousemoved

    event_t ev;

    Vector2 delta = GetMouseDelta();
    int motion = delta.x || delta.y;
    int button =
        IsMouseButtonPressed(0) ||
        IsMouseButtonPressed(1) ||
        IsMouseButtonPressed(2) ||
        IsMouseButtonReleased(0) ||
        IsMouseButtonReleased(1) ||
        IsMouseButtonReleased(2);

    if (mouse_captured && (button || motion)) {
        ev.type = ev_mouse;
        ev.data1 = IsMouseButtonDown(0) | (IsMouseButtonDown(1) ? 2 : 0) | (IsMouseButtonDown(2) ? 4 : 0);
        ev.data2 = (int)delta.x;
        ev.data3 = (int)-delta.y;
        D_PostEvent(&ev);
    }

    struct joystick_state jcurrent = poll_joystick();
    int jchange =
        jcurrent.axis1 != joystate.axis1 ||
        jcurrent.axis2 != joystate.axis2 ||
        jcurrent.buttons != joystate.buttons;
    if(jchange) {
        joystate = jcurrent;
        ev.type = ev_joystick;
        ev.data3 = joystate.axis2;
        ev.data2 = joystate.axis1;
        ev.data1 = joystate.buttons;
        D_PostEvent(&ev);
    }

    for (int i = 0; i < num_single_keys; i++) {
        struct single_key *sk = &keyboard[i];
        poll_single_key(&sk->state, sk->rlkey, sk->doomkey);
    }

    poll_multi_key(&mkctrl);
    poll_multi_key(&mkshift);
    poll_multi_key(&mkalt);
    poll_multi_key(&mksub);
    poll_multi_key(&mkdel);
    poll_multi_key(&mkequal);

    if (IsKeyDown(KEY_GRAVE)) { EnableCursor(); mouse_captured = false; }
    if (IsMouseButtonDown(1)) { DisableCursor(); mouse_captured = true; }

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

    //DrawFPS(1,1);

    EndDrawing();

    I_Sleep(0.025);

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

    if (!IsWindowReady()) {
        I_Error("I_InitGraphics: InitWindow failed\n");
    }

    video_initialized = true;

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
        int monitor = GetCurrentMonitor();
        int monitor_w = GetMonitorWidth(monitor);
        int monitor_h = GetMonitorHeight(monitor);

        if (M_CheckParm("-1")) multiply = 1;
        else if (M_CheckParm("-2")) multiply = 2;
        else if (M_CheckParm("-3")) multiply = 3;
        else if (M_CheckParm("-4")) multiply = 4;
        else if (M_CheckParm("-5")) multiply = 5;
        else if (M_CheckParm("-10")) multiply = 10;
        else if (monitor_h >= 2400) multiply = 10;
        else if (monitor_h >= 1200) multiply = 5;
        else if (monitor_h >= 960) multiply = 4;
        else if (monitor_h >= 720) multiply = 3;
        else if (monitor_h >= 480) multiply = 2;

        window_w = 320 * multiply;
        window_h = 240 * multiply;
        offset_x = 0;

        SetWindowPosition(monitor_w/2 - window_w/2, monitor_h/2 - window_h/2);
        SetWindowSize(window_w, window_h);
    }

    screen_img = GenImageColor(SCREENWIDTH, SCREENHEIGHT, GREEN);
    screen_tex = LoadTextureFromImage(screen_img);

    SetExitKey(KEY_HOME);

    add_key(KEY_SPACE, ' ');

    add_key('\'', '\'');
    add_key(',', ',');
    //add_key('-', '-');
    add_key('.', '.');
    add_key('/', '/');
    for (int k = '0'; k <= '9'; k++) add_key(k, k);
    add_key(';', ';');
    //add_key('=', '=');
    for (int k = 'A'; k <= 'Z'; k++) add_key(k, k - 'A' + 'a');
    add_key('[', '[');
    add_key('\\', '\\');
    add_key(']', ']');
    add_key('`', '`');
    add_key(KEY_ESCAPE, DOOM_KEY_ESCAPE);
    add_key(KEY_ENTER, DOOM_KEY_ENTER);
    add_key(KEY_TAB, DOOM_KEY_TAB);
    //add_key(KEY_BACKSPACE, DOOM_KEY_BACKSPACE);
    add_key(KEY_PAUSE, DOOM_KEY_PAUSE);
    add_key(KEY_LEFT, DOOM_KEY_LEFTARROW);
    add_key(KEY_RIGHT, DOOM_KEY_RIGHTARROW);
    add_key(KEY_DOWN, DOOM_KEY_DOWNARROW);
    add_key(KEY_UP, DOOM_KEY_UPARROW);
    for (int i = 0; i < 12; i++) add_key(KEY_F1 + i, DOOM_KEY_F1 + i);

    // two obscure controllers I have
    SetGamepadMappings("03000000790000004e95000011010000,DragonRise Inc. NGC USB Gamepad,a:b1,b:b0,dpdown:b14,dpleft:b15,dpright:b13,dpup:b12,leftshoulder:b4,lefttrigger:a3,leftx:a0,lefty:a1~,rightshoulder:b5,righttrigger:a4,rightx:a5,righty:a2~,start:b9,x:b2,y:b3,platform:Linux,");
    SetGamepadMappings("03000000790000001100000010010000,!NNEXT Gamepad,a:b2,b:b1,x:b3,y:b0,leftshoulder:b4,rightshoulder:b5,back:b8,start:b9,leftx:a0,lefty:a1,dpup:-a1,dpdown:+a1,dpleft:-a0,dpright:+a0");

}
