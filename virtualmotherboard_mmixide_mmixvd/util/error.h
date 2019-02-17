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

#ifndef ERROR_H
#define ERROR_H

extern char *vmb_program_name;
extern unsigned int vmb_debug_flag;
extern unsigned int vmb_verbose_flag;
#define VMB_DEBUG_FATAL			0x0001
#define VMB_DEBUG_ERROR		    0x0002
#define VMB_DEBUG_WARN			0x0004
#define VMB_DEBUG_NOTIFY		0x0008
#define VMB_DEBUG_PROGRESS		0x0010
#define VMB_DEBUG_INFO			0x0020
#define VMB_DEBUG_MSG			0x0040
#define VMB_DEBUG_PAYLOAD		0x0080
#define VMB_DEBUG_DEFAULT		0xFFF0

/* to be used as if (vmb_debugging(VMB_DEBUG_INFO)) ... */
#define VMB_DEBUGGING(level) ((vmb_debug_flag)&&((level&~vmb_debug_mask)!=0)) 

extern unsigned int vmb_debug_mask; 

extern void vmb_message(char *msg);
extern void vmb_error(int line, char *msg);
extern void vmb_error2(int line, char *msg, char *info);
extern void vmb_errori(int line, char *msg, int code);
extern void vmb_fatal_error(int line,char *msg); /* no return */
extern void vmb_fatal_errori(int line,char *msg, int code); /* no return */
extern void vmb_debug(int level, char *msg);
extern void vmb_debugi(int level, char *msg,int i);
extern void vmb_debugs(int level, char *msg, char *s);
extern void vmb_debugx(int level, char *msg, unsigned char *s, int n); /* message with hex info */
extern void vmb_debuga(int level, char *msg, uint64_t i);
extern void vmb_debugm(int level, unsigned char mtype,unsigned char msize, 
                       unsigned char mslot,unsigned char mid,
		       unsigned char maddress[8], unsigned char *mpayload);


extern void (*vmb_message_hook)(char *msg);
extern void (*vmb_debug_hook)(char *msg);
extern void (*vmb_error_init_hook)(int i);
extern void (*vmb_exit_hook)(int i);


/* two functions to switch on and off debugging */
extern int vmb_debug_on(void);
extern int vmb_debug_off(void);


#endif
