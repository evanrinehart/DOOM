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
// DESCRIPTION:  heads-up text and input code
//
//-----------------------------------------------------------------------------

#include <ctype.h>

#include "doomdef.h"

#include "v_video.h"
#include "m_swap.h"

#include "hu_lib.h"
#include "r_local.h"
#include "r_draw.h"

// boolean : whether the screen is always erased
#define noterased viewwindowx

int measure_last_utf8_sequence(const unsigned char *buf, size_t len);

extern boolean	automapactive;	// in AM_map.c

void HUlib_init(void)
{
}

void HUlib_clearTextLine(hu_textline_t* t)
{
    t->len = 0;
    t->l[0] = 0;
    t->needsupdate = true;
}

void
HUlib_initTextLine
( hu_textline_t*	t,
  int			x,
  int			y,
  patch_t**		f,
  int			sc )
{
    t->x = x;
    t->y = y;
    t->f = f;
    t->sc = sc;
    HUlib_clearTextLine(t);
}

boolean
HUlib_addCharToTextLine
( hu_textline_t*	t,
  char			ch )
{

    if (t->len == HU_MAXLINELENGTH)
	return false;
    else
    {
	t->l[t->len++] = ch;
	t->l[t->len] = 0;
	t->needsupdate = 4;
	return true;
    }

}

boolean HUlib_delCharFromTextLine(hu_textline_t* t)
{

    int n = measure_last_utf8_sequence((unsigned char *)t->l, t->len);

    if (!t->len) return false;
    else
    {
	for (int i = 0; i < n; i++) t->l[--t->len] = 0;
	t->needsupdate = 4;
	return true;
    }

}

int splat_glyph(hu_textline_t *l, int x, unsigned char c) {
    patch_t *patch = l->f[c - l->sc];
    int w = SHORT(patch->width);
    V_DrawPatchDirect(x, l->y, FG, patch);
    return x + w;
}

int get_utf8_size(unsigned char);
int decode_single_utf8(const unsigned char *, size_t);
int encode_single_utf8(int, unsigned char*);

// placeholder in lieu of actual font graphics
unsigned char spanish(const char *src, size_t len) {
    int u = decode_single_utf8((const unsigned char *)src, len);
    if (u < 0) return '@';
    char buf[6];
    buf[encode_single_utf8(u, (unsigned char *)buf)] = 0;
    if (strcmp(buf, "¿")==0) return '?';
    if (strcmp(buf, "¡")==0) return '!';
    if (strcmp(buf, "«")==0) return '"';
    if (strcmp(buf, "»")==0) return '"';
    if (strcmp(buf, "Ñ")==0) return 'N';
    if (strcmp(buf, "ñ")==0) return 'N';
    if (strcmp(buf, "á")==0) return 'A';
    if (strcmp(buf, "é")==0) return 'E';
    if (strcmp(buf, "í")==0) return 'I';
    if (strcmp(buf, "ó")==0) return 'O';
    if (strcmp(buf, "ú")==0) return 'U';
    if (strcmp(buf, "Á")==0) return 'A';
    if (strcmp(buf, "É")==0) return 'E';
    if (strcmp(buf, "Í")==0) return 'I';
    if (strcmp(buf, "Ó")==0) return 'O';
    if (strcmp(buf, "Ú")==0) return 'U';
    return '?';
}

void
HUlib_drawTextLine
( hu_textline_t*	l,
  boolean		drawcursor )
{

    // draw the new stuff
    int x = l->x;
    int i = 0;
    int left = l->len;
    while (left > 0) {

        int n = get_utf8_size(l->l[i]);
        unsigned char c = ' ';

        if (n == 1) c = toupper(l->l[i]);
        else if (n > 1) c = spanish(&l->l[i], left);
        else c = '_';

        if (n==1 && (c==' ' || c < l->sc || c > '_'))
            x += 4;
        else
            x = splat_glyph(l, x, c);

        if (x >= SCREENWIDTH) break;
        if (left <= n) break;
        i += n;
        left -= n;
    }

    // draw the cursor if requested
    if (drawcursor
	&& x + SHORT(l->f['_' - l->sc]->width) <= SCREENWIDTH)
    {
	V_DrawPatchDirect(x, l->y, FG, l->f['_' - l->sc]);
    }
}


// sorta called by HU_Erase and just better darn get things straight
void HUlib_eraseTextLine(hu_textline_t* l)
{
    int			lh;
    int			y;
    int			yoffset;

    // Only erases when NOT in automap and the screen is reduced,
    // and the text must either need updating or refreshing
    // (because of a recent change back from the automap)

    if (!automapactive &&
	viewwindowx && l->needsupdate)
    {
	lh = SHORT(l->f[0]->height) + 1;
	for (y=l->y,yoffset=y*SCREENWIDTH ; y<l->y+lh ; y++,yoffset+=SCREENWIDTH)
	{
	    if (y < viewwindowy || y >= viewwindowy + viewheight)
		R_VideoErase(yoffset, SCREENWIDTH); // erase entire line
	    else
	    {
		R_VideoErase(yoffset, viewwindowx); // erase left border
		R_VideoErase(yoffset + viewwindowx + viewwidth, viewwindowx);
		// erase right border
	    }
	}
    }

    if (l->needsupdate) l->needsupdate--;

}

void
HUlib_initSText
( hu_stext_t*	s,
  int		x,
  int		y,
  int		h,
  patch_t**	font,
  int		startchar,
  boolean*	on )
{

    int i;

    s->h = h;
    s->on = on;
    s->laston = true;
    s->cl = 0;
    for (i=0;i<h;i++)
	HUlib_initTextLine(&s->l[i],
			   x, y - i*(SHORT(font[0]->height)+1),
			   font, startchar);

}

void HUlib_addLineToSText(hu_stext_t* s)
{

    int i;

    // add a clear line
    if (++s->cl == s->h)
	s->cl = 0;
    HUlib_clearTextLine(&s->l[s->cl]);

    // everything needs updating
    for (i=0 ; i<s->h ; i++)
	s->l[i].needsupdate = 4;

}

void
HUlib_addMessageToSText
( hu_stext_t*	s,
  char*		prefix,
  char*		msg )
{
    HUlib_addLineToSText(s);
    if (prefix)
	while (*prefix)
	    HUlib_addCharToTextLine(&s->l[s->cl], *(prefix++));

    while (*msg)
	HUlib_addCharToTextLine(&s->l[s->cl], *(msg++));
}

void HUlib_drawSText(hu_stext_t* s)
{
    int i, idx;
    hu_textline_t *l;

    if (!*s->on)
	return; // if not on, don't draw

    // draw everything
    for (i=0 ; i<s->h ; i++)
    {
	idx = s->cl - i;
	if (idx < 0)
	    idx += s->h; // handle queue of lines
	
	l = &s->l[idx];

	// need a decision made here on whether to skip the draw
	HUlib_drawTextLine(l, false); // no cursor, please
    }

}

void HUlib_eraseSText(hu_stext_t* s)
{

    int i;

    for (i=0 ; i<s->h ; i++)
    {
	if (s->laston && !*s->on)
	    s->l[i].needsupdate = 4;
	HUlib_eraseTextLine(&s->l[i]);
    }
    s->laston = *s->on;

}

void
HUlib_initIText
( hu_itext_t*	it,
  int		x,
  int		y,
  patch_t**	font,
  int		startchar,
  boolean*	on )
{
    it->lm = 0; // default left margin is start of text
    it->on = on;
    it->laston = true;
    HUlib_initTextLine(&it->l, x, y, font, startchar);
}


// The following deletion routines adhere to the left margin restriction
void HUlib_delCharFromIText(hu_itext_t* it)
{
    if (it->l.len != it->lm)
	HUlib_delCharFromTextLine(&it->l);
}

void HUlib_eraseLineFromIText(hu_itext_t* it)
{
    while (it->lm != it->l.len)
	HUlib_delCharFromTextLine(&it->l);
}

// Resets left margin as well
void HUlib_resetIText(hu_itext_t* it)
{
    it->lm = 0;
    HUlib_clearTextLine(&it->l);
}

void
HUlib_addPrefixToIText
( hu_itext_t*	it,
  char*		str )
{
    while (*str)
	HUlib_addCharToTextLine(&it->l, *(str++));
    it->lm = it->l.len;
}

void HUlib_drawIText(hu_itext_t* it)
{

    hu_textline_t *l = &it->l;

    if (!*it->on)
	return;
    HUlib_drawTextLine(l, true); // draw the line w/ cursor

}

void HUlib_eraseIText(hu_itext_t* it)
{
    if (it->laston && !*it->on)
	it->l.needsupdate = 4;
    HUlib_eraseTextLine(&it->l);
    it->laston = *it->on;
}

