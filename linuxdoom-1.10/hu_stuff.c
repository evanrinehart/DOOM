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
// DESCRIPTION:  Heads-up displays
//
//-----------------------------------------------------------------------------

#include <ctype.h>

#include "doomdef.h"

#include "z_zone.h"

#include "m_swap.h"

#include "hu_stuff.h"
#include "hu_lib.h"
#include "w_wad.h"

#include "s_sound.h"

#include "doomstat.h"

// Data.
#include "dstrings.h"
#include "sounds.h"

#include "f_finale.h"

//
// Locally used constants, shortcuts.
//
#define HU_TITLE	(mapnames[(gameepisode-1)*9+gamemap-1])
#define HU_TITLE2	(mapnames2[gamemap-1])
#define HU_TITLEP	(mapnamesp[gamemap-1])
#define HU_TITLET	(mapnamest[gamemap-1])
#define HU_TITLEHEIGHT	1
#define HU_TITLEX	0
#define HU_TITLEY	(167 - SHORT(hu_font[0]->height))

#define HU_INPUTTOGGLE	't'
#define HU_INPUTX	HU_MSGX
#define HU_INPUTY	(HU_MSGY + HU_MSGHEIGHT*(SHORT(hu_font[0]->height) +1))
#define HU_INPUTWIDTH	64
#define HU_INPUTHEIGHT	1



char*	chat_macros[] =
{
    HUSTR_CHATMACRO0,
    HUSTR_CHATMACRO1,
    HUSTR_CHATMACRO2,
    HUSTR_CHATMACRO3,
    HUSTR_CHATMACRO4,
    HUSTR_CHATMACRO5,
    HUSTR_CHATMACRO6,
    HUSTR_CHATMACRO7,
    HUSTR_CHATMACRO8,
    HUSTR_CHATMACRO9
};

char*	player_names[] =
{
    HUSTR_PLRGREEN,
    HUSTR_PLRINDIGO,
    HUSTR_PLRBROWN,
    HUSTR_PLRRED
};


char			chat_char; // remove later.
static player_t*	plr;
patch_t*		hu_font[HU_FONTSIZE];
static hu_textline_t	w_title;
boolean			chat_on;
static hu_itext_t	w_chat;
static boolean		always_off = false;
static char		chat_dest[MAXPLAYERS];
static hu_itext_t w_inputbuffer[MAXPLAYERS];

static boolean		message_on;
boolean			message_dontfuckwithme;
static boolean		message_nottobefuckedwith;

static hu_stext_t	w_message;
static int		message_counter;

extern int		showMessages;
extern boolean		automapactive;

static boolean		headsupactive = false;

//
// Builtin map names.
// The actual names can be found in DStrings.h.
//

char*	mapnames[] =	// DOOM shareware/registered/retail (Ultimate) names.
{

    HUSTR_E1M1,
    HUSTR_E1M2,
    HUSTR_E1M3,
    HUSTR_E1M4,
    HUSTR_E1M5,
    HUSTR_E1M6,
    HUSTR_E1M7,
    HUSTR_E1M8,
    HUSTR_E1M9,

    HUSTR_E2M1,
    HUSTR_E2M2,
    HUSTR_E2M3,
    HUSTR_E2M4,
    HUSTR_E2M5,
    HUSTR_E2M6,
    HUSTR_E2M7,
    HUSTR_E2M8,
    HUSTR_E2M9,

    HUSTR_E3M1,
    HUSTR_E3M2,
    HUSTR_E3M3,
    HUSTR_E3M4,
    HUSTR_E3M5,
    HUSTR_E3M6,
    HUSTR_E3M7,
    HUSTR_E3M8,
    HUSTR_E3M9,

    HUSTR_E4M1,
    HUSTR_E4M2,
    HUSTR_E4M3,
    HUSTR_E4M4,
    HUSTR_E4M5,
    HUSTR_E4M6,
    HUSTR_E4M7,
    HUSTR_E4M8,
    HUSTR_E4M9,

    "NEWLEVEL",
    "NEWLEVEL",
    "NEWLEVEL",
    "NEWLEVEL",
    "NEWLEVEL",
    "NEWLEVEL",
    "NEWLEVEL",
    "NEWLEVEL",
    "NEWLEVEL"
};

char*	mapnames2[] =	// DOOM 2 map names.
{
    HUSTR_1,
    HUSTR_2,
    HUSTR_3,
    HUSTR_4,
    HUSTR_5,
    HUSTR_6,
    HUSTR_7,
    HUSTR_8,
    HUSTR_9,
    HUSTR_10,
    HUSTR_11,
	
    HUSTR_12,
    HUSTR_13,
    HUSTR_14,
    HUSTR_15,
    HUSTR_16,
    HUSTR_17,
    HUSTR_18,
    HUSTR_19,
    HUSTR_20,
	
    HUSTR_21,
    HUSTR_22,
    HUSTR_23,
    HUSTR_24,
    HUSTR_25,
    HUSTR_26,
    HUSTR_27,
    HUSTR_28,
    HUSTR_29,
    HUSTR_30,
    HUSTR_31,
    HUSTR_32
};


char*	mapnamesp[] =	// Plutonia WAD map names.
{
    PHUSTR_1,
    PHUSTR_2,
    PHUSTR_3,
    PHUSTR_4,
    PHUSTR_5,
    PHUSTR_6,
    PHUSTR_7,
    PHUSTR_8,
    PHUSTR_9,
    PHUSTR_10,
    PHUSTR_11,
	
    PHUSTR_12,
    PHUSTR_13,
    PHUSTR_14,
    PHUSTR_15,
    PHUSTR_16,
    PHUSTR_17,
    PHUSTR_18,
    PHUSTR_19,
    PHUSTR_20,
	
    PHUSTR_21,
    PHUSTR_22,
    PHUSTR_23,
    PHUSTR_24,
    PHUSTR_25,
    PHUSTR_26,
    PHUSTR_27,
    PHUSTR_28,
    PHUSTR_29,
    PHUSTR_30,
    PHUSTR_31,
    PHUSTR_32
};


char *mapnamest[] =	// TNT WAD map names.
{
    THUSTR_1,
    THUSTR_2,
    THUSTR_3,
    THUSTR_4,
    THUSTR_5,
    THUSTR_6,
    THUSTR_7,
    THUSTR_8,
    THUSTR_9,
    THUSTR_10,
    THUSTR_11,
	
    THUSTR_12,
    THUSTR_13,
    THUSTR_14,
    THUSTR_15,
    THUSTR_16,
    THUSTR_17,
    THUSTR_18,
    THUSTR_19,
    THUSTR_20,
	
    THUSTR_21,
    THUSTR_22,
    THUSTR_23,
    THUSTR_24,
    THUSTR_25,
    THUSTR_26,
    THUSTR_27,
    THUSTR_28,
    THUSTR_29,
    THUSTR_30,
    THUSTR_31,
    THUSTR_32
};



char *GetDoomMapName(int episode, int map) {
    if (episode < 1 || episode > 4 || map < 1 || map > 9)
        return F_GETSTRING(HUSTR_E1M1);

    switch (episode) {
        case 1:
            switch (map) {
                case 1: return F_GETSTRING(HUSTR_E1M1);
                case 2: return F_GETSTRING(HUSTR_E1M2);
                case 3: return F_GETSTRING(HUSTR_E1M3);
                case 4: return F_GETSTRING(HUSTR_E1M4);
                case 5: return F_GETSTRING(HUSTR_E1M5);
                case 6: return F_GETSTRING(HUSTR_E1M6);
                case 7: return F_GETSTRING(HUSTR_E1M7);
                case 8: return F_GETSTRING(HUSTR_E1M8);
                default: return F_GETSTRING(HUSTR_E1M9);
            }
        case 2:
            switch (map) {
                case 1: return F_GETSTRING(HUSTR_E2M1);
                case 2: return F_GETSTRING(HUSTR_E2M2);
                case 3: return F_GETSTRING(HUSTR_E2M3);
                case 4: return F_GETSTRING(HUSTR_E2M4);
                case 5: return F_GETSTRING(HUSTR_E2M5);
                case 6: return F_GETSTRING(HUSTR_E2M6);
                case 7: return F_GETSTRING(HUSTR_E2M7);
                case 8: return F_GETSTRING(HUSTR_E2M8);
                default: return F_GETSTRING(HUSTR_E2M9);
            }
        case 3:
            switch (map) {
                case 1: return F_GETSTRING(HUSTR_E3M1);
                case 2: return F_GETSTRING(HUSTR_E3M2);
                case 3: return F_GETSTRING(HUSTR_E3M3);
                case 4: return F_GETSTRING(HUSTR_E3M4);
                case 5: return F_GETSTRING(HUSTR_E3M5);
                case 6: return F_GETSTRING(HUSTR_E3M6);
                case 7: return F_GETSTRING(HUSTR_E3M7);
                case 8: return F_GETSTRING(HUSTR_E3M8);
                default: return F_GETSTRING(HUSTR_E3M9);
            }
        default:
            switch (map) {
                case 1: return F_GETSTRING(HUSTR_E4M1);
                case 2: return F_GETSTRING(HUSTR_E4M2);
                case 3: return F_GETSTRING(HUSTR_E4M3);
                case 4: return F_GETSTRING(HUSTR_E4M4);
                case 5: return F_GETSTRING(HUSTR_E4M5);
                case 6: return F_GETSTRING(HUSTR_E4M6);
                case 7: return F_GETSTRING(HUSTR_E4M7);
                case 8: return F_GETSTRING(HUSTR_E4M8);
                default: return F_GETSTRING(HUSTR_E4M9);
            }
    }
}

char *GetDoom2MapName(int map) {
    switch (map) {
        case 1: return F_GETSTRING(HUSTR_1);
        case 2: return F_GETSTRING(HUSTR_2);
        case 3: return F_GETSTRING(HUSTR_3);
        case 4: return F_GETSTRING(HUSTR_4);
        case 5: return F_GETSTRING(HUSTR_5);
        case 6: return F_GETSTRING(HUSTR_6);
        case 7: return F_GETSTRING(HUSTR_7);
        case 8: return F_GETSTRING(HUSTR_8);
        case 9: return F_GETSTRING(HUSTR_9);
        case 10: return F_GETSTRING(HUSTR_10);
        case 11: return F_GETSTRING(HUSTR_11);
        case 12: return F_GETSTRING(HUSTR_12);
        case 13: return F_GETSTRING(HUSTR_13);
        case 14: return F_GETSTRING(HUSTR_14);
        case 15: return F_GETSTRING(HUSTR_15);
        case 16: return F_GETSTRING(HUSTR_16);
        case 17: return F_GETSTRING(HUSTR_17);
        case 18: return F_GETSTRING(HUSTR_18);
        case 19: return F_GETSTRING(HUSTR_19);
        case 20: return F_GETSTRING(HUSTR_20);
        case 21: return F_GETSTRING(HUSTR_21);
        case 22: return F_GETSTRING(HUSTR_22);
        case 23: return F_GETSTRING(HUSTR_23);
        case 24: return F_GETSTRING(HUSTR_24);
        case 25: return F_GETSTRING(HUSTR_25);
        case 26: return F_GETSTRING(HUSTR_26);
        case 27: return F_GETSTRING(HUSTR_27);
        case 28: return F_GETSTRING(HUSTR_28);
        case 29: return F_GETSTRING(HUSTR_29);
        case 30: return F_GETSTRING(HUSTR_30);
        case 31: return F_GETSTRING(HUSTR_31);
        case 32: return F_GETSTRING(HUSTR_32);
    }

    return F_GETSTRING(HUSTR_1);
}

char *GetTNTMapName(int map) {
    switch (map) {
        case 1: return F_GETSTRING(THUSTR_1);
        case 2: return F_GETSTRING(THUSTR_2);
        case 3: return F_GETSTRING(THUSTR_3);
        case 4: return F_GETSTRING(THUSTR_4);
        case 5: return F_GETSTRING(THUSTR_5);
        case 6: return F_GETSTRING(THUSTR_6);
        case 7: return F_GETSTRING(THUSTR_7);
        case 8: return F_GETSTRING(THUSTR_8);
        case 9: return F_GETSTRING(THUSTR_9);
        case 10: return F_GETSTRING(THUSTR_10);
        case 11: return F_GETSTRING(THUSTR_11);
        case 12: return F_GETSTRING(THUSTR_12);
        case 13: return F_GETSTRING(THUSTR_13);
        case 14: return F_GETSTRING(THUSTR_14);
        case 15: return F_GETSTRING(THUSTR_15);
        case 16: return F_GETSTRING(THUSTR_16);
        case 17: return F_GETSTRING(THUSTR_17);
        case 18: return F_GETSTRING(THUSTR_18);
        case 19: return F_GETSTRING(THUSTR_19);
        case 20: return F_GETSTRING(THUSTR_20);
        case 21: return F_GETSTRING(THUSTR_21);
        case 22: return F_GETSTRING(THUSTR_22);
        case 23: return F_GETSTRING(THUSTR_23);
        case 24: return F_GETSTRING(THUSTR_24);
        case 25: return F_GETSTRING(THUSTR_25);
        case 26: return F_GETSTRING(THUSTR_26);
        case 27: return F_GETSTRING(THUSTR_27);
        case 28: return F_GETSTRING(THUSTR_28);
        case 29: return F_GETSTRING(THUSTR_29);
        case 30: return F_GETSTRING(THUSTR_30);
        case 31: return F_GETSTRING(THUSTR_31);
        case 32: return F_GETSTRING(THUSTR_32);
    }

    return F_GETSTRING(THUSTR_1);
}

char *GetPlutoniaMapName(int map) {
    switch (map) {
        case 1: return F_GETSTRING(PHUSTR_1);
        case 2: return F_GETSTRING(PHUSTR_2);
        case 3: return F_GETSTRING(PHUSTR_3);
        case 4: return F_GETSTRING(PHUSTR_4);
        case 5: return F_GETSTRING(PHUSTR_5);
        case 6: return F_GETSTRING(PHUSTR_6);
        case 7: return F_GETSTRING(PHUSTR_7);
        case 8: return F_GETSTRING(PHUSTR_8);
        case 9: return F_GETSTRING(PHUSTR_9);
        case 10: return F_GETSTRING(PHUSTR_10);
        case 11: return F_GETSTRING(PHUSTR_11);
        case 12: return F_GETSTRING(PHUSTR_12);
        case 13: return F_GETSTRING(PHUSTR_13);
        case 14: return F_GETSTRING(PHUSTR_14);
        case 15: return F_GETSTRING(PHUSTR_15);
        case 16: return F_GETSTRING(PHUSTR_16);
        case 17: return F_GETSTRING(PHUSTR_17);
        case 18: return F_GETSTRING(PHUSTR_18);
        case 19: return F_GETSTRING(PHUSTR_19);
        case 20: return F_GETSTRING(PHUSTR_20);
        case 21: return F_GETSTRING(PHUSTR_21);
        case 22: return F_GETSTRING(PHUSTR_22);
        case 23: return F_GETSTRING(PHUSTR_23);
        case 24: return F_GETSTRING(PHUSTR_24);
        case 25: return F_GETSTRING(PHUSTR_25);
        case 26: return F_GETSTRING(PHUSTR_26);
        case 27: return F_GETSTRING(PHUSTR_27);
        case 28: return F_GETSTRING(PHUSTR_28);
        case 29: return F_GETSTRING(PHUSTR_29);
        case 30: return F_GETSTRING(PHUSTR_30);
        case 31: return F_GETSTRING(PHUSTR_31);
        case 32: return F_GETSTRING(PHUSTR_32);
    }

    return F_GETSTRING(THUSTR_1);
}

char *HU_GetMapName(int gamemode, int gamemission, int gameepisode, int gamemap) {
    switch (gamemode) {
        case shareware:
        case registered:
        case retail:
            return GetDoomMapName(gameepisode, gamemap);
        case commercial:
            switch (gamemission) {
                case doom2: return GetDoom2MapName(gamemap);
                case pack_tnt: return GetTNTMapName(gamemap);
                case pack_plut: return GetPlutoniaMapName(gamemap);
            }
    }

    return GetDoomMapName(gameepisode, gamemap);
}
    

void HU_Init(void)
{

    int		i;
    int		j;
    char	buffer[17];

    // load the heads-up font
    j = HU_FONTSTART;
    for (i=0;i<HU_FONTSIZE;i++)
    {
	sprintf(buffer, "STCFN%.3d", j++);
	hu_font[i] = (patch_t *) W_CacheLumpName(buffer, PU_STATIC);
    }

}

void HU_Stop(void)
{
    headsupactive = false;
}

void HU_Start(void)
{

    int		i;
    char*	s;

    if (headsupactive)
	HU_Stop();

    plr = &players[consoleplayer];
    message_on = false;
    message_dontfuckwithme = false;
    message_nottobefuckedwith = false;
    chat_on = false;

    // create the message widget
    HUlib_initSText(&w_message,
		    HU_MSGX, HU_MSGY, HU_MSGHEIGHT,
		    hu_font,
		    HU_FONTSTART, &message_on);

    // create the map title widget
    HUlib_initTextLine(&w_title,
		       HU_TITLEX, HU_TITLEY,
		       hu_font,
		       HU_FONTSTART);

    s = HU_GetMapName(gamemode, gamemission, gameepisode, gamemap);
    
    while (*s)
	HUlib_addCharToTextLine(&w_title, *(s++));

    // create the chat widget
    HUlib_initIText(&w_chat,
		    HU_INPUTX, HU_INPUTY,
		    hu_font,
		    HU_FONTSTART, &chat_on);

    // create the inputbuffer widgets
    for (i=0 ; i<MAXPLAYERS ; i++)
	HUlib_initIText(&w_inputbuffer[i], 0, 0, 0, 0, &always_off);

    headsupactive = true;

}

void HU_Drawer(void)
{

    HUlib_drawSText(&w_message);
    HUlib_drawIText(&w_chat);
    if (automapactive)
	HUlib_drawTextLine(&w_title, false);

}

void HU_Erase(void)
{

    HUlib_eraseSText(&w_message);
    HUlib_eraseIText(&w_chat);
    HUlib_eraseTextLine(&w_title);

}

void HU_Ticker(void)
{

    // tick down message counter if message is up
    if (message_counter && !--message_counter)
    {
	message_on = false;
	message_nottobefuckedwith = false;
    }

    if (showMessages || message_dontfuckwithme)
    {

	// display message if necessary
	if ((plr->message && !message_nottobefuckedwith)
	    || (plr->message && message_dontfuckwithme))
	{
	    HUlib_addMessageToSText(&w_message, 0, plr->message);
	    plr->message = 0;
	    message_on = true;
	    message_counter = HU_MSGTIMEOUT;
	    message_nottobefuckedwith = message_dontfuckwithme;
	    message_dontfuckwithme = 0;
	}

    } // else message_on = false;

    if (!netgame) return;

    // check for incoming chat characters
    for (int i=0 ; i<MAXPLAYERS; i++) {
        byte c = players[i].cmd.chatchar;

        if (!playeringame[i]) continue;
        if (i == consoleplayer) continue;
        if (c == 0) continue;

        if (c <= HU_BROADCAST) { // c less than HU_BROADCAST (5) means goes to specific player
            chat_dest[i] = c;
        }
        if (c == KEY_BACKSPACE) { // 127 is command code to erase previous character
            HUlib_delCharFromIText(&w_inputbuffer[i]);
        }
        else if (c == KEY_ENTER) { // 13 means "deliver" the inputbuffer as a message
            if (w_inputbuffer[i].l.len && (chat_dest[i] == consoleplayer+1 || chat_dest[i] == HU_BROADCAST)) {
                HUlib_addMessageToSText(&w_message, player_names[i], w_inputbuffer[i].l.l);
                printf("%s%s\n", player_names[i], w_inputbuffer[i].l.l);
                message_nottobefuckedwith = true;
                message_on = true;
                message_counter = HU_MSGTIMEOUT;
                int sfx = gamemode == commercial ? sfx_radio : sfx_tink;
                S_StartSound(0, sfx);
            }
            HUlib_resetIText(&w_inputbuffer[i]);
        }
        else if (' ' <= c && c < 127) { // basic printable character, ignore case
            HUlib_addCharToTextLine(&w_inputbuffer[i].l, toupper(c));
        }
        else if (c > 127) { // part of a non-trivial utf8 sequence, treated mostly as before
            HUlib_addCharToTextLine(&w_inputbuffer[i].l, c);
        }

        // incoming chat char if any was consumed
        players[i].cmd.chatchar = 0;
    }

}

#define QUEUESIZE		128

static int	chatchars[QUEUESIZE];
static int	head = 0;
static int	tail = 0;


void HU_queueChatChar(byte c)
{
    if (((head + 1) & (QUEUESIZE-1)) == tail)
    {
	plr->message = HUSTR_MSGU;
    }
    else
    {
	chatchars[head] = c;
	head = (head + 1) & (QUEUESIZE-1);
    }
}

byte HU_dequeueChatChar(void)
{
    byte c;

    if (head != tail)
    {
	c = chatchars[tail];
	tail = (tail + 1) & (QUEUESIZE-1);
    }
    else
    {
	c = 0;
    }

    return c;
}

int encode_single_utf8(long codepoint, unsigned char *buf);

boolean HU_Responder(event_t *ev)
{

    static char		lastmessage[HU_MAXLINELENGTH+1];
    char*		macromessage;
    //static boolean	shiftdown = false;
    static boolean	altdown = false;
    int			i;
    int			numplayers;
    
    static char		destination_keys[MAXPLAYERS] =
    {
	HUSTR_KEYGREEN,
	HUSTR_KEYINDIGO,
	HUSTR_KEYBROWN,
	HUSTR_KEYRED
    };
    
    static int		num_nobrainers = 0;

    numplayers = 0;
    for (i=0 ; i<MAXPLAYERS ; i++)
	numplayers += playeringame[i];

    // observe and remember if shift, alt are down
    //if (ev->data1 == KEY_RSHIFT && ev->type == ev_keydown) { shiftdown = true; return false; }
    //if (ev->data1 == KEY_RSHIFT && ev->type == ev_keyup) { shiftdown = false; return false; }
    if (ev->data1 == KEY_RALT && ev->type == ev_keydown) { altdown = true; return false; }
    if (ev->data1 == KEY_RALT && ev->type == ev_keyup) { altdown = false; return false; }


    // show last message
    if (!chat_on && ev->type == ev_keydown && ev->data1 == KEY_ENTER) {
        message_on = true;
        message_counter = HU_MSGTIMEOUT;
        return true;
    }

    // press T to begin broadcast chat mode
    if (!chat_on && ev->type == ev_character && netgame && ev->data1 == HU_INPUTTOGGLE) {
        chat_on = true;
        HUlib_resetIText(&w_chat);
        HU_queueChatChar(HU_BROADCAST); // 5 codes for broadcast
        return true;
    }

    // press I G B or R to begin private chat to respective player
    if (!chat_on && ev->type == ev_character && netgame && numplayers > 2) {
        int p = -1;
        for (int i=0; i<MAXPLAYERS ; i++) if(ev->data1 == destination_keys[i]) { p = i; break; }
        if (p < 0 || !playeringame[p]) return false;

        // you're talking to yourself!
        if (p == consoleplayer) {
            num_nobrainers++;
            if (num_nobrainers < 3) plr->message = HUSTR_TALKTOSELF1;
            else if (num_nobrainers < 6) plr->message = HUSTR_TALKTOSELF2;
            else if (num_nobrainers < 9) plr->message = HUSTR_TALKTOSELF3;
            else if (num_nobrainers < 32) plr->message = HUSTR_TALKTOSELF4;
            else plr->message = HUSTR_TALKTOSELF5;
            return false;
        }

        chat_on = true;
        HUlib_resetIText(&w_chat);
        HU_queueChatChar(p+1); // 1 through 4 code for private message
        return true;
    }

    // alt + number sends chat macro
    if (chat_on && altdown && ev->type == ev_keydown && '0' <= ev->data1 && ev->data1 <= '9') {
        int c = ev->data1 - '0';

        macromessage = chat_macros[c];

        // kill last message with a '\n'
        HU_queueChatChar(KEY_ENTER);

        // send the macro message
        while (*macromessage) HU_queueChatChar(*macromessage++);
        HU_queueChatChar(KEY_ENTER);

        // leave chat mode and notify that it was sent
        chat_on = false;
        strcpy(lastmessage, chat_macros[c]);
        int l = strlen(lastmessage);
        for (int i = 0; i < l; i++)
            if ('a' <= lastmessage[i] && lastmessage[i] <= 'z') lastmessage[i] = toupper(lastmessage[i]);
        plr->message = lastmessage;
        printf("%s: %s\n", "You", plr->message);
        return true;
    }

    // handle backspace key by queueing 127 and erasing last character
    if (chat_on && ev->type == ev_keydown && ev->data1 == KEY_BACKSPACE) {
        HU_queueChatChar(127); // code for rub out
        HUlib_delCharFromIText(&w_chat);
        return true;
    }

    // handle enter key by enqueuing 13, close chat, copy message to HUD
    if (chat_on && ev->type == ev_keydown && ev->data1 == KEY_ENTER) {
        HU_queueChatChar(13); // code for ENTER
        chat_on = false;
        if (w_chat.l.len) {
            strcpy(lastmessage, w_chat.l.l);
            plr->message = lastmessage;
            printf("%s: %s\n", "You", plr->message);
        }
        return true;
    }

    // escape simply exits chat mode without altering the buffers
    if (chat_on && ev->type == ev_keydown && ev->data1 == KEY_ESCAPE) {
        chat_on = false;
        return true;
    }

    // proper chat, user typed a character
    if (chat_on && ev->type == ev_character) {
        long c = toupper(ev->data1);
        if (' ' <= c && c <= '_') { // uppercase range of ascii printchars
            HU_queueChatChar(c);
            HUlib_addCharToTextLine(&w_chat.l, c);
            return true;
        }
        else { // non ascii character
            unsigned char buf[4];
            int size = encode_single_utf8(c, buf); if (size < 0) return false;
            for (int i = 0; i < size; i++) {
                HU_queueChatChar(buf[i]);
                HUlib_addCharToTextLine(&w_chat.l, buf[i]);
            }
            return true;
        }
    }

    return false;

}
