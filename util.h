#ifndef UTIL_H
#define UTIL_H

#define LENGTH(x)  ((int)(sizeof (x) / sizeof *(x)))
#define MIN(a, b)  ((a) > (b) ? (b) : (a))
#define MAX(a, b)  ((a) < (b) ? (b) : (a))

/* is c the start of a utf8 sequence? */
#define ISUTF8(c)   (((c)&0xC0)!=0x80)
#define ISASCII(ch) ((unsigned char)ch < 0x80)

#endif
