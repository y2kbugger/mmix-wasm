#ifndef _ENDIAN_H_
#define _ENDIAN_H_

extern void inttochar(int val, unsigned char buffer[4]);
extern int chartoint(const unsigned char buffer[4]);
#if 0
/* seems like tis is unused */
extern void shorttochar(int val, unsigned char buffer[2]);
#endif
#endif