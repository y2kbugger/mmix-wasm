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

/* things we need to make timer.c work */

extern device_info vmb;

extern void update_display(void); 
/* make sure things get reflected in the GUI */

extern void timer_set_T0(void);
/* initialize host time and set the host time Reference T0 */


extern unsigned int timer_since_T0(void);
/* return the host time in ms since T0 */ 

extern void timer_set(unsigned int delay);
/* arrange the host timer to signal at absolute time T0 + delay */

extern void timer_stop(void);
/* cancel a running timer */

extern void timer_get_DateTime(void);
/* set the first two octas of the timer from host time */

extern void timer_terminate(void);
/* clean up if the timer simulation has to end */

/* things provided for others to call */

extern unsigned int tt, ti, t0, dt; /* copies of tmem fields */

extern void init_device(device_info *vmb);

extern void timer_signal(void);
/* raise the timer interrupt after the timer has expired */

extern unsigned int timer_get_now(void);
/* calls timer_since_T0 and
   return the simulated time in ms since T0 
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

#define TIMER_MEM	0x20
extern unsigned char tmem[TIMER_MEM];

#define YEAR     GET2(tmem+0x00)
#define MONTH    (tmem[0x02])
#define DAY      (tmem[0x03])

#define YEARDAY  GET2(tmem+0x04)
#define DST      (tmem[0x06])
#define WEEKDAY  (tmem[0x07])

#define HOUR     (tmem[0x09])
#define MIN      (tmem[0x0A])
#define SEC      (tmem[0x0B])
#define MILLISEC GET4(tmem+0x0C)
#define TT       GET4(tmem+0x10)
#define TI       GET4(tmem+0x14)
#define TT0      GET4(tmem+0x18)
#define TDT      GET4(tmem+0x1C)

#define SETYEAR(x)     SET2(tmem,x)
#define SETYEARDAY(x)  SET2(tmem+0x04,x)
#define SETMILLISEC(x) SET4(tmem+0x0C,x)
#define SETTT(x)       SET4(tmem+0x10,x)
#define SETTI(x)       SET4(tmem+0x14,x)
#define SETT0(x)       SET4(tmem+0x18,x)
#define SETDT(x)       SET4(tmem+0x1C,x)

