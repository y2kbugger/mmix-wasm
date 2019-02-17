/*
    Copyright 2007 Martin Ruckert
    
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

#ifndef VMB_H
#define VMB_H

#define	WM_VMB				(WM_APP)       
#define WM_VMB_ON			(WM_VMB+1)
#define WM_VMB_OFF			(WM_VMB+2)
#define WM_VMB_RESET		(WM_VMB+3)
#define WM_VMB_CONNECT		(WM_VMB+4)
#define WM_VMB_DISCONNECT	(WM_VMB+5)

#define WM_VMB_OTHER		(WM_VMB+7)


#ifdef WIN32
#include <windows.h>
#pragma warning(disable : 4996)
typedef INT8 int8_t;
typedef INT16 int16_t;
typedef INT32 int32_t;
typedef INT64 int64_t;
typedef UINT8 uint8_t;
typedef UINT16 uint16_t;
typedef UINT32 uint32_t;
typedef UINT64 uint64_t;
#else
#include <pthread.h>
#include <stdint.h>
#endif

#define HI32(vmb_address) ((unsigned int)((vmb_address)>>32))
#define LO32(vmb_address) ((unsigned int)((vmb_address)&0xFFFFFFFF))

typedef struct
{ unsigned int id; /* to tell appart different deviceinfos within the same application */
  unsigned char address[8];
  unsigned int size;
  unsigned int lo_mask;
  unsigned int hi_mask;
  unsigned int connected;
  unsigned int power;
  unsigned int reset_flag;
  /* Functions called by the Bus-Read thread */
  void (*poweron)(void);
  void (*poweroff)(void);
  void (*reset)(void);
  void (*terminate)(void);
  void (*disconnected)(void);
  void (*interrupt)(unsigned char i);
  unsigned char *(*get_payload)(unsigned int offset,int size);
  void (*put_payload)(unsigned int offset,int size, unsigned char *payload);
  void (*unknown)(unsigned char type,
                        unsigned char size,
                        unsigned char slot,
                        unsigned char id,
                        unsigned int offset,
			unsigned char *payload);
  void (*buserror)(unsigned char type, unsigned char address[8]);

  /* used only locally */
  unsigned int interrupt_lo;
  unsigned int interrupt_hi;  
  int fd;
  unsigned int cancel_wait_for_event;
  /* the following mutex/critical section is used to modify
     data from outside the bus thread, that "belongs" to the
     bus thread.
     this includes, but is not limited to:
     the interrupt_lo/hi variables
     the connected, power, reset_flag, cancel_wait_for_event variables
     the event_cond/hevent
     it might be used by applications for other data
     that is used for instance in get_payload or put_payload functions
     
     actually, I think also using raise_interrupt in other threads is
     not save without such a protection ...
  */

#ifdef WIN32
  HANDLE hevent;
  CRITICAL_SECTION   event_section;
#else
  pthread_mutex_t event_mutex;
  pthread_cond_t event_cond;
#endif
} device_info;

/* macros to acquire and release the vmb semaphore */

#ifdef WIN32
#define acquire(vmb)  EnterCriticalSection (&vmb->event_section)
#else
#define acquire(vmb)  ((pthread_mutex_lock(&vmb->event_mutex))?	\
		       (vmb_error(__LINE__,"Locking event mutex failed"), \
			pthread_exit(NULL)):0)
#endif 

#ifdef WIN32
#define release(vmb)  LeaveCriticalSection (&vmb->event_section);
#else
#define release(vmb)  ((pthread_mutex_unlock(&vmb->event_mutex))? \
                       (vmb_error(__LINE__,"Unlocking event mutex failed"), \
			pthread_exit(NULL)):0)
#endif

/* Functions and Varaibles called by the device thread */
extern void vmb_begin(void);
extern void vmb_end(void);
extern void vmb_connect(device_info *vmb, char *host, int port);
extern void vmb_register(device_info *vmb,unsigned int adress_hi, unsigned int address_lo,
                         unsigned int size,
                         unsigned int lo_mask, unsigned int hi_mask,
                         char *name, int major_version, int minor_version);
extern void vmb_wait_for_disconnect(device_info *vmb);
extern void vmb_disconnect(device_info *vmb);
extern int vmb_get_interrupt(device_info *vmb, unsigned int *hi, unsigned int *lo);
extern void vmb_raise_interrupt(device_info *vmb, unsigned char interrupt);
extern void vmb_raise_reset(device_info *vmb);
extern void vmb_wait_for_event(device_info *vmb);
extern int vmb_wait_for_event_timed(device_info *vmb, int ms);
extern void vmb_cancel_wait_for_event(device_info *vmb);
extern void vmb_wait_for_power(device_info *vmb);
extern void vmb_acquire(device_info *vmb);
extern void vmb_release(device_info *vmb);


typedef struct {
  unsigned char *data;
  int address_hi; 
  int address_lo;
  int id;
  int size;
  int status;  
} data_address;

#define STATUS_VALID 0
#define STATUS_INVALID 1
#define STATUS_WRITING 2
#define STATUS_READING 4

extern void vmb_init_data_address(data_address *da, int size);
extern void vmb_load(device_info *vmb, data_address *da);
extern void vmb_wait_for_valid(device_info *vmb, data_address *da);
extern void vmb_cancel_all_loads(void);
extern void vmb_store(device_info *vmb, data_address *da);




/* Functions called by the Bus Write thread */

#include "cache.h"

#include "error.h"

/* default implementations */
extern void vmb_disconnected(void);
extern unsigned char *vmb_get_payload(unsigned int offset,int size);
extern void vmb_interrupt(unsigned char interrupt);
extern void vmb_poweroff(void);
extern void vmb_poweron(void);
extern void vmb_put_payload(unsigned int offset,int size, unsigned char *payload);
extern void vmb_reset(void);
extern void vmb_terminate(void);
extern void vmb_unknown(unsigned char type,
                 unsigned char size,
                 unsigned char slot,
                 unsigned char id,
                 unsigned int offset,
                 unsigned char *payload);
extern void vmb_buserror(unsigned char type, unsigned char address[8]);


#endif
