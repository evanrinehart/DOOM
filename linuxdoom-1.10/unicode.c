#include <stdint.h>
#include <stddef.h>

int encode_single_utf8(int codepoint, unsigned char *buf)
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

int decode_single_utf8_1(const unsigned char *s);
int decode_single_utf8_2(const unsigned char *s);
int decode_single_utf8_3(const unsigned char *s);
int decode_single_utf8_4(const unsigned char *s);

int decode_single_utf8(const unsigned char *buf, size_t len) {
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

int decode_single_utf8_1(const unsigned char *s)
{
    return (s[0] <= 0x7F) ? s[0] : -1;
}

int decode_single_utf8_2(const unsigned char *s)
{
    if (s[0] < 0xC2 || s[0] > 0xDF) return -1;
    if ((s[1] & 0xC0) != 0x80) return -1;

    uint32_t cp = 0;
    cp |= (uint32_t)(s[0] & 0x1F) << 6;
    cp |= (uint32_t)(s[1] & 0x3F);

    return (int)cp;
}

int decode_single_utf8_3(const unsigned char *s)
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

    return (int)cp;
}

int decode_single_utf8_4(const unsigned char *s)
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

    return (int)cp;
}
