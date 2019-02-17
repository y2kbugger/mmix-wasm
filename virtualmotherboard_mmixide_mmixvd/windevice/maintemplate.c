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


#include "vmb.h"

extern int ramsize;

int main(int argc, char *argv[])
{
  /* the vmb library uses debuging output, that we need to switch on if we
     want to see it. It will use the variable vmb_program_name to mark the output
     as comming from this program.*/
  vmb_debug_on();
  vmb_program_name = "Device Template";

  /* establish a connection to the virtual bus on the localhost
     on port 9002. This port is the default port for the virtual bus. */
  vmb_connect("localhost",9002); 

  vmb_register(0x00000008,0x00000000, /* start address, hi 32bit, lo 32 bit
                                         where the device is mapped. */
               ramsize, /* the size of the mapped area */
               0x00000000,0x00000000, /* the mask for the 64 interupt lines
                                         only interrups with the corresponding
                                         bit set will reach this device. */
               "Simple RAM",major_version,minor_version);         /* the name of this device */

  /* now the virtual motherboard interface is set up.
     There is nothing left to do, because this is a pasive device
     it will just service read and write requests from the bus
     by the above callback functions. 
     We just wait until the motherboard disconnects,
     before returning home. */

  vmb_wait_for_disconnect();
  return 0;
}


/* the main thread could of course use some
   functions to play an active role on the virtual motherboard.
   these are listed here:

Events:
------
   The next function raises an interrupt (0<= interrupt <=64)
      
     vmb_raise_interrupt(unsigned char interrupt)

   This function raises a hard reset:
      
      vmb_raise_reset();

   The device can also wait for 
     vmb_power changing to 0,
     vmb_reset_flag changing to 1,
     vmb_connected changing to 0, or
     an interrupt to occur using

      vmb_wait_for_event();

  To wait for power to change from 0 to 1 use_

      vmb_wait_for_power();
    

   The status of power can also be seen from the variable
   vmb_power. If power is on, we have vmb_power == 1, otherwise
   we have vmb_power == 0.

   For the reset, there is a reset flag. If a reset is received,
   the flag is raised, i.e. the variabe vmb_reset_flag becomes 1.
   It is the duty of the device to reset it to zero again.

   The status of the connection can be seen from the variable
   vmb_connected. If connected, it has the value 1, otherwise 0.

   Instead of waiting for a disconnect, the device can also
   actively disconnect from the motherboard by calling

   vmb_disconnect();

   After that the device will need to call vmb_connect() and
   vmb_register() again before the virtual bus becomes available again. 

Reading and Writing Data:
-------------------------

   This is a more complex issue.

   To read and write data, a structure called 
   data_addess is used. It looks like this:

     typedef struct {
       unsigned char *data;
       int address_hi; 
       int address_lo;
       int size;
       int status;  
     } data_address;

   Create such a structure like this:

     data_address da;

   Then initialize it to contain size byte (0 < size <= 2048)
   by calling:

     vmb_init_data_address(&da, size);

   This call will initialize the data pointer and the size field.
   There is no magic in the initialization. Just setting the fields
   to reasonable values and allocating size byte for data will do
   fine as well.

   The status will receive the value STATUS_INVALID.
   There are other values for the status: STATUS_VALID,
   STATUS_WRITING, and STATUS_READING.
   The main thread can use the da structure only if the
   status is either INVALID or VALID. In any other state
   the structure is used by the reading and writing threads.
   IF the status is VALID, then the data pointer points to valid
   data either fetched from the virtual bus or from the cache.

   To get such valid data. Set the address information like this:
     da.address_hi = 0x00000000;
     da.address_lo = 0x00000100;
  
   and then call

      vmb_load(&da);

   you must make sure that the read access does not cross the
   boundary of the address space beween different devices.
   The motherboard at present does not split requests for reading
   or writing for multiple devices. It will just accept a read
   or write request for a single target.

   Of course, it may take a while until the structure is again
   valid after such a call. To wait for the load to complete
   call

     vmb_wait_for_valid(&da);

   You can now alter the data in the structure.
   To write the data back call

     vmb_store(&da);

   There is no need to wait until the data becomes valid here.
   After the call has returned the data is valid.


Caches:
-------

   Reading and writing data over the bus in small packets can
   be very slow, like it is with real hardware. Like with real
   hardware, the virtual motherboard package offers caches to
   mitigate this problem, a data cache and an instruction cache.

      extern cache  vmb_d_cache;
      extern cache  vmb_i_cache;

   These caches can be configured at compiletime. For that see the
   file cache.h in this directory.

   These caches are automatically available but there is no need to
   use them. They speed up, however, reading and writing and
   they are convenient to use if implementing something like a CPU.

   You can read from a cache calling the function

   const unsigned char *
     vmb_cache_read(cache *c, unsigned int hi_address,
                              unsigned int lo_address)
   for example
    
   const unsigned char *p = vmb_cache_read(&vmb_d_cache,0,0x100);

   will return a pointer p into an array of byte (actually the
   cache line itself) where to find the cached content of memmory
   at address 0x0000 0000 0000 0100. The hi 32 bit and the lo 32 bit
   are passed in different integers. You can read additional
   bytes if you know that they are in the same cache line.
   You must not modify these bytes. It will otherwise confuse the cache.

   For convenience there is a second function.

   unsigned int 
     vmb_cache_read_int(cache *c, unsigned int hi_address,
                                  unsigned int lo_address)
   
   this function returns a four byte integer from the cache.
   Of course the cache takes care of reading the cacheline for you.

   Writing to the cache is similar with this function.
                                  
   unsigned char *
     vmb_cache_write(cache *c, unsigned int hi_address,
                               unsigned int lo_address)

   It will again return a pointer into the cache line. But this
   time, you are allowed to write directly to the cache.
   (Even if you dont write, the cache line is now considered "dirty"
    and will be written back to memory eventually.)

   You should not write to the instruction cache!

   Clever CPUs can produce code that will start loading data
   into the cache before it is actually needed. You can do this with
   the function

    vmb_cache_preload(cache *c, unsigned int hi_address,
                               unsigned int lo_address)

   This function will initiate the loading, but will not
   wait until the data is avaliable. A later read or write
   access to the same cache line may however be faster.

Cache Maintenance:
------------------

There are a few functions available for cache maintenance.

    extern void vmb_cache_clear(cache *c);
    makes the cache to be completely empty. This happens
    automatically if the device receives a reset.

    void vmb_cache_clear_line(cache *c, int address_hi, int address_lo);
    the less drastic version, it clears only the line containing
    the given address.

    extern void vmb_cache_flush(cache *c);
    will write all the dirty lines to memory

    extern void vmb_cache_flush_line(cache *c,int address_hi,int address_lo);
    this is less drastic. It will flush only the one cache line
    containing the given address if this line is dirty.


    extern void vmb_cache_zero_line(cache *c,int address_hi,int address_lo)
    This will fill the cache line containing the given address
    completely with zeros and marks the line as dirty.
    This function is faster than writing a complete line
    with zeros using  vmb_cache_write. Because the latter would
    first read a valid line from memory only to see it all getting
    replaced by zeros.


*/
