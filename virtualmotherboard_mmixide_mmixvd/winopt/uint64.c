
#include "uint64.h"
#define hexdigit(c) ((c)<10? (c)+'0':(c-10)+'A')

void uint64_to_hex(uint64_t u, char *c)
/* converts to hex respresetnation. c needs at least 19 characters */
{ int i;
  c[18] = 0;
  for (i=17;i>1;i--)
  {  c[i] = hexdigit((unsigned char)(u&0xF));
     u = u>> 4;
  }
  c[1] = 'x';
  c[0] = '0';
}

uint64_t hex_to_uint64(char *str)
{ uint64_t u=0;
  while (isspace(*str)) str++;
  if (*str=='0' && (*(str+1)=='x' || *(str+1)=='X')) str=str+2;
  else if (*str=='#') str=str+1;
  while (*str!=0)
  { if (isdigit(*str))
      u=u*16+((*str)-'0');
    else if ('a'<=*str && *str<='f')
      u=u*16+((*str)-'a'+10);
    else if ('A'<=*str && *str<='F')
      u=u*16+((*str)-'A'+10);
    else
	  break;
    str++;
  }
  return u;
}

uint64_t strtouint64(char *arg)
{ uint64_t r = 0;
  while(isspace(*arg)) arg++;
  if (strncmp(arg,"0x",2)==0 || strncmp(arg,"0X",2)==0) /* hex */
  { arg = arg+2;
	while (isxdigit(*arg))
	{ unsigned int x;
	  if (isdigit(*arg)) x = *arg - '0'; 
	  else if (isupper(*arg)) x = *arg - 'A'+10;
	  else x = *arg -'a'+10;
	  r = (r<<4) + x;
	  arg++;
	}
  }
  else /* decimal */
      while (isdigit(*arg))
	  {unsigned int d;
       d = *arg -'0';
	   r = r*10+d;
	   arg++;
	  }
  return r;
}
