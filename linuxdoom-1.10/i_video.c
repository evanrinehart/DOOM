/*
 *  Platform specific window and graphics backend
 */

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

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

#include "hooks.h"
#include "netscope.h"

#define MIN(A,B) ((A) < (B) ? (A) : (B))
#define MAX(A,B) ((A) > (B) ? (A) : (B))

void (*X_AlternateRender)(struct canvas, struct viewpoint) = NULL;

extern struct netstatus *netstatus;
void render_netstatus(struct netstatus *s);

extern boolean verbose;

static bool video_initialized = false;

extern byte *screens[5];
extern byte gammatable[5][256];
extern int usegamma;

static Color current_palette[256];
static Texture screen_tex;
static Image screen_img;

static int window_w;
static int window_h;
static int offset_x;

static long frame_num = 0;

static bool fullscreen = 0;
static bool window_focused = 1;
static bool mouse_affinity = 0; // capture the mouse if you have the chance
static bool mouse_residual = 0; // mouse was captured at some point
static bool mouse_captured = 0;

// updated at a distance by m_misc.c "defaults"
bool mouse_walk = true;
bool show_debug = true;

void I_ReleaseMouse(void) {
    if (!mouse_captured) return;
    EnableCursor();
    mouse_captured = false;
}

void I_CaptureMouse(void) {
    if (mouse_captured) return;
    DisableCursor();
    mouse_captured = true;
    mouse_residual = true;
}

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

const char *gamepads[8];
int current_gamepad = 0;

struct joystick_state poll_joystick() {
    struct joystick_state js = {0,0,0};
    if (!IsGamepadAvailable(current_gamepad)) return js;
    float a1 = GetGamepadAxisMovement(current_gamepad, 0);
    js.axis1 = a1 < -0.1 ? -1 : (a1 > 0.1 ? 1 : 0);
    float a2 = GetGamepadAxisMovement(current_gamepad, 1);
    js.axis2 = a2 < -0.1 ? -1 : (a2 > 0.1 ? 1 : 0);
    js.buttons |= IsGamepadButtonDown(current_gamepad, GAMEPAD_BUTTON_RIGHT_FACE_LEFT) ? 1 : 0;
    js.buttons |= IsGamepadButtonDown(current_gamepad, GAMEPAD_BUTTON_RIGHT_FACE_DOWN) ? 2 : 0;
    js.buttons |= IsGamepadButtonDown(current_gamepad, GAMEPAD_BUTTON_RIGHT_FACE_UP) ? 4 : 0;
    js.buttons |= IsGamepadButtonDown(current_gamepad, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT) ? 8 : 0;
    js.buttons |= IsGamepadButtonDown(current_gamepad, GAMEPAD_BUTTON_LEFT_TRIGGER_1) ? 16 : 0;
    js.buttons |= IsGamepadButtonDown(current_gamepad, GAMEPAD_BUTTON_RIGHT_TRIGGER_1) ? 32 : 0;
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
    if (rlkey == KEY_GRAVE) return;

    event_t ev = {0};

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

    event_t ev = {0};

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

void DumpGamepads(void) {
    for (int i = 0; i < 8; i++) {
        if (IsGamepadAvailable(i)) {
            printf("GAMEPAD %d = \"%s\"\n", i, GetGamepadName(i));
        }
        else {
            printf("GAMEPAD %d = not there\n", i);
        }
    }
    printf("\n");
}

void I_GetEvent(void) {
    // was called from I_StartTic
    // get events and post them with D_PostEvent
    // also updated mousemoved

    PollInputEvents();

    for (int i = 0; i < 8; i++) {
        if (IsGamepadAvailable(i) && gamepads[i]==NULL) {
            printf("NOTICE gamepad %d = \"%s\" appeared!\n", i, GetGamepadName(i));
            gamepads[i] = GetGamepadName(i);
            DumpGamepads();
            current_gamepad = i;
        }
        else if (!IsGamepadAvailable(i) && gamepads[i]) {
            printf("NOTICE gamepad %d is gone!\n", i);
            gamepads[i] = NULL;
            DumpGamepads();
            if (i == current_gamepad) current_gamepad = 0;
        }
    }

    event_t ev = {0};

    // chat char input event
    for (;;) {
        int codepoint = GetCharPressed();
        if (!codepoint) break;

        int size;
        const char *src = CodepointToUTF8(codepoint, &size);

        ev.type = ev_character;
        ev.data1 = codepoint;
        ev.data2 = size;
        memcpy(ev.data4, src, size);

        D_PostEvent(&ev);
    }


    Vector2 mouse = {GetMouseX(), GetMouseY()};
    Vector2 delta = GetMouseDelta();
    if (!mouse_captured) {
        delta.x = 0.0f;
        delta.y = 0.0f;
    }
    int motion = delta.x || delta.y;
    int mousepress = IsMouseButtonPressed(0) || IsMouseButtonPressed(1) || IsMouseButtonPressed(2);
    int mouserelease = IsMouseButtonReleased(0) || IsMouseButtonReleased(1) || IsMouseButtonReleased(2);
    int mousebutton = mousepress || mouserelease;

    Rectangle winrect = {0, 0, window_w, window_h};
    if (!mouse_captured && mousepress && CheckCollisionPointRec(mouse, winrect)) I_CaptureMouse();

    if (mouse_captured && (mousebutton || motion)) {
        ev.type = ev_mouse;
        ev.data1 = IsMouseButtonDown(0) | (IsMouseButtonDown(1) ? 2 : 0) | (IsMouseButtonDown(2) ? 4 : 0);
        ev.data2 = (int)delta.x;
        ev.data3 = mouse_walk ? (int)-delta.y : 0;
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

    if (IsWindowFocused() && !window_focused) {
        window_focused = 1;
        if (mouse_affinity) I_CaptureMouse();
    }

    if (!IsWindowFocused() && window_focused) {
        window_focused = 0;
        I_ReleaseMouse();
    }

    if (IsKeyPressed(KEY_GRAVE)) show_debug = !show_debug;

}

void I_StartTic (void) {
    // called from d_net, d_main
    // calls I_GetEvent repeatedly

    if (video_initialized) I_GetEvent();
}

void I_PumpEvents(void) {
    if (video_initialized) I_GetEvent();
}

void I_UpdateNoBlit (void) {
    // called from d_main
}

extern boolean noblit;

double frame_times[8];
double fps;

void I_FinishUpdate (void) {
    // called from d_main
    // copied image data from screens[0] into some shm image and synced
    // optionally expanding the picture 2x 3x ...

    if (noblit) return;

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

    if (show_debug) {
        render_netstatus(netstatus);
        DrawText(TextFormat("%d FPS", (int)fps), window_w - 50, 1, 10, MAGENTA);
    }

    EndDrawing();

    SwapScreenBuffer();

    frame_num++;
    frame_times[frame_num % 8] = I_GetMonotime() / 35.0;
    double min_time = INFINITY; for (int i = 0; i < 8; i++) min_time = MIN(min_time, frame_times[i]);
    double max_time = -INFINITY; for (int i = 0; i < 8; i++) max_time = MAX(max_time, frame_times[i]);
    fps = 7.0 / (max_time - min_time);
}

void I_ReadScreen (byte* scr) {
    // called from f_wipe to read the screen
    memcpy (scr, screens[0], SCREENWIDTH*SCREENHEIGHT);
}

void I_SetPalette (byte* palette) {
    // called from m_menu, d_main, st_stuff
    // to set the palette

    for (int i = 0; i < 256; i++) {
        current_palette[i].r = gammatable[usegamma][palette[3*i + 0]];
        current_palette[i].g = gammatable[usegamma][palette[3*i + 1]];
        current_palette[i].b = gammatable[usegamma][palette[3*i + 2]];
        current_palette[i].a = 255;
    }

}

void null_render(struct canvas cv, struct viewpoint vp) {
    byte *p = cv.screen + cv.top * SCREENWIDTH + cv.left;
    int skip = SCREENWIDTH - cv.width;
    for (int j = 0; j < cv.height; j++) {
        for (int i = 0; i < cv.width; i++) {
            *p++ = 0;
        }
        p += skip;
    }
}

void I_InitGraphics(char *title) {
    // called from d_main

    // set up video backend and possibly override screen[0]
    // which is where rendering ultimately writes to
    // before FinishUpdate presents it, whatever is in screen[0]

    SetTraceLogLevel(verbose ? LOG_INFO : LOG_NONE);

    SetConfigFlags(FLAG_WINDOW_HIGHDPI);
    InitWindow(320, 240, title);

    if (!IsWindowReady())
        I_Error("I_InitGraphics: InitWindow failed\n");

    int multiply = 1;

    if (M_CheckParm("-fullscreen")) {
        int monitor = GetCurrentMonitor();
        window_h = GetMonitorHeight(monitor);
        window_w = window_h * 4 / 3;
        offset_x = GetMonitorWidth(monitor)/2 - window_w/2;
        mouse_affinity = true;
        fullscreen = true;
    }
    else {
        int monitor = GetCurrentMonitor();
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
    }

    // close the window we got and try again, giving WM a chance to position it better
    if (fullscreen || multiply > 1) {

        CloseWindow();
        SetConfigFlags(FLAG_WINDOW_HIGHDPI);
        InitWindow(window_w, window_h, title);

        if (!IsWindowReady())
            I_Error("I_InitGraphics: InitWindow failed (switching to larger window)\n");

        if (fullscreen) {
            ToggleBorderlessWindowed();
            I_CaptureMouse();
        }

    }

    video_initialized = true;

    screen_img = GenImageColor(SCREENWIDTH, SCREENHEIGHT, GREEN);
    screen_tex = LoadTextureFromImage(screen_img);

    SetExitKey(0);

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

    for (int i = 0; i < 10; i++) {
        add_key(KEY_F1 + i, DOOM_KEY_F1 + i);
    }

    add_key(KEY_F11, DOOM_KEY_F11);
    add_key(KEY_F12, DOOM_KEY_F12);

    // two obscure controllers I have
    SetGamepadMappings("03000000790000004e95000011010000,DragonRise Inc. NGC USB Gamepad,a:b1,b:b0,dpdown:b14,dpleft:b15,dpright:b13,dpup:b12,leftshoulder:b4,lefttrigger:a3,leftx:a0,lefty:a1,rightshoulder:b5,righttrigger:a4,rightx:a5,righty:a2,start:b9,x:b2,y:b3,platform:Linux,");
    SetGamepadMappings("03000000790000001100000010010000,!NNEXT Gamepad,a:b2,b:b1,x:b3,y:b0,leftshoulder:b4,rightshoulder:b5,back:b8,start:b9,leftx:a0,lefty:a1,dpup:-a1,dpdown:+a1,dpleft:-a0,dpright:+a0");

    for (int i = 0; i < 8; i++) {
        if (IsGamepadAvailable(i)) gamepads[i] = GetGamepadName(i);
    }

    if (verbose) DumpGamepads();

}

void X_OnEvent(int type, int data1, int data2, int data3, unsigned char data4[4]) {
    //printf("processing event = %d %d %d %d\n", type, data1, data2, data3);
}

void X_AfterSummonMenu(void) {
    if (!fullscreen && mouse_captured) I_ReleaseMouse();
}

void X_AfterUnsummonMenu(void) {
    if (!fullscreen && mouse_residual) I_CaptureMouse();
}


void render_netstatus(struct netstatus *s) {

    int right_side = 28 + 11 + 11 * s->num_nodes;

    for (int i = 0; i < 15; i++) {
        int label = s->maketic - i + 3;
        Color color = label == s->maketic ? GREEN : RAYWHITE;
        DrawText(TextFormat("%d", label % 10000), 2, i*10, 10, color);
    }

    for (int i = 0; i < 15; i++) {
        int label = s->maketic - i + 3;
        int filled = label < s->maketic;
        if (filled)
            DrawRectangle(28, i*10, 10, 8, GOLD);
        else
            DrawRectangleLines(28, i*10, 10, 8, GOLD);
    }

    for (int n = 0; n < s->num_nodes; n++) {
        for (int i = 0; i < 15; i++) {
            int label = s->maketic - i + 3;
            int filled = label < s->nettics[n];
            if (filled)
                DrawRectangle(28 + 11 + 11*n, i*10, 10, 8, SKYBLUE);
            else
                DrawRectangleLines(28 + 11 + 11*n, i*10, 10, 8, SKYBLUE);
        }

        const char *text = TextFormat("%d", 1 + s->players[n]);
        int width = MeasureText(text, 10);
        int adjust = (10 - width) / 2;
        DrawText(text, 28 + 11 + 11*n + adjust, 15*10, 10, RAYWHITE);

        text = TextFormat("%d", n);
        width = MeasureText(text, 10);
        adjust = (10 - width) / 2;
        DrawText(text, 28 + 11 + 11*n + adjust, 16*10, 10, RAYWHITE);
    }

    for (int n = 0; n < s->num_nodes; n++) {
        if (s->botch_start[n] == 0) continue;
        for (int i = 0; i < 15; i++) {
            int label = s->maketic - i + 3;
            int botch_end = s->botch_start[n] + s->botch_num[n] - 1;
            int filled = label >= s->botch_start[n] && label <= botch_end;
            if (filled) DrawRectangle(28 + 11 + 11*n, i*10, 10, 8, RED);
        }
    }

    DrawText("plyr", 2, 15*10, 10, RAYWHITE);
    DrawText("node", 2, 16*10, 10, RAYWHITE);

    int lowtic_line = (s->maketic - s->lowtic + 4)*10 - 1;
    DrawLine(28, lowtic_line, 100, lowtic_line, RED);

    int gametic_line = (s->maketic - s->gametic + 4)*10;
    DrawLine(28, gametic_line, 100, gametic_line, BLUE);

    int brakes_age = (s->maketic - s->recent_brakes + 3);
    if (brakes_age < 40) {
        Color color = ORANGE;
        color.a = 255 - brakes_age*5;
        DrawText("BRAKES", right_side + brakes_age/2, 30 - brakes_age/6, 10, color);
    }

    int nitro_age = (s->maketic - s->recent_nitro + 3);
    if (nitro_age < 40) {
        Color color = SKYBLUE;
        color.a = 255 - nitro_age*5;
        DrawText("NITRO", right_side + nitro_age/2, 40 + nitro_age/6, 10, color);
    }

    int botch_age = (s->maketic - s->recent_botch + 3);
    if (botch_age < 12) {
        Color color = PINK;
        DrawText("OUT OF ORDER!", right_side + 2, 30 + botch_age*10, 10, color);
    }

    if (s->wayahead) {
        DrawText("WAY AHEAD", right_side + 2, 30, 10, ORANGE);
    }
}

void show_netstatus(struct netstatus *s) {
    int N = s->num_nodes;
    printf("netstatus {\n");
    printf("\tgametic = %d\n", s->gametic);
    printf("\tmaketic = %d\n", s->maketic);
    printf("\tlowtic = %d\n", s->lowtic);
    printf("\tnettics[] = {");
        for(int i = 0; i < N; i++) printf("%d ", s->nettics[i]);
        printf("}\n");
    printf("\tplayers[] = {");
        for(int i = 0; i < N; i++) printf("%d ", s->players[i]);
        printf("}\n");
    printf("\tcurrent_time = %g\n", s->current_time);
    printf("\tcontact_time[] = {");
        for(int i = 0; i < N; i++) printf("%g ", s->contact_time[i]);
        printf("}\n");
    printf("\trecent_nitro = %d\n", s->recent_nitro);
    printf("\trecent_brakes = %d\n", s->recent_brakes);
    printf("\tbotch_start[] = {");
        for(int i = 0; i < N; i++) printf("%d ", s->botch_start[i]);
        printf("}\n");
    printf("\tbotch_num[] = {");
        for(int i = 0; i < N; i++) printf("%d ", s->botch_num[i]);
        printf("}\n");
    printf("}\n");
}
