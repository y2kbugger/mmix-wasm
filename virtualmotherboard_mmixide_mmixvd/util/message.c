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

#include <stdio.h>
#include <string.h>
#ifdef WIN32
#include <winsock2.h>
#include <windows.h>
#pragma warning(disable : 4996)
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#define Sleep(x) usleep((x)*1000)
#endif
#include <fcntl.h>
#include <errno.h>
#include "vmb.h"
#include "error.h"
#include "message.h"
#include "bus-arith.h"



/* functions to send and receive a message */
static int write_socket(int *socket, unsigned char *msg, int size)
/* write buffer to the socket either completely or, if blocking is false, not at all */
{ int snd = 0;
  while(snd < size)
  { int i;
    i = send(*socket, &msg[snd], size-snd,0);
    if (i<0) /* error */
    {   
#ifdef WIN32 
		int e = WSAGetLastError();
		if (e == WSAEWOULDBLOCK)
		{  /* if the socket was used in nonblocking mode (motherboadr) 
		      it might return 0 */
			continue; /* Bussy wait */
		}
		i= -e;
#endif		
		bus_write_error(socket);
	  return i;
	}
    else if (i==0) /* connection has closed */
    { bus_write_error(socket);
      return -3;
    }
    else
      snd += i;
  }
  return 1;
}

int send_msg(int *socket,
         unsigned char type,
         unsigned char size,
         unsigned char slot,
         unsigned char id,
         unsigned int  time,
         unsigned char address[8],
         unsigned char *payload)
{ unsigned char buf[MAXMESSAGE] = {0}, *msg=buf;
  int msg_size;
        
  if (!valid_socket(*socket))
    return -1;

  if (address == NULL)
	type = type & ~TYPE_ADDRESS;
  if (payload == NULL)
	type = type & ~TYPE_PAYLOAD;

  *msg++ = type;
  *msg++ = size;
  *msg++ = slot;
  *msg++ = id;
  msg_size = message_size(buf);

  if(type & TYPE_TIME)
  {
	inttochar(time, msg);
	msg += 4;
  }

  if(type & TYPE_ADDRESS)
  {
	memmove(msg, address, 8);
	msg += 8;
  }

  if(type & TYPE_PAYLOAD)
  {
	memmove(msg, payload, (size+1)*8);
  }
  return write_socket(socket,buf,msg_size);
}
#ifdef WIN32
HANDLE  recv_event;
#endif

static int read_socket(int *socket, unsigned char *msg, int size)
/* read a complete message from the socket */
{ int rcv = 0;
  while(rcv < size)
  { int i;
    i = recv(*socket, &msg[rcv], size-rcv,0);
    if (i<0) /* error */
    { 
#ifdef WIN32 
		i = WSAGetLastError();
		if (i == WSAEWOULDBLOCK)
		{  /* if the socket was used in nonblocking mode (motherboard) 
		      it might return 0 */
			return 0;
		}
#endif		
        vmb_debug(VMB_DEBUG_NOTIFY,"Socket Read Error\n");
		bus_read_error(socket);
	  return -1;
	}
    else if (i==0) /* connection has closed */
    { 
    vmb_debug(VMB_DEBUG_PROGRESS,"Socket was closed\n");

/* there was a #ifdef WIN32 #else here to skip the disconnect with WIN32 */
      bus_read_error(socket); 
 	  return -1;
    } 
    else
      rcv += i;
  }
  return rcv;
}



int receive_msg(int *socket,
         unsigned char *type,
         unsigned char *size,
         unsigned char *slot,
         unsigned char *id,
         unsigned int  *time,
         unsigned char address[8],
         unsigned char *payload)
{ /* recieve header and compute messgage size */
  unsigned char buf[MAXMESSAGE] = {0}, *msg=buf;
  int msg_size;
  int rcv;

  if (!valid_socket(*socket))
    return -1;
  rcv = read_socket(socket,msg,4);
  if (rcv <= 0)
    return rcv;

  { int len;
    len = rcv;
	  /*recieve header of message */
    while (4 > len)
    { rcv = read_socket(socket,msg+len,4-len);
      if (rcv <0 )
        return rcv;
	  len = len+rcv;
    }
    msg_size = message_size(msg);

    /*recieve rest of message */
    while (msg_size > len)
    { int sleepcount=0;
	  rcv = read_socket(socket,msg+len,msg_size-len);
	  if (rcv==0)
	  { if (sleepcount++ > 5) return -1;
	    Sleep(50); /* sleep 50 ms */
	  }
	  else if (rcv <0 )
        return rcv;
	  len = len+rcv;
    }
  }
  /* transfer data to pointers */
  *type = *msg++;
  *size = *msg++;
  *slot = *msg++;
  *id   = *msg++;

  if(*type & TYPE_TIME)
  { *time = chartoint(msg); msg=msg+4; }
  if(*type & TYPE_ADDRESS)
  { memmove(address, msg, 8); msg=msg+8;}
  if(*type & TYPE_PAYLOAD)
    memmove(payload,msg, (*size+1) * 8);
  return 1;
}


int message_size(unsigned char msg[4])
{ int size = 4;
  if(msg[0] & TYPE_TIME)
	size += 4;
  if(msg[0] & TYPE_ADDRESS)
	size += 8;
  if(msg[0] & TYPE_PAYLOAD)
	size += (msg[1]+1)*8;
  return size;
}


