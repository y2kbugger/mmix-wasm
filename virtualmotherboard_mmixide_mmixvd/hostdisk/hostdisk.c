/*
 * hostdisk.c -- hostdisk simulation
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
#include "message.h"
#include "error.h"
#include "bus-arith.h"
#include "param.h"

extern device_info vmb;
int major_version=2, minor_version=1;
char title[] ="VMB Host Disk";

char version[]="$Revision: 664 $ $Date: 2018-12-10 18:10:14 +0100 (Mon, 10 Dec 2018) $";

char howto[] =
"The hostdisk simulates a disk controller but is using the host file szstem";

#ifdef WIN32
struct register_def disk_regs[] = {
	/* name no offset size chunk format */
	{"Status"   , 0,0x00,4,tetra_chunk,hex_format},
	{"Control"  , 1,0x04,4,tetra_chunk,hex_format},
    {"Handle"   , 2,0x08,8,octa_chunk,unsigned_format},
    {"Mode"     , 3,0x10,8,octa_chunk,hex_format},
	{"Position" , 4,0x18,8,octa_chunk,unsigned_format},
    {"DMA0 Addr", 5,0x20,8,octa_chunk,hex_format},
    {"DMA0 Size", 6,0x28,8,octa_chunk,unsigned_format},
    {"DMA1 Addr", 7,0x30,8,octa_chunk,hex_format},
    {"DMA1 Size", 8,0x38,8,octa_chunk,unsigned_format},
    {"DMA2 Addr", 9,0x40,8,octa_chunk,hex_format},
    {"DMA2 Size",10,0x48,8,octa_chunk,unsigned_format},
    {"DMA3 Addr",11,0x50,8,octa_chunk,hex_format},
    {"DMA3 Size",12,0x58,8,octa_chunk,unsigned_format},
    {"DMA4 Addr",13,0x60,8,octa_chunk,hex_format},
    {"DMA4 Size",14,0x68,8,octa_chunk,unsigned_format},
    {"DMA5 Addr",15,0x70,8,octa_chunk,hex_format},
    {"DMA5 Size",16,0x78,8,octa_chunk,unsigned_format},
    {"DMA6 Addr",17,0x80,8,octa_chunk,hex_format},
    {"DMA6 Size",18,0x88,8,octa_chunk,unsigned_format},
    {"DMA7 Addr",19,0x90,8,octa_chunk,hex_format},
    {"DMA7 Size",20,0x98,8,octa_chunk,unsigned_format},
    {"DMA8 Addr",21,0xa0,8,octa_chunk,hex_format},
    {"DMA8 Size",22,0xa8,8,octa_chunk,unsigned_format},
    {"DMA9 Addr",23,0xb0,8,octa_chunk,hex_format},
    {"DMA9 Size",24,0xb8,8,octa_chunk,unsigned_format},
    {"DMAa Addr",25,0xc0,8,octa_chunk,hex_format},
    {"DMAa Size",26,0xc8,8,octa_chunk,unsigned_format},
    {"DMAb Addr",27,0xd0,8,octa_chunk,hex_format},
    {"DMAb Size",28,0xd8,8,octa_chunk,unsigned_format},
    {"DMAc Addr",29,0xe0,8,octa_chunk,hex_format},
    {"DMAc Size",30,0xe8,8,octa_chunk,unsigned_format},
    {"DMAd Addr",31,0xf0,8,octa_chunk,hex_format},
    {"DMAd Size",32,0xf8,8,octa_chunk,unsigned_format},
    {"DMAe Addr",33,0x100,8,octa_chunk,hex_format},
    {"DMAe Size",34,0x108,8,octa_chunk,unsigned_format},
    {"DMAf Addr",35,0x110,8,octa_chunk,hex_format},
    {"DMAf Size",36,0x118,8,octa_chunk,unsigned_format},
	{0}};
#endif

#define NUM_REGS 36
#define DISK_MEM 0x120
static unsigned char mem[DISK_MEM] = {0};
static FILE *files[0x100] = {0};

#define SECTOR_SIZE	512	/* sector size in bytes */

#define DISK_STAT	     GET4(mem)       /* status register */
#define DISK_CTRL_OFFSET 0x04			
#define DISK_CTRL	     GET4(mem+0x04)	    /* control register */
#define DISK_HND_OFFSET  0x08
#define DISK_HND	     (mem[0x08+7])	/* disk capacity register */
#define DISK_MOD	     (mem[0x10+7])	/* disk sector register */
#define DISK_POS	     GET8(mem+0x18)	/* sector count register */
#define DISK_DMA_OFFSET  0x20
#define DISK_DMA_ADDR(i) GET8(mem+0x20+i*0x10)	/* DMA address register */
#define DISK_DMA_SIZE(i) GET8(mem+0x28+i*0x10)	/* DMA size register */

#define SET_DISK_STAT(x)	   SET4(mem,x)       /* status register */
#define SET_DISK_CTRL(x)	   SET4(mem+4,x)	    /* control register */
#define SET_DISK_HND(x)	       SET8(mem+0x08,x)	/* disk capacity register */
#define SET_DISK_MOD(x)	       SET8(mem+0x10,x)	/* disk sector register */
#define SET_DISK_POS(x)	       SET8(mem+0x18,x)	/* sector count register */
#define SET_DISK_DMA_ADDR(i,x) SET8(mem+0x20+i*0x10,x)	/* DMA address register */
#define SET_DISK_DMA_SIZE(i,x) SET8(mem+0x28+i*0x10,x)	/* DMA size register */

/* command Bits */
#define DISK_GO  	0x01	/* a 1 written here starts the disk command */
#define DISK_IEN	0x02	/* enable disk interrupt */

#define DISK_OPEN	1	/* command */
#define DISK_CLOSE	2
#define DISK_READ	3
#define DISK_WRITE	4
#define DISK_SEEK	5
#define DISK_TELL	6

/* Status Bits */
#define DISK_ERR	0x80000000	/* 0 = ok, 1 = error; valid when BUSY = 0 */
#define DISK_BUSY	0x00000001	/* 1 = disk is working on a command */


#define MOD_READ	0x01
#define MOD_WRITE	0x02 
#define MOD_BINARY	0x04 
#define MOD_APPEND	0x08 

static char *modestring[0x10]=
{NULL,"rt","wt","r+t",NULL,"rb","wb","r+b",
 NULL,NULL,"at","a+t",NULL,NULL,"ab","a+b"};

/* Copies of mem values */
static unsigned int diskStatus;
static unsigned int diskCtrl;
static uint64_t diskHnd;
static uint64_t diskMod;
static uint64_t diskPos;
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

static unsigned char *dma_buffer = NULL;
static int dma_alloc =0; /* size allocated */
static int dma_size =0; /* size used */

int dma_grow(int size)
/* grow the buffer to the requested size */
{ unsigned char *p;
  if (size<=dma_alloc) return 1;
  size = (size+7)&~7; /* round to a multiple of 8 */
  if (dma_buffer==NULL)
  {	p=malloc(size);
    if (p == NULL)
	{ vmb_error(__LINE__,"Out of Memory");
	  return 0;
	}
	dma_buffer =p;
	dma_alloc = size;
	return 1;
  }
  p = realloc(dma_buffer,size);
  if (p == NULL)
  { vmb_error(__LINE__,"Out of Memory");
    return 0;
  }
  dma_buffer =p;
  dma_alloc = size;
  return 1;
}


static void getMemory(void);
static void diskRead(void);
static void diskWrite(void);
static void diskBussy(void);
static void diskDone(void);

static int disk_server_running = 0;
static char *rootdir=NULL;

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
      diskStatus=diskStatus&~DISK_ERR;
	  diskBussy();
	  switch((diskCtrl>>2)&0x3F)
	  { case DISK_OPEN: 
			if (files[diskHnd]!=NULL)
			{ diskStatus=diskStatus| DISK_ERR;
			  break;
			}
			if (modestring[diskMod]==NULL)
			{ diskStatus=diskStatus| DISK_ERR;
			  break;
			}
			{ int i;
			  for (i=1;i<16;i++)
			    diskDma[i].size=0;
			}
			getMemory(); /* filename */
			{ char *name;
			  if (rootdir==NULL)
                            name=(char *)dma_buffer;
			  else
			  { int len = (int)strlen((char *)dma_buffer)+(int)strlen(rootdir);
		        name = malloc(len+1);
			    if (name==NULL)
				  vmb_error(__LINE__,"Out of Memory");
				else
				{ strcpy(name,rootdir);
				  strcat(name,(char *)dma_buffer);
				}
			  }
			  if (name!=NULL)
 			    files[diskHnd] = fopen(name,modestring[diskMod]);
			  else
                files[diskHnd] = NULL;
			  if (files[diskHnd]==NULL)
			  { diskStatus=diskStatus| DISK_ERR;
			    break;
			  }
			  if (rootdir!=NULL)
				  free(name);
			}
			break;
	    case DISK_CLOSE:
			if (files[diskHnd]==NULL)
			{ diskStatus=diskStatus| DISK_ERR;
			  break;
			}
            if (fclose(files[diskHnd])!=0)
			{ diskStatus=diskStatus| DISK_ERR;
			  break;
			}
	        break;
	    case DISK_READ:
			if (files[diskHnd]==NULL)
			{ diskStatus=diskStatus| DISK_ERR;
			  break;
			}
			diskRead();
			break;
	    case DISK_WRITE:
			if (files[diskHnd]==NULL)
			{ diskStatus=diskStatus| DISK_ERR;
			  break;
			}
			diskWrite();
			break;
	    case DISK_SEEK:
			if (files[diskHnd]==NULL)
			{ diskStatus=diskStatus| DISK_ERR;
			  break;
			}
			{ int r;
			  if (diskPos<0)
				r = fseek(files[diskHnd],-((int)diskPos)-1,SEEK_END); 
			  else
				r = fseek(files[diskHnd],(int)diskPos,SEEK_SET); 
 			  if (r!=0)
			  { diskStatus=diskStatus| DISK_ERR;
			    break;
			  }
			}
			break;
	    case DISK_TELL:
			if (files[diskHnd]==NULL)
			{ diskStatus=diskStatus| DISK_ERR;
			  break;
			}
			diskPos=ftell(files[diskHnd]);
	    default:
			vmb_debugi(VMB_DEBUG_NOTIFY,"Unknown command: %x",diskCtrl);
            diskStatus=diskStatus| DISK_ERR;
			break;
	  }
	  diskDone();
     }
   }
  disk_server_shutdown=0;
  disk_server_running=0;
  return 0;
}


void start_disk_server(void)
{  
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
   SET_DISK_HND(diskHnd);	  
   SET_DISK_MOD(diskMod);	    
   SET_DISK_POS(diskPos);
   for (i=0; i<16;i++)
   { SET_DISK_DMA_ADDR(i,diskDma[i].address);
     SET_DISK_DMA_SIZE(i,diskDma[i].size);
   }
   mem_update(0,DISK_MEM);
}

static void mem_to_register(int offset, int size)
{  int i;
   diskHnd = DISK_HND;
   diskPos  = DISK_POS;
   diskMod  = DISK_MOD;
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
  diskHnd=0;
  diskPos = 0;
  diskMod = 0;
  diskStatus = 0;
  for (i=0;i<16;i++) 
  { diskDma[i].address = 0;
    diskDma[i].size = 0;
  }
  set_diskCtrl(0);
  memset(mem,0,sizeof(mem));
  for (i=0;i<0x100;i++)
    if (files[i]!=NULL) 
	{ fclose(files[i]);
      files[i]=NULL;
    }
}

static void diskInit(void) 
{
  diskReset();
  vmb_debug(VMB_DEBUG_PROGRESS, "Initializing Disk");
  if (rootdir == NULL) 
  { char *cwd=vmb_get_cwd();
    set_option(&rootdir,cwd);
  }
  start_disk_server();
  register_to_mem();
}



static void diskExit(void) {
  vmb_debug(VMB_DEBUG_PROGRESS, "Closing Disk");
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


data_address da ={NULL,0,0,0,MAXPAYLOAD,STATUS_INVALID};

static void getMemory(void) 
{ /* memory --> dma_buffer */
  int i;
  dma_size=0;
  for (i=0; i<16; i++)
  { uint64_t size = diskDma[i].size;
	uint64_t address = diskDma[i].address;
	if (size==0) break;
	if (!dma_grow(dma_size+(int)size))
	{ diskStatus=diskStatus| DISK_ERR;
      return;
	}
	while(size>0) 
	{ int part;
	  if (size>MAXPAYLOAD)
		part = MAXPAYLOAD;
	  else
		part = (int)size;
      da.address_lo = LOTETRA(address);
      da.address_hi = HITETRA(address);
      da.status = STATUS_VALID;
      da.size = part;
	  da.data = dma_buffer+dma_size;
      vmb_load(&vmb,&da);
      vmb_wait_for_valid(&vmb,&da);
      if (da.status!=STATUS_VALID) {
          vmb_error(__LINE__,"cannot read memory");
		  diskStatus=diskStatus| DISK_ERR;
          return;
      }
      vmb_debugi(VMB_DEBUG_PROGRESS, "%d bytes read from memory",(int)part);
	  address = address+part;
	  size = size-part;
	  dma_size = dma_size+part;
	}
  }       
}

static void diskWrite(void) 
{ /* memory --> disk */
  int r,i;
  getMemory();
  r = (int)fwrite(dma_buffer,1,dma_size,files[diskHnd]);
  if (r< dma_size);
    diskStatus=diskStatus| DISK_ERR;
  for (i=0; i<16; i++)
  { uint64_t size = diskDma[i].size;
    if (size>r)
    { diskDma[i].size=r;
      r =0;
    }
    else
      r = r-(int)size;
  }
}

static void diskRead(void) 
{  /* disk --> memory */
  int i;
  for (i=0; i<16; i++)
  { uint64_t size = diskDma[i].size;
	uint64_t address = diskDma[i].address;
	if (size==0) break;
	while(size>0) 
	{ int part;
	  if (size>MAXPAYLOAD)
		part = MAXPAYLOAD;
	  else
		part = (int)size;
	  if (!dma_grow(part))
	  { diskStatus=diskStatus| DISK_ERR;
        return;
	  }
	  dma_size=(int)fread(dma_buffer, 1, part, files[diskHnd]);
      if (dma_size != part)  
	  { vmb_debug(VMB_DEBUG_ERROR,"cannot read from disk");
		diskStatus=diskStatus| DISK_ERR;
        return;
      }
	  vmb_debugi(VMB_DEBUG_PROGRESS, "Read %d byte",(int)part);
      da.address_lo = LOTETRA(address);
      da.address_hi = HITETRA(address);
      da.status = STATUS_VALID;
      da.size = part;
      vmb_store(&vmb,&da);
	  address = address+part;
	  size = size-part;
	}
  }       
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
	{"Registers",DISK_MEM,disk_reg_read,disk_get_payload,disk_put_payload,0,0,0,NUM_REGS,disk_regs},
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
  if (rootdir == NULL) 
  { char *cwd=vmb_get_cwd();
    set_option(&rootdir,cwd);
  }
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
