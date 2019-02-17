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

#include <stdlib.h>
#include <string.h>
#ifdef WIN32
#include <windows.h>
#include "resource.h"
#pragma warning(disable : 4996)
extern HWND hMainWnd;
#include <io.h>
#else
#include <unistd.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>

#include "bus-arith.h"
#include "option.h"
#include "param.h"
#include "vmb.h"
#ifdef WIN32
#include "winopt.h"
#include "opt.h"
#include "inspect.h"
#endif

int major_version=2, minor_version=1;
char title[] ="VMB ROM";

char version[]="$Revision: 664 $ $Date: 2018-12-10 18:10:14 +0100 (Mon, 10 Dec 2018) $";

char howto[] =
"\n"
"The program will contact the motherboard at host:port\n"
"and register itself with the given satrt address.\n"
"Then, the program will read the file and answer read requests from the bus.\n"
"\n"
;

extern device_info vmb;
static unsigned char *rom=NULL;
static unsigned int registered_size=0;
static uint64_t registered_address;
typedef unsigned long Word;

#define PAGESIZE (1<<13) /* eight kbyte */
#define BIOSFILEID	0x0253504D
#define WORDLEN 4

/*!
 * \fn void processUMPSFile(Word* biosBuf,long lSize)
 * \author Martin Hauser <info@martin-hauser.net>
 * \param biosBuf Buff filled with command words
 * \param lSize the size of the buffer
 * \brief read words into rom specificed by biosBuf
 * \warning Make sure that biosBuf is a malloc'ed structure, nothing local!
 *
 * This function will essentially walk through all the words of biosBuf and
 * attempt to convert them one by one into the char structure of ::rom. Then
 * it'll calculate the size to be passed on to the rom file and finally free
 * the buffer turned over.
 */

void processUMPSFile(Word* biosBuf,long lSize)
{
	int i,k;
	int j = 0;
	
	rom = malloc(lSize * WORDLEN); /*!< allocate real rom buffer (is of char) */
	
	for(i = 0; i < lSize; i++) /*!< walk through biosBuf */
	{  
	    for(k=0;k<4;k++)    
		    rom[j + k] = (unsigned char)((biosBuf[i] >> (32 - (k+1)*8))&0xFF); /*!< do an int to char conversion */
		
		j += WORDLEN;
	}
	free(biosBuf);
	vmb_size = lSize * WORDLEN; /*!< calc real size of rom */
}

/*
 * \fn void readUMPSFile(File* bFile)
 * \author Martin Hauser <info@martin-hauser.net>
 * \param bFile The file to read from
 * \brief does some safty checks on the file and turns the results over to ::processUMPSFile
 *
 * This function will take any File descriptor as argument, read the BIOS tag from it and if that's 
 * okay it'll turn over to read the size. If both are okay the rest of the file will be read into
 * a freshly allocated buffer, which will then be turned over to ::processUMPSFile where it will
 * be freed.
 */

void readUMPSFile(FILE* bFile)
{
    Word tag;
	Word * biosBuf;
    
    unsigned long lSize;
	unsigned long* sizep = &lSize;
	
	/*! check whether tag and size can be read from the file */
    if (fread((void *) &tag, WORDLEN, 1, bFile) != 1 || tag != BIOSFILEID || \
         fread((void *) sizep, WORDLEN, 1, bFile) != 1)
	{
	    vmb_fatal_error(__LINE__,"couldn't read umps file.\n");
	}
	
	/*! alloc buffer and read data into it */
	biosBuf = malloc(lSize * WORDLEN); 
	if (fread((void *) biosBuf, WORDLEN, *sizep, bFile) != *sizep)
	{
		// wrong file format
		free(biosBuf);
        vmb_fatal_error(__LINE__,"wrong UMPS file format.\n");
	}
	/*! turn data over */
	processUMPSFile(biosBuf,lSize);
	
}

void open_file(void)
{ 
    FILE *f;
    char *c;
    struct stat fs;
    
    f = NULL;
	vmb_size=0;

    if (vmb_filename==NULL || strcmp(vmb_filename,"") == 0)
	 vmb_error(__LINE__,"No filename for image file given");
	else
	{ vmb_debugs(VMB_DEBUG_PROGRESS, "Reading image file: %s",vmb_filename);
      if ((f = vmb_fopen(vmb_filename, "rb")) == NULL)
        vmb_error2(__LINE__,"Unable to open image file",vmb_filename);
	  else
	  { c = strrchr(vmb_filename,'.');
		if(strcmp(c,".umps") == 0)
		{
			readUMPSFile(f);
		}
		else
		{ int rc;
       
			if (fstat(fileno(f),&fs)<0)
				vmb_fatal_error(__LINE__,"Unable to get file size");
    
			vmb_size = (fs.st_size+PAGESIZE-1)&~(PAGESIZE-1); /*round up to whole pages*/
			if (rom!=NULL) free(rom);
			rom = malloc(vmb_size);
			if (rom==NULL) vmb_fatal_error(__LINE__,"Out of memory");
			rc=(int)fread(rom,1,vmb_size,f);
			if (rc < 0) vmb_fatal_error(__LINE__,"Unable to read file");
			if (rc == 0) vmb_fatal_error(__LINE__,"Empty file");
		}
		fclose(f);
	  }
	}
	if (vmb_size==0)
	{ vmb_size = PAGESIZE;
	  rom = malloc(vmb_size);
      if (rom==NULL) vmb_fatal_error(__LINE__,"Out of memory");
	}
#ifdef WIN32
	inspector[0].address=vmb_address;
	inspector[0].size=vmb_size;
	mem_update(0,vmb_size);
#endif
	vmb_debug(VMB_DEBUG_PROGRESS, "Done reading image file");
	if ((vmb_size!=registered_size || vmb_address!=registered_address)&& vmb.connected && vmb.power)
	{ vmb_register(&vmb,HI32(vmb_address),LO32(vmb_address),vmb_size,0,0,defined,major_version,minor_version);
          registered_size=vmb_size;
	  registered_address=vmb_address;
	}
}



/* Interface to the virtual motherboard */


unsigned char *rom_get_payload(unsigned int offset,int size)
{
    return rom+offset;
}


void rom_poweron(void)
{  open_file();
#ifdef WIN32
   mem_update(0,vmb_size);
   PostMessage(hMainWnd,WM_VMB_ON,0,0);
#endif
}

#ifdef WIN32
static int rom_read(unsigned int offset,int size,unsigned char *buf)
{ if (offset>vmb_size) return 0;
  if (offset+size>vmb_size) size =vmb_size-offset;
  memmove(buf,rom+offset,size);
  return size;
}


struct inspector_def inspector[2] = {
    /* name size get_mem address num_regs regs */
	{"Memory",0,rom_read,rom_get_payload, NULL,0,0,8,0,NULL},
	{0}
};
#endif

void init_device(device_info *vmb)
{  vmb_debugi(VMB_DEBUG_INFO, "address hi: %x",HI32(vmb_address));
   vmb_debugi(VMB_DEBUG_INFO, "address lo: %x",LO32(vmb_address));
   vmb->poweron=rom_poweron;
   vmb->poweroff=vmb_poweroff;
   vmb->disconnected=vmb_disconnected;
   vmb->reset=open_file;
   vmb->terminate=vmb_terminate;
   vmb->get_payload=rom_get_payload;
   close(0);
}
