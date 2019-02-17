/*
    Copyright 2008  Martin Ruckert
    
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

/* the files maintemplate and devicetemplate
   is a template for a simple device, containing just what is
   necessary. Everyting else was omited.
   Its a good starting pont for complex devices and a complete
   reference to the functions needed and provided by the
   vmb library.
*/
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <signal.h>
#include "vmb.h"
#include "option.h"
#include "param.h"
#include "bus-arith.h"
#include "timer.h"

/* implementing the timer */

static pthread_mutex_t action_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t action_cond = PTHREAD_COND_INITIALIZER;
static void clean_up_action_mutex(void *_dummy)
{ pthread_mutex_unlock(&action_mutex); /* needed if canceled waiting */
}

static volatile enum action { idle=0, expired, quit } event_action = idle;

void alarm_handler(int signum)
{ int rc;
  event_action=expired;
  rc = pthread_cond_signal(&action_cond);
  if (rc) 
    vmb_debug(VMB_DEBUG_ERROR, "signaling failed");
}


static void set_handler(void)
{ static struct sigaction action;
  sigset_t block_mask;

  event_action = idle;
   
  sigemptyset (&block_mask);
  action.sa_mask = block_mask;

  action.sa_flags = SA_RESTART;
  action.sa_handler = alarm_handler;
  sigaction(SIGALRM,&action,NULL);
}


static void wait_for_action(void)
{ vmb_debug(VMB_DEBUG_INFO, "waiting ...");
  { int rc = pthread_mutex_lock(&action_mutex);
    if (rc) 
    { vmb_error(__LINE__,"Locking action mutex failed");
      pthread_exit(NULL);
    }
  }
  pthread_cleanup_push(clean_up_action_mutex,NULL);
  /* in the meantime the event might have happend */
  if (event_action == idle)
     pthread_cond_wait(&action_cond,&action_mutex);
  pthread_cleanup_pop(1);
  vmb_debug(VMB_DEBUG_INFO, "done waiting.");
}




/* things we have to provide */

device_info vmb = {0};

extern void update_display(void)
/* make sure things get reflected in the GUI */
{ }

static struct timeval T0; /* the global T0 of my program at0:00:00 midnight*/

extern void timer_set_T0(void)
/* initialize host time and set the host time Reference T0 */
{ int r;
  struct tm D;
  r = gettimeofday(&T0,NULL); 
  if (r!=0)
     vmb_error(__LINE__,"Setting T0 failed");
  D = *localtime(&T0.tv_sec);
  T0.tv_usec = 0;
  T0.tv_sec = T0.tv_sec - D.tm_hour*60*60 - T0.tv_sec%(60*60);
}

extern unsigned int timer_since_T0(void)
/* return the host time in ms since T0 */ 
{ int r;
  struct timeval t0;
  r = gettimeofday(&t0,NULL); 
  if (r!=0)
     vmb_error(__LINE__,"Getting time failed");
  return ((t0.tv_sec-T0.tv_sec)*1000 + 
          (t0.tv_usec-T0.tv_usec)/1000)%(24*60*60*1000);
}


extern void timer_set(unsigned int delay)
/* arrange the host timer to signal at absolute time T0 + delay */
{  struct itimerval delta = {{0}};
   delta.it_value.tv_sec = delay/1000;
   delta.it_value.tv_usec = (delay%1000)*1000;
   setitimer(ITIMER_REAL,&delta,NULL);

}

extern void timer_stop(void)
/* cancel a running timer */
{  struct itimerval delta = {{0}};
   setitimer(ITIMER_REAL,&delta,NULL);
}


extern void timer_get_DateTime(void)
/* set the first two octas of the timer from host time */
{ struct tm D;
  unsigned int ms;
  struct timeval t;

  gettimeofday(&t,NULL); 
  D = *localtime(&t.tv_sec);
  ms = timer_get_now();

  SETYEAR(D.tm_year+1900);
  MONTH = (unsigned char)(D.tm_mon);
  DAY = (unsigned char)D.tm_mday;
  DST = (unsigned char)D.tm_isdst;
  SETYEARDAY(D.tm_yday);
  WEEKDAY = (unsigned char)D.tm_wday;
  HOUR = (unsigned char)D.tm_hour;
  MIN = (unsigned char) D.tm_min;
  SEC = (unsigned char) D.tm_sec;
  SETMILLISEC(ms);
}


void timer_terminate(void)
{ int rc;
  vmb_debug(VMB_DEBUG_PROGRESS, "Timer terminating.");
  event_action=quit;
  rc = pthread_cond_signal(&action_cond);
  if (rc) 
    vmb_debug(VMB_DEBUG_ERROR, "signaling failed");
}


int main(int argc, char *argv[])
{
  param_init(argc, argv);
  if (vmb_verbose_flag) vmb_debug_mask=0;
  vmb_debugs(VMB_DEBUG_INFO, "%s ",vmb_program_name);
  vmb_debugs(VMB_DEBUG_INFO, "%s ", version);
  vmb_debugs(VMB_DEBUG_INFO, "host: %s ",host);
  vmb_debugi(VMB_DEBUG_INFO, "port: %d ",port);
  close(0);
  init_device(&vmb);
  vmb_debugi(VMB_DEBUG_INFO, "address hi: %x",HI32(vmb_address));
  vmb_debugi(VMB_DEBUG_INFO, "address lo: %x",LO32(vmb_address));
  vmb_debugi(VMB_DEBUG_INFO, "size: %x ",vmb_size);

  set_handler();

  vmb_connect(&vmb, host,port); 
  vmb_register(&vmb,HI32(vmb_address),LO32(vmb_address),vmb_size,
               0, 0, vmb_program_name,major_version,minor_version);

 
  while (vmb.connected) 
  { if (event_action == idle)
      wait_for_action();
    if (event_action == expired)
    { event_action = idle; 
      timer_signal();
    }
    else if  (event_action == quit)
    { vmb_disconnect(&vmb);
      break;
    }
  }

 vmb_debug(VMB_DEBUG_PROGRESS, "Timer exit.");
 return 0;
}
