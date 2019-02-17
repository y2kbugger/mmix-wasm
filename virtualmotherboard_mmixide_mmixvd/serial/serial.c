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

int major_version=2, minor_version=1;
char version[]="$Revision: 664 $ $Date: 2018-12-10 18:10:14 +0100 (Mon, 10 Dec 2018) $";
char title[] ="VMB Serial";

char howto[] = "see http://vmb.sourceforge.net/serial\r\n";

#ifdef WIN32
#include <windows.h>
#include "winopt.h"
#include "inspect.h"
extern HWND hMainWnd;
#else
#define _XOPEN_SOURCE 600
#define _BSD_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/select.h>
#include <sys/ioctl.h>
#include <ctype.h>
#include <errno.h>
/* make mem_update a no-op */
#define  mem_update(offset,size)
#endif

#include "vmb.h"
#include "error.h"
#include "bus-arith.h"
#include "param.h"
#include "option.h"

/* options */
extern int rinterrupt, winterrupt, rdisable, wdisable;
static int wrequested=0, rrequested=0;
extern char *serial;
extern int unbuffered;
device_info vmb;


#define SERIAL_MEM	0x10
unsigned char smem[SERIAL_MEM];
#define REE (smem[0])
#define RCC (smem[3])
#define RDD (smem[7])
#define WEE (smem[8])
#define WCC (smem[11])
#define WDD (smem[15])

/* events/actions: 
    getpayload from the vmb thread
      if needed take data from input buffer
      return data from the memory
      
    putpayload from the vmb thread
      put data into memory
      if a character needs to be written
      place it in the output buffer
      
    input from the ptty
      place input in the input buffer
      signal available data if required
*/

/* Buffer management: two circular buffers for io 
   ----------------------------------------------
*/
struct buf {
  unsigned int buf_max, buf_mask;
  unsigned char *buf;
  int head, tail;
#ifdef WIN32
  HANDLE hbuf;
  CRITICAL_SECTION   buf_section;
#else
  pthread_mutex_t buf_mutex;
  pthread_cond_t buf_cond;
#endif
} inbuf, outbuf;

#ifdef WIN32
#define bufacquire(buf)  EnterCriticalSection (&(buf->buf_section))
#define bufrelease(buf)  LeaveCriticalSection (&(buf->buf_section))
#define bufinit(buf) InitializeCriticalSection (&(buf->buf_section)),\
	                 buf->hbuf =CreateEvent(NULL,FALSE,FALSE,NULL)
#else
#define bufacquire(buf)  ((pthread_mutex_lock(&buf->buf_mutex))?	\
		       (vmb_error(__LINE__,"Locking buf mutex failed"), \
			pthread_exit(NULL)):0)

#define bufrelease(buf)  ((pthread_mutex_unlock(&buf->buf_mutex))? \
                       (vmb_error(__LINE__,"Unlocking buf mutex failed"), \
			pthread_exit(NULL)):0)
#define bufinit(buf) pthread_mutex_init(&buf->buf_mutex,NULL),\
                     pthread_cond_init(&buf->buf_cond,NULL)
#endif

void buf_init(struct buf *buf)
{	buf->buf_max = (1<<6);
	buf->buf_mask = buf->buf_max-1;
	buf->buf = malloc(buf->buf_max);
	if (buf->buf==NULL)
		vmb_fatal_error(__LINE__,"Unable to allocate Buffer");
	buf->head=buf->tail=0;
	bufinit(buf);
}
static void clear(struct buf *buf)
{ bufacquire(buf);
  buf->head=buf->tail=0;
  bufrelease(buf);
}

static int is_empty(struct buf *buf)
{ return buf->head==buf->tail;
}

static int is_full(struct buf *buf)
{ return ((buf->tail+1)&buf->buf_mask)==buf->head;
}
static int bufcopy(struct buf *buf, unsigned char b[], int size)
/* copy buffer to string, needs aquired buffer */
{ int i=0;
  while (i<size && ! is_empty(buf))
    { b[i] = *(buf->buf+buf->head);
      buf->head = (buf->head+1)&buf->buf_mask;
    i++;
  }
  return i;
}

static void buf_grow(struct buf *buf)
/* double buffer size, needs aquired buffer */
{ unsigned char *bigbuf;
  int bigmax, bigtail;
  bigmax = buf->buf_max*2;
  bigbuf = malloc(bigmax);
  if (bigbuf==NULL)
		vmb_fatal_error(__LINE__,"Unable to allocate Buffer");
  bigtail=bufcopy(buf, bigbuf, bigmax);
  free(buf->buf);
  buf->buf=bigbuf;
  buf->buf_max=bigmax;
  buf->buf_mask=bigmax-1;
  buf->head=0;
  buf->tail=bigtail;
}


#ifndef WIN32
static void clean_up_buffer_mutex(void *buf)
{ pthread_mutex_unlock(&(((struct buf *)buf)->buf_mutex)); /* needed if canceled waiting */
}
#endif
static void wait_not_empty(struct buf *buf)
{ 
 #ifndef WIN32
  { int rc;
    rc = pthread_mutex_lock(&buf->buf_mutex);
    if (rc) 
    { vmb_error(__LINE__,"Locking buffer mutex failed");
      pthread_exit(NULL);
    }
    pthread_cleanup_push(clean_up_buffer_mutex,buf);
    /* in the meantime power might be on */
    while (is_empty(buf))
    rc = pthread_cond_wait(&buf->buf_cond,&buf->buf_mutex);
    pthread_cleanup_pop(1);
  }
#else
  /* in the meantime the buf might not be empty */
  while (is_empty(buf))
    WaitForSingleObject(buf->hbuf,INFINITE);
#endif
}

static int get(struct buf *buf)
/* return -1 on failure, return the character on success */
{ unsigned char c;
  bufacquire(buf);
  if (is_empty(buf))
    { bufrelease(buf);
      return -1;
    }
  c = *(buf->buf+buf->head);
  buf->head = (buf->head+1)&buf->buf_mask;
  bufrelease(buf);
  return c;
}


static int getall(struct buf *buf, unsigned char b[], int size)
/* return -1 on failure,
   return the number of charactes written to b on success */
{ int i=0;
  bufacquire(buf);
  i = bufcopy(buf,b,size);
  bufrelease(buf);
  return i;
}

static int put(struct buf *buf, unsigned char c)
/* return -1 on failure, return 1 on success, put c into buffer 
   grow buffer if necessary.
*/
{ int next;
  if (is_full(buf)) 
  { bufacquire(buf);
	buf_grow(buf);
  }
  else
    bufacquire(buf);
  next = (buf->tail+1)&buf->buf_mask;
  if (next==buf->head)
    { bufrelease(buf);
      return -1;
    }
  *(buf->buf+buf->tail)=c;
  buf->tail=next;

#ifdef WIN32
  bufrelease(buf);
  SetEvent (buf->hbuf);
#else
  pthread_cond_signal(&buf->buf_cond);
  bufrelease(buf);
#endif  
return 1;
}

/* Low level Input, output and status
   -------------------------------------------
*/

#ifdef WIN32
static HANDLE hCom=INVALID_HANDLE_VALUE;

int get_pins(void)
{  DWORD      dwCommEvent;
   OVERLAPPED osStatus = {0};

   osStatus.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
   if (osStatus.hEvent == NULL)
	  vmb_fatal_error(__LINE__,"Unable to create overlapped event");
   
   if (!WaitCommEvent(hCom, &dwCommEvent, &osStatus) &&
        (WaitForSingleObject(osStatus.hEvent, INFINITE)!=WAIT_OBJECT_0 || 
         !GetOverlappedResult(hCom, &osStatus, &dwCommEvent, FALSE)))
	  dwCommEvent = -1;
   else
      GetCommModemStatus(hCom,&dwCommEvent);
   CloseHandle(osStatus.hEvent);
   return dwCommEvent;
}

int writetty(unsigned char *buf, int size)
{ 
   OVERLAPPED osWrite = {0};
   DWORD dwWritten;

   osWrite.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
   if (osWrite.hEvent == NULL)
	  vmb_fatal_error(__LINE__,"Unable to create overlapped event");
   if (!WriteFile(hCom, buf, size, &dwWritten, &osWrite))
   { dwWritten = -1;
     if (GetLastError() != ERROR_IO_PENDING)
       vmb_debug(VMB_DEBUG_NOTIFY,"Output Error");
     else if (WaitForSingleObject(osWrite.hEvent, INFINITE)!= WAIT_OBJECT_0)
       vmb_debug(VMB_DEBUG_INFO,"Spurious Output Event");
	 else if (!GetOverlappedResult(hCom, &osWrite, &dwWritten, FALSE))
	 { vmb_debug(VMB_DEBUG_INFO,"Spurious Output Error");
       dwWritten = -1;
	 }
   }
   CloseHandle(osWrite.hEvent);
   return dwWritten;
}


int readtty(unsigned char *buf, int size)
{ DWORD dwRead=0;
  OVERLAPPED osReader = {0};
  osReader.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
  if (osReader.hEvent == NULL)
	  vmb_fatal_error(__LINE__,"Unable to create overlapped event");
  while (dwRead==0)
  { vmb_debug(VMB_DEBUG_INFO,"Wait for Input");
    if (!ReadFile(hCom, buf, size, &dwRead, &osReader))
	{ dwRead=-1;
      vmb_debug(VMB_DEBUG_INFO,"Input Empty");
      if (GetLastError() == ERROR_IO_PENDING)
	  {  vmb_debug(VMB_DEBUG_INFO,"Input Pending");
         if (WaitForSingleObject(osReader.hEvent, INFINITE)==WAIT_OBJECT_0)
		 { vmb_debug(VMB_DEBUG_INFO,"Input Event");
           if (!GetOverlappedResult(hCom, &osReader, &dwRead, FALSE))
		   { vmb_debug(VMB_DEBUG_NOTIFY,"Could not get Input Result");
		     dwRead=-1;
		   }
		 }
		 else
           vmb_debug(VMB_DEBUG_INFO,"Spurious Input Event");
	  }
    }
  }
  CloseHandle(osReader.hEvent);
  return dwRead;
}

#else
static int ttyfd=-1;
int writetty(unsigned char *buf, int size)
{ return write(ttyfd,buf,size);
}

int readtty(unsigned char *buf, int size)
{ return read(ttyfd,buf,size);
}
#endif

/* Threads to read, write, and get status
   --------------------------------------
*/

int pins=-1, npins=0;

#ifdef WIN32
extern void display_pins(void);

static DWORD WINAPI status_thread(LPVOID dummy)
{ do 
  { pins = get_pins();
	if (pins==-1) {
        vmb_debug(VMB_DEBUG_ERROR,"Error reading status");
        return 0;
	}
	vmb_debugi(VMB_DEBUG_INFO,"Status change %X",pins);
	if (pins& MS_CTS_ON) vmb_debug(VMB_DEBUG_INFO,"Status CTS"); 
	if (pins& MS_DSR_ON) vmb_debug(VMB_DEBUG_INFO,"Status DSR"); 
	if (pins& MS_RING_ON) vmb_debug(VMB_DEBUG_INFO,"Status RING"); 
    if (pins& MS_RLSD_ON) vmb_debug(VMB_DEBUG_INFO,"Status RLSD"); 
    display_pins();
  } while (1);
}
#else
static void *status_thread(void *dummy)
{ return NULL;
}
#endif
/* wait for a status change and report
*/

#ifdef WIN32
static DWORD WINAPI write_thread(LPVOID dummy)
#else
static void *write_thread(void *dummy)
#endif
/* wait for outbuf to be not empty and write (blocking)
  the content to ttyfd, if the thread blocks in the end the
  device blocks to the application, which is what we want.
*/
{ do 
  { wait_not_empty(&outbuf);
	while (!is_empty(&outbuf))
	{ int size, i=0;
      unsigned char buf[0x100];
      size=getall(&outbuf,buf,0x100);
      while (i<size)
      { int ret;
        ret = writetty(buf+i,size-i);
        if (ret<=0)
        { vmb_debug(VMB_DEBUG_ERROR,"Error in Output");
          return 0;
        }
        vmb_debugx(VMB_DEBUG_PROGRESS,"Output %s", buf+i, ret);
        i=i+ret;
      }
	}
	if (wrequested && !wdisable)
    { wrequested=0;
      vmb_raise_interrupt(&vmb,winterrupt);
      vmb_debug(VMB_DEBUG_PROGRESS,"Read Interrupt sent");
    }
  } while (1);
}


#ifdef WIN32
static DWORD WINAPI read_thread(LPVOID dummy)
#else
static void *read_thread(void *dummy)
#endif
/* wait (blocking) for a character to become available
   on the tty, 
   if buffered input, put it in the inbuf,
   else directly into rdd.
 */
{ do 
  { int size;
    unsigned char buf[0x100];
#if 0
  start:
#endif
    size = readtty(buf,0x100);
    if (size==0) 
      { vmb_debug(VMB_DEBUG_PROGRESS,"End of Input");
        return 0;
      }
    else if (size<0)
      {
#if 0
        if (errno==EIO)
	  { int i, flags;
            static int old_flags=-1;
            i=ioctl(ttyfd,TIOCMGET,&flags);
	    if (i!=0)
	    { vmb_debugi(VMB_DEBUG_ERROR,"Reading Modem Lines returned %d", i);
              vmb_debugi(VMB_DEBUG_ERROR,"Error %d Reading Modem Lines", errno);
	    }
            if (flags!=old_flags)
	      { vmb_debugi(VMB_DEBUG_ERROR,"Modem Lines %X", flags);
                old_flags=flags;
	      }
            usleep(10000);
            goto start;
	  }
	else
#endif
        vmb_debugi(VMB_DEBUG_ERROR,"Input Error %d ", errno);
        return 0;
      }
    else
      { int i;
        vmb_debugi(VMB_DEBUG_INFO,"Input %d byte", size);
        vmb_debugx(VMB_DEBUG_INFO,"Input %s", buf,size);
        for (i=0; i<size;i++)
	    { put(&inbuf,buf[i]);
	    }
		if (rrequested && !rdisable)
        { rrequested=0;
          vmb_raise_interrupt(&vmb,rinterrupt);
          vmb_debug(VMB_DEBUG_PROGRESS,"Read Interrupt sent");
        }
	  }
  } while(1);
}


#ifdef WIN32
#else
static int read_tid;
static pthread_t read_thr;
static int write_tid;
static pthread_t write_thr;
#endif

static void init_threads(void)
{
#ifdef WIN32
    DWORD dwThreadId;
    HANDLE hThread;

    hThread = CreateThread( 
            NULL,              // default security attributes
            0,                 // use default stack size  
            read_thread,        // thread function 
            NULL,             // argument to thread function 
            0,                 // use default creation flags 
            &dwThreadId);   // returns the thread identifier 
        // Check the return value for success. 
    if (hThread == NULL) 
      vmb_fatal_error(__LINE__, "Creation of read thread failed");
/* in the moment, I really dont use the handle */
    CloseHandle(hThread);
    hThread = CreateThread( 
            NULL,              // default security attributes
            0,                 // use default stack size  
            write_thread,        // thread function 
            NULL,             // argument to thread function 
            0,                 // use default creation flags 
            &dwThreadId);   // returns the thread identifier 
        // Check the return value for success. 
    if (hThread == NULL) 
      vmb_fatal_error(__LINE__, "Creation of write thread failed");
/* in the moment, I really dont use the handle */
    CloseHandle(hThread);

	    hThread = CreateThread( 
            NULL,              // default security attributes
            0,                 // use default stack size  
            status_thread,        // thread function 
            NULL,             // argument to thread function 
            0,                 // use default creation flags 
            &dwThreadId);   // returns the thread identifier 
        // Check the return value for success. 
    if (hThread == NULL) 
      vmb_fatal_error(__LINE__, "Creation of status thread failed");
/* in the moment, I really dont use the handle */
    CloseHandle(hThread);
	vmb_debug(VMB_DEBUG_INFO,"Threads created");

#else
   read_tid  = pthread_create(&read_thr,NULL,read_thread,NULL);
   write_tid = pthread_create(&write_thr,NULL,write_thread,NULL);
#endif
}

static void set_read(unsigned char c)
{ RDD=c;
  if (RCC<0xFF) RCC++;
  if (RCC>1) REE=0x80;
}

unsigned char *serial_get_payload(unsigned int offset,int size)
{ static unsigned char payload[SERIAL_MEM];
  if (RCC==0 && !is_empty(&inbuf)) set_read(get(&inbuf));
  if (unbuffered) while(!is_empty(&inbuf)) set_read(get(&inbuf));
  if (offset<=3 && offset+size>3) /* reading RCC==00 */
  {  if (RCC==0) 
     { rrequested=1;
  	   vmb_debugi(VMB_DEBUG_INFO,"Read Count %d",RCC);
     }
     else
  	   vmb_debugi(VMB_DEBUG_PROGRESS,"Read Count %d",RCC);
  }
  if (offset<=11 && offset+size>11)/* reading WCC */
  { if (!unbuffered || is_empty(&outbuf)) WCC=0;
	if (WCC!=0) wrequested=1;
	vmb_debugi(VMB_DEBUG_PROGRESS,"Write Count %d",WCC);
  }
  if (offset<=7 && offset+size>7 && RCC!=0) /* reading RDD */
	vmb_debugi(VMB_DEBUG_PROGRESS,"Get Read Data 0x%02X",RDD);
  memcpy(payload,smem+offset,size);
  if (offset<=7 && offset+size>7) /* reading RDD */
	  REE=RCC=RDD=0;
  if (RCC==0 && !is_empty(&inbuf)) set_read(get(&inbuf));
  return payload;
}

void serial_put_payload(unsigned int offset,int size, unsigned char *payload)
{ if (offset<=15 && offset+size>15) /* writing WDD*/
  { WDD= payload[15-offset]; /* the only writable byte the rest is ignored */
	if (!unbuffered || is_empty(&outbuf))
	{ put(&outbuf,WDD);
	  vmb_debugi(VMB_DEBUG_PROGRESS,"Set Write Data 0x%02X",WDD);
	  if (!unbuffered) WCC=0; else WCC=1;
	  WEE=0;
	}
	else
	{ if (WCC<0xFF) WCC++;
	  if (WCC>1) WEE=0x80;
      vmb_debugi(VMB_DEBUG_NOTIFY,"unable to write charcter 0x%02X",WDD);
    }
  }
    mem_update(0,SERIAL_MEM);
}


void serial_null(void)
{ memset(smem,0,sizeof(smem));
  wrequested=0, rrequested=0;
  clear(&inbuf);
  clear(&outbuf);
}

void serial_reset(void)
{ vmb_debug(VMB_DEBUG_PROGRESS,"Serial reset");
  serial_null();
}
void serial_poweron(void)
/* this function is called when the virtual power is turned off */
{ vmb_debug(VMB_DEBUG_PROGRESS,"Serial power on");
  serial_null();
#ifdef WIN32
  PostMessage(hMainWnd,WM_VMB_ON,0,0);
#endif
}

void serial_poweroff(void)
/* this function is called when the virtual power is turned off */
{ vmb_debug(VMB_DEBUG_PROGRESS,"Serial power off");
  serial_null();
#ifdef WIN32
  PostMessage(hMainWnd,WM_VMB_OFF,0,0);
#endif
}


void serial_disconnected(void)
/* this function is called when the reading thread disconnects from the virtual bus. */
{ 
  vmb_debug(VMB_DEBUG_PROGRESS,"Serial disconnected");

#ifdef WIN32
  PostMessage(hMainWnd,WM_VMB_DISCONNECT,0,0);
  CloseHandle(hCom);
#else
  close(ttyfd);
#endif
}





static char *slave_tty_path;

int create_pseudo_tty(void)
/* returns 1 for success 0 on failure
*/
{

#ifdef WIN32
	COMMTIMEOUTS ctm;
  if (hCom!=INVALID_HANDLE_VALUE)
  { CloseHandle(hCom);
    hCom=INVALID_HANDLE_VALUE;
  }
  hCom=CreateFile(serial, 
	              GENERIC_READ | GENERIC_WRITE, 
                  0, 
                  0, 
                  OPEN_EXISTING,
                  FILE_FLAG_OVERLAPPED,
                  0);
  if (hCom==INVALID_HANDLE_VALUE)
  { vmb_error(__LINE__,"Unable to open COM Port");
    return 0;
  }
  vmb_debugs(VMB_DEBUG_PROGRESS,"COM Port %s opened", serial);
  GetCommTimeouts(hCom,&ctm);
  ctm.ReadIntervalTimeout=MAXDWORD; //10;
  ctm.ReadTotalTimeoutMultiplier=MAXDWORD; //1;
  ctm.ReadTotalTimeoutConstant=MAXDWORD-1; //5000;
  ctm.WriteTotalTimeoutConstant=MAXDWORD; //5000;
  ctm.WriteTotalTimeoutMultiplier=MAXDWORD; //1;
  SetCommTimeouts(hCom,&ctm);
  vmb_debug(VMB_DEBUG_INFO,"COM Port timeouts set");

npins=8;
pins=0;
if (!SetCommMask(hCom, EV_BREAK | EV_CTS   | EV_DSR | EV_ERR | EV_RING |
                       EV_RLSD | EV_RXCHAR | EV_RXFLAG | EV_TXEMPTY))
  vmb_debug(VMB_DEBUG_ERROR,"Unable to activate Communication Events");


#else
  if (ttyfd!=-1)
  { close(ttyfd);
    ttyfd=-1;
  }
  ttyfd = open(serial, O_RDWR);
  if (ttyfd == -1)
  { vmb_error(__LINE__,"Error opening serial device\n");
    return 0;
  }
  slave_tty_path =  "/dev/pty/s99";

#if 0
  ttyfd = posix_openpt(O_RDWR /* | O_NOCTTY */ );
  if (ttyfd == -1)
    vmb_debug(VMB_DEBUG_ERROR,"Error opening a pseudo terminal.\n");

  if (grantpt(ttyfd)!=0)
    vmb_debug(VMB_DEBUG_ERROR,"Error opening slave pseudo terminal.\n");

  if (unlockpt(ttyfd)!=0)
    vmb_debug(VMB_DEBUG_ERROR,"Error unlocking pseudo terminal.\n");


  slave_tty_path = ptsname(ttyfd);
  if (slave_tty_path==NULL)
    vmb_debug(VMB_DEBUG_ERROR,"unable to get Name of slave tty device.\n");
#endif
#endif

 /* to remove echos we do this 
     input with echo keeps the thing alive
     input with cat file > /dev/pts/4 produces an end of file
     seems ok !
  */
  { 
#ifdef WIN32
#else
	  struct termios tios;
#if 0   
#define BAUDRATE B38400
 
        memset(&tios,0, sizeof(tios));
        tios.c_cflag = BAUDRATE | CRTSCTS | CS8 | CREAD;
        tios.c_iflag = IGNPAR;
        tios.c_oflag = 0;
        
        /* set input mode (non-canonical, no echo,...) */
        tios.c_lflag = 0;
         
        tios.c_cc[VTIME]    = 0;   /* inter-character timer unused */
        tios.c_cc[VMIN]     = 1;   /* blocking read until 5 chars received */
        
        tcflush(fd, TCIFLUSH);
        tcsetattr(fd,TCSANOW,&tios);

        /* allow the process to receive SIGIO */
        fcntl(fd, F_SETOWN, getpid());

#else
    tcgetattr(ttyfd,&tios);
    /* make it a raw tty (like cfmakeraw(&tios) ) */
    tios.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP
                      | INLCR | IGNCR | ICRNL | IXON);
    tios.c_oflag &= ~OPOST;
    tios.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
    tios.c_cflag &= ~(CSIZE | PARENB);
    tios.c_cflag |= CS8;
    tios.c_cc[VMIN]=1;
    tios.c_cc[VTIME]=0;
    tcsetattr(ttyfd,TCSANOW,&tios);
#endif
	if (ttyfd<0) vmb_fatal_error(__LINE__,"Unable to open pseudo tty");

#endif

  }
 return 1;
}


void serial_init(void)
{ create_pseudo_tty();
  fprintf(stderr,"Created device %s\n",slave_tty_path);
  buf_init(&inbuf);
  buf_init(&outbuf);
  serial_null();
}
  
#ifdef WIN32
int serial_reg_read(unsigned int offset, int size, unsigned char *buf)
{ if (offset>SERIAL_MEM) return 0;
  if (offset+size>SERIAL_MEM) size =SERIAL_MEM-offset;
  memmove(buf,smem+offset,size);
  return size;
}
struct register_def serial_regs[7] = {
	/* name no offset size chunk format */
	{"ErrorIn"  ,0,1,byte_chunk,hex_format},
	{"CountIn"  ,3,1,byte_chunk,unsigned_format},
    {"DataIn"   ,7,1,byte_chunk,ascii_format},
    {"ErrorOut" ,8,1,byte_chunk,hex_format},
    {"CountOut" ,11,1,byte_chunk,unsigned_format},
    {"DataOut"  ,15,1,byte_chunk,ascii_format},
	{0}};


struct inspector_def inspector[2] = {
    /* name size get_mem address num_regs regs */
	{"Registers",SERIAL_MEM,serial_reg_read,serial_get_payload,serial_put_payload,0,0,8,6,serial_regs},
	{0}
};

#endif

void init_device(device_info *vmb)
{ vmb_debug(VMB_DEBUG_PROGRESS,"Serial initializing");
  serial_init();
  vmb_size = SERIAL_MEM;
  vmb->poweron=serial_poweron;
  vmb->poweroff=serial_poweroff;
  vmb->disconnected=serial_disconnected;
  vmb->reset=serial_reset;
  vmb->terminate=vmb_terminate;
  vmb->put_payload=serial_put_payload;
  vmb->get_payload=serial_get_payload;
  init_threads();
}



#ifndef WIN32
static
void sighndl(int signo)
{
        /* signal(signo, sighndl); */
	fprintf(stderr, "Got signal %d\n", signo);
	fflush(stderr);
        exit(1);
}


int main(int argc, char *argv[])
{
  signal(SIGINT,sighndl);
  signal(SIGTERM,sighndl);
  /* signal(SIGKILL,sighndl); a bit dangerous */
  signal(SIGHUP, sighndl);
  signal(SIGIO, sighndl);

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
  
 

  vmb_connect(&vmb, host,port); 
  vmb_register(&vmb,HI32(vmb_address),LO32(vmb_address),vmb_size,
               0, 0, vmb_program_name,major_version,minor_version);
#if 0
  while (vmb.connected)
  { fd_set except_set;
    int ready;
    FD_ZERO(&except_set);
    FD_SET((unsigned)ttyfd, &except_set);
    ready = select (ttyfd+1, NULL, NULL, &except_set, NULL);
    if (ready<0)
      perror("select()");
    else if (FD_ISSET(ttyfd,&except_set))
    { int arg;
      vmb_debug(VMB_DEBUG_PROGRESS,"Exception\n");
      if(ioctl(ttyfd, TIOCMGET, &arg) < 0)
            perror("TIOCMGET ioctl failed");
      else
      {  vmb_debugi(VMB_DEBUG_PROGRESS,"ioctl %08X\n",arg);
      }
    }
  }
#else
  vmb_wait_for_disconnect(&vmb);
#endif
  pthread_kill(read_thr,SIGTERM);
  pthread_kill(write_thr,SIGTERM);
  vmb_debug(VMB_DEBUG_PROGRESS, "Serial exit.");
  return 0;
}

#endif
