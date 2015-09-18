/* libutf8 © 2012-2015 Connor Lane Smith <cls@lubutu.com> */
#include "libutf.h"
#include "util.h"

int
runelen(Rune r)
{
	if(r <= 0x7F)
		return 1;
	else if(r <= 0x07FF)
		return 2;
	else if(r <= 0xD7FF)
		return 3;
	else if(r <= 0xDFFF)
		return 0; /* surrogate character */
	else if(r <= 0xFFFD)
		return 3;
	else if(r <= 0xFFFF)
		return 0; /* illegal character */
	else if(r <= Runemax)
		return 4;
	else
		return 0; /* rune too large */
}

int
runetochar(char *s, const Rune *p)
{
	Rune r = *p;

	switch(runelen(r)) {
	case 1: /* 0aaaaaaa */
		s[0] = r;
		return 1;
	case 2: /* 00000aaa aabbbbbb */
		s[0] = 0xC0 | ((r & 0x0007C0) >>  6); /* 110aaaaa */
		s[1] = 0x80 |  (r & 0x00003F);        /* 10bbbbbb */
		return 2;
	case 3: /* aaaabbbb bbcccccc */
		s[0] = 0xE0 | ((r & 0x00F000) >> 12); /* 1110aaaa */
		s[1] = 0x80 | ((r & 0x000FC0) >>  6); /* 10bbbbbb */
		s[2] = 0x80 |  (r & 0x00003F);        /* 10cccccc */
		return 3;
	case 4: /* 000aaabb bbbbcccc ccdddddd */
		s[0] = 0xF0 | ((r & 0x1C0000) >> 18); /* 11110aaa */
		s[1] = 0x80 | ((r & 0x03F000) >> 12); /* 10bbbbbb */
		s[2] = 0x80 | ((r & 0x000FC0) >>  6); /* 10cccccc */
		s[3] = 0x80 |  (r & 0x00003F);        /* 10dddddd */
		return 4;
	default:
		return 0; /* error */
	}
}
