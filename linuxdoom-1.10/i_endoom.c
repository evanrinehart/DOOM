/*
 *  Platform specific ENDOOM rendering
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include <unistd.h>

#include <locale.h>
#include <langinfo.h>

bool show_endoom = true;

void I_Quit (void);

/* attempt to deduce term capabilities. Needs improvement */
void detect_term(int *rev, int *color, int *unicode) {
    *rev = 1; // unless it's a dumb terminal it probably supports reverse video
    *color = 1; // the big question
    *unicode = 0; // 1 if and only if UTF-8 locale

    setlocale(LC_ALL, "");
    char *codeset = nl_langinfo(CODESET);
    if (strcmp(codeset, "UTF-8") == 0) *unicode = 1;

    char *term = getenv("TERM");
    int atty = isatty(STDOUT_FILENO);
    int dumb = term && strcmp(term,"dumb")==0;
    if (!atty || term==NULL || dumb) { *color=0; *rev=0; } // spartan endoom
    if (strncmp(term,"vt",2)==0) { *color = 0; } // assume no color
}


char *encode_color(int color, int bg) {
    switch (color) {
        case 0: return bg ? "40" : "30"; // black
        case 1: return bg ? "44" : "34"; // blue
        case 2: return bg ? "42" : "32"; // green
        case 3: return bg ? "46" : "36"; // cyan
        case 4: return bg ? "41" : "31"; // red
        case 5: return bg ? "45" : "35"; // magenta
        case 6: return bg ? "53" : "33"; // "brown"
        case 7: return bg ? "47" : "37"; // "light gray"
        // the following are foreground only
        case 8: return "90"; // "dark gray"
        case 9: return "94"; // light blue
        case 10: return "92"; // light green
        case 11: return "96"; // light cyan
        case 12: return "91"; // light red
        case 13: return "95"; // light magenta
        case 14: return "93"; // yellow
        case 15: return "97"; // bright white
        default: return bg ? "40" : "30";
    }
}

char codepage437[][65] = {
    "ÇüéâäàåçêëèïîìÄÅ",
    "ÉæÆôöòûùÿÖÜ¢£¥₧ƒ",
    "áíóúñÑªº¿⌐¬½¼¡«»",
    "░▒▓│┤╡╢╖╕╣║╗╝╜╛┐",
    "└┴┬├─┼╞╟╚╔╩╦╠═╬╧",
    "╨╤╥╙╘╒╓╫╪┘┌█▄▌▐▀",
    "αßΓπΣσµτΦΘΩδ∞φε∩",
    "≡±≥≤⌠⌡÷≈°∙·√ⁿ²■ "
};

int is_start(char c) {
    return !((c & 0xc0) == 0x80);
}

static char *nextchar(char *p) {
    p++;
    while (*p && !is_start(*p)) p++;
    return p;
}

void copychar(char *buf, char *p) {
    int i = 0;
    do { buf[i++] = *p++; } while (!is_start(*p));
    buf[i] = 0;
}


char *encode_ext(int c, int use_unicode) {

    if (!use_unicode) {
        if (c == 0xc4) return "-";
        if (c == 0xcd) return "=";
        if (c == 0xba) return "H";
        if (c == 0xbc) return "=";
        if (c == 0xbb) return "=";
        if (c == 0xcb) return "=";
        if (c == 0xc8) return "=";
        if (c == 0xc9) return "=";

        if (c == 0xb0) return "%";
        if (c == 0xb2) return "#";
        if (c == 0xb1) return "&";
        if (c == 0xdb) return "M";
        return " ";
    }

    int row = (c - 128) / 16;
    int col = c % 16;

    static char buf[5];
    char *p = codepage437[row];
    for (int i=0; i<col; i++) p=nextchar(p);
    copychar(buf, p);
    return buf;

}

void spartan_endoom(unsigned char *endoom, int unicode) {
    for (int j = 0; j < 25; j++) {
        for (int i = 0; i < 80; i++) {
            int c = endoom[2*(80*j + i)];
            if (0 <= c && c <= 127)
                putchar(c);
            else
                printf("%s", encode_ext(c, unicode));
        }
        putchar('\n');
    }
}

void reverse_endoom(unsigned char *endoom, int unicode) {
    int cooldown = 0;
    unsigned char *p = endoom;
    for (int j = 0; j < 25; j++) {
        for (int i = 0; i < 80; i++) {
            int c = *p;
            int c1 = i+1 < 80 ? p[2] : ' ';
            int c2 = i+2 < 80 ? p[4] : ' ';
            int c3 = i+3 < 80 ? p[6] : ' ';
            char pattern[5] = {c,c1,c2,c3,0};
            if (cooldown > 0) cooldown--;
            if (cooldown==3) { printf("\x1b[7m"); }
            if (cooldown==1) { printf("\x1b[0m"); }
            if (strcmp(pattern, " id ")==0) { cooldown=4; }
            if (0 <= c && c <= 127)
                putchar(c);
            else
                printf("%s", encode_ext(c, unicode));
            p += 2;
        }
        printf("\x1b[0m\n");
    }
}

void color_endoom(unsigned char *endoom, int unicode) {
    int current_fg = -1;
    int current_bg = -1;
    unsigned char *p = endoom;
    for (int j = 0; j < 25; j++) {
        for (int i = 0; i < 80; i++) {
            int c = *p++;
            int ctrl = *p++;
            int fg = ctrl & 0x0f;
            int bg = (ctrl & 0x70) >> 4;
            //int blink = !!(ctrl & 0x80);

            if (bg != current_bg || fg != current_fg) {
                current_bg = bg;
                current_fg = fg;
                if (bg == 0) {
                    printf("\x1b[0m");
                    printf("\x1b[%sm", encode_color(fg,0));
                }
                else {
                    printf("\x1b[%s;%sm", encode_color(bg,1), encode_color(fg,0));
                }
            }

            if (0 <= c && c <= 127)
                putchar(c);
            else
                printf("%s", encode_ext(c, unicode));
        }
        printf("\x1b[0m\n");
    }
    printf("\x1b[0m");
}

void I_QuitEndoom(unsigned char *endoom, int len) {

    if (!show_endoom) goto quit;
    if (len != 4000) goto quit;

    int unicode;
    int color;
    int rev;

    detect_term(&rev, &color, &unicode);

    if (!color && !rev) spartan_endoom(endoom, unicode);
    else if (!color)    reverse_endoom(endoom, unicode);
    else                color_endoom(endoom, unicode);

    quit: I_Quit();

}
