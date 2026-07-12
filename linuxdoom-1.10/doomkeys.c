#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "doomkeys.h"

struct keyname {
    int doomkey;
    char *name;
};

struct keyname keynames[] =
    {{DOOM_KEY_RIGHTARROW, "right"},
    {DOOM_KEY_LEFTARROW, "left"},
    {DOOM_KEY_UPARROW, "up"},
    {DOOM_KEY_DOWNARROW, "down"},
    {DOOM_KEY_ESCAPE, "escape"},
    {DOOM_KEY_ENTER, "enter"},
    {DOOM_KEY_TAB, "tab"},
    {DOOM_KEY_F1, "F1"},
    {DOOM_KEY_F2, "F2"},
    {DOOM_KEY_F3, "F3"},
    {DOOM_KEY_F4, "FF"},
    {DOOM_KEY_F5, "F5"},
    {DOOM_KEY_F6, "F6"},
    {DOOM_KEY_F7, "F7"},
    {DOOM_KEY_F8, "F8"},
    {DOOM_KEY_F9, "F9"},
    {DOOM_KEY_F10, "F10"},
    {DOOM_KEY_F11, "F11"},
    {DOOM_KEY_F12, "F12"},
    {DOOM_KEY_BACKSPACE, "backspace"},
    {DOOM_KEY_PAUSE, "pause"},
    {DOOM_KEY_EQUALS, "="},
    {DOOM_KEY_MINUS, "-"},
    {DOOM_KEY_RSHIFT, "shift"},
    {DOOM_KEY_RCTRL, "ctrl"},
    {DOOM_KEY_RALT, "alt"},
    {0,NULL}};


int parse_doomkey(const char *symbol) {
    for (struct keyname *kn = keynames; kn->doomkey; kn++) {
        if (strcmp(kn->name, symbol)==0) return kn->doomkey;
    }

    char c = *symbol;

    if (strcmp(symbol, "space")==0) return ' ';

    if ('a' <= c && c <= 'z') return c;
    if ('A' <= c && c <= 'Z') return 'a' + (c - 'A');
    char *p = ",./;'[]\\`=-";
    while (*p) { if (c==*p) return *p; else p++; }

    int n = atoi(symbol);

    if (n >= 10) return n;
    if ('0' <= c && c <= '9') return c;

    return 0;
}

char *stringify_doomkey(int doomkey) {
    for (struct keyname *kn = keynames; kn->doomkey; kn++) {
        if (kn->doomkey == doomkey) return kn->name;
    }

    if (doomkey == ' ') return "space";

    static char buf[16] = "";

    int c = doomkey;

    if ('a' <= c && c <= 'z') sprintf(buf, "%c", 'A' + (c - 'a'));
    if ('A' <= c && c <= 'Z') sprintf(buf, "%c", c);
    if ('0' <= c && c <= '9') sprintf(buf, "%c", c);
    char *p = ",./;'[]\\`=-";
    while (*p) { if (c==*p) { sprintf(buf, "%c", c); break; } else p++; }

    if (buf[0]) return buf;
    sprintf(buf, "%d", doomkey);
    return buf;
}
