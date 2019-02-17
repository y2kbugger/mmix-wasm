/*
  Copyright 2005 Anton Wolf, Martin Ruckert
    
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

extern int terminate_flag;

#ifdef WIN32
#pragma warning(disable : 4996)
#include <winsock2.h>
#include <windows.h>
#include <commctrl.h>
#include <process.h>
#include "resource.h"
#include "winopt.h"
#include "opt.h"


typedef int socklen_t;

HWND hpower;

#define WM_SOCKET (WM_USER+1)

/* Windows needs a different main program (taken from win32main.c) */
HINSTANCE hInst;
HWND hMainWnd;
HBITMAP hBmp=NULL;
HMENU hMenu;
HBITMAP hon, hoff, hconnect;
device_info vmb = {0};

#else

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include "errno.h"

#define Sleep(x) usleep((x)*1000)
#define closesocket(x) close(x)

#endif
#include "vmb.h"
#include "option.h"
#include "param.h"
#include "error.h"  
#include "message.h"
#include "bus-arith.h"

extern int vmb_power_flag;
int major_version=2, minor_version=1;
char version[] = "$Revision: 664 $ $Date: 2018-12-10 18:10:14 +0100 (Mon, 10 Dec 2018) $";
char title[] = "VMB Motherboard";
char howto[] =
  "\n"
  "The program first reads the configuration file, \"default.vmb\".\n"
  "Then, the program waits for other simulated hardware modules\n"
  "to connect. It recieves and redirects messages\n"
  "from and to these modules.\n"
#ifndef WIN32
  "Further it executes the following commands\n"
  "\n"
  "help\t\tdisplay this help message\n"
  "debug\t\tswitch on debugging output\n"
  "nodebug\t\tswitch off debugging output\n"
  "verbose\t\tincrease verbosity (more output)\n"
  "quiet\t\tdecrease verbosity (less output)\n"
  "quit\t\tterminte all slots and the motherboard\n"
  "on\t\tsend all slots the power on message\n"
  "off\t\tsend all slots the power off message\n"
  "reset\t\tsend all slots the reset message\n"
  "terminate\t\tsend all slots the terminate message\n"
  "info\t\tdisplay information about the slots in use\n"
  "close n\t\tclose slot number n\n"
  "slot n\t\tdisplay details about slot n\n"
  "signal n\t\tsend signal n to the devices\n" "\n"
#endif
  ;

#define SLOTS 256

static struct
{
  int fd;			/* File Descriptor */

  unsigned char *name;
  unsigned int major_version, minor_version;
  unsigned char from_addr[8];	/* Address from */
  unsigned char to_addr[8];	/* Address to */
  unsigned int hi_mask, low_mask;	/* Enabled for interrupts */
  struct sockaddr addr;		/* Network Address */
  int answers_pending;
  int has_power;
} slot[SLOTS];

static int exitflag = 0;
static int powerflag = 0;
static int read_ops = 0;
static int write_ops = 0;
static int server_terminating=0;

static int max_slot = 0;	/* Strict upper Bound on Slot Numbers */
static int mother_fd = INVALID_SOCKET;	/* Server File Descriptor */
static int max_fd = 0;		/* Biggest File Descriptor */
static fd_set read_fdset, write_fdset;
static struct sockaddr_in mother_addr;	/* Server Network Address */

/*Message*/

static unsigned char mtype;
static unsigned char msize;
static unsigned char mslot;
static unsigned char mid;
static unsigned int mtime;
static unsigned char maddress[8];
static unsigned char mpayload[MAXPAYLOAD];

void
initialize_slots ()
{
  int i;
  memset (slot, 0, sizeof (slot));

  for (i = 0; i < SLOTS; i++)
  {
    slot[i].fd = INVALID_SOCKET;
  }
  max_fd = mother_fd;
  max_slot = 0;
}

void
create_server ()
{
  mother_fd = (int)socket (AF_INET, SOCK_STREAM, IPPROTO_TCP /* was 0 */);
  if (!valid_socket (mother_fd))
    vmb_fatal_error (__LINE__, "Can't create new server socket");

  

  /* Allow rapid reuse of this port. */
  {
    int tmp = 1;
    if (0!=setsockopt (mother_fd, SOL_SOCKET, SO_REUSEADDR, (char *) &tmp,
		sizeof (tmp)))
      { int e;
#ifdef WIN32
	e = WSAGetLastError();
#else
        e = errno;
#endif
      vmb_fatal_errori(__LINE__,"Unable to set socket options",e);
      }
  }
  max_fd = mother_fd;

  mother_addr.sin_family = AF_INET;
  mother_addr.sin_port = htons ((unsigned short) port);
  mother_addr.sin_addr.s_addr = htonl(INADDR_ANY);

  if ((bind
       (mother_fd, (struct sockaddr *) &mother_addr,
	sizeof (mother_addr))) != 0)
  {
    closesocket(mother_fd);
    vmb_fatal_error (__LINE__, "Can't bind server socket to address");
  }


  if (listen (mother_fd, SOMAXCONN) != 0)
  {
    closesocket(mother_fd);
    vmb_fatal_error (__LINE__, "Can't listen");
  }



/* make the socket non blocking */
#ifdef WIN32
  if (!hMainWnd) vmb_fatal_error(__LINE__,"Window undefined");
  if (0!=WSAAsyncSelect(mother_fd, hMainWnd, WM_SOCKET, FD_READ | FD_CONNECT |FD_ACCEPT | FD_CLOSE))
    vmb_fatal_errori(__LINE__,"Unable to set up events",WSAGetLastError());
#else
  {
    int flags;
    flags = fcntl (mother_fd, F_GETFL);
    flags |= O_NONBLOCK;
    fcntl (mother_fd, F_SETFL, flags);
  }
#endif
  vmb_debugi(VMB_DEBUG_PROGRESS,"Created server at Port %d", port);
}

void
remove_slot (int slotnr)
{
  if (slot[slotnr].name != NULL)
    free (slot[slotnr].name);
  memset(&slot[slotnr],0,sizeof(slot[slotnr]));
  slot[slotnr].fd = INVALID_SOCKET;
  while (max_slot > 0 && !valid_socket (slot[max_slot - 1].fd))
    max_slot--;
  vmb_debugi(VMB_DEBUG_INFO,"Removed Slot %d", slotnr);
}


int
write_to_slot (int i)
{ if ((mtype & TYPE_ROUTE) && (slot[i].answers_pending>0))
    slot[i].answers_pending--;
  if (send_msg(&(slot[i].fd), mtype, msize, mslot, mid, mtime, maddress,mpayload) <= 0)
  { if (server_terminating) 
      return 1; /* dont bother */
    vmb_error2 (__LINE__, "Unable to deliver message to:", (char *)slot[i].name);
    remove_slot (i);
    return 1;
  }
  vmb_debugs(VMB_DEBUG_PROGRESS,"Send to %s:", (char *)slot[i].name);
  vmb_debugm(VMB_DEBUG_MSG, mtype, msize, mslot, mid, maddress, mpayload);
  write_ops++;
  return 0;
}
static int
bus_msg (unsigned char id, int i)
{
  mtype = TYPE_BUS;
  msize = 0;
  mslot = 0;
  mid = id;
  return write_to_slot (i);
}
static void
power_on (int i)
{
  if (!bus_msg (ID_POWERON, i))
  {  vmb_debugi(VMB_DEBUG_INFO,"Sent Power On to Slot %d", i);
     slot[i].has_power=1;
  }
  else
	  vmb_error(__LINE__,"Power On was not sent");
}

static void
power_off (int i)
{
  if (!bus_msg (ID_POWEROFF, i))
  {  vmb_debugi(VMB_DEBUG_INFO,"Sent Power Off to Slot %d", i);
       slot[i].has_power=0;
  }
  else if (!server_terminating)
    vmb_error(__LINE__,"Power Off was not sent");
}

static void
reset (int i)
{
  if (!bus_msg (ID_RESET, i))
    vmb_debugi(VMB_DEBUG_INFO,"Sent Reset to Slot %d", i);
  else
    vmb_error(__LINE__,"Reset was not sent");
}
static void
terminate (int i)
{
  if (!bus_msg (ID_TERMINATE, i))
    vmb_debugi(VMB_DEBUG_INFO,"Sent Terminate to Slot %d", i);
  else if (!server_terminating)
    vmb_error(__LINE__,"Terminate was not sent");
}

static void
send_dummy_answer (unsigned char id, int dest_slot)
{
  if (valid_socket (slot[dest_slot].fd))
  {
    mtype =  TYPE_ADDRESS | TYPE_ROUTE; /* mark it as an answer with address */
    mid = id;     /* this is a dummy reply */
    if (!write_to_slot (dest_slot))
      vmb_debugi(VMB_DEBUG_NOTIFY,"Sent dummy answer to Slot %d", dest_slot);
    else if (!server_terminating)
      vmb_error(__LINE__,"Dummy answer was not sent");
  }
}

static void
disconnect_device (int slotnr)
{ int error;
  while (slot[slotnr].answers_pending > 0)
  { vmb_debugs(VMB_DEBUG_NOTIFY,"\tpending answers for %s", (char *)slot[slotnr].name);
    vmb_debugi(VMB_DEBUG_NOTIFY,"\tpending answers:    %d", slot[slotnr].answers_pending);
    send_dummy_answer (ID_NOREPLY, slotnr);
  }
  if (powerflag&& slot[slotnr].has_power)
    power_off (slotnr);
  terminate(slotnr);
  Sleep(50);
#ifdef WIN32
   error = shutdown(slot[slotnr].fd,SD_BOTH);
#else
   error = shutdown(slot[slotnr].fd,SHUT_RDWR);
#endif
   if (!error)
    vmb_debugi(VMB_DEBUG_NOTIFY,"Shutdown of Slot %d", slotnr);
  else if (!server_terminating)
	  vmb_error2(__LINE__,"Unable to shut down slot",(char *)slot[slotnr].name);
  remove_slot (slotnr);
}

static int
find_slot (unsigned char address[8])
{
  int i;
  for (i = 0; i < max_slot; i++)
    if (valid_socket (slot[i].fd) &&
	in_range (slot[i].from_addr, address, slot[i].to_addr))
      return i;
  return -1;
}

static void
for_all_slots (void proc (int i))
{
  int i;
  for (i = 0; i < max_slot; i++)
    if (valid_socket (slot[i].fd))
      proc (i);
}

static void
terminate_all (void)
{
  for_all_slots (terminate);
  vmb_debug (0, "All slots terminated");
  Sleep(50);
}

static void
shutdown_server ()
{ server_terminating=1;
  terminate_all();
  for_all_slots (disconnect_device);
  if (closesocket(mother_fd)==0)
    vmb_debugi(VMB_DEBUG_PROGRESS,"Shutdown server at Port %d : Successful", port);
}

static void
add_to_read_fdset (int i)
{
  FD_SET (slot[i].fd, &read_fdset);
}

static void
build_read_fdset ()
{
  FD_ZERO (&read_fdset);
  FD_SET (0, &read_fdset);
  FD_SET (mother_fd, &read_fdset);
  for_all_slots (add_to_read_fdset);
}

static void
send_interrupt_to (int i)
{
  if (((mslot < 32) && (slot[i].low_mask & BIT (mslot))) ||
      ((mslot >= 32) && (slot[i].hi_mask & BIT (mslot - 32))))
  {
    if (!write_to_slot (i))
      vmb_debugi(VMB_DEBUG_PROGRESS,"Sent Interrupt to Slot %d", i);
    else if (!server_terminating)
      vmb_error(__LINE__,"Interrupt was not sent");
  }
}

static void
interrupt_all (int i)
{
  mtype = TYPE_BUS;
  msize = 0;
  mslot = i;
  mid = ID_INTERRUPT;
  vmb_debugi(VMB_DEBUG_PROGRESS,"Sending Interrupt %d", mslot);
  for_all_slots (send_interrupt_to);
}

static void
power_all (int state)
{
  if (powerflag == state)
    return;
  powerflag = state;
  vmb_debugs(VMB_DEBUG_PROGRESS,"All slots  power %s", state ? "on" : "off");
  if (powerflag)
    for_all_slots (power_on);
  else
    for_all_slots (power_off);
#ifdef WIN32
  if (powerflag)
    SendMessage (hpower, STM_SETIMAGE, (WPARAM) IMAGE_BITMAP, (LPARAM) hon);
  else
    SendMessage (hpower, STM_SETIMAGE, (WPARAM) IMAGE_BITMAP, (LPARAM) hoff);
#endif
}

static void
reset_all (void)
{
  for_all_slots (reset);
  vmb_debug (0, "All slots reset");
}

static void
reset_all_except (int x)
{ int i;
  for (i = 0; i < max_slot; i++)
    if (i!=x && valid_socket (slot[i].fd))
      reset (i);
  vmb_debugi (0, "All slots except %d reset", x);
}

static void
interpret_message (int source_slot)
{ read_ops++;
  if (slot[source_slot].name!=NULL)
    vmb_debugs(VMB_DEBUG_PROGRESS,"Message received from %s.",(char *)slot[source_slot].name);
  else
	vmb_debugi(VMB_DEBUG_PROGRESS,"Message received from slot %d.",source_slot);
  vmb_debugm(VMB_DEBUG_MSG, mtype, msize, mslot, mid, maddress, mpayload);
  if (mtype & TYPE_REQUEST)
    slot[source_slot].answers_pending++;
  if (mtype & TYPE_REQUEST)
	mslot = source_slot;
  if (mtype & TYPE_BUS)
  {
    switch (mid)
    {
    case (ID_IGNORE):
      break;
    case (ID_INTERRUPT):
      interrupt_all (mslot);
      break;
    case (ID_RESET):
      reset_all_except(source_slot);
      break;
    case (ID_POWERON):
      power_all(1);
      break;
    case (ID_POWEROFF):
      power_all(0);
      break;
    
    case (ID_REGISTER):
      memmove (slot[source_slot].from_addr, &mpayload[0], 8);
      memmove (slot[source_slot].to_addr, &mpayload[8], 8);
      slot[source_slot].hi_mask = chartoint (&mpayload[16]);
      slot[source_slot].low_mask = chartoint (&mpayload[20]);
      if (msize > 2)
      { int i, n;
        n = (int)strlen((char *)mpayload + 24) + 1;
	i=(n+7)/8; /* number of octas used by name */
	if (slot[source_slot].name!=NULL) free(slot[source_slot].name);
	    slot[source_slot].name = malloc (n);
	if (slot[source_slot].name == NULL)
	      vmb_error(__LINE__,"Out of memory");
	else
	      strncpy ((char *)slot[source_slot].name, (char *)mpayload + 24,n);
	if (msize > 2+i)
	{ slot[source_slot].major_version = chartoint (&mpayload[24+i*8]);
	  slot[source_slot].minor_version = chartoint (&mpayload[24+i*8+4]);
	}
      }
      if (powerflag&&!slot[source_slot].has_power)
	    power_on (source_slot);
      break;
    default:
      break;
    }
  }
  else if (mtype & TYPE_ROUTE)
  {
    int dest_slot = mslot;
    if (mslot >= max_slot)
    {
      vmb_debugi(VMB_DEBUG_ERROR,"Slot number %d not available", mslot);
      if (mtype & TYPE_REQUEST)
	     send_dummy_answer (ID_NOREPLY, source_slot);
	  else
	     send_dummy_answer (ID_NOWRITE, source_slot);
      return;
    }
    if (mtype & TYPE_REQUEST)
      mslot = source_slot;
    if (write_to_slot (dest_slot) && (mtype & TYPE_REQUEST))
      send_dummy_answer (ID_NOREPLY, source_slot);
  }
  else if (mtype & TYPE_ADDRESS)
  {
    int dest_slot = find_slot (maddress);
    if (dest_slot < 0)
    {
      vmb_debugx(VMB_DEBUG_ERROR,"Address #%s not available", maddress, 8);
      if (mtype & TYPE_REQUEST)
	     send_dummy_answer (ID_NOREPLY, source_slot);
	  else
	     send_dummy_answer (ID_NOWRITE, source_slot);
    }
    else
    {
      if (write_to_slot (dest_slot) && (mtype & TYPE_REQUEST))
	  { if (mtype & TYPE_REQUEST)
	      send_dummy_answer (ID_NOREPLY, source_slot);
	    else
	      send_dummy_answer (ID_NOWRITE, source_slot);
	  }
    }
  }
}

static void
close_slot (int i)
{
  if (i >= 0 && i < SLOTS && valid_socket (slot[i].fd))
    disconnect_device (i);
  else
    vmb_error(__LINE__,"Invalid slot number");
}

#if 0
static void make_blocking(int fd)
{  /* make the socket blocking */
#ifdef WIN32
  if (hMainWnd)
    WSAAsyncSelect(fd, hMainWnd, WM_SOCKET, 0);
  else
  {
    WSAEVENT e;
    e = WSACreateEvent ();
    WSAEventSelect (mother_fd, e,0);
  }
  { u_long arg=0;
    ioctlsocket(fd,FIONBIO,&arg);
  }
#else
  {
    int flags;
    flags = fcntl (fd, F_GETFL);
    flags &= ~O_NONBLOCK;
    fcntl (mother_fd, F_SETFL, flags);
  }
#endif
}
#endif

static int
connect_new_device (void)
/* return true if successful, possibly more connections can be made */
{
  int i;
  for (i = 0; i < SLOTS; i++)
    if (!valid_socket (slot[i].fd))
    { int fd;
	  socklen_t addrlen;
      addrlen = sizeof (slot[i].addr);
      fd = (int)accept (mother_fd, &(slot[i].addr), &addrlen);
	  if (!valid_socket (fd))
	  {
#ifdef WIN32
	    int error = WSAGetLastError();
            if (error == WSAEWOULDBLOCK) return 0; /* we are done */
		if (error == WSAECONNRESET ||
			error == WSAEINTR ||
			error == WSAEINPROGRESS ||
			error == WSAEMFILE ||
			error == WSATRY_AGAIN)  
		{ vmb_debug(VMB_DEBUG_NOTIFY,"Connection delays");
			return 1; /* try again */
		}
#endif
	    vmb_debug(VMB_DEBUG_ERROR,"Unsuccessful connection attempt");
	    return 0; /* give up */
	  }
	  vmb_debugi(VMB_DEBUG_PROGRESS,"Got new device on fd %d",fd);
	  slot[i].fd = fd; 
	  slot[i].answers_pending = 0;
	  slot[i].has_power = 0;
      if (i >= max_slot)
	     max_slot = i + 1;
      if (slot[i].fd > max_fd)
	    max_fd = slot[i].fd;
      vmb_debugi(VMB_DEBUG_PROGRESS,"New Connection established on slot %d",i);
	  /* to delay small packets.  This greatly speeds up
				   interactive response. */
	  { int tmp = 1;
	    if (setsockopt (slot[i].fd, IPPROTO_TCP, TCP_NODELAY,
		    (char *) &tmp, sizeof (tmp))!=0)
        {
#ifdef WIN32
		  vmb_debugi(VMB_DEBUG_ERROR,"Unable to set socket options, error: %d", WSAGetLastError());
#else
		  vmb_debug(VMB_DEBUG_ERROR,"Unable to set socket options");
#endif
          return 1; /* try again */
        }
	  }
#if 0
	  make_blocking(slot[i].fd);  
#endif
	vmb_debug(VMB_DEBUG_PROGRESS,"Socket connected");
      return 1;
    }
  vmb_error(__LINE__,"Can't connect any more client's");
  return 0;
}

#ifdef WIN32

void bus_read_error(int *socket)
{ int error;
  error = shutdown(*socket,SD_BOTH);
  if (error!=0)
	    vmb_errori(__LINE__,"Shutdown socket",WSAGetLastError());
  *socket=INVALID_SOCKET;
}

void bus_write_error(int *socket)
{ int error;
  error = shutdown(*socket,SD_BOTH);
  if (error!=0)
	    vmb_errori(__LINE__,"Shutdown socket",WSAGetLastError());
    *socket=INVALID_SOCKET;
}

static void
close_socket (int fd)
{
  int i;
  for (i = 0; i < max_slot; i++)
    if (valid_socket (slot[i].fd) && slot[i].fd == fd)
    { 
      remove_slot (i);
      return;
    }
}
#endif

#ifdef WIN32
static void
process_read_fdset (int fd)
{
  int i;
  for (i = 0; i < max_slot; i++)
  {
    if (valid_socket (slot[i].fd) && slot[i].fd == fd)
    {
      int error;
      error =
	receive_msg (&(slot[i].fd), &mtype, &msize, &mslot, &mid, &mtime,
		     maddress, mpayload);
      if (error < 0)
	  { vmb_error2(__LINE__, "Unable to read message, closing ", (char *)slot[i].name);
	    remove_slot (i);
	  }
      else if (error > 0)
	    interpret_message (i);
	  /* ignore error == 0 */
	  return;
    }
  }
}
#else
static void
process_read_fdset ()
{
  static int i = 0;
  int k;
  for (k = 0; k < max_slot; k++)
  {
    i = (i + 1) % (max_slot);
    if (valid_socket (slot[i].fd) && FD_ISSET (slot[i].fd, &read_fdset))
    {
      int error;
      error =
	receive_msg (&(slot[i].fd), &mtype, &msize, &mslot, &mid, &mtime,
		     maddress, mpayload);
      if (error < 0)
	remove_slot (i);
      else if (error > 0)
	interpret_message (i);
	  /* ignore error == 0 */
	  return;
    }
  }
}
#endif

#define MAX_EXEC 256

static int command_count=0;
static char *commands[MAX_EXEC]={0};


void store_command(char *command)
{ vmb_debugs(VMB_DEBUG_PROGRESS, "storing command: %s",command);
  if (command_count>=MAX_EXEC)
    vmb_error(__LINE__,"Too many commands");
  else
  { set_option(&commands[command_count],command);
    command_count++;
  }
}

#ifdef WIN32
static char * quote(char *str)
/* allocate string and return "str" */
{ int n;
  char *quoted;
  n = (int)strlen(str);
    quoted = malloc(n+3);
  if (quoted == NULL)
	{ vmb_fatal_error(__LINE__,"Out of Memory for quoting");
	  return NULL;
	}
   if (str[0]!='"')
   { strcpy(quoted+1, str);
     quoted[0]=quoted[n+1]='"';
	 quoted[n+2]=0;
   }
   else
	   strcpy(quoted, str);
   return quoted;
}


static char * unquote(char *str)
/* allocate string and return str with quotes removed */
{ int n;
  char *unquoted;
  n = (int)strlen(str);
  unquoted = malloc(n+1);
  if (unquoted == NULL)
	{ vmb_fatal_error(__LINE__,"Out of Memory for unquoting");
	  return NULL;
	}
   if (str[0]=='"')
   { strcpy(unquoted, str+1);
     unquoted[n-2]=0;
   }
   else
	 strcpy(unquoted, str);
   return unquoted;
}

#endif

void do_commands(void)
{ int i;
  for (i=0; i<command_count; i++)
    if (commands[i]!=NULL)
      { 
        char *argv[MAXARG] = {0};
	char argc;
	vmb_debugs(VMB_DEBUG_PROGRESS, "executing command: %s",commands[i]);
        argc = mk_argv(argv,commands[i],0);
	if (argc<=0)
          continue;
	Sleep(50); /* so delay 50 ms start processes in order given */
#ifdef WIN32
	
#define MAXPROG 512
	{ char prog[MAXPROG];
	  char *name;
      char *FilePart;
	  name = unquote(argv[0]);
	  if (SearchPath(NULL,name,".exe",MAXPROG,prog,&FilePart)<=0)
		  vmb_error2(__LINE__,"Unable to find command",argv[0]);
	  else 
	  { free(name);
		argv[0]= name = quote(prog);
#if 0
        STARTUPINFO sui = {0};
	    PROCESS_INFORMATION pi = {0};
		sui.cb=sizeof(sui);
	    if (CreateProcess(argv[0],commands[i],NULL,NULL,FALSE,
			DETACHED_PROCESS,NULL,NULL,&sui,&pi))
		{ CloseHandle(pi.hProcess);
		  CloseHandle(pi.hThread);
		}
		else
#else
		if (_spawnvp(_P_NOWAIT,prog,argv)<0)
#endif
	      vmb_error2(__LINE__,"Unable to execute command",argv[0]);
	  }
	  free(name);
	}
#else
    { pid_t p;
          p = fork();
          if (p<0) vmb_error(__LINE__,"Unable to create new process");
          else if (p==0) /* child */
	  { execvp(argv[0],argv);
            vmb_error2(__LINE__,"Unable to execute command",argv[0]);
	  }
	}
#endif
	}
}





#ifdef WIN32


#define MAX_LOADSTRING 100
/* Global Variables: */
TCHAR szClassName[MAX_LOADSTRING] = "VMB";

static int infoslot = -1;

static void
display_slot (HWND hDlg, int i)
{
  if (valid_socket (slot[i].fd))
  {
    char hex[17] = { 0 };
    char buffer[4];
    int size;
    infoslot = i;
    if (slot[i].name != NULL)
	{ char nv[100];
	  _snprintf(nv,100-1,"%s %d.%d",slot[i].name,slot[i].major_version,slot[i].minor_version);
      SetDlgItemText (hDlg, IDC_SLOTNAME, nv);
	}
    else
      SetDlgItemText (hDlg, IDC_SLOTNAME, "Unnamed");
    chartohex (slot[i].from_addr, hex, 8);
    SetDlgItemText (hDlg, IDC_SLOTSTART, hex);
    chartohex (slot[i].to_addr, hex, 8);
    SetDlgItemText (hDlg, IDC_SLOTLAST, hex);
    size = get_offset (slot[i].from_addr, slot[i].to_addr);
    inttochar (hi_offset, buffer);
    chartohex (buffer, hex, 4);
    inttochar (size, buffer);
    chartohex (buffer, hex + 8, 4);
    SetDlgItemText (hDlg, IDC_SLOTSIZE, hex);
    inttochar (slot[i].hi_mask, buffer);
    chartohex (buffer, hex, 4);
    inttochar (slot[i].low_mask, buffer);
    chartohex (buffer, hex + 8, 4);
    SetDlgItemText (hDlg, IDC_SLOTMASK, hex);
  }
  else
  {
    SetDlgItemText (hDlg, IDC_SLOTNR, "Not connected");
    SetDlgItemText (hDlg, IDC_SLOTSTART, "");
    SetDlgItemText (hDlg, IDC_SLOTLAST, "");
    SetDlgItemText (hDlg, IDC_SLOTSIZE, "");
    SetDlgItemText (hDlg, IDC_SLOTMASK, "");
  }
}

INT_PTR CALLBACK 
InfoDialogProc (HWND hDlg, UINT message, WPARAM wparam, LPARAM lparam)
{
  switch (message)
  {
  case WM_INITDIALOG:
    {
      TCITEM tie;
#define TAB_MAX 32
      char label[TAB_MAX];
      int i;
      tie.mask = TCIF_TEXT;
      tie.iImage = -1;
      for (i = 0; i < max_slot; i++)
      {
    if (slot[i].name==NULL)
		_snprintf (label,TAB_MAX-1,"%d: ", i);
	else
		_snprintf (label,TAB_MAX-1,"%d: %s", i,slot[i].name);
	tie.pszText = label;
	TabCtrl_InsertItem (GetDlgItem (hDlg, IDC_SLOTS), i, &tie);
      }
      if (max_slot > 0)
      {
	TabCtrl_SetCurSel (GetDlgItem (hDlg, IDC_SLOTS), 0);
	display_slot (hDlg, 0);
      }
      return TRUE;
    }
  case WM_NOTIFY:
    { NMHDR *p = (NMHDR *)lparam;
      int i = TabCtrl_GetCurSel (GetDlgItem (hDlg, IDC_SLOTS));
	  if (p->code==TCN_SELCHANGE)
        display_slot (hDlg, i);
    }
    break;
  case WM_SYSCOMMAND:
    if (wparam == SC_CLOSE)
    {
      EndDialog (hDlg, TRUE);
      infoslot = -1;
      return TRUE;
    }
    break;
  case WM_COMMAND:
    if (wparam == IDC_SLOTCLOSE && infoslot >= 0
	&& valid_socket (slot[infoslot].fd))
    {
      close_slot (infoslot);
      display_slot (hDlg, infoslot);
    }
    else if (wparam == IDC_SLOTRESET && infoslot >= 0
	     && valid_socket (slot[infoslot].fd))
      reset (infoslot);
    else if (wparam == IDOK)
    {
      EndDialog (hDlg, TRUE);
      infoslot = -1;
      return TRUE;
    }
    break;
  }
  return FALSE;
}



INT_PTR CALLBACK 
OnOffDialogProc (HWND hDlg, UINT message, WPARAM wparam, LPARAM lparam)
{
  switch (message)
  {
  case WM_INITDIALOG:
    return TRUE;
  case WM_COMMAND:
    if (HIWORD (wparam) == BN_CLICKED)
    {
      power_all (!powerflag);
      return TRUE;
    }
    break;
  }
  return FALSE;
}

INT_PTR CALLBACK 
ResetDialogProc (HWND hDlg, UINT message, WPARAM wparam, LPARAM lparam)
{
  switch (message)
  {
  case WM_INITDIALOG:
    return TRUE;
  case WM_COMMAND:
    if (HIWORD (wparam) == BN_CLICKED)
    {
      reset_all ();
      return TRUE;
    }
    break;
  }
  return FALSE;
}

INT_PTR CALLBACK   
SettingsDialogProc( HWND hDlg, UINT message, WPARAM wparam, LPARAM lparam )
{ return 0; /* dummy stub needed by OptWndProc */
}

LRESULT CALLBACK
WndProc (HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  switch (message)
  {
  case WM_CREATE:
    { 
      HWND h;
      h =
	CreateDialog (hInst, MAKEINTRESOURCE (IDD_ONOFF), hWnd,
		      OnOffDialogProc);
      SetWindowPos (h, HWND_TOP, 235, 15, 0, 0, SWP_NOSIZE | SWP_SHOWWINDOW);
      h =
	CreateDialog (hInst, MAKEINTRESOURCE (IDD_RESET), hWnd,
		      ResetDialogProc);
      SetWindowPos (h, HWND_TOP, 235, 55, 0, 0, SWP_NOSIZE | SWP_SHOWWINDOW);
    }
    hpower =
      CreateWindow ("STATIC", NULL,
		    WS_CHILD | WS_VISIBLE | SS_BITMAP | SS_REALSIZEIMAGE,
		    265, 10, 32, 32, hWnd, NULL, hInst, 0);
    SendMessage (hpower, STM_SETIMAGE, (WPARAM) IMAGE_BITMAP, (LPARAM) hoff);
    return 0;
  case WM_SOCKET:
    {
      int error = WSAGETSELECTERROR (lParam);
      int event = WSAGETSELECTEVENT (lParam);
	  if (error == WSAECONNABORTED)
	  { vmb_debug(VMB_DEBUG_PROGRESS,"Socket Connection aborted");
	  }
      else if (error != 0) 
	  {	 vmb_debugi(VMB_DEBUG_ERROR,"Socket error %d", error);
	  }
      if (event == FD_CLOSE || error != 0)
	  { int error;
	    vmb_debug(VMB_DEBUG_PROGRESS,"Socket Close");
	  	error= closesocket((int)wParam);
	    if (error!=0)
	      vmb_errori(__LINE__,"Close socket",WSAGetLastError());
      	close_socket ((int)wParam);
	  }
      else if (event == FD_READ)
	  { vmb_debug(VMB_DEBUG_PROGRESS,"Socket Read");
    	process_read_fdset ((int)wParam);
	  }
      else if (event == FD_ACCEPT)
	  { vmb_debug(VMB_DEBUG_PROGRESS,"Socket Accept");
	    do ; while(connect_new_device () == 1);
	  }
      else if (event == FD_WRITE)
	    vmb_debug(VMB_DEBUG_PROGRESS,"Socket Write");
      else if (event == FD_CONNECT)
        vmb_debug(VMB_DEBUG_PROGRESS,"Socket Connect");
	  else
		vmb_debug(VMB_DEBUG_NOTIFY,"Unknown Socket Event");
    }
    return 0;
  case WM_COMMAND:
    if (HIWORD (wParam) == 0)	/* Menu */
      switch (LOWORD (wParam))
      {
      case ID_INFO:
	       DialogBox (hInst, MAKEINTRESOURCE (IDD_INFO), hMainWnd,
		   InfoDialogProc);
	       return 0;
      }
    break;
  }
  return (OptWndProc (hWnd, message, wParam, lParam));
}


extern HRGN BitmapToRegion (HBITMAP hBmp);


BOOL
InitInstance (HINSTANCE hInstance)
{
  WNDCLASSEX wcex;
  BITMAP bm;
  hInst = hInstance;
  ZeroMemory (&wcex, sizeof (wcex));
  wcex.cbSize = sizeof (WNDCLASSEX);
  wcex.style = CS_HREDRAW | CS_VREDRAW;
  wcex.lpfnWndProc = (WNDPROC) WndProc;
  wcex.cbClsExtra = 0;
  wcex.cbWndExtra = 0;
  wcex.hInstance = hInstance;
  wcex.hIcon = LoadIcon (hInstance, MAKEINTRESOURCE (IDI_ICON));
  wcex.hCursor = LoadCursor (NULL, IDC_ARROW);
  /*    wcex.hbrBackground      = (HBRUSH)(COLOR_WINDOW+1);
   */
  wcex.lpszMenuName = NULL;
  wcex.lpszClassName = szClassName;
  wcex.hIconSm = LoadIcon (hInstance, MAKEINTRESOURCE (IDI_ICON));
  if (!RegisterClassEx (&wcex))
    return FALSE;
  GetObject (hBmp, sizeof (bm), &bm);
  hMainWnd = CreateWindow (szClassName, title, WS_POPUP,
			   CW_USEDEFAULT,CW_USEDEFAULT , bm.bmWidth, bm.bmHeight,
			   NULL, NULL, hInstance, NULL);
  if (hMainWnd)
  {
    HRGN h = BitmapToRegion (hBmp);
    if (h)
      SetWindowRgn (hMainWnd, h, TRUE);
  }
  return TRUE;
}



int APIENTRY
WinMain (HINSTANCE hInstance,
	 HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
  HACCEL hAccelTable;
  MSG msg;
  WSADATA wsadata;
  HANDLE hMutex;

  hMutex=CreateMutex(NULL,TRUE,"VMB-motherboard-running");
  if (hMutex!=NULL && 
	  GetLastError()==ERROR_ALREADY_EXISTS &&
      MessageBox(NULL,"Previous Instance of motherboard detected! Start a second instance?",
		            "START of VMB motherboard",MB_YESNO|MB_ICONWARNING)!=IDYES)
    { CloseHandle(hMutex);
      return 0;
    }
  
  vmb_message_hook = win32_message;
  vmb_debug_hook = win32_log;
  vmb_error_init_hook = win32_error_init;

  LoadString (hInstance, IDS_CLASS, szClassName, MAX_LOADSTRING);
  hMenu = LoadMenu (hInstance, MAKEINTRESOURCE (IDR_MENU));
  hBmp = (HBITMAP) LoadImage (hInstance, MAKEINTRESOURCE (IDB_BITMAP),
			      IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
  hon = (HBITMAP) LoadImage (hInstance, MAKEINTRESOURCE (IDB_ON),
			     IMAGE_BITMAP, 32, 32, LR_CREATEDIBSECTION);
  hconnect = (HBITMAP) LoadImage (hInstance, MAKEINTRESOURCE (IDB_CONNECT),
				  IMAGE_BITMAP, 32, 32, LR_CREATEDIBSECTION);
  hoff = (HBITMAP) LoadImage (hInstance, MAKEINTRESOURCE (IDB_OFF),
			      IMAGE_BITMAP, 32, 32, LR_CREATEDIBSECTION);
  if (hBmp == NULL)
    return FALSE;
  if (WSAStartup (MAKEWORD (1, 1), &wsadata) != 0)
  {
    vmb_error(__LINE__,"Unable to initialize Winsock dll");
    return FALSE;
  }
  InitCommonControls ();
  if (!InitInstance (hInstance))
    return FALSE;
  init_layout(0);
  hAccelTable =
    LoadAccelerators (hInstance, MAKEINTRESOURCE (IDR_ACCELERATOR));
  win32_param_init ();
  if (terminate_flag)
  {   do_commands ();
      return 0;
  }
  SetWindowPos(hMainWnd,HWND_TOP,xpos,ypos,0,0,SWP_NOSIZE|SWP_NOZORDER|SWP_SHOWWINDOW);
  UpdateWindow(hMainWnd);
  initialize_slots ();
  create_server ();
  do_commands ();
  if (vmb_power_flag) power_all(1);
  if (!terminate_flag)
    while (GetMessage (&msg, NULL, 0, 0))
      if (!TranslateAccelerator (msg.hwnd, hAccelTable, &msg))
      {
        TranslateMessage (&msg);
        DispatchMessage (&msg);
      }
  shutdown_server ();
  WSACleanup ();
  if (hMutex!=NULL) CloseHandle(hMutex);
  return (int)msg.wParam;
}
#else

static void
bus_info (void)
{
  int i, count = 0;
  for (i = 0; i < max_slot; i++)
  {
    if (valid_socket (slot[i].fd))
    {
      if (slot[i].name != NULL)
	printf ("%d: %s\n", i, slot[i].name);
      else
	printf ("%d: Unnamed\n", i);
      count++;
    }
  }
  printf ("%d of %d/%d Slots in use \n", count, max_slot, SLOTS);
  printf ("Read-Ops: %d  Write-Ops: %d \n", read_ops, write_ops);
}

static void
slot_info (int i)
{
  if (i >= 0 && i < SLOTS && valid_socket (slot[i].fd))
  {
    char hex[17] = { 0 };
    if (slot[i].name != NULL)
      printf ("%s\n", slot[i].name);
    else
      printf ("Unnamed\n");
    printf ("Slot %d Socket: %d\n", i, slot[i].fd);
    chartohex (slot[i].from_addr, hex, 8);
    printf ("Slot %d Starting Address: #%s\n", i, hex);
    chartohex (slot[i].to_addr, hex, 8);
    printf ("Slot %d End Address     : #%s\n", i, hex);
    printf ("Slot %d High Interrupt Mask: %X\n", i,
	    (unsigned) slot[i].hi_mask);
    printf ("Slot %d Low Interrupt Mask : %X\n", i,
	    (unsigned) slot[i].low_mask);
  }
  else
    vmb_error(__LINE__,"Invalid slot number");
}

static void
process_stdin ()
{
  char buffer[300] = { 0 };
  read (0, buffer, 300);
  if (strncmp (buffer, "help", 4) == 0)
    vmb_message(howto);
  else if (strncmp (buffer, "debug", 5) == 0)
    vmb_debug_on();
  else if (strncmp (buffer, "nodebug", 7) == 0)
    vmb_debug_off();
  else if (strncmp (buffer, "quiet", 5) == 0)
    { vmb_debug_mask=VMB_DEBUG_DEFAULT;
      vmb_debugi(VMB_DEBUG_NOTIFY,"debug mask is now %x",vmb_debug_mask);
    }
  else if (strncmp (buffer, "verbose", 7) == 0)
    { vmb_debug_mask=0;
      vmb_debugi(VMB_DEBUG_NOTIFY,"debug mask is now %x",vmb_debug_mask);
    }
   else if (strncmp (buffer, "mask", 4) == 0)
    { vmb_debug_mask=atoi (&buffer[4]);
      vmb_debugi(VMB_DEBUG_NOTIFY,"debug mask is now %x",vmb_debug_mask);
    }
  else if (strncmp (buffer, "quit", 4) == 0)
    exitflag = 1;
  else if (strncmp (buffer, "on", 2) == 0 && !powerflag)
    power_all (1);
  else if (strncmp (buffer, "off", 3) == 0 && powerflag)
    power_all (0);
  else if (strncmp (buffer, "reset", 5) == 0)
    reset_all ();
  else if (strncmp (buffer, "terminate", 9) == 0)
    terminate_all ();
  else if (strncmp (buffer, "info", 4) == 0)
    bus_info ();
  else if (strncmp (buffer, "close", 5) == 0)
    close_slot (atoi (&buffer[5]));
  else if (strncmp (buffer, "slot", 4) == 0)
    slot_info (atoi (&buffer[4]));
  else if (strncmp (buffer, "signal", 6) == 0)
    interrupt_all (atoi (&buffer[6]));
  else
    { vmb_debugs(VMB_DEBUG_ERROR, "Unknown command %s", buffer);
      vmb_message("Type \"help\" to get a list of commands");
    }
}


int
main (int argc, char *argv[])
{
  printf ("Type 'quit' to shutdown the motherboard\n");
  param_init (argc, argv);
  if (terminate_flag)
  {   do_commands ();
      return 0;
  }
  initialize_slots ();
  create_server ();
  do_commands ();
  if (vmb_power_flag) power_all(1);
  while (!exitflag)
  {
    build_read_fdset ();
    select (max_fd + 1, &read_fdset, &write_fdset, NULL, NULL);
    if (FD_ISSET (0, &read_fdset))
      process_stdin ();
    if (FD_ISSET (mother_fd, &read_fdset))
      connect_new_device ();
    process_read_fdset ();
  }
  shutdown_server ();
  fprintf (stdout, "Read-Ops: %d  Write-Ops: %d \n", read_ops, write_ops);
  return 0;
}

#endif
