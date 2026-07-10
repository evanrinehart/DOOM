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
//
// $Log:$
//
// DESCRIPTION:
//	Main loop menu stuff.
//	Default Config File.
//	PCX Screenshots.
//
//-----------------------------------------------------------------------------

#define _POSIX_C_SOURCE 200809L

#include <stdint.h>
#include <string.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

#include <ctype.h>


#include "doomdef.h"

#include "z_zone.h"

#include "m_swap.h"
#include "m_argv.h"

#include "w_wad.h"

#include "i_system.h"
#include "i_video.h"
#include "v_video.h"

#include "hu_stuff.h"

// State.
#include "doomstat.h"

// Data.
#include "dstrings.h"

#include "m_misc.h"

//
// M_DrawText
// Returns the final X coordinate
// HU_Init must have been called to init the font
//
extern patch_t*		hu_font[HU_FONTSIZE];

int
M_DrawText
( int		x,
  int		y,
  boolean	direct,
  char*		string )
{
    int 	c;
    int		w;

    while (*string)
    {
	c = toupper(*string) - HU_FONTSTART;
	string++;
	if (c < 0 || c> HU_FONTSIZE)
	{
	    x += 4;
	    continue;
	}
		
	w = SHORT (hu_font[c]->width);
	if (x+w > SCREENWIDTH)
	    break;
	if (direct)
	    V_DrawPatchDirect(x, y, 0, hu_font[c]);
	else
	    V_DrawPatch(x, y, 0, hu_font[c]);
	x+=w;
    }

    return x;
}




//
// M_WriteFile
//
#ifndef O_BINARY
#define O_BINARY 0
#endif

boolean
M_WriteFile
( char const*	name,
  void*		source,
  int		length )
{
    int		handle;
    int		count;
	
    handle = open ( name, O_WRONLY | O_CREAT | O_TRUNC | O_BINARY, 0666);

    if (handle == -1)
	return false;

    count = write (handle, source, length);
    close (handle);
	
    if (count < length)
	return false;
		
    return true;
}


//
// M_ReadFile
//
int
M_ReadFile
( char const*	name,
  byte**	buffer )
{
    int	handle, count, length;
    struct stat	fileinfo;
    byte		*buf;
	
    handle = open (name, O_RDONLY | O_BINARY, 0666);
    if (handle == -1)
	I_Error ("Couldn't read file %s", name);
    if (fstat (handle,&fileinfo) == -1)
	I_Error ("Couldn't read file %s", name);
    length = fileinfo.st_size;
    buf = Z_Malloc (length, PU_STATIC, NULL);
    count = read (handle, buf, length);
    close (handle);
	
    if (count < length)
	I_Error ("Couldn't read file %s", name);
		
    *buffer = buf;
    return length;
}


//
// DEFAULTS
//
int		usemouse;
int		usejoystick;

extern int	key_right;
extern int	key_left;
extern int	key_up;
extern int	key_down;

extern int	key_strafeleft;
extern int	key_straferight;

extern int	key_fire;
extern int	key_use;
extern int	key_strafe;
extern int	key_speed;

extern int	mousebfire;
extern int	mousebstrafe;
extern int	mousebforward;

extern int	joybfire;
extern int	joybstrafe;
extern int	joybuse;
extern int	joybspeed;

extern int	viewwidth;
extern int	viewheight;

extern int	mouseSensitivity;
extern int	showMessages;

extern int	detailLevel;

extern int	screenblocks;

extern int	showMessages;

// machine-independent sound params
extern	int	numChannels;


extern char*	chat_macros[];

extern bool mouse_walk;
extern bool show_debug;
extern bool show_endoom;

struct config_entry {
    char *keyword;
    void *global;
    void (*splat)(FILE *file, char *, /* same type as global */ void *);
};

static struct config_entry config_entries[64];
static size_t numentries;

static char* config_path;

static void splat_int(FILE *file, char *keyword, void *global_int) {
    int *v = global_int;
    fprintf(file, "%-24s %d\n", keyword, *v);
}

static void splat_bool(FILE *file, char *keyword, void *global_bool) {
    bool *v = global_bool;
    fprintf(file, "%-24s %s\n", keyword, *v ? "true" : "false");
}

static void splat_string(FILE *file, char *keyword, void *global_string) {
    char **to_string = global_string;
    char *p = *to_string;
    fprintf(file, "%-24s \"", keyword);
    while (*p) {
        if (*p == '"') fprintf(file, "\\\"");
        else if (*p == '\\') fprintf(file, "\\\\");
        else fprintf(file, "%c", *p);
        p++;
    }
    fprintf(file, "\"");
    fprintf(file, "\n");
}

char *parse_keyword(char *str, char *pat) {
    while (*str==' ' || *str=='\t') str++;
    while (*str == *pat && *pat) { str++; pat++; }
    return *pat || isgraph((unsigned char) *str) ? NULL : str;
}

void LoadConfigInt(FILE *file, char *keyword, int *global_int, int def) {
    rewind(file);
    char line[256];
    int v = def;
    while (fgets(line, 256, file)) {
        char *p = line;
        p = parse_keyword(p, keyword); if (p==NULL) continue;
        v = atoi(p);
        break;
    }
    *global_int = v;
    struct config_entry *ent = &config_entries[numentries++];
    ent->keyword = keyword;
    ent->splat = splat_int;
    ent->global = global_int;
}

void LoadConfigString(FILE *file, char *keyword, char **global_string, char *def) {
    rewind(file);
    char line[256];
    char *out = def;
    while (fgets(line, 256, file)) {
        char *p = line;
        p = parse_keyword(p, keyword); if (p==NULL) continue;
        while (*p == ' ' || *p == '\t') p++;

        out = malloc(strlen(p));
        char *dest = out;
        if (*p == '"') {
            p++;
            while (*p != '"' && *p != '\n' && *p != '\r' && *p) {
                if (*p == '\\' && *(p+1) == '"') { *dest++ = '"'; p += 2; }
                else if (*p == '\\' && *(p+1) == '\\') { *dest++ = '\\'; p += 2; }
                else { *dest++ = *p; p++; }
            }
        }
        else {
            while (*p && *p != '\n' && *p != '\r') *dest++ = *p++;
        }
        *dest = 0;
        break;
    }
    *global_string = out;
    struct config_entry *ent = &config_entries[numentries++];
    ent->keyword = keyword;
    ent->splat = splat_string;
    ent->global = global_string;
}

void LoadConfigBool(FILE *file, char *keyword, bool *global_bool, bool def) {
    rewind(file);
    char line[256];
    bool v = def;
    while (fgets(line, 256, file)) {
        char *p = line;
        p = parse_keyword(p, keyword); if (p==NULL) continue;
        while (*p == ' ' || *p == '\t') p++;
        if (*p == 't' || *p == 'y' || *p == '1') v = true;
        else v = false;
        break;
    }
    *global_bool = v;
    struct config_entry *ent = &config_entries[numentries++];
    ent->keyword = keyword;
    ent->splat = splat_bool;
    ent->global = global_bool;
}


//
// M_SaveDefaults
//
void M_SaveDefaults (void)
{
    FILE *file = fopen (config_path, "w"); if (!file) return;
    for (size_t i = 0; i < numentries; i++) {
        struct config_entry *ent = &config_entries[i];
        ent->splat(file, ent->keyword, ent->global);
    }
    fclose(file);
}


//
// M_LoadDefaults
//
extern byte	scantokey[128];
extern boolean verbose;

void M_LoadDefaults (void)
{
    config_path = malloc(1024);
    int i = M_CheckParm ("-config");
    if (i && i < myargc - 1) strcpy(config_path, myargv[i+1]);
    else sprintf(config_path, "%s/%s", GetHomeDir(), ".doomrc");

    if (verbose) printf ("	config file: %s\n", config_path);

    FILE *file = fopen(config_path, "r");
    if (file == NULL) I_Error("can't open config %s\n", config_path);

    LoadConfigInt(file, "mouse_sensitivity", &mouseSensitivity, 5);
    LoadConfigInt(file, "sfx_volume", &snd_SfxVolume, 8);
    LoadConfigInt(file, "music_volume", &snd_MusicVolume, 8);
    LoadConfigInt(file, "show_messages", &showMessages, 1);

    LoadConfigInt(file, "key_right", &key_right, KEY_RIGHTARROW);
    LoadConfigInt(file, "key_left", &key_left, KEY_LEFTARROW);
    LoadConfigInt(file, "key_up", &key_up, KEY_UPARROW);
    LoadConfigInt(file, "key_down", &key_down, KEY_DOWNARROW);
    LoadConfigInt(file, "key_strafeleft", &key_strafeleft, ',');
    LoadConfigInt(file, "key_straferight", &key_straferight, '.');

    LoadConfigInt(file, "key_fire", &key_fire, KEY_RCTRL);
    LoadConfigInt(file, "key_use", &key_use, ' ');
    LoadConfigInt(file, "key_strafe", &key_strafe, KEY_RALT);
    LoadConfigInt(file, "key_speed", &key_speed, KEY_RSHIFT);
    
    LoadConfigInt(file, "use_mouse", &usemouse, 1);
    LoadConfigInt(file, "mouseb_fire", &mousebfire, 0);
    LoadConfigInt(file, "mouseb_strafe", &mousebstrafe, 1);
    LoadConfigInt(file, "mouseb_forward", &mousebforward, 2);

    LoadConfigInt(file, "use_joystick", &usejoystick, 0);
    LoadConfigInt(file, "joyb_fire", &joybfire, 0);
    LoadConfigInt(file, "joyb_strafe", &joybstrafe, 1);
    LoadConfigInt(file, "joyb_use", &joybuse, 2);
    LoadConfigInt(file, "joyb_speed", &joybspeed, 3);

    LoadConfigInt(file, "screenblocks", &screenblocks, 9);
    LoadConfigInt(file, "detaillevel", &detailLevel, 0);

    LoadConfigInt(file, "snd_channels", &numChannels, 3);

    LoadConfigInt(file, "usegamma", &usegamma, 0);

    LoadConfigString(file, "chatmacro0", &chat_macros[0], HUSTR_CHATMACRO0);
    LoadConfigString(file, "chatmacro1", &chat_macros[1], HUSTR_CHATMACRO1);
    LoadConfigString(file, "chatmacro2", &chat_macros[2], HUSTR_CHATMACRO2);
    LoadConfigString(file, "chatmacro3", &chat_macros[3], HUSTR_CHATMACRO3);
    LoadConfigString(file, "chatmacro4", &chat_macros[4], HUSTR_CHATMACRO4);
    LoadConfigString(file, "chatmacro5", &chat_macros[5], HUSTR_CHATMACRO5);
    LoadConfigString(file, "chatmacro6", &chat_macros[6], HUSTR_CHATMACRO6);
    LoadConfigString(file, "chatmacro7", &chat_macros[7], HUSTR_CHATMACRO7);
    LoadConfigString(file, "chatmacro8", &chat_macros[8], HUSTR_CHATMACRO8);
    LoadConfigString(file, "chatmacro9", &chat_macros[9], HUSTR_CHATMACRO9);

    LoadConfigBool(file, "show_debug", &show_debug, false);
    LoadConfigBool(file, "mousewalk", &mouse_walk, true);
    LoadConfigBool(file, "show_endoom", &show_endoom, true);

    fclose(file);
}


//
// SCREEN SHOTS
//


typedef struct
{
    char		manufacturer;
    char		version;
    char		encoding;
    char		bits_per_pixel;

    unsigned short	xmin;
    unsigned short	ymin;
    unsigned short	xmax;
    unsigned short	ymax;
    
    unsigned short	hres;
    unsigned short	vres;

    unsigned char	palette[48];
    
    char		reserved;
    char		color_planes;
    unsigned short	bytes_per_line;
    unsigned short	palette_type;
    
    char		filler[58];
    unsigned char	data;		// unbounded
} pcx_t;


//
// WritePCXfile
//
void
WritePCXfile
( char*		filename,
  byte*		data,
  int		width,
  int		height,
  byte*		palette )
{
    int		i;
    int		length;
    pcx_t*	pcx;
    byte*	pack;
	
    pcx = Z_Malloc (width*height*2+1000, PU_STATIC, NULL);

    pcx->manufacturer = 0x0a;		// PCX id
    pcx->version = 5;			// 256 color
    pcx->encoding = 1;			// uncompressed
    pcx->bits_per_pixel = 8;		// 256 color
    pcx->xmin = 0;
    pcx->ymin = 0;
    pcx->xmax = SHORT(width-1);
    pcx->ymax = SHORT(height-1);
    pcx->hres = SHORT(width);
    pcx->vres = SHORT(height);
    memset (pcx->palette,0,sizeof(pcx->palette));
    pcx->color_planes = 1;		// chunky image
    pcx->bytes_per_line = SHORT(width);
    pcx->palette_type = SHORT(2);	// not a grey scale
    memset (pcx->filler,0,sizeof(pcx->filler));


    // pack the image
    pack = &pcx->data;
	
    for (i=0 ; i<width*height ; i++)
    {
	if ( (*data & 0xc0) != 0xc0)
	    *pack++ = *data++;
	else
	{
	    *pack++ = 0xc1;
	    *pack++ = *data++;
	}
    }
    
    // write the palette
    *pack++ = 0x0c;	// palette ID byte
    for (i=0 ; i<768 ; i++)
	*pack++ = *palette++;
    
    // write output file
    length = pack - (byte *)pcx;
    M_WriteFile (filename, pcx, length);

    Z_Free (pcx);
}


//
// M_ScreenShot
//
void M_ScreenShot (void)
{
    int		i;
    byte*	linear;
    char	lbmname[12];
    
    // munge planar buffer to linear
    linear = screens[2];
    I_ReadScreen (linear);
    
    // find a file name to save it to
    strcpy(lbmname,"DOOM00.pcx");
		
    for (i=0 ; i<=99 ; i++)
    {
	lbmname[4] = i/10 + '0';
	lbmname[5] = i%10 + '0';
	if (access(lbmname,0) == -1)
	    break;	// file doesn't exist
    }
    if (i==100)
	I_Error ("M_ScreenShot: Couldn't create a PCX");
    
    // save the pcx file
    WritePCXfile (lbmname, linear,
		  SCREENWIDTH, SCREENHEIGHT,
		  W_CacheLumpName ("PLAYPAL",PU_CACHE));
	
    players[consoleplayer].message = "screen shot";
}


