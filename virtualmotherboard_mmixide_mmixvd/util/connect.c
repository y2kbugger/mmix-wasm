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
#endif
#include <fcntl.h>
#include <errno.h>
#include "vmb.h"
#include "error.h"
#include "message.h"
#include "bus-arith.h"


/* functions to connect, register, unregister, and disconnect */

int bus_connect(char *hostname,int port)
{ int i;
  int fd = 0;
  struct sockaddr_in host_addr;
  static char localhost[] = "localhost";

  if (hostname == NULL)
    hostname = localhost;
  else if (hostname[0]==0)
    hostname = localhost;

#ifdef WIN32
  {	WSADATA wsadata;
	if(WSAStartup(MAKEWORD(1,1), &wsadata) != 0)
    {  vmb_error(__LINE__,"Unable to initialize Winsock dll");
	   return INVALID_SOCKET;
	}
  }
#endif
  fd = (int)socket( PF_INET, SOCK_STREAM, 0);
  vmb_debug(VMB_DEBUG_PROGRESS,"Creating socket");
  if (!valid_socket(fd))
  {
#ifdef WIN32
	  WSACleanup();
#endif
	  vmb_error(__LINE__,"Unable to create a socket");    
	  return INVALID_SOCKET;
  }

#if 0
   /* create an event corresponding to the socket */
	 recv_event = CreateEvent(NULL,False,False,NULL);
	 if (recv_event == NULL)
       return INVALID_SOCKET;
	 WSAEventSelect (fd, e,FD_READ );
#endif

{ 
  unsigned long server_ip;

  server_ip = inet_addr(hostname);
  if (server_ip == INADDR_NONE)
  { struct hostent *hp;
    hp = gethostbyname(hostname);
    if (hp==NULL)
    { server_ip = 0;
#ifdef WIN32
	  WSACleanup();
#endif
	  vmb_error(__LINE__,"Unable to get host by name");    
      return INVALID_SOCKET;
    }
    memcpy(&server_ip,hp->h_addr,sizeof(server_ip));
  }
  host_addr.sin_addr.s_addr = server_ip;
}
  vmb_debug(VMB_DEBUG_PROGRESS,"Got server IP");
  host_addr.sin_family = AF_INET;
  host_addr.sin_port = htons((unsigned short)port);

  i = connect(fd,(struct sockaddr *)&host_addr,sizeof(host_addr));
  vmb_debugi(VMB_DEBUG_PROGRESS,"Connecting to server (%d)",i);
  if (i < 0 )
  { 
#ifdef WIN32
    int error;
    error = WSAGetLastError();
	if (error == WSAECONNREFUSED) /* try a second time */
	{ i = connect(fd,(struct sockaddr *)&host_addr,sizeof(host_addr));
      if (i < 0 )
	  {	error = WSAGetLastError();
	    vmb_error(error,"Unable to connect to socket");    
		WSACleanup();
        return INVALID_SOCKET;
      }
	}
#else
          return INVALID_SOCKET;
#endif
  }
    /* wait until it is writable, then the connection has succeeded */
  { 
    fd_set write_set;
    FD_ZERO(&write_set);
    FD_SET((unsigned)fd, &write_set);
	i = select (fd+1, NULL, &write_set, NULL, NULL);
	if (i!=1)
    {
#ifdef WIN32
		  WSACleanup();
#endif
	  vmb_error(__LINE__,"Unable to get a writeable socket");    
      return INVALID_SOCKET;
    }
	vmb_debug(VMB_DEBUG_PROGRESS,"Socket is writable");
  }

  {
    /* Tell TCP not to delay small packets.  This greatly speeds up
       interactive response. */
    int tmp = 1;
    i = setsockopt (fd, IPPROTO_TCP, TCP_NODELAY,
		(char *) &tmp, sizeof (tmp));
	if (i==0)
	  vmb_debug(VMB_DEBUG_PROGRESS,"Socket is set to TCP_NODELAY");
	else
	{
#ifdef WIN32
          i = WSAGetLastError();
#endif
	  vmb_debugi(VMB_DEBUG_ERROR,"Unable to set Socket to TCP_NODELAY, error %d",i);
	}
  }
  return fd; 
}

int bus_register(int *socket,
                unsigned char address[8],
                unsigned char limit[8],
                unsigned int hi_mask, unsigned int low_mask,
				char *name,
				int major_version, int minor_version)
{ unsigned char size;
  unsigned char msg[MAXMESSAGE] = {0};
  if (socket<0)
    return -4;
  if (address!=NULL && limit!=NULL)
  { memmove(msg, address, 8);
    memmove(msg+8, limit, 8);
  }
  else
    memset(msg,0,16);
  inttochar(hi_mask, &msg[16]);
  inttochar(low_mask, &msg[20]);

  size = 2;

  if (name!=NULL)

  { int n;

    n = (int)strlen(name)+1;
    if ((3+(n+7)/8)> 255)
       n = (255-4)*8;

    strncpy((char *)(msg+24),name,n);
    size += (n+7)/8;
  }
  if (size<255 && major_version>0 && minor_version>=0)
  {   inttochar(major_version, &msg[8*(size+1)]);
      inttochar(minor_version, &msg[8*(size+1)+4]);
	  size++;
  }
  /* send bus register message */
  return send_msg(socket, TYPE_BUS|TYPE_PAYLOAD, size, 0, ID_REGISTER, 0, 0, msg);
}


int bus_unregister(int socket)
{ return 0;
}

int bus_disconnect(int *fd)
{ 	int socket=*fd;
	*fd=INVALID_SOCKET;
#ifdef WIN32
    if (valid_socket(socket))
	{ int error;
	  error = shutdown(socket,SD_BOTH);
	  if (error!=0)
	    vmb_errori(__LINE__,"Shutdown socket",WSAGetLastError());
	  error = closesocket(socket);
	  if (error!=0)
	    vmb_errori(__LINE__,"Close socket",WSAGetLastError());
	  error = WSACleanup();
	  if (error!=0)
	    vmb_errori(__LINE__,"WSACleanup",WSAGetLastError());
	}
#else
    if (valid_socket(socket))
	{ shutdown(socket,SHUT_RDWR);
      close(socket);
	}
#endif
  
  return 0;
}

