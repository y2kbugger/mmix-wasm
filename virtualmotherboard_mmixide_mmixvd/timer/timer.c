/*
    Copyright 2011  Martin Ruckert
    
    ruckertm@acm.org

    This file is part of the Virtual Motherboard project

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

#ifdef WIN32
#include <windows.h>
extern HWND hMainWnd;
#include "winopt.h"
#include "inspect.h"
#else
#include <unistd.h>
/* make mem_update a no-op */
#define  mem_update(offset,size)
#endif
#include <string.h>
#include "vmb.h"
#include "bus-arith.h"
#include "param.h"
#include "option.h"
#include "timer.h"

/* contains operating system independent timer code */

#define TIMER_MEM	0x20
unsigned char tmem[TIMER_MEM] = {0};
/* memory layout:
tmem[0]	2 Byte int current year
tmem[1]
tmem[2] current month 0=January - 11=December
tmem[3] current day

tmem[4] (unix only) 1 if daylight savings time else 0
tmem[5] (unix only) 2 byte current day of the year
tmem[6] 
tmem[7] current day of the week 0=Sunday - 6=Saturday

tmem[8] zero
tmem[9] current hour
tmem[A] current minute
tmem[B] current second

tmem[C]  4 Byte int, current total time since midnight in milliseconds
tmem[D]
tmem[E]
tmem[F]

tmem[10] 4 Byte int, offset
tmem[11]
tmem[12]
tmem[13]

tmem[14] 4 Byte int, interval
tmem[15]
tmem[16]
tmem[17]

tmem[18] 4 Byte int, t0
tmem[19]
tmem[1A]
tmem[1B]

tmem[1C] 4 Byte int, dt
tmem[1D]
tmem[1E]
tmem[1F]

see help.html
*/
#ifdef WIN32
#define TIMER_REGS	14
struct register_def timer_regs[TIMER_REGS+1] = {
	/* name no offset size chunk format */
	{"Year"             ,0x00,2,wyde_chunk,unsigned_format},
	{"Month"            ,0x02,1,byte_chunk,unsigned_format},
    {"Day"              ,0x03,1,byte_chunk,unsigned_format},
    {"Day per Year"     ,0x04,2,wyde_chunk,unsigned_format},
    {"DST"              ,0x06,1,byte_chunk,unsigned_format},
    {"Weekday"          ,0x07,1,byte_chunk,unsigned_format},
    {"Hour"             ,0x09,1,byte_chunk,unsigned_format},
    {"Minute"           ,0x0A,1,byte_chunk,unsigned_format},
    {"Second"           ,0x0B,1,byte_chunk,unsigned_format},
    {"Milliseconds"     ,0x0C,4,tetra_chunk,unsigned_format},
    {"t"                ,0x10,4,tetra_chunk,unsigned_format},
    {"ti"               ,0x14,4,tetra_chunk,unsigned_format},
    {"t0"               ,0x18,4,tetra_chunk,unsigned_format},
    {"dt"               ,0x1C,4,tetra_chunk,unsigned_format},
	{0}};
#endif
unsigned int tt=0, ti=0, t0=0, dt=0; /* copies of tmem fields */

/* functions to operate the timer simulation */
static unsigned int last_time=0;
static unsigned int time_delay=0;
static unsigned int expire_time=0; 
/* setting a timer will set this to the expected expire time */

static void debug_last_time(void)
{ char tstr[15];
  int t,h,m,s,ms; 
  if (!VMB_DEBUGGING(VMB_DEBUG_INFO)) return;
  ms = last_time%1000;
  t = last_time/1000;
  s = t%60;
  t=t/60;
  m= t%60;
  t= t/60;
  h = t%24;
  sprintf(tstr,"%02d:%02d:%02d:%03d",h,m,s,ms);
  vmb_debugs(VMB_DEBUG_INFO,"Time: %s",tstr);
}



/*problem: det timer ist etwas zu schnell dadurch ist t0 
  nach der erhöhung etwas nach timer_get_now
  damit geht dann hier gar nichts mehr t0 läuft timer_get_now davon (weil
			 er aufholen will!)
*/

static void advance_time(unsigned int new_time)
{ if (((signed int)(new_time-last_time))>0) 
       /* should always happen except if last time is set by expire_time */
       /* we never step the clock backward */
    last_time=new_time;
  else
    time_delay = last_time-new_time; /* our time is past the host time */
}


void timer_start(void)
/* arrange the timer to signal at absolute time T0 + t0 +dt */
{ unsigned int d, ms;
  vmb_debugi(VMB_DEBUG_INFO,"Timer requested to  start at %u", t0 +dt); 
  update_display();
  ms = timer_get_now(); /* might be too small by timer_delay */
  d = ms-t0; /* t0 is always in the past */
  if (d>=dt)
  { vmb_debugi(VMB_DEBUG_NOTIFY,"Timer signaled in the past: %u",d-dt);
    if (time_delay>0)
    { expire_time=ms+time_delay;
      timer_set(time_delay);
      vmb_debugi(VMB_DEBUG_NOTIFY,"Timer delayed %u",time_delay);
    }
    else
    { vmb_debug(VMB_DEBUG_NOTIFY,"Timer signaled immediately");
      timer_signal(); /* too late */
    }
  }
  else
  { expire_time=t0+dt;
    timer_set(dt+time_delay-d);
    vmb_debugi(VMB_DEBUG_PROGRESS,"Timer started %u",dt+time_delay-d);
  }
}

unsigned int timer_get_now(void)
/* return the simulated time in ms since T0 
   if the host time wraps around, make sure to call this function
   often enough (once per hour) to make sure that the host time
   since T0 and the time returned from this function last time
   dont differ by (wrap around)/2 or more.
   The values from this function are monoton increasing.
  
   If timer_signal() is called
   this time will advance at least to T0+delay from the last call
   of timer_set(delay).  If this happens to early (due to jitter),
   host time and time returned from this will differ still the
   clock will not step backwards by calling it again and setting it
   from host time.
*/
{  advance_time(timer_since_T0());
   debug_last_time();
   return last_time;
}


void timer_signal()
/* raise the timer interrupt after the timer has expired */
{ advance_time(expire_time);
  vmb_raise_interrupt(&vmb,interrupt_no);
  vmb_debugi(VMB_DEBUG_PROGRESS,"Timer expired sending interrupt %d",interrupt_no);
  debug_last_time();
  t0 = t0+dt;
  dt = ti;
  SETT0(t0);
  SETDT(dt);
  if (ti==0) 
  { tt = 0;
    SETTT(0);
    update_display();
  }
  else
    timer_start();
  mem_update(0,TIMER_MEM);
}



int major_version=2, minor_version=1;
char title[] ="VMB Timer";

char version[]="$Revision: 664 $ $Date: 2018-12-10 18:10:14 +0100 (Mon, 10 Dec 2018) $";

char howto[] =
"\n"
"The program will contact the motherboard at [host:]port\n"
"and register itself with the given start address.\n"
"It will offer four octa byte at the given address.\n"
"Then, the program will answer read and write requests from the bus.\n"
"The first two octabyte contain the host date and time\n"
"The next two octa implement a millisecond timer\n"
"\n"
;

/* command interface to the timer simulation */

/* Interface to the virtual motherboard */

unsigned char *timer_get_payload(unsigned int offset,int size)
{  vmb_debug(VMB_DEBUG_INFO,"Reading timer information");
   if (offset<0x10)
   { timer_get_DateTime();
     if (offset<0x08)
     { vmb_debugi(VMB_DEBUG_INFO,"year:        %d", YEAR);
       vmb_debugi(VMB_DEBUG_INFO,"month:       %d", MONTH);
       vmb_debugi(VMB_DEBUG_INFO,"day:         %d", DAY);
       vmb_debugi(VMB_DEBUG_INFO,"weekday:     %d", WEEKDAY);
     }
     if (offset<0x0C && offset+size>0x08)
     { vmb_debugi(VMB_DEBUG_INFO,"hour:        %d", HOUR);
       vmb_debugi(VMB_DEBUG_INFO,"minute:      %d", MIN);
       vmb_debugi(VMB_DEBUG_INFO,"second:      %d", SEC);
     }
     if (offset+size>0x0C)
       vmb_debugi(VMB_DEBUG_INFO,"millisecond: %d", MILLISEC);
   }
   if  (offset+size>0x10)
   { int d = offset<0x10?0x10-offset:0;
     vmb_debugx(VMB_DEBUG_INFO,"extended information: %s",tmem+offset+d,size-d);
   }
   mem_update(0,TIMER_MEM);
   return tmem+offset;
}


void timer_put_payload(unsigned int offset,int size, unsigned char *payload)
{ vmb_debug(VMB_DEBUG_INFO,"Writing timer information");
  if (!vmb.power)
  { vmb_debug(VMB_DEBUG_NOTIFY,"Power off, Write ignored.");
	return;
  }
  if (offset<0x10)  /* the first 16 byte are read only */
  { int d = 0x10-offset;
    offset = offset+d;
    size = size-d;
    vmb_debugi(VMB_DEBUG_NOTIFY,"Writing of %d byte readonly data ignored.",d);
  }
  if (size>0)
  { int to_tt=offset<0x14; /* write to offset */
	int to_ti=offset<0x18&& offset+size>0x14; 
	int to_t0= offset<0x1C&& offset+size>0x18; 
	int to_dt= offset+size>0x1C;
	memmove(tmem+offset,payload, size);
    vmb_debugi(VMB_DEBUG_PROGRESS,"Writing %d byte of data.",size);

    if (to_ti) {
      ti = TI;
      vmb_debugi(VMB_DEBUG_INFO,"Writing %d to i",ti);
    }
    if (to_tt) tt = TT;
    if (to_tt && tt==0)
    {  vmb_debug(VMB_DEBUG_INFO,"Writing zero to t");
       dt = tt;
       SETDT(dt);
       timer_stop();
       vmb_debug(VMB_DEBUG_PROGRESS,"Timer stopped");
    }
    else
    { if (to_t0 || to_dt)
      { /* takes precedence over writing to tt */
        t0 = TT0;
	    dt = TDT;
        tt = 1;
        SETTT(tt);
        vmb_debug(VMB_DEBUG_INFO,"Writing to t0/dt");
        timer_start();
      } 
      else if (to_tt && tt!=0)
      { t0 = timer_get_now();
        dt = tt;
        SETT0(t0);
        SETDT(dt);
        vmb_debug(VMB_DEBUG_INFO,"Writing to t, set t0/dt");
        timer_start();
      }
    }
	mem_update(0,TIMER_MEM);
  }
}

static void clear_timer(void)
{ tt = ti = t0 = dt = 0;
  SETTT(tt);
  SETTI(ti);
  SETT0(t0);
  SETDT(dt);
  update_display();
  mem_update(0,TIMER_MEM);
}
void timer_poweroff(void)
/* this function is called when the virtual power is turned off */
{ vmb_debug(VMB_DEBUG_PROGRESS,"Timer power off");
  if (tt!=0) 
  { timer_stop();
    vmb_debug(VMB_DEBUG_PROGRESS,"Timer stopped");
  }
  clear_timer();
#ifdef WIN32
  PostMessage(hMainWnd,WM_VMB_OFF,0,0);
#endif
}

void timer_poweron(void)
/* this function is called when the virtual power is turned off */
{ vmb_debug(VMB_DEBUG_PROGRESS,"Timer power on");
  clear_timer();
#ifdef WIN32
  PostMessage(hMainWnd,WM_VMB_ON,0,0);
#endif
}

void timer_reset(void)
{ vmb_debug(VMB_DEBUG_PROGRESS,"Timer reset");
  if (tt!=0) 
  { timer_stop();
    vmb_debug(VMB_DEBUG_PROGRESS,"Timer stopped");
  }
  clear_timer();
}

void timer_disconnected(void)
/* this function is called when the reading thread disconnects from the virtual bus. */
{ 
  vmb_debug(VMB_DEBUG_PROGRESS,"Timer disconnected");
  timer_reset();
#ifdef WIN32
  PostMessage(hMainWnd,WM_VMB_DISCONNECT,0,0);
#endif
}

#ifdef WIN32
static int timer_mem_read(unsigned int offset, int size, unsigned char *buf)
{ if (offset+size>sizeof(tmem))
    size=sizeof(tmem)-offset;
  if (size<=0) return 0;
  memmove(buf,tmem+offset,size);
  return size;
}


struct inspector_def inspector[3] = {
    /* name size get_mem load store  format chunk de_offset sb_rng num_regs regs address*/
	{"Memory",TIMER_MEM,timer_mem_read,timer_get_payload,timer_put_payload,hex_format, byte_chunk,8},
	{"Registers",TIMER_MEM,timer_mem_read,timer_get_payload,timer_put_payload,hex_format, byte_chunk,8,TIMER_REGS,timer_regs},
	{0}
};

#endif

void init_device(device_info *vmb)
{ vmb_debug(VMB_DEBUG_PROGRESS,"Timer initializing");
  tt = ti = t0 = dt = 0;
  memset(tmem,0,sizeof(tmem));
  timer_set_T0();
  last_time = timer_since_T0();
  time_delay = 0;
  timer_get_DateTime();
  vmb_size = TIMER_MEM;
  vmb->poweron=timer_poweron;
  vmb->poweroff=timer_poweroff;
  vmb->disconnected=timer_disconnected;
  vmb->reset=timer_reset;
  vmb->terminate=timer_terminate;
  vmb->put_payload=timer_put_payload;
  vmb->get_payload=timer_get_payload;
#ifdef WIN32
  inspector[0].address=vmb_address;
  inspector[1].address=vmb_address;
#endif
}

