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

#ifndef MESSAGE_H
#define MESSAGE_H

/* any message consists of a header with 4 byte: type, size, slot; and id.
   after the header are optionaly 
      a four byte time value (if the TYPE_TIME bit is set)
      an eith byte address (if the TYPE_ADDRESS bit is set) and
      up to 256 x 8 byte payload (if the TYPE_PAYLOAD bit is set)
*/


/* some usefull definitions for low level use of the bus */


/* the type bits */

#define TYPE_NULL      0x00
#define TYPE_BUS       0x80
#define TYPE_TIME      0x40
#define TYPE_ADDRESS   0x20
#define TYPE_ROUTE     0x10
#define TYPE_PAYLOAD   0x08
#define TYPE_REQUEST   0x04
#define TYPE_LOCK      0x02
#define TYPE_UNUSED    0x01

/* two versions to send and receive messages 
   either the message is processed completely or not at all.
   if blocking == 0 it might return not having sent/received anything
   return value is -1 on error
                is  0 if not sent/received
                is  1 if sent/received
   payload is used only if the payload bit is set
   time is used only if the time bit is set
   address is used only if the address bit is set
*/

#define MAXPAYLOAD (256*8)
#define MAXMESSAGE (4 + 4 + 8+ MAXPAYLOAD)
/* header time address payload */

extern int send_msg(int *socket,
         unsigned char type,
         unsigned char size,
         unsigned char slot,
         unsigned char id,
         unsigned int  time,
         unsigned char address[8],
         unsigned char *payload);

extern int receive_msg(int *socket,
         unsigned char *type,
         unsigned char *size,
         unsigned char *slot,
         unsigned char *id,
         unsigned int  *time,
         unsigned char address[8],
         unsigned char *payload);

extern int message_size(unsigned char msg[4]);

/* predefined Ids */
#define ID_IGNORE     0
#define ID_READ       1
#define ID_WRITE      2
#define ID_READREPLY  3
#define ID_NOREPLY    4
#define ID_READBYTE   5
#define ID_READWYDE   6
#define ID_READTETRA  7
#define ID_WRITEBYTE  8
#define ID_WRITEWYDE  9
#define ID_WRITETETRA 10
#define ID_BYTEREPLY  11
#define ID_WYDEREPLY  12
#define ID_TETRAREPLY 13
#define ID_NOWRITE    14

/* predefined IDs for BUS messages */
/* a polite request to terminate the device */
#define ID_TERMINATE  0xF9

#define ID_REGISTER   0xFA
#define ID_UNREGISTER 0xFB
#define ID_INTERRUPT  0xFC
/* the next are hard wired power on/off and reset signals not to be confused
   with software interrupts for reset or power fail delivered via a ID_INTERUPT */
#define ID_RESET      0xFD
#define ID_POWEROFF   0xFE
#define ID_POWERON    0xFF


/* interrupt bits for the rQ and rK Registers */
/* The low 8 bit of the high tetra are for program interrups */
#define INT_READ   7 /*instruction tries to load from a page without read permission;*/
#define INT_WRITE  6 /*instruction tries to store to a page without write permission;*/
#define INT_EXEC   5 /*instruction appears in a page without execute permission;*/
#define INT_NEG    4 /*instruction refers to a negative virtual address;*/
#define INT_KERNEL 3 /*instruction is privileged, for use by the ``kernel'' only;*/
#define INT_BREAK  2 /*instruction breaks the rules of MMIX;*/
#define INT_SECURE 1 /*instruction violates security;*/
#define INT_PRIV   0 /*instruction comes from a privileged (negative) virtual address.*/



/* The low 8 bit of the low tetra are for machine interrups */
#define INT_POWER  0    /* power fail */
#define INT_PARITY 1    /* memmory parity error */
#define INT_NOMEM  2    /* nonexistent memory */
#define INT_REBOOT 3    /* reboot */
#define INT_PAGEFAULT 4 /* instruction causes a page fault */
#define INT_PAGEERROR 5 /* a page table lookup finds an error in the page table */
/* the rest is yet to be defined */

#define BIT(interrupt) (1<<(interrupt))

/* functions to connect, register, unregister, and disconnect */


extern  int bus_connect(char *hostname,int port);
/* returns the socket or -1 on error */
extern  int bus_register(int *socket,
                unsigned char address[8],unsigned char limit[8],
			unsigned int hi_mask, unsigned int low_mask,
			char *name, int major_version, int minor_version);
extern  int bus_unregister(int socket);
extern int bus_disconnect(int *socket);

extern void bus_read_error(int *socket);
extern void bus_write_error(int *socket);

#ifndef INVALID_SOCKET
#define INVALID_SOCKET  (~0)
#endif

#define valid_socket(socket)  ((socket) != INVALID_SOCKET)

#endif
