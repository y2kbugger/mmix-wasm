#ifdef WIN32
  #include <windows.h>
#else
  #include <pthread.h>
  #include <time.h>
  #include <errno.h>

  extern void clean_up_event_mutex(void *vmb);

#endif
#include <stdio.h>
#include "vmb.h"
#include "error.h"

int vmb_wait_for_event_timed(device_info *vmb, int ms)
/* waits for a  power off, reset, disconnect, or an interrupt
   or until the Time in ms expires.
   returns waiting time left
*/
{ int d;
#ifndef WIN32
  int w = 0;
#define WAIT_TIMEOUT ETIMEDOUT
  struct timespec ts, goal_ts, end_ts;
  clock_gettime(CLOCK_REALTIME, &ts);
  ts.tv_sec += ms/1000;
  ts.tv_nsec += (ms%1000)*1000000;
  if (ts.tv_nsec>=1000000000)
  {  ts.tv_sec += 1;
     ts.tv_nsec -= 1000000000;
  }
  goal_ts=ts;
  { int rc = pthread_mutex_lock(&vmb->event_mutex);
    if (rc) 
    { vmb_error(__LINE__,"Locking event mutex failed");
      pthread_exit(NULL);
    }
  }
  pthread_cleanup_push(clean_up_event_mutex,vmb);
#else
  DWORD w = 0;
  SYSTEMTIME now;
  FILETIME fnow;
  ULARGE_INTEGER goal_ul, end_ul;
  GetLocalTime(&now);
  SystemTimeToFileTime(&now, &fnow);  /* in 100 nano s */
  goal_ul.HighPart = fnow.dwHighDateTime;
  goal_ul.LowPart = fnow.dwLowDateTime;
  goal_ul.QuadPart += ms*(ULONGLONG)10000;
#endif
  /* in the meantime the event might have happend */
  while (vmb->power &&
         !vmb->reset_flag && 
         vmb->connected &&
	     vmb->interrupt_lo == 0 &&
		 vmb->interrupt_hi ==0 &&
         !vmb->cancel_wait_for_event &&
		 w != WAIT_TIMEOUT
         )
#ifdef WIN32
     w = WaitForSingleObject(vmb->hevent,ms);

  GetLocalTime(&now);
  SystemTimeToFileTime(&now, &fnow);  /* in 100 nano s */
  end_ul.HighPart = fnow.dwHighDateTime;
  end_ul.LowPart = fnow.dwLowDateTime;
  d = (int)((goal_ul.QuadPart -end_ul.QuadPart)/(ULONGLONG)10000);
#else
     w = pthread_cond_timedwait(&vmb->event_cond,&vmb->event_mutex, &ts);

  pthread_cleanup_pop(1);
  clock_gettime(CLOCK_REALTIME, &end_ts);
  d = 1000*(goal_ts.tv_sec-end_ts.tv_sec)
    + (goal_ts.tv_nsec-end_ts.tv_nsec)/1000000;
#endif
  if (d<0) d = 0;
  else if (d>ms) d = ms;
  vmb->cancel_wait_for_event=0;
#if 0
  fprintf(stderr,"Timeout %d ms remaining %d ms\n", ms, d);
#endif
  return d;
}
