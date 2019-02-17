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

#include <stdlib.h>
#include <stdio.h>

#ifdef WIN32
#pragma warning(disable : 4996)
#endif

#include <string.h>
#include "vmb.h"
#include "error.h"
#include "bus-arith.h"
#include "message.h"


unsigned int vmb_debug_flag = 0;
unsigned int vmb_verbose_flag = 0;
unsigned int vmb_debug_mask = VMB_DEBUG_DEFAULT;

char *vmb_program_name = "Unknown";
void (*vmb_message_hook)(char *msg) = NULL;
void (*vmb_debug_hook)(char *msg) = NULL;
void (*vmb_error_init_hook)(int i) =NULL;
void (*vmb_exit_hook)(int i) = NULL;


int vmb_debug_on(void)
{  vmb_debug_flag = 1;
   if (vmb_error_init_hook != NULL)
     vmb_error_init_hook(vmb_debug_flag);
   return 0;
}

int vmb_debug_off(void)
{  vmb_debug_flag = 0;
   if (vmb_error_init_hook != NULL)
     vmb_error_init_hook(vmb_debug_flag);
   return 0;
}


void vmb_message(char *message)
{  if (vmb_message_hook != NULL)
     vmb_message_hook(message);
   else
 	 fprintf(stderr,"%s\r\n", message);
}

void vmb_fatal_error(int line, char *message)
{   vmb_error(line, message);
    if (vmb_exit_hook!=NULL) 
      vmb_exit_hook(1);
	else 
	  exit(1);
}
void vmb_fatal_errori(int line, char *message, int code)
{   vmb_errori(line, message,code);
    if (vmb_exit_hook!=NULL) 
      vmb_exit_hook(1);
	else 
	  exit(1);
}


void vmb_error(int line, char *message)
{ static char tmp[1000];
  sprintf(tmp,"ERROR (%s, %d): %s\r\n",vmb_program_name,line,message);
  vmb_message(tmp);
}

void vmb_error2(int line, char *message, char *info)
{ static char tmp[1000];
  sprintf(tmp,"ERROR (%s, %d): %s\r\n%s\r\n",vmb_program_name,line,message,info);
  vmb_message(tmp);
}

void vmb_errori(int line, char *message, int code)
{ static char tmp[1000];
  sprintf(tmp,"ERROR %d (%s, %d): %s\r\n",code,vmb_program_name,line,message);
  vmb_message(tmp);
}
void vmb_debug(int level, char *msg)
{ 
  if (!vmb_debug_flag) return;
  if ((level&~vmb_debug_mask)==0) return;
  if (vmb_debug_hook == NULL)
	fprintf(stderr,"DEBUG (%s): %s\n",vmb_program_name, msg);
  else  
  { vmb_debug_hook(msg);
    vmb_debug_hook("\n");
  }
}

void vmb_debugi(int level, char *msg, int i)
/* a function to call to display debug messages */
{ 
  static char tmp[1000];
  if (!vmb_debug_flag) return;
  if ((level&~vmb_debug_mask)==0) return;
  sprintf(tmp,msg,i);
  vmb_debug(level, tmp);
}

void vmb_debuga(int level, char *msg, uint64_t i)
/* a function to call to display debug messages */
{ 
  static char tmp[1000];
  if (!vmb_debug_flag) return;
  if ((level&~vmb_debug_mask)==0) return;
  sprintf(tmp,msg,HI32(i),LO32(i));
  vmb_debug(level, tmp);
}


void vmb_debugs(int level, char *msg, char *s)
/* a function to call to display debug messages */
{ 
  static char tmp[1000];
  if (!vmb_debug_flag) return;
  if ((level&~vmb_debug_mask)==0) return;
  sprintf(tmp,msg,s);
  vmb_debug(level, tmp);
}

static char *hexstr(unsigned char *s, int n)
#define HEXMAX (256*8) 
{ 
  static char hex[HEXMAX*2+ HEXMAX/8 +1];/*each character 2 hex digits,
                                          one space every eight' digit, 
                                          one zero byte */
  char *p;
  int i;
  if (n>HEXMAX) n = HEXMAX;
  i = 0;   
  hex[0] = 0;
  p = hex;
  while (n-i>=8)
  { chartohex(s+i,p,8);
    p=p+8*2;
    *p = ' ';
    p = p+1;
    i = i+8;
  }
  if (n-i>0)
  { chartohex(s+i,p,n-i);
    p = p + (n-i)*2;
  }
  *p = 0;
  return hex;
}

void vmb_debugx(int level, char *msg, unsigned char *s, int n)
     /* a function to call to display debug messages */
{  if (!vmb_debug_flag) return;
  if ((level&~vmb_debug_mask)==0) return;
  vmb_debugs(level, msg,hexstr(s,n));
}



static char *
debug_type (unsigned char mtype)
{
  static char typestr[100];
  typestr[0] = 0;
  if (mtype & TYPE_BUS)
    strcat (typestr, "BUS ");
  if (mtype & TYPE_TIME)
    strcat (typestr, "TIME ");
  if (mtype & TYPE_ADDRESS)
    strcat (typestr, "ADDRESS ");
  if (mtype & TYPE_REQUEST)
    strcat (typestr, "REQUEST ");
  if (mtype & TYPE_ROUTE)
    strcat (typestr, "ROUTE ");
  if (mtype & TYPE_PAYLOAD)
    strcat (typestr, "PAYLOAD ");
  if (mtype & TYPE_LOCK)
    strcat (typestr, "LOCK ");
  if (mtype & TYPE_UNUSED)
    strcat (typestr, "UNUSED ");
  return typestr;
}
static char *
debug_id (unsigned char id)
{
  switch (id)
  {
  case ID_IGNORE:
    return "IGNORE";
  case ID_READ:
    return "READ";
  case ID_WRITE:
    return "WRITE";
  case ID_READREPLY:
    return "READREPLY";
  case ID_NOREPLY:
    return "NOREPLY";
  case ID_READBYTE:
    return "READBYTE";
  case ID_READWYDE:
    return "READWYDE";
  case ID_READTETRA:
    return "READTETRA";
  case ID_WRITEBYTE:
    return "WRITEBYTE";
  case ID_WRITEWYDE:
    return "WRITEWYDE";
  case ID_WRITETETRA:
    return "WRITETETRA";
  case ID_BYTEREPLY:
    return "BYTEREPLY";
  case ID_WYDEREPLY:
    return "WYDEREPLY";
  case ID_TETRAREPLY:
    return "TETRAREPLY";
/* predefined IDs for BUS messages */
  case ID_REGISTER:
    return "REGISTER";
  case ID_UNREGISTER:
    return "UNREGISTER";
  case ID_INTERRUPT:
    return "INTERRUPT";
  case ID_TERMINATE:
    return "TERMINATE";
  case ID_RESET:
    return "RESET";
  case ID_POWEROFF:
    return "POWEROFF";
  case ID_POWERON:
    return "POWERON";
  default:
    return NULL;
  }
}

void vmb_debugm(int level, unsigned char mtype,unsigned char msize, unsigned char mslot,unsigned char mid,
                   unsigned char maddress[8], unsigned char *mpayload)
{ static char tmp[1000];
  int n;
  char *id_str;
  if (!vmb_debug_flag) return;
  if ((level&~vmb_debug_mask)==0) return;
  id_str = debug_id (mid);
  if (id_str == NULL) id_str = hexstr(&mid,1);
  n = sprintf(tmp,"\ttype:\t%s\r\n"
	          "\tsize:\t%d\r\n"
			  "\tslot:\t%d\r\n"
			  "\tid  :\t%s\r\n",
			  debug_type(mtype),
			  msize,
			  mslot,
			  id_str);

  if (mtype & TYPE_ADDRESS)
    n += sprintf(tmp+n,"\taddress: %s\r\n", hexstr(maddress, 8));
  if ((mtype & TYPE_PAYLOAD)&&!(VMB_DEBUG_PAYLOAD&vmb_debug_mask))
    n += sprintf(tmp+n,"\tpayload: %s\r\n", hexstr(mpayload, 8 * (msize + 1)));
#if 0
  if (mtype & TYPE_TIME)
     n += sprintf(tmp+n,"\r\n\ttime: %d", mtime);
#endif

  vmb_debug(level, tmp);
}
