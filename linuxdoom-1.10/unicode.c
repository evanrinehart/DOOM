#include <stdint.h>
#include <stddef.h>

int encode_single_utf8(long codepoint, unsigned char *buf)
{
    if (codepoint < 0 || codepoint > 0x10FFFF)
        return -1;

    // UTF-16 surrogate halves are invalid Unicode scalar values.
    if (codepoint >= 0xD800 && codepoint <= 0xDFFF)
        return -1;

    if (codepoint <= 0x7F) {
        buf[0] = (unsigned char)codepoint;
        return 1;
    }

    if (codepoint <= 0x7FF) {
        buf[0] = 0xC0 | (codepoint >> 6);
        buf[1] = 0x80 | (codepoint & 0x3F);
        return 2;
    }

    if (codepoint <= 0xFFFF) {
        buf[0] = 0xE0 | (codepoint >> 12);
        buf[1] = 0x80 | ((codepoint >> 6) & 0x3F);
        buf[2] = 0x80 | (codepoint & 0x3F);
        return 3;
    }

    buf[0] = 0xF0 | (codepoint >> 18);
    buf[1] = 0x80 | ((codepoint >> 12) & 0x3F);
    buf[2] = 0x80 | ((codepoint >> 6) & 0x3F);
    buf[3] = 0x80 | (codepoint & 0x3F);
    return 4;
}

// also works on incomplete potentially valid sequences we might have
int measure_last_utf8_sequence(const unsigned char *buf, size_t len) {
    if (len == 0) return 0;
    size_t i = len - 1;
    int n = 1;
    while (n < 4 && i > 0 && (buf[i] & 0xC0) == 0x80) { i--; n++; }
    return n;
}

int get_utf8_size(unsigned char c) {
    if (c <= 0x7F) return 1;
    else if (c >= 0xC2 && c <= 0xDF) return 2;
    else if (c >= 0xE0 && c <= 0xEF) return 3;
    else if (c >= 0xF0 && c <= 0xF4) return 4;
    else return -1;
}

long decode_single_utf8_1(const unsigned char *s);
long decode_single_utf8_2(const unsigned char *s);
long decode_single_utf8_3(const unsigned char *s);
long decode_single_utf8_4(const unsigned char *s);

long decode_single_utf8(const unsigned char *buf, size_t len) {
    if (len==0) return -1;
    int size = get_utf8_size(buf[0]);
    switch (size) {
        case 1: return decode_single_utf8_1(buf);
        case 2: return len < 2 ? -1 : decode_single_utf8_2(buf);
        case 3: return len < 3 ? -1 : decode_single_utf8_3(buf);
        case 4: return len < 4 ? -1 : decode_single_utf8_4(buf);
        default: return -1;
    }
}

long decode_single_utf8_1(const unsigned char *s)
{
    return (s[0] <= 0x7F) ? s[0] : -1;
}

long decode_single_utf8_2(const unsigned char *s)
{
    if (s[0] < 0xC2 || s[0] > 0xDF) return -1;
    if ((s[1] & 0xC0) != 0x80) return -1;

    uint32_t cp = 0;
    cp |= (uint32_t)(s[0] & 0x1F) << 6;
    cp |= (uint32_t)(s[1] & 0x3F);

    return (long)cp;
}

long decode_single_utf8_3(const unsigned char *s)
{
    if (s[0] < 0xE0 || s[0] > 0xEF) return -1;
    if ((s[1] & 0xC0) != 0x80 || (s[2] & 0xC0) != 0x80) return -1;

    /* Reject overlong encodings. */
    if (s[0] == 0xE0 && s[1] < 0xA0) return -1;

    /* Reject UTF-16 surrogate halves. */
    if (s[0] == 0xED && s[1] >= 0xA0) return -1;

    uint32_t cp = 0;
    cp |= (uint32_t)(s[0] & 0x0F) << 12;
    cp |= (uint32_t)(s[1] & 0x3F) << 6;
    cp |= (uint32_t)(s[2] & 0x3F);

    return (long)cp;
}

long decode_single_utf8_4(const unsigned char *s)
{
    if (s[0] < 0xF0 || s[0] > 0xF4) return -1;
    if ((s[1] & 0xC0) != 0x80) return -1;
    if ((s[2] & 0xC0) != 0x80) return -1;
    if ((s[3] & 0xC0) != 0x80) return -1;

    /* Reject overlong encodings. */
    if (s[0] == 0xF0 && s[1] < 0x90) return -1;

    /* Reject code points above U+10FFFF. */
    if (s[0] == 0xF4 && s[1] > 0x8F) return -1;

    uint32_t cp = 0;
    cp |= (uint32_t)(s[0] & 0x07) << 18;
    cp |= (uint32_t)(s[1] & 0x3F) << 12;
    cp |= (uint32_t)(s[2] & 0x3F) << 6;
    cp |= (uint32_t)(s[3] & 0x3F);

    return (long)cp;
}


long latin_toupper(long codepoint)
{

    uint32_t ch = codepoint;

    if (ch >= 'a' && ch <= 'z') return ch - 0x20;

    switch (ch) {
        /* Spanish */
        case 0x00E1: return 0x00C1; // á
        case 0x00E9: return 0x00C9; // é
        case 0x00ED: return 0x00CD; // í
        case 0x00F3: return 0x00D3; // ó
        case 0x00FA: return 0x00DA; // ú
        case 0x00FC: return 0x00DC; // ü
        case 0x00F1: return 0x00D1; // ñ

        /* French */
        case 0x00E0: return 0x00C0; // à
        case 0x00E2: return 0x00C2; // â
        case 0x00E4: return 0x00C4; // ä
        case 0x00E6: return 0x00C6; // æ
        case 0x00E7: return 0x00C7; // ç
        case 0x00E8: return 0x00C8; // è
        case 0x00EA: return 0x00CA; // ê
        case 0x00EB: return 0x00CB; // ë
        case 0x00EE: return 0x00CE; // î
        case 0x00EF: return 0x00CF; // ï
        case 0x00F4: return 0x00D4; // ô
        case 0x00F6: return 0x00D6; // ö
        case 0x00F9: return 0x00D9; // ù
        case 0x00FB: return 0x00DB; // û
        case 0x00FF: return 0x0178; // ÿ
        case 0x0153: return 0x0152; // œ

        default: return ch;
    }
}

long latin_fallback(long codepoint)
{

    uint32_t ch = codepoint;

    /* ASCII */
    if (ch < 0x80)
        return (char)ch;

    switch (ch) {
        /* A */
        case 0x00C0: case 0x00C1: case 0x00C2: case 0x00C4:
        case 0x00E0: case 0x00E1: case 0x00E2: case 0x00E4:
        case 0x00C6: case 0x00E6:     /* Æ/æ */
            return 'A';

        /* C */
        case 0x00C7: case 0x00E7:
            return 'C';

        /* E */
        case 0x00C8: case 0x00C9: case 0x00CA: case 0x00CB:
        case 0x00E8: case 0x00E9: case 0x00EA: case 0x00EB:
            return 'E';

        /* I */
        case 0x00CC: case 0x00CD: case 0x00CE: case 0x00CF:
        case 0x00EC: case 0x00ED: case 0x00EE: case 0x00EF:
            return 'I';

        /* N */
        case 0x00D1: case 0x00F1:
            return 'N';

        /* O */
        case 0x00D2: case 0x00D3: case 0x00D4: case 0x00D6:
        case 0x00F2: case 0x00F3: case 0x00F4: case 0x00F6:
        case 0x0152: case 0x0153:     /* Œ/œ */
            return 'O';

        /* U */
        case 0x00D9: case 0x00DA: case 0x00DB: case 0x00DC:
        case 0x00F9: case 0x00FA: case 0x00FB: case 0x00FC:
            return 'U';

        /* Y */
        case 0x0178: case 0x00FF:
            return 'Y';

        default:
            return '?';
    }
}
