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
#include <sys/types.h>
#include <ctype.h>

#ifdef WIN32
#include <windows.h>
#include <htmlhelp.h> 
#pragma warning(disable : 4996)
#else
#include <unistd.h>
#include <stdint.h>
#endif

#include "../util/vmb.h"
#include "param.h"
//#include "option.h"
//#include "bus-arith.h"



char *host=NULL;
char *vmb_filename=NULL;
int port = 9002;
int interrupt_no = 16;
int disable_interrupt = 0;
int xpos=0, ypos=0; /* Window position */
int width=64,height=64; /* Window dimensions */
int minimized = 0;  /* start the window minimized */


uint64_t vmb_address;
unsigned int vmb_size;




extern option_spec options[];

#ifdef WIN32
extern HWND hMainWnd;
#endif

void usage(char *message)
{
#if defined WIN32 && !defined _CONSOLE
 HANDLE hstdout;
 HWND ok;
 if (programhelpfile!=NULL)
 { ok=HtmlHelp(hMainWnd,programhelpfile,HH_DISPLAY_TOPIC,(DWORD_PTR)NULL) ;
   if (ok!=NULL) return;
 }
 hstdout=GetStdHandle(STD_OUTPUT_HANDLE);
 if (hstdout==NULL) return;
#endif
  printf("Version %s\n", version);
  printf("%s\n",howto);
  option_usage(stdout);
  printf("\n");

}


void do_argument(int pos, char * arg)
{ 
  vmb_debug(VMB_DEBUG_ERROR, "too many arguments\n"); 
}


int do_define(char *arg)
{ 
 if (arg==NULL || arg[0]=='-')
    return 0;
 set_option(&defined,arg);
 { char *p=defined; while (*p) { *p=tolower(*p); p++; }}
 vmb_debugs(VMB_DEBUG_PROGRESS, "Program identity: %s\n", defined);
 return 1;
}

int mk_argv(char *argv[MAXARG],char *command, int unquote)
/* splits command into arguments, knows how to handle strings.
   if unquote is true, double-quote characters are removed
   before putting them im the vector
   returns argc.
*/

{ int argc=0;  

  if (command==NULL||*command==0)
  { argv[0]=NULL;
    return argc;
  }
  for (argc=0;argc<MAXARG-1;argc++)
  { 
    while (isspace(*command)) command++;

    if (*command==0)
    { argv[argc]=NULL;
        return argc;
    }

	if( *command=='"')
	{ if (unquote) argv[argc]=++command;
	  else argv[argc]=command++;
      while (*command!='"' && *command!=0) command++;
	  if (*command=='"' && !unquote) command++;
	}
	else
	{ argv[argc]=command;
      while (!isspace(*command) && *command!=0) command++;
	}
    if (*command!=0)
    { *command=0;
      command++;
    }
  }
  argc=MAXARG-1;
  argv[argc]=NULL;
  return argc;
}



void param_init(int argc, char *argv[])
{ int i;
  option_defaults();
  do_program(argv[0]);
  if (do_define(argv[1]))  i=2; else i=1;
  parse_commandline(argc-i, argv+i);
  if (vmb_verbose_flag) vmb_debug_mask=0; 
}
