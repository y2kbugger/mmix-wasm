/*
    Copyright 2005 Martin Ruckert
    
    ruckertm@acm.org

    This file is part of the MMIX Motherboard project

    This file is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This software is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this software; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

*/

#ifndef BUS_ARITH_H
#define BUS_ARITH_H



/* utility function to pack/unpack an integer into a four byte big endian buffer */
extern void inttochar(int val, unsigned char buffer[4]);
extern int chartoint(const unsigned char buffer[4]);
/* macro versions */
#define HITETRA(x) ((unsigned int)(sizeof(x)>4?(((x)>>32)&0xFFFFFFFF):0))
#define LOTETRA(x) ((unsigned int)((x)&0xFFFFFFFF))
#define HIWYDE(x) ((unsigned int)(((x)>>16)&0xFFFF))
#define LOWYDE(x) ((unsigned int)((x)&0xFFFF))
#ifndef HIBYTE
#define HIBYTE(x) ((unsigned int)(((x)>>8)&0xFF))
#define LOBYTE(x) ((unsigned int)((x)&0xFF))
#endif

#define GET2(a)   ((unsigned int)(((a)[0]<<8)+(a)[1]))
#define GET4(a)   ((unsigned int)(((a)[0]<<24)+((a)[1]<<16)+((a)[2]<<8)+(a)[3]))
#define GET8(a)   ((uint64_t)((((uint64_t)GET4(a))<<32)+GET4((a)+4)))

#define SET2(a,x) ((a)[0]=HIBYTE(x),(a)[1]=LOBYTE(x))
#define SET4(a,x) (SET2(a,HIWYDE(x)),SET2((a)+2,LOWYDE(x)))
#define SET8(a,x) (SET4(a,HITETRA(x)),SET4((a)+4,LOTETRA(x)))


/* utility functions to compare addresses */
extern int less_equal(unsigned char low[8],unsigned char addr[8]);
extern int equal(unsigned char addr1[8],unsigned char addr2[8]);
extern int less(unsigned char low[8],unsigned char addr[8]);
extern int in_range(unsigned char low[8], unsigned char addr[8], unsigned char hi[8]);

/* converting a hex string to/from a byte array */
extern int fromhex(char c);
extern char tohex(int i);
extern char *hextochar(char *hex, unsigned char buffer[], int size);
extern void chartohex(unsigned char buffer[], char hex[], int size);

/* converting a hex string to/from an integer */
extern char *hextoint(char *hex, int *i);
extern void inttohex (int from, char *to);


void add_offset(unsigned char base[8],  unsigned int size, unsigned char address[8]);
extern unsigned int hi_offset;
extern unsigned int overflow_offset;
extern unsigned int get_offset(unsigned char base[8], unsigned char address[8]);


#endif
