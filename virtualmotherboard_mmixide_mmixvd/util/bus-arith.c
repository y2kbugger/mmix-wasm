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

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "bus-arith.h"


/* functions to test for an address range */

int equal(unsigned char addr1[8],unsigned char addr2[8])
{ int i;
  for (i=0;i<8;i++)
    if (addr1[i]!=addr2[i]) return 0;
  return 1;
}

int less(unsigned char low[8],unsigned char addr[8])
{ int i;
  for (i=0;i<8;i++)
  { if (low[i]<addr[i]) return 1;
    if (low[i]>addr[i]) return 0;
  }
  return 0;
}

int less_equal(unsigned char low[8],unsigned char addr[8])
{ int i;
  for (i=0;i<8;i++)
  { if (low[i]<addr[i]) return 1;
    if (low[i]>addr[i]) return 0;
  }
  return 1;
}


int in_range(unsigned char low[8],
             unsigned char addr[8],
             unsigned char hi[8])
{ return less_equal(low, addr) && less(addr,hi);
}


void add_offset(unsigned char base[8],  unsigned int size, unsigned char address[8])
     /* stores into address the sum of base+size
        where size is the range in byte.
        base and address may be the same.
      */
{
  int i;
  unsigned char sum;
  for(i=7; i >= 0; i--)
  { sum = base[i] + (size&0xFF);
    size = size >> 8;
    if (base[i] > sum) size++; /* carry */
    address[i]=sum;
  }
}

unsigned int hi_offset;
unsigned int overflow_offset;

unsigned int get_offset(unsigned char base[8], unsigned char address[8])
     /* compute a 64 bit offset such that base+offset == address
        the low 32 bit are returned the hi 32 bit are in the variable hi_offset
        the overflow in overflow_offset.
    */

{ unsigned int base_h = chartoint(base);
  unsigned int base_l = chartoint(base+4);
  unsigned int address_h = chartoint(address);
  unsigned int address_l = chartoint(address+4);
  unsigned int offset = address_l - base_l;
  hi_offset = address_h- base_h;
  if (offset > address_l) hi_offset--;
  overflow_offset = (hi_offset>address_h);
  return offset;
}   




/* conversion of hexadecimal represenation to/from byte representation */

int fromhex(char c)
{ if ('0' <= c && c <= '9') return c -'0';
  if ('a'<= c && c <= 'f') return c -'a'+10;
  if ('A'<= c && c <= 'F') return c -'A'+10;
  return -1;
}

char tohex(int i)
{ static char digits[]="0123456789abcdef";
  if (0<=i && i<=15) return digits[i];
  else return ' ';
}


char *hextoint(char *hex, int *i)
     /* puts the value of the hex string into i.
        use at most 8 hex digits
        returns a pointer past the end of the hex string.
   */
{ int n=8;
  *i=0;
  while(n-- > 0 && isxdigit(*hex))
    *i = (*i << 4)| fromhex(*hex++);
  return hex;
}


char *hextochar(char *hex, unsigned char *buffer, int size)
     /* puts the value of the hex sting right aligned into buffer.
        assumes a buffer of size byte. fills the buffer with leading zeros
        if the hex string is too short. 
        returns a pointer past the end of the hex string.
   */
{ int digits;
  char *end;
  for (digits=0;digits<size*2; digits++) if(!isxdigit(hex[digits])) break;
  end = hex+digits;
  while (size>0 && digits>0)
  { size--;
    digits--;
    buffer[size]= fromhex(hex[digits]);
      if (digits>0)
     buffer[size] = buffer[size] | (fromhex(hex[--digits])<<4);
  }
  while (size>0)
    buffer[--size]= 0;
  return end;
}

void chartohex(unsigned char buffer[], char hex[], int size)
     /* writes a hexadecimal representation of size bytes from buffer 
        into hex (which is assumed to hold a string of length size*2
        and a trailing zero byte */
{ int i;
 for (i=0;i<size;i++)
   { hex[2*i] = tohex((buffer[i]>>4) & 0xF);
     hex[2*i+1] = tohex(buffer[i] & 0xF);
   }
 hex[2*i]=0;
}

void inttohex (int from, char *to)
{
  int nib;
  char ch;
  int n=4;
  while (n--)
    {
      ch = (from >> (8*n))&0xFF;
      nib = ((ch & 0xf0) >> 4) & 0x0f;
      *to++ = tohex (nib);
      nib = ch & 0x0f;
      *to++ = tohex (nib);
    }
  *to++ = '\0';
}

