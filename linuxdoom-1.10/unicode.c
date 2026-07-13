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
