#ifndef LIBUTF_H
#define LIBUTF_H

/* libutf8 © 2012-2015 Connor Lane Smith <cls@lubutu.com> */
#include <stddef.h>
#include <stdint.h>

#if __STDC_VERSION__ >= 201112L
#include <uchar.h>
#ifdef __STDC_UTF_32__
#define RUNE_C INT32_C
typedef char32_t Rune;
#endif
#endif

#ifndef RUNE_C
#ifdef INT32_C
#define RUNE_C INT32_C
typedef uint_least32_t Rune;
#else
#define RUNE_C(x) x##L
typedef unsigned long Rune;
#endif
#endif

#define UTFmax 4 /* maximum bytes per rune */

#define Runeself 0x80             /* rune and utf are equal (<) */
#define Runemax  RUNE_C(0x10FFFF) /* maximum rune value */

int runelen(Rune r);
int runetochar(char *s, const Rune *p);

#endif
