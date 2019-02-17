/*
 * sound.c -- sound simulation
   The following code is thanks to the gimmix project.
   It was ported to fitt the mmix motherboard by Martin Ruckert. 2005

 */

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#pragma warning(disable : 4996)

#include "winopt.h"
#include "resource.h"
extern HWND hMainWnd;
#include "inspect.h"
#include "vmb.h"
#include "error.h"
#include "bus-arith.h"
#include "param.h"

int major_version=2, minor_version=1;
extern device_info vmb;

char title[] ="VMB Sound";

char version[]="$Revision: 664 $ $Date: 2018-12-10 18:10:14 +0100 (Mon, 10 Dec 2018) $";

char howto[] =
"This simulates a sound card, see the HTML Help.\n";

#define NUM_REGS 43
struct register_def sound_regs[NUM_REGS+1] = {
	/* name offset size chunk format */
	{"Control"  ,0x00,1,byte_chunk,hex_format},
	{"DMA No 0" ,0x01,1,byte_chunk,unsigned_format},
	{"DMA No 1" ,0x02,1,byte_chunk,unsigned_format},
	{"DataFormat"	,0x03,1,byte_chunk,hex_format},
	{"Channels", 0x04,1,byte_chunk,unsigned_format},
	{"Bits per sample", 0x05,1,byte_chunk,unsigned_format},
	{"Samplerate", 0x06,2,wyde_chunk,unsigned_format},
	{"Unused", 0x08,2,byte_chunk,hex_format},
	{"Finished" ,0x0A,1,byte_chunk,unsigned_format},
	{"Playing"  ,0x0B,1,byte_chunk,unsigned_format},
	{"Position" ,0x0C,4,tetra_chunk,unsigned_format},
    {"1 Addr",0x10,8,octa_chunk,hex_format},
    {"1 Size",0x18,8,octa_chunk,unsigned_format},
    {"2 Addr",0x20,8,octa_chunk,hex_format},
    {"2 Size",0x28,8,octa_chunk,unsigned_format},
    {"3 Addr",0x30,8,octa_chunk,hex_format},
    {"3 Size",0x38,8,octa_chunk,unsigned_format},
    {"4 Addr",0x40,8,octa_chunk,hex_format},
    {"4 Size",0x48,8,octa_chunk,unsigned_format},
    {"5 Addr",0x50,8,octa_chunk,hex_format},
    {"5 Size",0x58,8,octa_chunk,unsigned_format},
    {"6 Addr",0x60,8,octa_chunk,hex_format},
    {"6 Size",0x68,8,octa_chunk,unsigned_format},
    {"7 Addr",0x70,8,octa_chunk,hex_format},
    {"7 Size",0x78,8,octa_chunk,unsigned_format},
    {"8 Addr",0x80,8,octa_chunk,hex_format},
    {"8 Size",0x88,8,octa_chunk,unsigned_format},
    {"9 Addr",0x90,8,octa_chunk,hex_format},
    {"9 Size",0x98,8,octa_chunk,unsigned_format},
    {"10 Addr",0xa0,8,octa_chunk,hex_format},
    {"10 Size",0xa8,8,octa_chunk,unsigned_format},
    {"11 Addr",0xb0,8,octa_chunk,hex_format},
    {"11 Size",0xb8,8,octa_chunk,unsigned_format},
    {"12 Addr",0xc0,8,octa_chunk,hex_format},
    {"12 Size",0xc8,8,octa_chunk,unsigned_format},
    {"13 Addr",0xd0,8,octa_chunk,hex_format},
    {"13 Size",0xd8,8,octa_chunk,unsigned_format},
    {"14 Addr",0xe0,8,octa_chunk,hex_format},
    {"14 Size",0xe8,8,octa_chunk,unsigned_format},
    {"15 Addr",0xf0,8,octa_chunk,hex_format},
    {"15 Size",0xf8,8,octa_chunk,unsigned_format},
    {"16 Addr",0x100,8,octa_chunk,hex_format},
    {"16 Size",0x108,8,octa_chunk,unsigned_format},
	{0}};

#define SOUND_MEM 0x110
static unsigned char mem[SOUND_MEM] = {0};

#define SECTOR_SIZE	0x800	/* sector size in bytes */

#define SOUND_CTRL_OFFSET		0x00			
#define SOUND_CTRL				(mem[SOUND_CTRL_OFFSET])	    /* control register */
#define SOUND_BUF0				(mem[1])
#define SOUND_BUF1				(mem[2])
#define SOUND_DATAFORMAT		(mem[3])	
#define SOUND_CHANNELS			(mem[4])
#define SOUND_BPS				(mem[5])
#define SOUND_SAMPLERATE		GET2(mem+0x6)
#define SET_SOUND_SAMPLERATE(x)	SET2(mem+0x6,x)
#define SOUND_FORMAT			GET4(mem+0x4)		/* channels, bps, samplerate */

#define SOUND_FINISHED			(mem[0xA])
#define SET_SOUND_FINISHED(x)	(SOUND_FINISHED=(x))	    /* finished register */
#define SOUND_PLAYING			(mem[0xB])
#define SET_SOUND_PLAYING(x)	(SOUND_PLAYING=(x))       /* playing register */

#define SOUND_BUFFERS			GET2(mem+0x1)  /* all both buffers */
#define SOUND_POS				GET4(mem+0x0C) /* sound position */
#define SET_SOUND_POS(x)		SET4(mem+0x0C,x)	/* sound position register */

#define SOUND_DMA_OFFSET		0x10
#define SOUND_DMA_ADDR(i)		GET8(mem+SOUND_DMA_OFFSET+i*0x10)	/* DMA address register */
#define SOUND_DMA_SIZE(i)		GET4(mem+SOUND_DMA_OFFSET+0xC+i*0x10)	/* DMA size register */
#define SET_SOUND_DMA_ADDR(i,x)	SET8(mem+SOUND_DMA_OFFSET+i*0x10,x)	/* DMA address register */
#define SET_SOUND_DMA_SIZE(i,x)	SET8(mem+SOUND_DMA_OFFSET+8+i*0x10,x)	/* DMA size register */

/* commands Bits */
#define SOUND_IGNORE		0
#define SOUND_PLAYONCE_MP3	1
#define SOUND_PLAYONCE_PCM	2
#define SOUND_PRELOAD		3
#define SOUND_UNLOAD		4
#define SOUND_PLAYLOOP_MP3	5
#define SOUND_PLAYLOOP_PCM	6
#define SOUND_INTERRUPT  	0x80	/* a 1 written here to enable interrupts*/
#define SOUND_CANCEL		0x7E
#define SOUND_RESET			0x7F


/* copies of read only registers */
DWORD dwSoundThreadId;
static unsigned int position=0; /* owned by player thread */
static unsigned char playing; /* owned by player thread */
static unsigned char finished; /* owned by player thread */
static unsigned char pcm_channels; /* owned by player thread */
static unsigned char pcm_bps; /* owned by player thread */
static unsigned int pcm_samplerate; /* owned by player thread */
/* copies of registers that are never changed by the player thread */
static struct {
	uint64_t address; /* owned by vmb thread, read only for player thread */
	unsigned int size; /* owned by vmb thread, read only for player thread */
	unsigned int loaded; /* owned by player thread, buffer if existing is loaded from 0 to loaded-1 */
	unsigned int allocated; /* owned by player thread */
	unsigned int played; /* owned by player thread, is played from 0 to played-1  */
	unsigned char *buffer; /* owned by player thread, if not NULL has allocated size */
}	soundDma[16];

/* the sound dma server */
/* messages to the sound dma server thread */
#define WM_SOUND_IGNORE				(WM_APP+1)
#define WM_SOUND_PRELOAD			(WM_APP+2)
#define WM_SOUND_CANCEL				(WM_APP+3)
#define WM_SOUND_RESET				(WM_APP+4)
#define WM_SOUND_TERMINATE			(WM_APP+5)
#define WM_SOUND_PLAYONCE_MP3		(WM_APP+6)
#define WM_SOUND_PLAYLOOP_MP3		(WM_APP+7)
#define WM_SOUND_PLAYONCE_PCM		(WM_APP+8)
#define WM_SOUND_PLAYLOOP_PCM		(WM_APP+9)
#define WM_SOUND_UNLOAD			    (WM_APP+10)


/* Utilities to manage the virtual registers */

static void mem_to_DMAregisters(int offset, int size)
/* moving mem data to read only registers */
{  int i;
   if (offset<SOUND_DMA_OFFSET) return;
for (i=(offset-SOUND_DMA_OFFSET)/0x10;i<16 && i*0x10+SOUND_DMA_OFFSET<offset+size;i++)
   { soundDma[i].address=SOUND_DMA_ADDR(i);
     soundDma[i].size=SOUND_DMA_SIZE(i);
     PostThreadMessage(dwSoundThreadId, WM_SOUND_UNLOAD,0,i+1); 
   }
}


static void register_to_mem(int offset, int size)
/* moving register data to read only memory */
{ SET_SOUND_PLAYING(playing);
  SET_SOUND_FINISHED(finished);
  SET_SOUND_POS(position);
}

static void register_update(void)
/* updates finished and playing position */
{ mem_update(0xA,6);
}

static void position_update(void)
/* updates position only */
{ mem_update(0xC,4);
}

/* connecting to the virtual motherbaord */

int sound_reg_read(unsigned int offset, int size, unsigned char *buf)
{ if (offset>SOUND_MEM) return 0;
  if (offset+size>SOUND_MEM) size =SOUND_MEM-offset;
  register_to_mem(offset,size);
  memmove(buf,mem+offset,size);
  return size;
}

unsigned char *sound_get_payload(unsigned int offset, int size)
     /* read an octabyte  (one of the five registers)*/
{  
   register_to_mem(offset,size);	
   return mem+offset;
}

static void set_soundCtrl(void);
static void soundInit(void);
static void soundExit(void);

void sound_put_payload(unsigned int offset, int size, unsigned char *payload)
{  memmove(mem+offset,payload,size);
   register_to_mem(offset, size);
   mem_update(offset,size);
   mem_to_DMAregisters(offset,size);
   if (offset==0 && size>0)
      set_soundCtrl();
}


void sound_poweron(void)
{  soundInit();
   PostMessage(hMainWnd,WM_VMB_ON,0,0);
}


void sound_poweroff(void)
{ soundExit();
  PostMessage(hMainWnd,WM_VMB_OFF,0,0);
}


void sound_reset(void)
{ soundExit();
  soundInit();
}

void sound_disconnected(void)
/* this function is called when the reading thread disconnects from the virtual bus. */
{ soundExit();
  PostMessage(hMainWnd,WM_VMB_DISCONNECT,0,0);
}


void sound_terminate(void)
/* this function is called when the motherboard politely asks the device to terminate.*/
{ soundExit();
  PostMessage(hMainWnd,WM_CLOSE,0,0);
}

struct inspector_def inspector[2] = {
    /* name size get_mem address num_regs regs */
	{"Registers",SOUND_MEM,sound_reg_read,sound_get_payload,sound_put_payload,hex_format,octa_chunk,8,NUM_REGS,sound_regs},
	{0}
};


void init_device(device_info *vmb)
{ vmb_size = SOUND_MEM;
  vmb->poweron=sound_poweron;
  vmb->poweroff=sound_poweroff;
  vmb->disconnected=sound_disconnected;
  vmb->reset=sound_reset;
  vmb->terminate=sound_terminate;
  vmb->put_payload=sound_put_payload;
  vmb->get_payload=sound_get_payload;
  inspector[0].address=vmb_address;
}


 
void set_soundCtrl(void)
{ 
  vmb_debugi(VMB_DEBUG_INFO, "Setting soundCtrl 0x%X",SOUND_CTRL);
  switch (SOUND_CTRL&~SOUND_INTERRUPT)
  {  case SOUND_IGNORE: 
         return;
     case SOUND_PLAYONCE_MP3:	
		 PostThreadMessage(dwSoundThreadId, WM_SOUND_PLAYONCE_MP3,(SOUND_CTRL&SOUND_INTERRUPT)<<24,SOUND_BUFFERS); 
		 return;
	 case SOUND_PLAYLOOP_MP3:	
		 PostThreadMessage(dwSoundThreadId, WM_SOUND_PLAYLOOP_MP3,(SOUND_CTRL&SOUND_INTERRUPT)<<24,SOUND_BUFFERS); 
		 return;	
     case SOUND_PLAYONCE_PCM:	
		 if (SOUND_DATAFORMAT==1) /*PCM */
		   PostThreadMessage(dwSoundThreadId, WM_SOUND_PLAYONCE_PCM,((SOUND_CTRL&SOUND_INTERRUPT)<<24)+SOUND_FORMAT,SOUND_BUFFERS); 
		 return;
	 case SOUND_PLAYLOOP_PCM:	
		 if (SOUND_DATAFORMAT==1) /*PCM */
		   PostThreadMessage(dwSoundThreadId, WM_SOUND_PLAYLOOP_PCM,((SOUND_CTRL&SOUND_INTERRUPT)<<24)+SOUND_FORMAT,SOUND_BUFFERS); 
		 return;	
	 case SOUND_PRELOAD:    
		 if (SOUND_BUF0) PostThreadMessage(dwSoundThreadId, WM_SOUND_PRELOAD,(SOUND_CTRL&SOUND_INTERRUPT)<<24,SOUND_BUF0); 
		 if (SOUND_BUF1) PostThreadMessage(dwSoundThreadId, WM_SOUND_PRELOAD,(SOUND_CTRL&SOUND_INTERRUPT)<<24,SOUND_BUF1); 
		 return;
	 case SOUND_UNLOAD:		
		 if (SOUND_BUF0) PostThreadMessage(dwSoundThreadId, WM_SOUND_UNLOAD,0,SOUND_BUF0); 
		 if (SOUND_BUF1) PostThreadMessage(dwSoundThreadId, WM_SOUND_UNLOAD,0,SOUND_BUF1); 
		 return;
	 case SOUND_CANCEL:	
		 PostThreadMessage(dwSoundThreadId, WM_SOUND_CANCEL,0,0); 
		 return;
	 case SOUND_RESET:	
		 PostThreadMessage(dwSoundThreadId, WM_SOUND_RESET,0,0); 
		 return;
	 default:
		 vmb_debugi(VMB_DEBUG_PROGRESS, "Unrecognized command (flags 0x%x)",SOUND_CTRL);
		 return;
  }
}



/* the thread serving dma requests is started when we 
   open the sound image and closed, when we close the sound Image */

static void soundWrite(void);
static void finished_loading(int i, int interrupt);
static void fill_bufer_cache(int i);

/* from wimp3.c */
extern void mp3_buffer_done(void);
extern void start_mp3_sound(void);
extern void start_pcm_sound(void);
extern void stop_sound(void);
static unsigned char buffers[2];
static int buffer_index=0; 
static int load_interrupts=0;
static enum {IDLE, ONCE, LOOP} mode=IDLE;

#define is_cached(i) (soundDma[i].loaded==soundDma[i].size)
#define needs_more_cacheing(i) (soundDma[i].loaded<soundDma[i].allocated)

static DWORD WINAPI sound_server(LPVOID dummy)
{  
   MSG msg;
   PeekMessage(&msg, NULL, WM_USER, WM_USER, PM_NOREMOVE);
   while (GetMessage(&msg,(HWND)-1,0,0))
   { switch (msg.message)
     { case WM_SOUND_IGNORE: break;
       case WM_SOUND_UNLOAD:
		 if (1<=msg.lParam && msg.lParam<=16)
		 { int i = (int)msg.lParam-1;
		   if (soundDma[i].buffer) 
		   { free(soundDma[i].buffer);
		     soundDma[i].buffer=NULL;
		   }
		   soundDma[i].allocated=0;
		   soundDma[i].loaded=0;
		   soundDma[i].played=0;
		 }
		 break;
	   case WM_SOUND_PRELOAD:
		 if (1<=msg.lParam && msg.lParam<=16 && !is_cached(msg.lParam-1))
		 { fill_bufer_cache((int)msg.lParam-1);
		   if (needs_more_cacheing(msg.lParam-1))
			 PostThreadMessage(dwSoundThreadId, WM_SOUND_PRELOAD,msg.wParam,msg.lParam); 
		   else 
			 finished_loading((int)msg.lParam,(int)msg.wParam);
		 }
         break;
       case WM_SOUND_CANCEL:
		 stop_sound();
		 break;
       case WM_SOUND_TERMINATE:
		   stop_sound();
		   return 0;
       case WM_SOUND_RESET:
		 stop_sound();
	     { int i;
           buffers[0]=buffers[1]=0;
           playing=finished=buffer_index=0;
           position=0;
		   register_update();
	       mode = IDLE;
	       for (i=0;i<16;i++) 
	       { soundDma[i].address = 0;
		     soundDma[i].size = 0;
		     soundDma[i].loaded=0;
		     soundDma[i].played=0;
		     if (soundDma[i].buffer!=NULL) free(soundDma[i].buffer);
		     soundDma[i].buffer=NULL;
			 soundDma[i].allocated=0;
	       }
         }
	     break;
       case WM_SOUND_PLAYONCE_MP3:
	   case WM_SOUND_PLAYLOOP_MP3:
		   stop_sound(); /* just in case */
		   SET2(buffers, msg.lParam);
		   position=0;
		   load_interrupts=(int)msg.wParam&(1<<31);
		   mode=((msg.message==WM_SOUND_PLAYONCE_MP3)?ONCE:LOOP);
		   buffer_index=0;
		   playing=buffers[buffer_index];
		   register_update();
		   start_mp3_sound();
		   break;
	     break;
       case WM_SOUND_PLAYONCE_PCM:
	   case WM_SOUND_PLAYLOOP_PCM:
		   stop_sound(); /* just in case */
		   SET2(buffers, msg.lParam);
		   position=0;
		   load_interrupts=(int)msg.wParam&(1<<31);
		   pcm_channels=((int)msg.wParam>>24)&0x7F;
		   pcm_bps=((int)msg.wParam>>16)&0xFF;
		   pcm_samplerate=(int)msg.wParam&0xFFFF;
		   mode=((msg.message==WM_SOUND_PLAYONCE_PCM)?ONCE:LOOP);
		   buffer_index=0;
		   playing=buffers[buffer_index];
		   register_update();
		   start_pcm_sound();
		   break;
	     break;
	   case MM_WOM_DONE:
	     if (((LPWAVEHDR)msg.lParam)->dwFlags&WHDR_INQUEUE) 
			 break;
		 vmb_debugi(VMB_DEBUG_PROGRESS, "Done playing buffer (flags 0x%x)",((LPWAVEHDR)msg.lParam)->dwFlags);
		 mp3_buffer_done();
		 break;
     } 
   }
   return 0;
}


void start_sound_server(void)
{  
    HANDLE hSoundThread;
    hSoundThread = CreateThread( 
            NULL,              // default security attributes
            0,                 // use default stack size  
            sound_server,        // thread function 
            NULL,             // argument to thread function 
            0,                 // use default creation flags 
            &dwSoundThreadId);   // returns the thread identifier 
        // Check the return value for success. 
    if (hSoundThread == NULL) 
      vmb_fatal_error(__LINE__, "Creation of sound thread failed");
/* in the moment, I really dont use the handle */
    CloseHandle(hSoundThread);
	do Sleep(1); while (!PostThreadMessage(dwSoundThreadId,WM_SOUND_RESET,0,0));
}


data_address da ={NULL,0,0,0,SECTOR_SIZE,STATUS_INVALID};

static int vmb_load_sector(int i, unsigned char *buffer, unsigned int position, unsigned int size)
/* load size bytes (at most one sector) of buffer No i data 
   from position into buffer
*/
{ uint64_t address;
  vmb_debugi(VMB_DEBUG_INFO, "requested %d byte of sound data",size);
  if (size>SECTOR_SIZE)
     size = SECTOR_SIZE;
  if (size>soundDma[i].size-position)
	  size = soundDma[i].size-position;
  if (size<=0) return 0; 
  address=soundDma[i].address+position;
  da.data=buffer;
  da.address_lo = LOTETRA(address);
  da.address_hi = HITETRA(address);
  da.status = STATUS_VALID;
  da.size = size;
  vmb_load(&vmb,&da);
  vmb_wait_for_valid(&vmb,&da);
  if (da.status!=STATUS_VALID) 
  { vmb_error(__LINE__,"cannot read sound buffer");
    return 0;
  }
  vmb_debugi(VMB_DEBUG_PROGRESS, "loaded %d byte of sound data",size);
  return size;
}

static void fill_bufer_cache(int i)
/* load one bus message of data into buffer i */
{ int size;
  if (soundDma[i].loaded>= soundDma[i].size) return;
  if (soundDma[i].buffer==NULL) 
  { soundDma[i].buffer=(unsigned char *)malloc((size_t)soundDma[i].size);
    if (soundDma[i].buffer==NULL) return;
	soundDma[i].allocated=soundDma[i].size;
  }
  if (soundDma[i].allocated!=soundDma[i].size) 
  { unsigned char *buffer=(unsigned char *)realloc(soundDma[i].buffer,(size_t)soundDma[i].size);
    if (buffer==NULL)
    { free(soundDma[i].buffer);
      soundDma[i].buffer=NULL;
      soundDma[i].allocated=0;
      return;
    }
	else
	{ soundDma[i].buffer=buffer;
	  soundDma[i].allocated=soundDma[i].size;
	}
  }
  /* now we have a buffer with loaded < allocated == size */
  size=vmb_load_sector(i, soundDma[i].buffer+soundDma[i].loaded, soundDma[i].loaded, soundDma[i].allocated-soundDma[i].loaded);
  soundDma[i].loaded=soundDma[i].loaded+size;
}

int get_buffer_data(int i, unsigned char *data, unsigned int position, unsigned int size)
/* get up to size bytes of data from buffer i at position
*/
{ unsigned int available;
  while (needs_more_cacheing(i)) 
	  fill_bufer_cache(i); /* complete cacheing if requested */
  if (is_cached(i)) 
  { available = (unsigned int)(soundDma[i].allocated-position); 
	if (available<size) size=available;
	if (size>0) memmove(data, soundDma[i].buffer+position, size);
  }
  else
  { unsigned int k, n;
	available = (unsigned int)(soundDma[i].size-position); 
	if (available<size) 
		size=available;
    /* now we have a buffer that is not preloaded */
	n=0;
	while (n<size && (k = vmb_load_sector(i, data+n, position+n, size-n))>0)
		n=n+k;
	size = n;
  }
  if (available-size <= 0) 
    finished_loading(i,load_interrupts);
  return size;
}


int input_read(int id, void *data, size_t size)
/* this function is called when we play a stream and need more data 
   position is the position in the buffer.
   buffers[0] and buffers[1], are the buffers to be played, eventualy as a loop
   buffer_index gives the current buffer.
*/
{ if (size<=0) 
	return (int)size;
  if (position>=soundDma[buffers[buffer_index]-1].size)
  { /* we need to advance to the next buffer */
	if (buffer_index==0 && buffers[1]!=0 && soundDma[buffers[1]-1].size>0) 
	{ buffer_index=1;
	  vmb_debug(VMB_DEBUG_INFO, "switching to buffer 1");
	}
	else if (mode!=ONCE && 
		     buffers[0]!=0 && soundDma[buffers[0]-1].size>0)
	{ buffer_index=0;
	  vmb_debug(VMB_DEBUG_INFO, "switching to buffer 0");
    }
	else 
	{ playing =0;
	  position=0;
	  register_update();
	  return 0;
	}
    position=0;
	playing = buffers[buffer_index];
	register_update();
  }
  size = get_buffer_data(playing-1, data, position, (unsigned int)size);
  position=position+size;
  position_update();
  return (int)size;
}

/* Operating the Sound */





void pcm_get_info(WAVEFORMATEX *wavefmtex)
/*set the wavefmtex */
{	wavefmtex->wFormatTag = WAVE_FORMAT_PCM;
	wavefmtex->wBitsPerSample = pcm_bps;
	wavefmtex->nChannels = pcm_channels;
	wavefmtex->nBlockAlign = wavefmtex->nChannels*(wavefmtex->wBitsPerSample/8);
	wavefmtex->nSamplesPerSec = pcm_samplerate;
	wavefmtex->nAvgBytesPerSec = wavefmtex->nSamplesPerSec *wavefmtex->nChannels*(wavefmtex->wBitsPerSample/8);
	wavefmtex->cbSize = 0;
}


static void soundInit(void) 
/* called at power on and reset */
{
  vmb_debug(VMB_DEBUG_PROGRESS, "Initializing Sound");
  memset(mem,0,sizeof(mem));
  mem_update(0,sizeof(mem));
  PostThreadMessage(dwSoundThreadId, WM_SOUND_RESET,0,0);
}

static void soundExit(void) 
/* called at power off, reset, disconnect, terminate */
{
  vmb_debug(VMB_DEBUG_PROGRESS, "Closing Sound");
  PostThreadMessage(dwSoundThreadId, WM_SOUND_CANCEL,0,0);
}

static void finished_loading(int i, int interrupt)
{   finished=i+1;
    if (interrupt)
	  vmb_raise_interrupt(&vmb,interrupt_no);
}

