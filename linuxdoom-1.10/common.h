// Defines suck. C sucks.
// C++ might sucks for OOP, but it sure is a better C.
// So there.
#define BASEWIDTH  320
#define BASEHEIGHT 200

#ifndef REZ_FACTOR
#define REZ_FACTOR 2
#endif

#define SCREENWIDTH  (REZ_FACTOR * BASEWIDTH)
#define SCREENHEIGHT (REZ_FACTOR * BASEHEIGHT)

// These are in values.h but we want these exact values
#define MAXCHAR		((char)0x7f)
#define MAXSHORT	((short)0x7fff)

// Max pos 32-bit int.
#define MAXINT		((int)0x7fffffff)
#define MAXLONG		((long)0x7fffffff)
#define MINCHAR		((char)0x80)
#define MINSHORT	((short)0x8000)

// Max negative 32-bit integer.
#define MININT		((int)0x80000000)
#define MINLONG		((long)0x80000000)
