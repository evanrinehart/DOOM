#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "i_system.h"
#include "w_wad.h"

struct mapinfo {
    char music[16];
};

static struct mapinfo mapinfo_doom1[4][9];
static struct mapinfo mapinfo_doom2[32];

void populate_doom2_music(void);
char *episode4_fallback(int map);

void X_InitMapinfo() {
    for (int e = 1; e <= 4; e++) {
        for (int m = 1; m <= 9; m++) {
            struct mapinfo *info = &mapinfo_doom1[e-1][m-1];
            sprintf(info->music, "D_E%dM%d", e, m);

            if (e==4 && W_CheckNumForName(info->music) < 0) {
                strcpy(info->music, episode4_fallback(m));
            }
        }
    }

    populate_doom2_music();
}

char *X_GetMapSong(int episode, int map, int doom) {
    if (doom==1 && (episode < 1 || episode > 4)) return NULL;
    if (doom==1 && (map < 1 || map > 9)) return NULL;
    if (doom==2 && (map < 1 || map > 32)) return NULL;

    switch (doom) {
        case 1: return mapinfo_doom1[episode-1][map-1].music;
        case 2: return mapinfo_doom2[map-1].music;
        case 3: I_Error("doom 3 not supported");
        default: I_Error("X_GetMapSong: bad doom (%d)", doom);
    }
}

void X_SetMapSong(int episode, int map, int doom, char *name) {
    switch (doom) {
        case 1: strcpy(mapinfo_doom1[episode][map].music, name); break;
        case 2: strcpy(mapinfo_doom2[map].music, name); break;
        case 3: I_Error("doom 3 not supported");
        default: I_Error("X_GetMapSong: bad doom (%d)", doom);
    }
}

void populate_doom2_music(void) {
    strcpy(mapinfo_doom2[1 - 1].music, "D_RUNNIN");
    strcpy(mapinfo_doom2[2 - 1].music, "D_STALKS");
    strcpy(mapinfo_doom2[3 - 1].music, "D_COUNTD");
    strcpy(mapinfo_doom2[4 - 1].music, "D_BETWEE");

    strcpy(mapinfo_doom2[5 - 1].music, "D_DOOM");
    strcpy(mapinfo_doom2[6 - 1].music, "D_THE_DA");
    strcpy(mapinfo_doom2[7 - 1].music, "D_SHAWN");
    strcpy(mapinfo_doom2[8 - 1].music, "D_DDTBLU");

    strcpy(mapinfo_doom2[9 - 1].music, "D_IN_CIT");
    strcpy(mapinfo_doom2[10 - 1].music, "D_DEAD");
    strcpy(mapinfo_doom2[11 - 1].music, "D_STLKS2"); // starts to repeat, but note alternate midis
    strcpy(mapinfo_doom2[12 - 1].music, "D_THEDA2");

    strcpy(mapinfo_doom2[13 - 1].music, "D_DOOM2");
    strcpy(mapinfo_doom2[14 - 1].music, "D_DDTBL2");
    strcpy(mapinfo_doom2[15 - 1].music, "D_RUNNI2");
    strcpy(mapinfo_doom2[16 - 1].music, "D_DEAD2");

    strcpy(mapinfo_doom2[17 - 1].music, "D_STLKS3");
    strcpy(mapinfo_doom2[18 - 1].music, "D_ROMERO");
    strcpy(mapinfo_doom2[19 - 1].music, "D_SHAWN2");
    strcpy(mapinfo_doom2[20 - 1].music, "D_MESSAG");

    strcpy(mapinfo_doom2[21 - 1].music, "D_COUNT2");
    strcpy(mapinfo_doom2[22 - 1].music, "D_DDTBL3");
    strcpy(mapinfo_doom2[23 - 1].music, "D_AMPIE");
    strcpy(mapinfo_doom2[24 - 1].music, "D_THEDA3");

    strcpy(mapinfo_doom2[25 - 1].music, "D_ADRIAN");
    strcpy(mapinfo_doom2[26 - 1].music, "D_MESSG2");
    strcpy(mapinfo_doom2[27 - 1].music, "D_ROMER2");
    strcpy(mapinfo_doom2[28 - 1].music, "D_TENSE");

    strcpy(mapinfo_doom2[29 - 1].music, "D_SHAWN3");
    strcpy(mapinfo_doom2[30 - 1].music, "D_OPENIN");
    strcpy(mapinfo_doom2[31 - 1].music, "D_EVIL");
    strcpy(mapinfo_doom2[32 - 1].music, "D_ULTIMA");
}

char *episode4_fallback(int map) {
    switch (map) {
        // Song - Who? - Where?
        case 1: return "D_E3M4"; // American	e4m1
        case 2: return "D_E3M2"; // Romero	e4m2
        case 3: return "D_E3M3"; // Shawn	e4m3
        case 4: return "D_E1M5"; // American	e4m4
        case 5: return "D_E2M7"; // Tim 	e4m5
        case 6: return "D_E2M4"; // Romero	e4m6
        case 7: return "D_E2M6"; // J.Anderson	e4m7 CHIRON.WAD
        case 8: return "D_E2M5"; // Shawn	e4m8
        case 9: return "D_E1M9"; // Tim 	e4m9
        default: return "D_E1M1";
    }
}



void X_LoadMapinfo(char *data, int len) {
    char *str = malloc(len + 1);
    memcpy(str, data, len);
    str[len-1] = 0;
    printf("%s\n", str);
    free(str);
}
