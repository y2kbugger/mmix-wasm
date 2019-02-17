/*
 * disk.c -- disk simulation
   The following code is thanks to the gimmix project.
   It was ported to fitt the mmix motherboard by Martin Ruckert. 2005

 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef WIN32
#pragma warning(disable : 4996)
#include <windows.h>
#include "winopt.h"
#include "resource.h"
extern HWND hMainWnd;
extern HBITMAP hbussy;
#include "inspect.h"
#else
#include <pthread.h>
#include <unistd.h>
/* make mem_update a no-op */
#define mem_update(offset, size)
#endif
#include "vmb.h"
#include "error.h"
#include "bus-arith.h"
#include "param.h"

int major_version=2, minor_version=1;
extern device_info vmb;

char title[] ="VMB Disk";

char version[]="$Revision: 664 $ $Date: 2018-12-10 18:10:14 +0100 (Mon, 10 Dec 2018) $";

char howto[] =
"The disk simulates a disk controller and the disk proper by using a\n"
"file of an appropriate size and organization on the host system. \n"
"\n"
"The disk understands two commands: read a block of data from the disk\n"
"or write a block of data to the disk. The amount of data transferred\n"
"is specified as an integral number of sectors. The sector size is\n"
"fixed at 512 bytes. The number of sectors to read or write is given\n"
"to the controller in the count register. The position of the starting\n"
"sector on the disk is given to the controller in the sector register.\n"
"The disk sectors are numbered starting at 0 through (and including)\n"
"the number of sectors on the disk minus one. The number of sectors on\n"
"the disk can be read by software from the capacity register.\n"
"\n"
"The memory address from where a block of data is read in a write operation\n"
"or to where a block of data is written in a read operation is determined\n"
"by the contents of the 16 DMA registers supporting scatter-gather IO.\n"
"Each DMA register consists of an address and a size register.\n"
"A read operation will scatter the data stream from the Count sectors requested\n"
"over the memory. It will put DMA0.size bytes at DMA0.address, then continue\n"
"with DMA1.size bytes at DMA1.address, and so on, until either all bytes read from\n"
"the disk are in memmory or the last DMA buffer is filled\n"
"A write operation will gather data from the buffers specified by the DMA\n"
"registers, concatenating it and write it to the specified disk sectors.\n"
"\n"
"a) Status register (4 byte, read only): 2 bits of this register are used to\n"
"get current status information from it. \n"
"These bits are:\n"
"ERROR (the high order bit) set by the hardware if any error prevented the successful\n"
"    completion of the last command.\n"
"BUSY (the low order bit) is set by the hardware during command execution. When\n"
"    the command is completed (or cannot be completed due to an error), BUSY\n"
"    is reset. This condition also raises the disk interrupt if it is enabled.\n"
"The bits are chosen to make testing the Status register simple.\n"
"Negative values indicate errors, zero indicates idle, one indicates bussy\n"
"\n"
"b) Control register (4 byte): 3 bits of this register are used to control the disk\n"
"These bits are from low order to high order:\n"
"START: As soon as a 1 is written to this bit,\n"
"    the next command is carried out. \n"
"IENABLE: enables (1) or disables (0) interrupts from the disk.\n"
"WRITE: is  set to 1 to write to the disk,\n"
"       or set to 0 to read from the disk.\n"
"\n"
"c) Capacity register: this read-only register holds the total number of\n"
"sectors on the disk.\n"
"\n"
"d) Sector register (8 byte): this register holds the disk sector\n"
"number of the first sector to be transferred in the next command.\n"
"\n"
"e) Count register (8 byte): this register holds the number of disk\n"
"sectors to be transferred in the next command.\n"
"\n"
"f) 16 DMA registers (16*2*8 byte): these register holds the addresses and sizes\n"
"of buffers in physical memory where the next transfer takes place.\n"
"\n";

#ifdef WIN32
struct register_def disk_regs[] = {
	/* name no offset size chunk format */
	{"Status"   ,0x00,4,tetra_chunk,hex_format},
	{"Control"  ,0x04,4,tetra_chunk,hex_format},
    {"Capacity" ,0x08,8,octa_chunk,unsigned_format},
    {"Sector"   ,0x10,8,octa_chunk,unsigned_format},
	{"Count"    ,0x18,8,octa_chunk,unsigned_format},
    {"DMA0 Addr",0x20,8,octa_chunk,hex_format},
    {"DMA0 Size",0x28,8,octa_chunk,unsigned_format},
    {"DMA1 Addr",0x30,8,octa_chunk,hex_format},
    {"DMA1 Size",0x38,8,octa_chunk,unsigned_format},
    {"DMA2 Addr",0x40,8,octa_chunk,hex_format},
    {"DMA2 Size",0x48,8,octa_chunk,unsigned_format},
    {"DMA3 Addr",0x50,8,octa_chunk,hex_format},
    {"DMA3 Size",0x58,8,octa_chunk,unsigned_format},
    {"DMA4 Addr",0x60,8,octa_chunk,hex_format},
    {"DMA4 Size",0x68,8,octa_chunk,unsigned_format},
    {"DMA5 Addr",0x70,8,octa_chunk,hex_format},
    {"DMA5 Size",0x78,8,octa_chunk,unsigned_format},
    {"DMA6 Addr",0x80,8,octa_chunk,hex_format},
    {"DMA6 Size",0x88,8,octa_chunk,unsigned_format},
    {"DMA7 Addr",0x90,8,octa_chunk,hex_format},
    {"DMA7 Size",0x98,8,octa_chunk,unsigned_format},
    {"DMA8 Addr",0xa0,8,octa_chunk,hex_format},
    {"DMA8 Size",0xa8,8,octa_chunk,unsigned_format},
    {"DMA9 Addr",0xb0,8,octa_chunk,hex_format},
    {"DMA9 Size",0xb8,8,octa_chunk,unsigned_format},
    {"DMAa Addr",0xc0,8,octa_chunk,hex_format},
    {"DMAa Size",0xc8,8,octa_chunk,unsigned_format},
    {"DMAb Addr",0xd0,8,octa_chunk,hex_format},
    {"DMAb Size",0xd8,8,octa_chunk,unsigned_format},
    {"DMAc Addr",0xe0,8,octa_chunk,hex_format},
    {"DMAc Size",0xe8,8,octa_chunk,unsigned_format},
    {"DMAd Addr",0xf0,8,octa_chunk,hex_format},
    {"DMAd Size",0xf8,8,octa_chunk,unsigned_format},
    {"DMAe Addr",0x100,8,octa_chunk,hex_format},
    {"DMAe Size",0x108,8,octa_chunk,unsigned_format},
    {"DMAf Addr",0x110,8,octa_chunk,hex_format},
    {"DMAf Size",0x118,8,octa_chunk,unsigned_format},
	{0}};

#endif
#define NUM_REGS 36
#define DISK_MEM 0x120
static unsigned char mem[DISK_MEM] = {0};

#define SECTOR_SIZE	512	/* sector size in bytes */

#define DISK_STAT	     GET4(mem)       /* status register */
#define DISK_CTRL_OFFSET 0x04			
#define DISK_CTRL	     GET4(mem+0x04)	    /* control register */
#define DISK_CAP_OFFSET  0x08
#define DISK_CAP	     GET8(mem+0x08)	/* disk capacity register */
#define DISK_SCT	     GET8(mem+0x10)	/* disk sector register */
#define DISK_CNT	     GET8(mem+0x18)	/* sector count register */
#define DISK_DMA_OFFSET  0x20
#define DISK_DMA_ADDR(i) GET8(mem+0x20+i*0x10)	/* DMA address register */
#define DISK_DMA_SIZE(i) GET8(mem+0x28+i*0x10)	/* DMA size register */

#define SET_DISK_STAT(x)	   SET4(mem,x)       /* status register */
#define SET_DISK_CTRL(x)	   SET4(mem+4,x)	    /* control register */
#define SET_DISK_CAP(x)	       SET8(mem+0x08,x)	/* disk capacity register */
#define SET_DISK_SCT(x)	       SET8(mem+0x10,x)	/* disk sector register */
#define SET_DISK_CNT(x)	       SET8(mem+0x18,x)	/* sector count register */
#define SET_DISK_DMA_ADDR(i,x) SET8(mem+0x20+i*0x10,x)	/* DMA address register */
#define SET_DISK_DMA_SIZE(i,x) SET8(mem+0x28+i*0x10,x)	/* DMA size register */

/* command Bits */
#define DISK_GO  	0x01	/* a 1 written here starts the disk command */
#define DISK_IEN	0x02	/* enable disk interrupt */
#define DISK_WRT	0x04	/* command type: 0 = read, 1 = write */
/* Status Bits */
#define DISK_ERR	0x80000000	/* 0 = ok, 1 = error; valid when BUSY = 0 */
#define DISK_BUSY	0x00000001	/* 1 = disk is working on a command */

/* Data and auxiliar Functions */
static FILE *diskImage;

/* Copies of mem values */
static unsigned int diskStatus;
static unsigned int diskCtrl;
static uint64_t diskCap;
static uint64_t diskSct;
static uint64_t diskCnt;
static struct {
	uint64_t address;
	uint64_t size;
	}	diskDma[16];



int disk_reg_read(unsigned int offset, int size, unsigned char *buf)
{ if (offset>DISK_MEM) return 0;
  if (offset+size>DISK_MEM) size =DISK_MEM-offset;
  memmove(buf,mem+offset,size);
  return size;
}


/* protecting the variable diskCtrl, to syncronize
   reading and writing the controllregisters and reading and writing 
   the disk with DMA */

#ifdef WIN32
HANDLE haction;
CRITICAL_SECTION   action_section;
#else
static pthread_mutex_t action_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t action_cond = PTHREAD_COND_INITIALIZER;

static void clean_up_action_mutex(void *_dummy)
{ pthread_mutex_unlock(&action_mutex); /* needed if canceled waiting */
}
#endif

void set_diskCtrl(unsigned int value)
{ int start;
  vmb_debugi(VMB_DEBUG_INFO, "Setting diskCtrl 0x%X",value);
  start = 0;
#ifdef WIN32
  EnterCriticalSection (&action_section);
#else
  { int rc = pthread_mutex_lock(&action_mutex);
    if (rc) 
    { vmb_error(__LINE__,"Locking action mutex failed");
      pthread_exit(NULL);
    }
  }
#endif
  diskCtrl = value;
  if (diskCtrl & DISK_GO) {
     start = 1;
  }
#ifdef WIN32
  LeaveCriticalSection (&action_section);
  if (start) SetEvent (haction);
#else
  if (start) pthread_cond_signal(&action_cond);
  { int rc;
    rc = pthread_mutex_unlock(&action_mutex);
    if (rc) 
    { vmb_error(__LINE__,"Unlocking action mutex failed");
      pthread_exit(NULL);
    }
  }
#endif
  if (start)
    vmb_debug(VMB_DEBUG_INFO, "Action triggered");
  SET_DISK_CTRL(diskCtrl);
  mem_update(DISK_CTRL_OFFSET,4);
}


static unsigned int disk_server_shutdown = 0;

void wait_for_action(void)
/* waits for setting the start bit or a cancelation
*/
{ 
#ifndef WIN32
  { int rc = pthread_mutex_lock(&action_mutex);
    if (rc) 
    { vmb_error(__LINE__,"Locking action mutex failed");
      pthread_exit(NULL);
    }
  }
  pthread_cleanup_push(clean_up_action_mutex,NULL);
#endif
  /* in the meantime the action might have happend */
  while (!(diskCtrl & DISK_GO) &&
           !disk_server_shutdown)
#ifdef WIN32
     WaitForSingleObject(haction,INFINITE);
#else
     pthread_cond_wait(&action_cond,&action_mutex);
  pthread_cleanup_pop(1);
#endif
}


/* the thread serving dma requests is started when we 
   open the disk image and closed, when we close the disk Image */

static void diskWrite(void);
static void diskRead(void);

static int disk_server_running = 0;

#ifdef WIN32
static DWORD WINAPI disk_server(LPVOID dummy)
#else
static void *disk_server(void *_dummy)
#endif
{  disk_server_running = 1;
   while (disk_server_running)
   { wait_for_action();
     if (disk_server_shutdown) break;
     if (diskCtrl & DISK_GO) 
     {
      set_diskCtrl(diskCtrl & ~DISK_GO);
      if (diskCtrl & DISK_WRT)
           diskWrite();
       else
           diskRead();
     }
   }
  disk_server_shutdown=0;
  disk_server_running=0;
  return 0;
}


void start_disk_server(void)
{  if (diskImage != NULL) {
#ifdef WIN32
    DWORD dwDiskThreadId;
    HANDLE hDiskThread;
    hDiskThread = CreateThread( 
            NULL,              // default security attributes
            0,                 // use default stack size  
            disk_server,        // thread function 
            NULL,             // argument to thread function 
            0,                 // use default creation flags 
            &dwDiskThreadId);   // returns the thread identifier 
        // Check the return value for success. 
    if (hDiskThread == NULL) 
      vmb_fatal_error(__LINE__, "Creation of disk thread failed");
/* in the moment, I really dont use the handle */
    CloseHandle(hDiskThread);
#else
   pthread_t disk_thr;
   pthread_create(&disk_thr,NULL,disk_server,NULL);
#endif
}
}

void stop_disk_server(void)
{ 
  vmb_debug(VMB_DEBUG_PROGRESS, "Stopping disk server");
  disk_server_shutdown = 1;
  while (disk_server_running)
  {
#ifdef WIN32
    SetEvent (haction);
#else
  { int rc = pthread_mutex_lock(&action_mutex);
    if (rc) 
    { vmb_error(__LINE__,"Locking action mutex failed");
      pthread_exit(NULL);
    }
    pthread_cond_signal(&action_cond);
    rc = pthread_mutex_unlock(&action_mutex);
    if (rc) 
    { vmb_error(__LINE__,"Unlocking action mutex failed");
      pthread_exit(NULL);
    }
  }
#endif
  }
}

/* Utilities to manage the virtual registers */

static void register_to_mem(void)
{  int i;
   SET_DISK_STAT(diskStatus);   
   SET_DISK_CTRL(diskCtrl);  	    
   SET_DISK_CAP(diskCap);	  
   SET_DISK_SCT(diskSct);	    
   SET_DISK_CNT(diskCnt);
   for (i=0; i<16;i++)
   { SET_DISK_DMA_ADDR(i,diskDma[i].address);
     SET_DISK_DMA_SIZE(i,diskDma[i].size);
   }
   mem_update(0,DISK_MEM);
}

static void mem_to_register(int offset, int size)
{  int i;
   diskCnt  = DISK_CNT;
   diskSct  = DISK_SCT;
   for (i=0;i<16 && i*0x10+DISK_DMA_OFFSET<offset+size;i++)
   { diskDma[i].address=DISK_DMA_ADDR(i);
     diskDma[i].size=DISK_DMA_SIZE(i);
   }
   if (offset < DISK_CTRL_OFFSET+4 && offset+size > DISK_CTRL_OFFSET)
     set_diskCtrl(DISK_CTRL);
   /* read only: diskCap and diskStatus */
}



/* Operating the Disk */


static void diskReset(void)
{ int i;
  vmb_debug(VMB_DEBUG_INFO, "Disk is Reset");
  diskCnt = 0;
  diskSct = 0;
  diskStatus = 0;
  for (i=0;i<16;i++) 
  { diskDma[i].address = 0;
    diskDma[i].size = 0;
  }
  set_diskCtrl(0);
  memset(mem,0,sizeof(mem));
}

static void diskInit(void) {
  long numBytes;

  diskImage = NULL;
  diskCap = 0;
  diskReset();
  vmb_debug(VMB_DEBUG_PROGRESS, "Initializing Disk");
  if (vmb_filename != NULL) {
    /* try to install disk */
    diskImage = vmb_fopen(vmb_filename, "r+b");
    if (diskImage == NULL)
      vmb_error(__LINE__,"cannot open disk image");
    else
    { fseek(diskImage, 0, SEEK_END);
      numBytes = ftell(diskImage);
      fseek(diskImage, 0, SEEK_SET);
      diskCap = numBytes / SECTOR_SIZE;
      vmb_debugi(VMB_DEBUG_INFO, "Disk of size %ld sectors installed.", (int)diskCap);
      start_disk_server();
    }
  }
  register_to_mem();
}



static void diskExit(void) {
  if (diskImage == NULL) 
    /* disk not installed */
    return;
  vmb_debug(VMB_DEBUG_PROGRESS, "Closing Disk");
  fclose(diskImage);
  diskImage = NULL;
  diskCap = 0;
  diskReset();
  register_to_mem();
  stop_disk_server();
}

static void diskBussy(void)
{ if ( diskStatus & DISK_BUSY)
     return;
#ifdef WIN32
  SendMessage(hMainWnd,WM_VMB_OTHER+1,0,0);
#endif
  vmb_debug(VMB_DEBUG_INFO, "Disk is Bussy");
  diskStatus = (diskStatus| DISK_BUSY) & ~DISK_ERR;
  SET_DISK_STAT(diskStatus);
  mem_update(0,4);
}


static void diskDone(void)
{ if ( !(diskStatus & DISK_BUSY))
     return;
#ifdef WIN32
  SendMessage(hMainWnd,WM_VMB_ON,0,0);
#endif
  vmb_debug(VMB_DEBUG_INFO, "Disk is Idle");
  diskStatus=(diskStatus & ~DISK_BUSY);
  SET_DISK_STAT(diskStatus);
  mem_update(0,4);
  if (diskCtrl & DISK_IEN) {
    vmb_raise_interrupt(&vmb,interrupt_no);
    vmb_debug(VMB_DEBUG_PROGRESS, "Raised interrupt");
  }
}

static int diskPosition(void)
{ vmb_debugi(VMB_DEBUG_PROGRESS, "Positioning to sector %d",(int)diskSct);

  if (diskCap > 0 &&
      diskCnt > 0 &&
      diskSct >= 0 &&
      diskSct < diskCap &&
      diskSct + diskCnt <= diskCap &&
      (fseek(diskImage, (int)diskSct * SECTOR_SIZE, SEEK_SET) == 0))
    { vmb_debug(VMB_DEBUG_INFO, "Positioned");
      return 1;
    }
  else
    {
      vmb_error(__LINE__,"cannot position to sector in disk image");
      diskStatus = diskStatus | DISK_ERR;
	  SET_DISK_STAT(diskStatus);
      mem_update(0,4);
      return 0;
    }
}

static unsigned char sector_buffer[SECTOR_SIZE];
data_address da ={sector_buffer,0,0,0,SECTOR_SIZE,STATUS_INVALID};

static void diskRead(void) 
{  /* disk --> memory */
  diskBussy();
  if (diskPosition())
  { int i;
    uint64_t total=diskCnt*SECTOR_SIZE;
    for (i=0; total>0; i++)
	{ uint64_t size = diskDma[i].size;
	  uint64_t address = diskDma[i].address;
	  while(size>0) 
	  { int part;
	    if (size>SECTOR_SIZE)
			part = SECTOR_SIZE;
		else
			part = (int)size;
		if (part>total)
		  size=part=(int)total;
        if (fread(sector_buffer, 1, part, diskImage) != part)  
	    { vmb_error(__LINE__,"cannot read from disk");
		  diskStatus=diskStatus| DISK_ERR;
		  diskDone();
          return;
        }
	vmb_debugi(VMB_DEBUG_PROGRESS, "Read sector %d",(int)diskSct);
	vmb_debugi(VMB_DEBUG_PROGRESS, "       size %d",(int)part);
        da.address_lo = LOTETRA(address);
        da.address_hi = HITETRA(address);
        da.status = STATUS_VALID;
        da.size = part;
        vmb_store(&vmb,&da);
	    address = address+part;
	    size = size-part;
        total=total-part;
	    { int d =(int)(diskCnt*SECTOR_SIZE-total)/SECTOR_SIZE;
	      if (d>0)
		  { diskCnt=diskCnt-d;
            diskSct=diskSct+d;
	        SET_DISK_CNT(diskCnt);
	        SET_DISK_SCT(diskSct);
	        mem_update(0x10,0x10);
		  }
	    }
	  }
	}
  }       
  diskDone();
}

static void diskWrite(void) 
{ /* memory --> disk */
  diskBussy();
  if (diskPosition())
  { int i;
    uint64_t total=diskCnt*SECTOR_SIZE;
    for (i=0; total>0; i++)
	{ uint64_t size = diskDma[i].size;
	  uint64_t address = diskDma[i].address;
	  while(size>0) 
	  { int part;
	    if (size>SECTOR_SIZE)
			part = SECTOR_SIZE;
		else
			part = (int)size;
		if (part>total)
		  size=part=(int)total;
		da.address_lo = LOTETRA(address);
        da.address_hi = HITETRA(address);
        da.status = STATUS_VALID;
        da.size = part;
        vmb_load(&vmb,&da);
        vmb_wait_for_valid(&vmb,&da);
        if (da.status!=STATUS_VALID) {
          vmb_error(__LINE__,"cannot read memory");
		  diskStatus=diskStatus| DISK_ERR;
		  diskDone();
          return;
        }
        if (fwrite(sector_buffer, 1, part, diskImage) != part)  {
          vmb_error(__LINE__,"cannot write to disk");
		  diskStatus=diskStatus| DISK_ERR;
		  diskDone();
          return;
        }
        vmb_debugi(VMB_DEBUG_PROGRESS, "Wrote sector %d",(int)diskSct);
	    address = address+part;
	    size = size-part;
        total=total-part;
	    { int d = (int)(diskCnt*SECTOR_SIZE-total)/SECTOR_SIZE;
	      if (d>0)
		  { diskCnt=diskCnt-d;
            diskSct=diskSct+d;
	        SET_DISK_CNT(diskCnt);
	        SET_DISK_SCT(diskSct);
	        mem_update(0x10,0x10);
		  }
	    }
	  }
	}
  }       
  diskDone();
}

/* connecting to the virtual motherbaord */


unsigned char *disk_get_payload(unsigned int offset, int size)
     /* read an octabyte  (one of the five registers)*/
{  
   register_to_mem();	
   return mem+offset;
}


void disk_put_payload(unsigned int offset, int size, unsigned char *payload)
     /* write an octabyte  (one of the five registers)*/
{  
   if ( diskStatus & DISK_BUSY)
   {  vmb_debug(VMB_DEBUG_NOTIFY, "Write ignored, disk bussy");
      return; /* no writing while we are bussy */
   }
   register_to_mem();
   memmove(mem+offset,payload,size);
   mem_to_register(offset,size);
   mem_update(offset,size);
}


void disk_poweron(void)
{  diskInit();
#ifdef WIN32
   PostMessage(hMainWnd,WM_VMB_ON,0,0);
#endif
}


void disk_poweroff(void)
{ diskExit();
#ifdef WIN32
   PostMessage(hMainWnd,WM_VMB_OFF,0,0);
#endif
}


void disk_reset(void)
{ diskExit();
  diskInit();
}

void disk_disconnected(void)
/* this function is called when the reading thread disconnects from the virtual bus. */
{ diskExit();
#ifdef WIN32
  PostMessage(hMainWnd,WM_VMB_DISCONNECT,0,0);
#endif
}


void disk_terminate(void)
/* this function is called when the motherboard politely asks the device to terminate.*/
{ diskExit();
#ifdef WIN32
  PostMessage(hMainWnd,WM_CLOSE,0,0);
#endif
}

#ifdef WIN32
struct inspector_def inspector[2] = {
    /* name size get_mem address num_regs regs */
	{"Registers",DISK_MEM,disk_reg_read,disk_get_payload,disk_put_payload,hex_format,octa_chunk,8,NUM_REGS,disk_regs},
	{0}
};
#endif

void init_device(device_info *vmb)
{	vmb_size = DISK_MEM;
#ifdef WIN32	
    haction =CreateEvent(NULL,FALSE,FALSE,NULL);
    InitializeCriticalSection (&action_section);
	hbussy = (HBITMAP)LoadImage(hInst, MAKEINTRESOURCE(IDB_BUSSY), 
				IMAGE_BITMAP, 32, 32, LR_CREATEDIBSECTION);
#endif
  vmb->poweron=disk_poweron;
  vmb->poweroff=disk_poweroff;
  vmb->disconnected=disk_disconnected;
  vmb->reset=disk_reset;
  vmb->terminate=disk_terminate;
  vmb->put_payload=disk_put_payload;
  vmb->get_payload=disk_get_payload;
#ifdef WIN32
  inspector[0].address=vmb_address;
#endif
}

#ifndef WIN32
device_info vmb = {0};
int main(int argc, char *argv[])
{
  param_init(argc, argv);
  if (vmb_verbose_flag) vmb_debug_mask=0;
  vmb_debugs(VMB_DEBUG_INFO, "%s ",vmb_program_name);
  vmb_debugs(VMB_DEBUG_INFO, "%s ", version);
  vmb_debugs(VMB_DEBUG_INFO, "host: %s ",host);
  vmb_debugi(VMB_DEBUG_INFO, "port: %d ",port);
  close(0); /* stdin */  init_device(&vmb);
  init_device(&vmb);
  vmb_debugi(VMB_DEBUG_INFO, "address hi: %x",HI32(vmb_address));
  vmb_debugi(VMB_DEBUG_INFO, "address lo: %x",LO32(vmb_address));
  vmb_debugi(VMB_DEBUG_INFO, "size: %x ",vmb_size);
  
  vmb_connect(&vmb,host,port); 

  vmb_register(&vmb,HI32(vmb_address),LO32(vmb_address),vmb_size,
               0, 0, vmb_program_name,major_version,minor_version);

  vmb_wait_for_disconnect(&vmb);
  return 0;
}

#endif
