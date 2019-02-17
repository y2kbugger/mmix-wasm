/* Remote utility routines for the remote server for GDB.
   Copyright 1986, 1989, 1993, 1994, 1995, 1996, 1997, 1998, 1999, 2000, 2001,
   2002
   Free Software Foundation, Inc.

   This file is part of GDB.

   adapded to the MMIX motherboard project 
   2005, by Martin Ruckert, ruckert@acm.org

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

#undef DEBUG

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifdef WIN32
#include <windows.h>
#include <winsock.h>
#else
#include <pthread.h>
#include <sys/ioctl.h>
#include <sys/file.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/tcp.h>
#include <sys/ioctl.h>

#include <fcntl.h>
#include <sys/time.h>
#include <unistd.h>
#include <arpa/inet.h>
#endif
#include <signal.h>
#include <ctype.h>
#include "vmb.h"
#include "bus-arith.h" 
#include "buffers.h"
#include "mmix-internals.h"
#include "gdb.h"

/* the local agent of gdb is fed with commands from gdb and
   from the simulator using the queue of command buffers 
*/

/* all buffers should fit in the free queue */
#define NUMBUF (QUEUESIZE-1) 


static queue free_buffers;
static queue cmd_buffers;

char *get_free_buffer(void)
{ return (char *)dequeue(&free_buffers);}

void put_free_buffer(char *buffer)
{ enqueue(&free_buffers,buffer);}

char *get_cmd_buffer(void)
{ return dequeue(&cmd_buffers);}

void put_cmd_buffer(char *buffer)
{ enqueue(&cmd_buffers,buffer);
}

static void buffers_init(void)
{ int i;
  static char buffer[NUMBUF][PBUFSIZ];
  init_queue(&free_buffers);
  init_queue(&cmd_buffers);
  for (i=0;i<NUMBUF;i++)
    enqueue(&free_buffers,buffer[i]);
}

/* end of buffers */

/* Functions to open and close server and remote connection sockets */

#if !defined(SOCKET)
#define SOCKET int
#endif

#if !defined(INVALID_SOCKET)
#define INVALID_SOCKET  (~0)
#endif

#define valid_socket(socket)  ((socket) != INVALID_SOCKET)

static SOCKET remote_fd=INVALID_SOCKET;
static SOCKET server_fd=INVALID_SOCKET;

static SOCKET
socket_close (SOCKET fd)
{
  if (valid_socket(fd))
#ifdef WIN32
    closesocket(fd);
#else
    close(fd);
#endif 
  return INVALID_SOCKET;
}

static void remote_close(void)
{ remote_fd=socket_close(remote_fd);
}

static void server_close(void)
{ server_fd=socket_close(server_fd);
}




#ifdef WIN32	
static void wsa_close(void)
{ WSACleanup();
}

static void wsa_init(void)
{
  static int wsa_ready=0;
  WSADATA wsadata;
  if (wsa_ready) return;
  if(WSAStartup(MAKEWORD(1,1), &wsadata) != 0)
    perror("Unable to initialize Winsock dll");
  wsa_ready = 1;
  atexit(wsa_close);
}
#endif


static struct sockaddr_in sockaddr;
#ifndef WIN32
void catchpipe(int n)
{  signal(SIGPIPE,catchpipe); /* now |catchint| will catch the next interrupt */
   fprintf(stderr,"Got SIGPIPE");
   remote_close();
} 
#endif

static int server_open(int port)
{
#ifdef WIN32
  int tmp;
  wsa_init();
#else
  socklen_t tmp;
#endif

  server_fd = (SOCKET)socket (PF_INET, SOCK_STREAM, 0);
  if (server_fd < 0)
  {  perror ("Can't open socket");
     return 0;
  }

      /* Allow rapid reuse of this port. */
  tmp = 1;
  setsockopt (server_fd, SOL_SOCKET, SO_REUSEADDR, (char *) &tmp,
		  sizeof (tmp));

  sockaddr.sin_family = PF_INET;
  sockaddr.sin_port = htons ((unsigned short)port);
  sockaddr.sin_addr.s_addr = INADDR_ANY;
  if (bind (server_fd, (struct sockaddr *) &sockaddr, sizeof (sockaddr))
	|| listen (server_fd, 1))
  { perror ("Can't bind address\n");
    server_fd=INVALID_SOCKET;
    return 0;
  }
#ifdef DEBUG
  fprintf(stderr, "Server open\n");
#endif
  return 1;
}


/* Open a connection to a remote debugger. */
static int remote_open (int port)
{
 
#ifdef WIN32
  int tmp;
#else
  socklen_t tmp;
#endif
  server_open(port);
  fprintf(stderr,"Connecting to gdb ...");
  tmp = sizeof (sockaddr);
  remote_fd = (int)accept (server_fd, (struct sockaddr *) &sockaddr, &tmp);
  server_close();
  if (!valid_socket(remote_fd))
  {  perror ("Accept failed");
     return 0;
  }
  else
  { /* Enable TCP keep alive process. */
    tmp = 1;
    setsockopt (remote_fd, SOL_SOCKET, SO_KEEPALIVE, (char *) &tmp, sizeof (tmp));

    /* Tell TCP not to delay small packets.  This greatly speeds up
       interactive response. */
    tmp = 1;
    setsockopt (remote_fd, IPPROTO_TCP, TCP_NODELAY,
	       (char *) &tmp, sizeof (tmp));

        /* Convert IP address to string. 
    fprintf (stderr, "Remote debugging from host %s\n", 
    inet_ntoa (sockaddr.sin_addr));
	*/
    fprintf(stderr,"Connected\n");
  }
  return 1;
}

/* end open and close sockets */

/* reading and writing the sockets */


/* flush output to gdb */
static char writebuf[BUFSIZ];
static int writebufcnt = 0;

static int flush(void)
/* return <0 for error */
{ int i;
 int error=0;
#ifdef DEBUG
  fprintf(stderr, "To gdb:>");
  for (i=0;i<writebufcnt;i++)
    fprintf(stderr, "%c",writebuf[i]);
  fprintf(stderr, "<\n");
#endif
  i=0;
  while (i <writebufcnt)
  { error = send (remote_fd, writebuf+i, writebufcnt - i,0);
    if (error<0)
	{
	  perror ("flushing");
          remote_close();
	  break;
	}
    else
      i = i+error;
  }
  writebufcnt=0;
  return error;
}

/* Write a char to remote GDB.  -1 if error.  */

static int
writechar (char c)
{
  writebuf[writebufcnt++]=c;
  if (writebufcnt>=BUFSIZ)
    return flush();
  else  
    return 1;
}



int
putpkt (char *buf)
/* Send a packet to the remote machine, with error checking.
   The data of the packet is in BUF.  Returns 1 on success, 0 otherwise. */
{
  int i;
  unsigned char csum = 0;

  writechar('$');

  for (i = 0; buf[i]!=0 ; i++)
    {
      csum += buf[i];
      writechar(buf[i]);
    }
  writechar('#');
  writechar(tohex ((csum >> 4) & 0xf));
  writechar(tohex (csum & 0xf));
  return (0 <= flush());
}

int getack(void)
/* get a positive ack.  */
{ char *cmd;
  char ack;
  cmd = dequeue(&cmd_buffers);
  if (cmd == NULL)
  {
     remote_close();
     return 0;
  }
  else 
    ack = *cmd;
#ifdef DEBUG
      fprintf(stderr, "got ack %c\n",ack);  
#endif   
  if (ack!=ACK && ack != REJECT)
     fprintf (stderr, "getack: Got %c(%02X) command %s ignored\n",ack,ack,cmd);
  put_free_buffer(cmd);

  return (ack == ACK);
}


/* Returns next char from remote GDB.  -1 if error.  */
  static unsigned char read_buf[BUFSIZ];
  static int read_bufcnt = 0;
  static unsigned char *read_bufp;

static int
readchar ()
{

  if (read_bufcnt-- > 0)
    return *read_bufp++;

  read_bufcnt = recv (remote_fd, read_buf, BUFSIZ,0);

  if (read_bufcnt <= 0)
    {
#ifdef DEBUG
      perror ("readchar");
#endif
      return -1;
    }
#ifdef DEBUG
  else
  { int i;
    fprintf(stderr, "From gdb:>");
    for (i=0;i<read_bufcnt;i++)
		if (read_buf[i]<=0x20 || read_buf[i]>=0x7F)
		   fprintf(stderr, "%c(%2X)",read_buf[i],read_buf[i]);
		else
		   fprintf(stderr, "%c",read_buf[i]);
    fprintf(stderr, "<\n");
  }
#endif

  read_bufp = read_buf;
  read_bufcnt--;
  return *read_bufp++;
}

/* Read a packet from the remote machine, with error checking,
   and store it in BUF.  Returns length of packet, or negative if error. */

static int
getpkt (char *buf)
/* return length of packet>0 on success or  0 on error */
{
  char *bp;
  unsigned char csum, csend;
  int c;
  
  csum = 0;
  buf[1]=0;
  if ((c = readchar ()) < 0)  return 0;
  else  if (c != '$') /* short command */
  { buf[0] = c; 
    return 1;
  }
  /* long command */
  bp = buf;
  while (1)
  {  if ((c = readchar ()) < 0)  return 0;
     if (c == '#') break;
     *bp++ = c;
     csum += c;
  }
  *bp = 0;
  if ((c = readchar ()) < 0)  return 0;
  csend = fromhex ((char)c);
  if ((c = readchar ()) < 0)  return 0;
  csend = (csend<<4)+ fromhex ((char)c);
  if (csum != csend)
  { fprintf (stderr, "Bad checksum, sent=0x%02x, computed=0x%02x, buf=%s\n",
	    csend, csum, buf);
    buf[0]=BAD; /* mark the package as bad */
  }
  return 1;
}


int putack(char c)
{ writechar(c); 
  return (0 <= flush());
}

/* end reading and writing */

/* WIN32 and posix threads*/
#ifdef WIN32
typedef LPVOID thread_data;
typedef DWORD thread_return;
#define THREAD_FUNCTION(name, data) DWORD WINAPI name(LPVOID data)
typedef THREAD_FUNCTION(thread_function_ptr, dummy);
#else
typedef void *thread_data;
typedef void *thread_return;
#define THREAD_FUNCTION(name, data) thread_return name(thread_data data)
typedef THREAD_FUNCTION(thread_function_ptr, dummy);
#endif

static void start_thread(thread_function_ptr f, thread_data d)
#ifdef WIN32
{ DWORD dwThreadId;
  HANDLE hThread;
  hThread = CreateThread( 
          NULL,              // default security attributes
          0,                 // use default stack size  
          f,        // thread function 
          d,             // argument to thread function 
          0,                 // use default creation flags 
          &dwThreadId);   // returns the thread identifier 
      // Check the return value for success. 
  if (hThread == NULL) 
      vmb_fatal_error(__LINE__, "Creation of gdb thread failed");
      /* in the moment, I really dont use the handle */
  CloseHandle(hThread);
}
#else
{ int tid;
  pthread_t thr;
  tid = pthread_create(&thr,NULL,f,d);
  if (tid != 0) 
    vmb_fatal_error(__LINE__, "Creation of gdb thread failed");
}
#endif


/* the read loop  makes sure it has a connection
   and forwards commands to the gdb_loop
*/




static THREAD_FUNCTION(gdb_read_loop, dummy)
{ char *read_buffer;
#ifdef WIN32
  int port = (int)(LPARAM)dummy;
#else
  int port = (int)dummy;
#endif
#ifdef DEBUG
  fprintf(stderr, "Starting read loop\n");
#endif
start_read:
   if (!valid_socket(remote_fd))
     remote_open(port);
   if (!valid_socket(remote_fd))  
   { fprintf(stderr, "Unable to get gdb connection socket\n");
     exit(1);
   }
  do {
    read_buffer = get_free_buffer();
    if (read_buffer==NULL)
    { perror("No free buffer available for gdb command");
      remote_close();
      exit(1);
    }
    if (!getpkt(read_buffer))
    { put_free_buffer(read_buffer);
      remote_close();
      goto start_read;
    }
#ifdef DEBUG
    fprintf(stderr, "New command: >%s<\n",read_buffer);
#endif
    put_cmd_buffer(read_buffer);
  } while (1);
  remote_close();
  return 0;
}


static THREAD_FUNCTION(gdb_loop, dummy)
{ char *cmd;
#ifdef DEBUG
  fprintf(stderr, "Starting gdb loop\n");
#endif
  do {
    cmd = get_cmd_buffer(); 
#ifdef DEBUG
      fprintf(stderr, "got command %s\n",cmd);  
#endif   
    if (cmd[0]== BAD) 
    { put_free_buffer(cmd);
      putack(REJECT);
      continue;
    }
    else if  (cmd[0]==ACK||cmd[0]==REJECT)
    { fprintf(stderr, "ignoring extra packet %s\n",cmd);
      put_free_buffer(cmd);
      continue;
    }
    else if  (cmd[0]==STOPED)
    {
#ifdef DEBUG
      fprintf(stderr, "ignoring TERM message %s\n",cmd);  
#endif   
      put_free_buffer(cmd);
      continue;
    }
    else
      putack(ACK);
    /* ask question */
#ifdef DEBUG
      fprintf(stderr, "handling command %s\n",cmd);  
#endif   
    handle_gdb_command(cmd);
    do {
      cmd = get_cmd_buffer(); 
#ifdef DEBUG
      fprintf(stderr, "got answer %s\n",cmd);  
#endif   
      if (async_gdb_command(cmd))
      { put_free_buffer(cmd);
        continue;
      } 
      else if  (cmd[0]==ACK||cmd[0]==REJECT||cmd[0]== BAD)
      { fprintf(stderr, "ignoring extra packet %s\n",cmd);
        put_free_buffer(cmd);
        continue;
      }
      /* got answer */
#ifdef DEBUG
      fprintf(stderr, "send answer %s\n",cmd);  
#endif   
     putpkt(cmd);
      if (!getack())
      { putpkt(cmd);
        getack();
      }
      put_free_buffer(cmd);
      break;
    } while (1);
  } while (1);
  return 0;
}


/* WIN32 and posix atomic data and events */

static int cpu_continue=0;
#ifdef WIN32
  static HANDLE hcontinue;
  static CRITICAL_SECTION   continue_section;
  #define LOCK   EnterCriticalSection (&(continue_section))
  #define UNLOCK LeaveCriticalSection (&(continue_section))
  #define WAIT   WaitForSingleObject(hcontinue,INFINITE)
  #define SIGNAL SetEvent (hcontinue);
#else
  static pthread_mutex_t continue_mutex;
  static pthread_cond_t continue_cond;
  static void clean_up_queue_mutex(void *dummy)
  { pthread_mutex_unlock(&(continue_mutex)); 
  }
  #define LOCK   pthread_mutex_lock(&(continue_mutex));\
                 pthread_cleanup_push(clean_up_queue_mutex,0)
  #define UNLOCK pthread_cleanup_pop(1);
  #define WAIT   pthread_cond_wait(&(continue_cond),&(continue_mutex))
  #define SIGNAL pthread_cond_signal(&(continue_cond))
#endif  

void continue_init(void)
{
#ifdef WIN32
  hcontinue =CreateEvent(NULL,FALSE,FALSE,NULL);
  if (!hcontinue) 
  { fprintf(stderr, "Unable to create event for cpu continue\n");
    exit(1);
  }
  InitializeCriticalSection (&(continue_section));
#else
  pthread_mutex_init(&(continue_mutex),NULL);
  pthread_cond_init(&(continue_cond),NULL);
#endif
}



int wait_for_continue(void)
/* returns 1 if the simulaton continues 
   return 0 if the simulation stops
*/
{ int r;
#ifdef WIN32
  LOCK;
  cpu_continue=0;
  while (!cpu_continue)
  { UNLOCK;
    WAIT;
    LOCK;
  }
  r = cpu_continue-1;
  UNLOCK;
#else
  LOCK;
  cpu_continue=0;
  while (!cpu_continue)
  { WAIT;
  }
  r = cpu_continue-1;
  UNLOCK;
#endif
  return r;
}



void signal_continue(int r)
/* r==1 if the simulaton continues 
   r==0 if the simulation stops
*/
{ 
#ifdef WIN32
  LOCK;
  cpu_continue=1+r;
  UNLOCK;
  SIGNAL;
#else
  LOCK;
  cpu_continue=1+r;
  SIGNAL;
  UNLOCK;
#endif
}



int gdb_init(int port)
     /* initialize the connection to gdb,
        return 1 on success, 0 on failure
     */
{ 
#ifdef DEBUG
  fprintf(stderr, "Initializing server on port %d\n", port);
#endif
  buffers_init();
  continue_init();
#ifndef WIN32
  /* If we don't do this, then gdbserver simply exits when the remote side dies. */
  signal (SIGPIPE, catchpipe);	
  start_thread(gdb_read_loop,(thread_data)port);
#else
  start_thread(gdb_read_loop,(thread_data)(LPARAM)port);
#endif
  start_thread(gdb_loop,0);
  return 1;
}
