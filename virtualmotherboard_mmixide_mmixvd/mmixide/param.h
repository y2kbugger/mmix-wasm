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

/* param.h
 */
#ifndef PARAM_H
#define PARAM_H 

#ifdef VMB
#include "option.h"
extern char *host;
extern int port;

extern uint64_t vmb_address;
extern unsigned int vmb_size;
extern int interrupt_no;
extern int disable_interrupt;
extern char *vmb_filename;

extern void store_command(char *command);

#endif
extern int xpos,ypos;
extern int width,height;
extern int minimized;


#define MAXARG 256
extern int mk_argv(char *argv[MAXARG],char *command, int unquote);

#ifdef WIN32
extern void param_init(void);
#else
extern void param_init(int argc, char *argv[]);
#endif

extern void usage(char *message);

extern char version[];
extern char howto[];
extern char title[];
extern int major_version, minor_version;
extern void do_program(char * arg);
extern void parse_commandline(int argc, char *argv[]);
extern int do_define(char *arg);
#endif
