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
#include <string.h>
#ifdef WIN32
#include <windows.h>
#pragma warning(disable : 4996)
#include "resource.h"
#include "winopt.h"
extern HBITMAP hBmpActive, hBmpInactive;
#include "inspect.h"
#else
#include <unistd.h>
#include <termios.h>
/* make mem_update a no-op */
#define  mem_update(offset,size)
#endif

#include "bus-arith.h"
#include "option.h"
#include "param.h"
#include "vmb.h"

void display_char(char c);

int major_version=2, minor_version=1;
char title[] ="VMB Keyboard";

char version[]="$Revision: 664 $ $Date: 2018-12-10 18:10:14 +0100 (Mon, 10 Dec 2018) $";

char howto[] =
"\n"
"The program will contact the motherboard at [host:]port\n"
"and register itself with the given address and interrupt.\n"
"Then, the program will read bytes from standard input.\n"
"For each byte read it will signal an interrupt with the\n"
"given interrupt number.\n"
"At the given address it will provide a read only octabyte\n"
"in the following format:\n"
"\n"
"   XX00 00YY 0000 00ZZ\n"
"\n"
"The XX byte signals errors it will be 0x80 if an error occured\n"
"and 0x00 otherwise.\n"
"\n"
"The YY byte contains the number of characters taken from \n"
"the input since the last read operation.  This should be 0\n"
"if no new character was received and 1 if one charcter was\n"
"received. Any other value will indicate that character were\n"
"lost since the last read operation.\n"
"\n"
"The ZZ byte contains the last character received. It is valid only\n"
"if YYYY is not zero.\n"
"\n"
"The complete ocatbyte will be reset to zero after a read operation to ZZ.\n"
"\n"
;

extern device_info vmb;

static unsigned char data[8];
#define ERROR 0
#define COUNT 3
#define EXTENSION 6
#define DATA  7


#define MAXIBUFFER (32*1024)
static char input_buffer[MAXIBUFFER];
static int input_buffer_first=0, input_buffer_last=0;

/* Interface to the virtual motherboard */

unsigned char *kb_get_payload(unsigned int offset,int size)
{  
    static unsigned char payload[8];
    
    memmove(payload,data,8);
    if(offset+size>=8) 
	{ memset(data,0,8);    /* reset to zero */
      if (input_buffer_first<input_buffer_last)
	  { data[DATA] = input_buffer[input_buffer_first++];
	    data[EXTENSION] = 0;
        data[COUNT] = 1;
        vmb_raise_interrupt(&vmb,interrupt_no);
        vmb_debug(VMB_DEBUG_INFO, "Raised interrupt");  
	  }
	  mem_update(0,8);
	}
    return payload+offset;
}

static void restore_input(void);

void kb_terminate(void)
/* this function is called when the motherboard politely asks the device to terminate.*/
{ 
#ifdef WIN32
   PostMessage(hMainWnd,WM_CLOSE,0,0);
#else
   restore_input();
   vmb_debug(VMB_DEBUG_INFO, "Closing Input.");  
   close(0);
#endif

}


void process_input_file(char *filename)
{ FILE *f;
  if (filename==NULL) return;
  f = vmb_fopen(filename,"rb");
  if (f==NULL) {vmb_debug(VMB_DEBUG_ERROR, "Unable to open input file"); return;}
  input_buffer_first = 0;
  input_buffer_last = (int)fread(input_buffer,1,MAXIBUFFER,f);
  if (input_buffer_last<0)  vmb_debug(VMB_DEBUG_ERROR, "Unable to read input file");
  if (input_buffer_last==0) {vmb_debug(VMB_DEBUG_NOTIFY, "Empty file"); return;}
  if (input_buffer_last==MAXIBUFFER) 
	  vmb_debugi(VMB_DEBUG_ERROR, "Maximum File size %d reached, File truncated",MAXIBUFFER);
  fclose(f);
  data[DATA] = input_buffer[input_buffer_first++];
  data[EXTENSION] = 0;
  if (data[COUNT]<0xFF) data[COUNT]++;
  if (data[COUNT]>1) data[ERROR] = 0x80;
  mem_update(0,8);
  vmb_raise_interrupt(&vmb,interrupt_no);
  vmb_debugi(VMB_DEBUG_PROGRESS, "Raised interrupt %d", interrupt_no);
}

void process_input(unsigned char c, unsigned char extended) 
{ /* The keyboard Interface */
  
  if (extended != 0 || (c<0x20 || c >= 0x7F))
    vmb_debugi(VMB_DEBUG_PROGRESS, "input (#%04x)\n",(extended<<8)|c);
  else
    vmb_debugi(VMB_DEBUG_PROGRESS, "input %c",c);
  if (input_buffer_first < input_buffer_last)
	  vmb_debugi(VMB_DEBUG_NOTIFY, "Ignored. Still %d characters in the input file buffer",input_buffer_last-input_buffer_first);
  else if (vmb.power)
  { data[DATA] = c;
    data[EXTENSION]=extended;
    if (data[COUNT]<0xFF) data[COUNT]++;
    if (data[COUNT]>1) data[ERROR] = 0x80;
	mem_update(0,8);
    vmb_raise_interrupt(&vmb,interrupt_no);
    vmb_debugi(VMB_DEBUG_PROGRESS, "Raised interrupt %d", interrupt_no);
  }
  else
  { vmb_debug(VMB_DEBUG_NOTIFY, "No power, character ignored");
#ifdef WIN32
   Beep(800,100);
#else
#endif
  }
}


#ifdef WIN32
#else

static struct termios oldtio;

static void restore_input(void)
{ tcsetattr(0,TCSADRAIN,&oldtio);
  vmb_debug(VMB_DEBUG_INFO, "Terminal restored.");
}

 
static void prepare_input(void)
{ struct termios newtio;
  
  tcgetattr(0,&oldtio); /* save current port settings */

  memset(&newtio, 0, sizeof(newtio));
  newtio.c_cflag = CS8 | CLOCAL | CREAD; /* input modes */
  newtio.c_iflag = 0; /* IGNBRK; */ /* control modes */
  newtio.c_oflag = 0;      /* output modes */

  /* set input mode (non-canonical, no echo,...) */
  newtio.c_lflag = 0;     /* local modes */
 
  /* control characters */
  newtio.c_cc[VTIME]    = 20;   /* inter-character timerin 1/10 s unused */
  newtio.c_cc[VMIN]     = 0;   /* blocking read until 0 chars received */

  tcflush(0, TCIFLUSH);
  tcsetattr(0,TCSANOW,&newtio);
  atexit(restore_input);
}
#endif

#ifdef WIN32
struct register_def kbd_regs[] = {
	/* name no offset size chunk format */
	{"Error" ,ERROR,1,byte_chunk,hex_format},
	{"Count" ,COUNT,1,byte_chunk,unsigned_format},
    {"Char"  ,DATA,1,byte_chunk,ascii_format},
    {"Char"  ,DATA,1,byte_chunk,hex_format},
	{0}};

int kbd_reg_read(unsigned int offset, int size, unsigned char *buf)
{ if (offset>8) return 0;
  if (offset+size>8) size =8-offset;
  memmove(buf,data+offset,size);
  return size;
}
struct inspector_def inspector[2] = {
    /* name size get_mem address num_regs regs */
	{"Registers",5*8,kbd_reg_read,kb_get_payload,NULL,0,0,8,4,kbd_regs},
	{0}
};

#endif

void kbd_poweron(void)
/* this function is called when the virtual power is turned on */
{ /* do nothing */
	memset(data,0,8);
	mem_update(0,8);
#ifdef WIN32
   PostMessage(hMainWnd,WM_VMB_ON,0,0);
#endif
}
void kbd_reset(void)
/* this function is called when the virtual power is turned on */
{ /* do nothing */
	memset(data,0,8);
	mem_update(0,8);
#ifdef WIN32
   PostMessage(hMainWnd,WM_VMB_RESET,0,0);
#endif
}
void init_device(device_info *vmb)
{ vmb_size = 8;
#ifdef WIN32
  hBmpActive = (HBITMAP)LoadImage(hInst, MAKEINTRESOURCE(IDB_BITMAPACTIVE), 
                                IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
  hBmpInactive=hBmp;

#else
   prepare_input();
#endif
  vmb->poweron=kbd_poweron;
  vmb->poweroff=vmb_poweroff;
  vmb->reset=kbd_reset;
  vmb->disconnected=vmb_disconnected;
  vmb->terminate=kb_terminate;
  vmb->get_payload=kb_get_payload;
#ifdef WIN32
  inspector[0].address=vmb_address;
#endif
}



#ifdef WIN32
#else

device_info vmb = {0};

int main(int argc, char *argv[])
{
  param_init(argc, argv);
  vmb_debugs(VMB_DEBUG_INFO, "%s ",vmb_program_name);
  vmb_debugs(VMB_DEBUG_INFO, "%s ", version);
  vmb_debugs(VMB_DEBUG_INFO, "host: %s ",host);
  vmb_debugi(VMB_DEBUG_INFO, "port: %d ",port);
  init_device(&vmb);
  vmb_debugi(VMB_DEBUG_INFO, "address hi: %x",HI32(vmb_address));
  vmb_debugi(VMB_DEBUG_INFO, "address lo: %x",LO32(vmb_address));
  vmb_debugi(VMB_DEBUG_INFO, "size: %x ",vmb_size);
  
  vmb_connect(&vmb,host,port); 

  vmb_register(&vmb,HI32(vmb_address),LO32(vmb_address),vmb_size,
               0, 0, vmb_program_name,major_version,minor_version);

  while (vmb.connected)
  { unsigned char c;
    int i;
    vmb_debug(VMB_DEBUG_INFO, "reading character:");
    while ((i = read(0,&c,1))==0 && vmb.connected)
      continue;
    if (i < 0)
    { vmb_error(__LINE__,"Read Error");
      break;
    }
    else if (i>0)
    { vmb_debugi(VMB_DEBUG_INFO, "got %02X",c&0xFF);
      process_input(c,0);
    }
  }
  vmb_disconnect(&vmb);
  return 0;
}


#endif

