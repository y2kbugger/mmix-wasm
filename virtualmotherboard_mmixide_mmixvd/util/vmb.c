
#ifdef WIN32
#include <winsock2.h>
#include <windows.h>
#else
#include <pthread.h>
#include <time.h>
#include <errno.h>
#endif
#include <stdlib.h>
#include <string.h>

#include "vmb.h"
#include "error.h"
#include "message.h"
#include "bus-arith.h"
#include "cache.h"




void vmb_raise_reset(device_info *vmb)
     /* trigger a hard reset on the bus */
{ send_msg(&(vmb->fd), TYPE_BUS, 0, 0, ID_RESET, 0, 0, 0);
}


#ifndef WIN32
void clean_up_event_mutex(void *vmb)
{ pthread_mutex_unlock(&(((device_info *)vmb)->event_mutex)); /* needed if canceled waiting */
}
#endif

int vmb_get_interrupt(device_info *vmb, unsigned int *hi, unsigned int *lo)
     /* return 0 if there was no recent interrupt 
        return 1 if there was, and add the interrupt bits to lo and hi
        return -1 if the bus disconnected
     */
{
  if (vmb->interrupt_lo == 0 && vmb->interrupt_hi ==0) return 0;
  acquire(vmb);
  *hi |= vmb->interrupt_hi; 
  *lo |= vmb->interrupt_lo; 
  vmb->interrupt_hi = 0;
  vmb->interrupt_lo = 0;
  release(vmb);
  if (vmb->connected) return 1;
  else return -1;
}

void vmb_raise_interrupt(device_info *vmb, unsigned char i)
     /* raise interrupt i to the bus */
{  if ( i >= 64)
    return;
   send_msg(&(vmb->fd), TYPE_BUS, 0, (unsigned char)i, ID_INTERRUPT, 0, 0, 0);
   vmb_debugi(VMB_DEBUG_INFO,"Raising interrupt %d\n",i);
}




void vmb_wait_for_event(device_info *vmb)
/* waits for a  power off, reset, disconnect, or an interrupt
*/
{ 
#ifndef WIN32
  { int rc = pthread_mutex_lock(&vmb->event_mutex);
    if (rc) 
    { vmb_error(__LINE__,"Locking event mutex failed");
      pthread_exit(NULL);
    }
  }
  pthread_cleanup_push(clean_up_event_mutex,vmb);
#endif
  /* in the meantime the event might have happend */
  while (vmb->power &&
         !vmb->reset_flag && 
         vmb->connected &&
	 vmb->interrupt_lo == 0 && vmb->interrupt_hi ==0 &&
         !vmb->cancel_wait_for_event)
#ifdef WIN32
     WaitForSingleObject(vmb->hevent,INFINITE);
#else
     pthread_cond_wait(&vmb->event_cond,&vmb->event_mutex);
  pthread_cleanup_pop(1);
#endif
  vmb->cancel_wait_for_event=0;
}



void vmb_wait_for_power(device_info *vmb)
/* waits for a  power on or for a disconnect */
{ 
#ifndef WIN32
  { int rc = pthread_mutex_lock(&vmb->event_mutex);
    if (rc) 
    { vmb_error(__LINE__,"Locking event mutex failed");
      pthread_exit(NULL);
    }
  }
  pthread_cleanup_push(clean_up_event_mutex,vmb);
#endif
  /* in the meantime power might be on */
  while (vmb->connected && !vmb->power)
#ifdef WIN32
     WaitForSingleObject(vmb->hevent,INFINITE);
#else
     pthread_cond_wait(&vmb->event_cond,&vmb->event_mutex);
  pthread_cleanup_pop(1);
#endif  
}

void vmb_wait_for_disconnect(device_info *vmb)
/* waits for a disconnect */
{ 
 #ifndef WIN32
  { int rc;rc = pthread_mutex_lock(&vmb->event_mutex);
    if (rc) 
    { vmb_error(__LINE__,"Locking event mutex failed");
      pthread_exit(NULL);
    }
    pthread_cleanup_push(clean_up_event_mutex,vmb);
#endif
  /* in the meantime power might be on */
  while (vmb->connected)
#ifdef WIN32
    WaitForSingleObject(vmb->hevent,INFINITE);
#else
    rc = pthread_cond_wait(&vmb->event_cond,&vmb->event_mutex);
  pthread_cleanup_pop(1);
  }
#endif 
}

static void change_event(device_info *vmb, unsigned int *event, unsigned int value)
/* change one of the variables protected by the event mutex */
{ 
  acquire(vmb);
  *event = value;
#ifdef WIN32
  release(vmb);
  SetEvent (vmb->hevent);
#else
  pthread_cond_signal(&vmb->event_cond);
  release(vmb);
#endif
}

extern void vmb_cancel_wait_for_event(device_info *vmb)
{ change_event(vmb, &vmb->cancel_wait_for_event,1);
}

#ifdef WIN32
#else
static int read_tid;
static pthread_t read_thr;
#endif

/* a circular buffer acting as a queue for pending read requests.
   the write tread manupilates pending_last
   and all elements in the buffer starting with pending_last
   up to but not including pending_first putting things in
   the read thread manipulates pending_first 
   and all elements in the buffer starting with pending_first
   up to but not including pending_last taking things out
   In order to be able to tell a full from an empty queueu,
   there is always one empty field and hence pending_first==pending_last
   means an empty queue not a full one.
*/

#define PENDINGREADMAX (1<<8)
#define PENDINGREADMASK (PENDINGREADMAX-1)
static data_address *pending_read[PENDINGREADMAX];
static unsigned int pending_first=0; /* points to the first pending read */
static unsigned int pending_last=0; /* points past the last pending read */

#ifdef WIN32
HANDLE hnot_full_pending=NULL;
#else
static pthread_mutex_t pending_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t pending_cond = PTHREAD_COND_INITIALIZER;
static void clean_up_pending_mutex(void *_dummy)
{ pthread_mutex_unlock(&pending_mutex); /* needed if canceled waiting */
}
#endif

#define pending_size ((pending_last-pending_first) &  PENDINGREADMASK)

static int full_pending_read(void)
{ return pending_size == (PENDINGREADMAX-1);
}
static int empty_pending_read(void)
{ return pending_size == 0;
}

static void wait_not_full_pending_read(void)
{ 
#ifndef WIN32
  int rc;
  rc = pthread_mutex_lock(&pending_mutex);
  if (rc) 
    { vmb_error(__LINE__,"Locking pending mutex failed");
      pthread_exit(NULL);
    }
  pthread_cleanup_push(clean_up_pending_mutex,NULL);
#endif
  /* in the meantime the queue might no longer be full */
  while (full_pending_read())
#ifdef WIN32
	     WaitForSingleObject(hnot_full_pending,INFINITE);
#else
    rc = pthread_cond_wait(&pending_cond,&pending_mutex);
  pthread_cleanup_pop(1);
#endif
}

static void enqueue_read_request(data_address *da)
{ if (full_pending_read()) wait_not_full_pending_read();
  pending_read[pending_last] = da;
  pending_last = (pending_last +1) & PENDINGREADMASK;
}

static void remove_queue_entry(int i)
/* remove entry i from the pending read queue. */
{ while(i!=pending_first)
  { int k;
    k = (i-1)& PENDINGREADMASK;
    pending_read[i] = pending_read[k];
    i = k;
   }
  pending_first = (pending_first +1) & PENDINGREADMASK;
  if (pending_size == (PENDINGREADMAX-2)) /* full -> not full */
#ifdef WIN32
  SetEvent(hnot_full_pending);
#else
  { int rc;
    rc = pthread_mutex_lock(&pending_mutex);
    if (rc) 
    { vmb_error(__LINE__,"Locking pending mutex failed");
      pthread_exit(NULL);
    }
    rc = pthread_cond_signal(&pending_cond);
    rc = pthread_mutex_unlock(&pending_mutex);
    if (rc) 
    { vmb_error(__LINE__,"Unlocking pending mutex failed");
      pthread_exit(NULL);
    }
  }
#endif
}

static data_address *dequeue_read_request(unsigned int address_hi, unsigned int address_lo)
{ int i;
  data_address *da;
  if (empty_pending_read()) return NULL;
  for (i = pending_first; i!= pending_last; i = (i+1)&PENDINGREADMASK)
  { da = pending_read[i];
    if (da->status != STATUS_READING) /* should not be here remove it */
      remove_queue_entry(i); 
    else if (da->address_hi == address_hi && da->address_lo == address_lo)
    { remove_queue_entry(i); 
      return da;
    }
  }
  /* no matching request found */
  return NULL;   
}

#ifdef WIN32
HANDLE hvalid=NULL;
CRITICAL_SECTION   valid_section;
#else
static pthread_mutex_t valid_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t valid_cond = PTHREAD_COND_INITIALIZER;

static void clean_up_valid_mutex(void *_dummy)
{ pthread_mutex_unlock(&valid_mutex); /* needed if canceled waiting */
}
#endif

static void deliver_answer(data_address *da, int size, unsigned char *payload)
{  
  
#ifdef WIN32
  EnterCriticalSection (&valid_section);
#else
  int rc;
  rc = pthread_mutex_lock(&valid_mutex);
  if (rc) 
  { vmb_error(__LINE__,"Locking valid mutex failed");
    pthread_exit(NULL);
  }
#endif
  if (da->status == STATUS_READING || da->status == STATUS_INVALID)
  { if (size>da->size) size = da->size;
    if (size>0)
    { if (payload != NULL) 
        memmove(da->data,payload, size);
      else
        memset(da->data,0,size);
    }
    da->status = STATUS_VALID;
  }
#ifdef WIN32
  LeaveCriticalSection (&valid_section);
  SetEvent(hvalid);
#else
  rc = pthread_cond_signal(&valid_cond);
  rc = pthread_mutex_unlock(&valid_mutex);
  if (rc) 
  { vmb_error(__LINE__,"Unlocking valid mutex failed");
    pthread_exit(NULL);
  }
#endif
}
static void flush_pending_read_queue(void)
{  data_address *da;
   while (!empty_pending_read())
    { da = pending_read[pending_first];
      remove_queue_entry(pending_first); 
      deliver_answer(da,0,NULL);
    }
}


void vmb_cancel_all_loads(void)
{ flush_pending_read_queue();
}

static void reply_payload(device_info *vmb, unsigned char address[8],int size, unsigned char *payload)
{ unsigned int address_hi = chartoint(address);
  unsigned int address_lo = chartoint(address+4);
  data_address *da;
  vmb_debugx(VMB_DEBUG_INFO, "Searching for read request matching %s\n",address,8);
  da = dequeue_read_request(address_hi,address_lo);
  if (da!=NULL)
  { vmb_debug(VMB_DEBUG_INFO, "Matching read request found\n");
    deliver_answer(da,size,payload);
  }
  else if (size == 0) /* this was a dummy answer, we drop all pending requests */
  { vmb_debug(VMB_DEBUG_NOTIFY, "No matching request for dummy answer\n");
    flush_pending_read_queue();
  }
  else
     vmb_debug(VMB_DEBUG_NOTIFY, "No matching read request found\n");
}

void vmb_wait_for_valid(device_info *vmb, data_address *da)
{ 
#ifndef WIN32
  int rc;
  rc = pthread_mutex_lock(&valid_mutex);
  if (rc) 
    { vmb_error(__LINE__,"Locking valid mutex failed");
      pthread_exit(NULL);
    }
  pthread_cleanup_push(clean_up_valid_mutex,NULL);
#endif
  /* in the meantime the da might be valid */
  if (vmb->connected)
    while (da->status != STATUS_VALID)
#ifdef WIN32
      WaitForSingleObject(hvalid,INFINITE);
#else
      rc = pthread_cond_wait(&valid_cond,&valid_mutex);
  pthread_cleanup_pop(1);
#endif
}

static void answer_readrequest(device_info *vmb, unsigned char slot,
   			   unsigned char address[8], int size, unsigned char *data)
{ unsigned char type, id;
  if (size == 0 || data == NULL)
    { type = TYPE_ADDRESS|TYPE_ROUTE;
      id = ID_NOREPLY; 
      size = 0;}
  else if (size == 1)
    { type = TYPE_ADDRESS|TYPE_ROUTE|TYPE_PAYLOAD;
      id = ID_BYTEREPLY; 
      size = 0;}
   else if (size == 2)
    { type = TYPE_ADDRESS|TYPE_ROUTE|TYPE_PAYLOAD;
      id = ID_WYDEREPLY; 
      size = 0;}
   else if (size == 4)
    { type = TYPE_ADDRESS|TYPE_ROUTE|TYPE_PAYLOAD;
      id = ID_TETRAREPLY; 
      size = 0;}
  else /* if (size>4) */
    { type = TYPE_ADDRESS|TYPE_ROUTE|TYPE_PAYLOAD;
      id = ID_READREPLY; 
      size = (size+7)/8-1;}
  send_msg(&(vmb->fd), type,(unsigned char)size,slot,id,0,address,data);
}

static void failed_writerequest(device_info *vmb, unsigned char slot,
   			   unsigned char address[8])
{ send_msg(&(vmb->fd), TYPE_ADDRESS|TYPE_ROUTE,0,slot,ID_NOWRITE,0,address,NULL);
}


static void read_request(device_info *vmb,  unsigned char a[8], int s, unsigned char slot, unsigned char p[])
{ unsigned int offset;
  unsigned char *data;

  offset = get_offset(vmb->address,a);
  if (hi_offset || overflow_offset || offset >= vmb->size)
  { vmb_debugx(VMB_DEBUG_WARN, "Read request out of range at %s\n",a,8);
    vmb_debug(VMB_DEBUG_INFO, "Sending empty answer\n");
    answer_readrequest(vmb, slot, a,0,NULL);
  }
  else if (offset + s > vmb->size)
  { vmb_debugx(VMB_DEBUG_NOTIFY, "Read request partly out of range %s\n",a,8);
	s = vmb->size-offset;
	vmb_debug(VMB_DEBUG_INFO, "Processing Read Request\n");
    if (vmb->get_payload)  data = vmb->get_payload(offset,s);
    else data = NULL;
    answer_readrequest(vmb, slot,a,s,data);
  }
  else
  { vmb_debug(VMB_DEBUG_INFO, "Processing Read Request\n");
    if (vmb->get_payload)  data = vmb->get_payload(offset,s);
    else data = NULL;
    answer_readrequest(vmb, slot,a,s,data);
  }
}

static void write_request(device_info *vmb, unsigned char a[8], int s, unsigned char slot, unsigned char p[])
{ unsigned int offset;
  offset = get_offset(vmb->address,a);
   if (hi_offset || overflow_offset || offset >= vmb->size)
  {
    vmb_debugx(VMB_DEBUG_WARN, "Write request out of range at %s\n",a,8);
    vmb_debugx(VMB_DEBUG_INFO, "Address: %s\n",vmb->address,8);
    vmb_debugi(VMB_DEBUG_INFO, "Size:    %d\n",vmb->size);
    vmb_debugi(VMB_DEBUG_INFO, "Offset:  %ud\n", offset);
    failed_writerequest(vmb, slot,a);
	return;
  }
  else if (offset + s > vmb->size)
  { vmb_debugx(VMB_DEBUG_NOTIFY, "Write request partly out of range %s\n",a,8);
	s = vmb->size-offset;
	vmb_debug(VMB_DEBUG_INFO, "Processing Write Request\n");
    if (vmb->put_payload)  vmb->put_payload(offset,s,p);
  }
  else
  { vmb_debug(VMB_DEBUG_INFO, "Processing Write Request\n");
    if (vmb->put_payload)  vmb->put_payload(offset,s,p);
  }
}

static void bus_error(device_info *vmb, unsigned char type, unsigned char a[8])
{ char hex[17]={0};
  chartohex(a,hex,8);
  vmb_debugs(VMB_DEBUG_ERROR, "Bus error detected at %s\n",hex);
  if (vmb->buserror) vmb->buserror(type,a);
  return;
}


static void dispatch_message(device_info *vmb, unsigned char type, 
                             unsigned char size, 
                             unsigned char slot, 
                             unsigned char id, 
                             unsigned char address[8], 
                             unsigned char *payload)
{ switch (id)
    { case ID_IGNORE: 
        return;
      case ID_READ:
	read_request(vmb, address, (size+1)*8, slot, payload);
        return;
      case ID_READBYTE:
	read_request(vmb, address, 1, slot, payload);
        return;
      case ID_READWYDE:
	read_request(vmb, address, 2, slot, payload);
        return;
      case ID_READTETRA:
	read_request(vmb, address, 4, slot, payload);
        return;
      case ID_WRITE:
	write_request(vmb, address, (size+1)*8,  slot, payload);
        return;
      case ID_WRITEBYTE:
	write_request(vmb, address, 1,  slot, payload);
        return;
      case ID_WRITEWYDE:
	write_request(vmb, address, 2,  slot, payload);
        return;
      case ID_WRITETETRA:
	write_request(vmb, address, 4,  slot, payload);
        return;
      case ID_READREPLY:
	reply_payload(vmb, address,(size+1)*8,payload);
        return;
      case ID_BYTEREPLY:
	reply_payload(vmb, address,1,payload);
        return;
      case ID_WYDEREPLY:
	reply_payload(vmb, address,2,payload);
        return;
      case ID_TETRAREPLY:
	reply_payload(vmb, address,4,payload);
        return;
      case ID_NOREPLY:
	reply_payload(vmb, address,0,payload);
        bus_error(vmb, ID_NOREPLY, address);
        return;
      case ID_NOWRITE:
        bus_error(vmb, ID_NOWRITE, address);
        return;
      case ID_TERMINATE:
        if (vmb->terminate) vmb->terminate();
	return;
      case ID_RESET:
        change_event(vmb, &vmb->reset_flag, 1);
	if(vmb->reset) vmb->reset();
        return;
      case ID_POWEROFF:
        vmb->reset_flag = 0;
        change_event(vmb, &vmb->power,0);
	    if (vmb->poweroff) vmb->poweroff();
        return;
      case ID_POWERON:
        vmb->reset_flag = 0;
        change_event(vmb, &vmb->power, 1);
		if (vmb->poweron) vmb->poweron();
        return;
      case ID_INTERRUPT:
        if (slot<32)
           change_event(vmb, &vmb->interrupt_lo, vmb->interrupt_lo| BIT(slot));
        else 
           change_event(vmb, &vmb->interrupt_hi, vmb->interrupt_hi| BIT(slot-32));
	    if (vmb->interrupt) vmb->interrupt(slot);
        return;
      default:
        if (vmb->unknown) vmb->unknown(type,size,slot,id,get_offset(vmb->address,address),payload);
	return;
      }
}

static int get_request(device_info *vmb, 
					   unsigned char *type, 
                        unsigned char *size, 
                        unsigned char *slot,
                        unsigned char *id,
   			unsigned char address[8],
                        unsigned char *payload)
{ unsigned int  time;

  int i;
  i=receive_msg(&(vmb->fd),type,size,slot,id,&time,address,payload);
  if (i<0) 
	return 0;
  vmb_debugm(VMB_DEBUG_MSG, *type,*size,*slot,*id,address,payload);
  return 1;
}


static void clean_up_read_thread(void *dummy)
{ device_info *vmb = (device_info *)dummy;
  vmb_debug(VMB_DEBUG_PROGRESS, "Terminating bus interface.\n");
  if (vmb->fd>=0)
  { vmb_debug(VMB_DEBUG_INFO, "closing connection.\n");
    bus_unregister(vmb->fd);
    bus_disconnect(&(vmb->fd)); 
    vmb->fd = INVALID_SOCKET;
    vmb_debug(VMB_DEBUG_INFO, "connection closed.\n");
  }
  change_event(vmb, &vmb->connected, 0);
  flush_pending_read_queue();
  vmb_debug(VMB_DEBUG_INFO, "read queue flushed.\n");
  if (vmb->disconnected) vmb->disconnected();
#ifdef WIN32
   CloseHandle(vmb->hevent); vmb->hevent = NULL;
   DeleteCriticalSection(&vmb->event_section);
#endif
  vmb_debug(VMB_DEBUG_INFO, "read tread cleaned.\n");
}

#ifdef WIN32
static DWORD WINAPI read_loop(LPVOID dummy)
#else
static void *read_loop(void *dummy)
#endif
{ unsigned char type,size,slot,id;
  unsigned char address[8];
  unsigned char payload[MAXPAYLOAD];
  device_info *vmb = (device_info *)dummy;

  /* the loop will exit if the bus disconnects. Then clean up */
#ifndef WIN32
  pthread_cleanup_push(clean_up_read_thread,vmb);
#endif
  while (get_request(vmb, &type,&size,&slot,&id,address,payload))
    dispatch_message(vmb, type,size,slot,id,address,payload);
#ifdef WIN32
  clean_up_read_thread(vmb);
#else
  pthread_cleanup_pop(1);
#endif
    vmb_debug(VMB_DEBUG_INFO, "read loop terminated.\n");
  return 0;
}



void vmb_disconnect(device_info *vmb)
     /* call this to terminate the reading thread properly before
        terminating the main program.
     */
{ 
#ifndef WIN32
  void *exitcode;
  int rc = pthread_mutex_lock(&vmb->event_mutex);
  if (rc) 
    { vmb_error(__LINE__,"Locking event mutex failed");
      pthread_exit(NULL);
    }
#endif 
  if (vmb->connected)
#ifdef WIN32
	  bus_disconnect(&(vmb->fd));
#else
    /* should not be used. Kills the thread immediately without cleaning up. */
    pthread_cancel(read_thr);
  rc = pthread_mutex_unlock(&vmb->event_mutex);
  if (rc) 
  { vmb_error(__LINE__,"Unlocking event mutex failed");
    pthread_exit(NULL);
  }
  pthread_join(read_thr,&exitcode);
#endif
 
}

/* Functions called by the CPU thread */
void vmb_begin(void)
{  
#ifdef WIN32
   if (hnot_full_pending==NULL)
     hnot_full_pending = CreateEvent(NULL,FALSE,FALSE,NULL);
   if (hvalid==NULL)
   { hvalid =CreateEvent(NULL,FALSE,FALSE,NULL);
     InitializeCriticalSection (&valid_section);
   }
#else
#endif
}


void vmb_end(void)
{  
#ifdef WIN32
   if (hnot_full_pending!=NULL)
   { CloseHandle(hnot_full_pending); hnot_full_pending=NULL; }
   if (hvalid!=NULL)
   { CloseHandle(hvalid); hvalid=NULL;
     DeleteCriticalSection(&valid_section);
   }
#else
#endif
}

void vmb_connect(device_info *vmb, char *host, int port)
/* call to initialize the interface to the virtual bus 
   must be called before any of the other functions
*/
{  vmb->fd= bus_connect(host,port);
   if (vmb->fd<0) 
   { vmb_fatal_error(__LINE__,"Unable to connect to motherboard");
     vmb->connected = 0;
     return;
   }

  vmb->connected = 1;
#ifdef WIN32
  { DWORD dwReadThreadId;
    HANDLE hReadThread;
    vmb->hevent =CreateEvent(NULL,FALSE,FALSE,NULL);
    InitializeCriticalSection (&vmb->event_section);

    hReadThread = CreateThread( 
            NULL,              // default security attributes
            0,                 // use default stack size  
            read_loop,        // thread function 
            vmb,             // argument to thread function 
            0,                 // use default creation flags 
            &dwReadThreadId);   // returns the thread identifier 
        // Check the return value for success. 
    if (hReadThread == NULL) 
      vmb_fatal_error(__LINE__, "Creation of bus thread failed");
/* in the moment, I really dont use the handle */
    CloseHandle(hReadThread);
  }
#else
   read_tid = pthread_create(&read_thr,NULL,read_loop,vmb);
#endif
}

void vmb_register(device_info *vmb, unsigned int address_hi, unsigned int address_lo,
		  unsigned int size,
                  unsigned int lo_mask, unsigned int hi_mask,
                  char *name,
				  int major_version, int minor_version)
/* call after initialize to register with the  virtual bus 
   must be called before any of the other funcions
*/
{  unsigned char limit[8];
   int r;
   inttochar(address_hi,vmb->address);
   inttochar(address_lo,vmb->address+4);
   vmb->size = size;
   add_offset(vmb->address,size,limit);
   vmb->lo_mask = lo_mask;
   vmb->hi_mask = hi_mask;
   r = bus_register(&(vmb->fd),vmb->address,limit,lo_mask,hi_mask,name,major_version,minor_version);
#ifdef WIN32
   if (r<0)
   { TIMEVAL tv;
     vmb_debug(VMB_DEBUG_ERROR,"Second attempt to register\n");
     tv.tv_sec=0;
	 tv.tv_usec= 10000;
	 select(0,NULL,NULL,NULL,&tv);
	 /* wait and try again */
	 r = bus_register(&(vmb->fd),vmb->address,limit,lo_mask,hi_mask,name,major_version,minor_version);
   }
#endif
   if (r<0)
       vmb_fatal_errori(__LINE__,"Unable to register with motherboard",r);
}

void vmb_init_data_address(data_address *da, int size)
     /* initialize a data_address structure */
{  da->size=size;
   size = (size+7)&~0x7; /* round size up to a multiple of 8 */
   if (da->data!=NULL) 
   { free(da->data);
     da->data = NULL;
   }
   if (size > 0)
   { da->data=malloc(size);
     if (da->data == NULL)
       vmb_fatal_error(__LINE__,"Out of memory initializing data address pair");
   }
   da->address_hi = 0;
   da->address_lo = 0; 
   da->id = 0;
   da->status = STATUS_INVALID; 
}

void vmb_store(device_info *vmb, data_address *da)
/* writing 1, 2, 4, or a multiple of 8 (up to 256*8) byte */
{ unsigned char id;
  unsigned char address[8];
  unsigned char size;
  if (da->size>0) 
  { if (da->size==1)
    { id = ID_WRITEBYTE; size = 0;}
    else   if (da->size==2)
    { id = ID_WRITEWYDE; size = 0;}
    else  if (da->size<=4)
    { id = ID_WRITETETRA; size = 0;}
    else
    { id = ID_WRITE; size = (da->size+7)/8-1; }
 
    inttochar(da->address_hi,address);
    inttochar(da->address_lo,address+4);
    vmb_debugx(VMB_DEBUG_INFO, "Writing to address %s\n",address,8);
    send_msg(&(vmb->fd),
           (unsigned char)(TYPE_ADDRESS|TYPE_PAYLOAD),
           size,0,id,0,address,da->data);
  }
  da->status = STATUS_VALID;
}

void vmb_load(device_info *vmb, data_address *da)
/* reading 1, 2, 4, or a multiple of 8 (up to 256*8) byte */
{ unsigned char id;
  unsigned char address[8];
  unsigned char size;
  if (da->size>0)
  { da->status = STATUS_READING;
    if (da->size==1)
    { id = ID_READBYTE; size = 0;}
    else   if (da->size==2)
    { id = ID_READWYDE; size = 0;}
    else  if (da->size<=4)
    { id = ID_READTETRA; size = 0;}
    else
    { id = ID_READ; size = (da->size+7)/8-1; }
    inttochar(da->address_hi,address);
    inttochar(da->address_lo,address+4);
    enqueue_read_request(da);
    vmb_debugx(VMB_DEBUG_INFO, "Read request added for address %s\n",address,8);
    send_msg(&(vmb->fd),
           (unsigned char)(TYPE_ADDRESS|TYPE_REQUEST),
           size,0,id,0,address,NULL);
  }
  else
    da->status = STATUS_VALID;
}

