#include <winsock2.h>
#include <windows.h>
#include <commctrl.h>
#include <afxres.h>
#include <math.h>
#include "message.h"
#include "vmb.h"
#include "bus-arith.h"
#include "resource.h"
#include "param.h"
#include "option.h"
#include "winopt.h"
#include "opt.h"
#include "inspect.h"

int major_version=2, minor_version=1;
char version[]="$Revision: 664 $ $Date: 2018-12-10 18:10:14 +0100 (Mon, 10 Dec 2018) $";
char title[] ="VMB Video Ram";
#define WS_VRAM (WS_OVERLAPPEDWINDOW&(~WS_MAXIMIZEBOX)&(~WS_THICKFRAME)) 


/*
 *     The Screen Stuff
 *
 */
char howto[] =
  "\n"
  "The program first reads the configuration file, \"default.vmb\".\n"
  "Then, the program simulates a video ram, a mouse, and a GPU.\n";

extern int framewidth, frameheight, fontwidth, fontheight;
extern double zoom;
static HDC hCanvas = NULL;
static CRITICAL_SECTION   bitmap_section;

unsigned char *screen_get_payload(unsigned int offset,int size)
{ static unsigned char payload[MAXPAYLOAD];
  int i = 0;
  EnterCriticalSection (&bitmap_section);
  if ((offset&0x3)!=0)
  { int x, y;
    COLORREF c;
    y = (offset/4) / framewidth;
	x = (offset/4) % framewidth;
	c = GetPixel(hCanvas,x,y);
    if ((offset&0x3)<2) payload[i++]= GetRValue(c);
    if ((offset&0x3)<3) payload[i++]= GetGValue(c);
    if ((offset&0x3)<4) payload[i++]= GetBValue(c);
  }
  while (size >= 4)
  { int x, y;
    COLORREF c;
    y = (offset/4) / framewidth;
	x = (offset/4) % framewidth;
	c = GetPixel(hCanvas,x,y);
    payload[i++]= 0;
    payload[i++]= GetRValue(c);
    payload[i++]= GetGValue(c);
    payload[i++]= GetBValue(c);
	offset = offset+4;
	size = size-4;
  }
  if (size > 0)
  { int x, y;
    COLORREF c;
    y = (offset/4) / framewidth;
	x = (offset/4) % framewidth;
	c = GetPixel(hCanvas,x,y);
    payload[i++]= 0;
    if (size>1) payload[i++]= GetRValue(c);
    if (size>2) payload[i++]= GetGValue(c);
    if (size>3) payload[i++]= GetBValue(c);
  }
  LeaveCriticalSection (&bitmap_section);
  return payload;
}

static void put_one_pixel(int x, int y, unsigned char color[4])
{  COLORREF c;
   c = RGB(color[1],color[2],color[3]);
   c = SetPixel(hCanvas,x,y,c);
}
static void get_one_pixel(int x, int y, unsigned char color[4])
{  COLORREF c;
      c = GetPixel(hCanvas,x,y);
	  color[0]=0;
	  color[1]=GetRValue(c);
	  color[2]=GetGValue(c);
	  color[3]=GetBValue(c);
}

static screen_put_byte(unsigned int pos, unsigned char p)
{ int x, y, z;
  unsigned char xrgb[4];
  COLORREF c;
	x = (pos/4) % framewidth;
    y = (pos/4) / framewidth;
	z = (pos%4);
	c = GetPixel(hCanvas,x,y);
	xrgb[0]=0;
	xrgb[1] = GetRValue(c);
    xrgb[2] = GetGValue(c);
    xrgb[3] = GetBValue(c);
	xrgb[z]= p;
	c = RGB(xrgb[1],xrgb[2],xrgb[3]);
 	c = SetPixel(hCanvas,x,y,c);
}

void screen_put_payload(unsigned int offset,int size, unsigned char *payload)
{ RECT rect;
  int i;
  int pos,remaining;
  int x, y;
  int minx,miny,maxx,maxy;
  pos=offset;
  i=0;
  remaining=size;
  y = (pos/4) / framewidth;
  x = (pos/4) % framewidth;
  maxx=minx=x;
  maxy=miny=y;
  EnterCriticalSection (&bitmap_section);
  while (pos%4!=0 && remaining>0)
  { screen_put_byte(pos,payload[i]);
    pos++;
	i++;
	remaining--;
  }
  while (remaining >= 4)
  { y = (pos/4) / framewidth;
	x = (pos/4) % framewidth;
	if (x>maxx)maxx=x;
	if (y>maxy)maxy=y;
	if (x<minx)minx=x;
	if (y<miny)miny=y;
	put_one_pixel(x,y,payload+i);
	i = i+4;
	pos = pos+4;
	remaining = remaining-4;
  }
  while (remaining > 0)
  { screen_put_byte(pos,payload[i]);
    y = (pos/4) / framewidth;
	x = (pos/4) % framewidth;
	if (x>maxx)maxx=x;
	if (y>maxy)maxy=y;
	if (x<minx)minx=x;
	if (y<miny)miny=y;
	pos++;
	i++;
	remaining--;
  }
  rect.top=(int)(miny*zoom);
  rect.left=(int)(minx*zoom);
  rect.bottom=(int)((maxy+1)*zoom);
  rect.right=(int)((maxx+1)*zoom);
  InvalidateRect(hMainWnd,&rect,FALSE);
  LeaveCriticalSection (&bitmap_section);
  mem_update_i(0,offset,size);
}

void screen_poweron(void)
{ RECT rect;
  int rc;
  HBRUSH hbr;
  EnterCriticalSection (&bitmap_section);
  hbr = CreateSolidBrush(RGB(0,0,0));
  rect.top=0, rect.left=0, rect.bottom=frameheight+1, rect.right=framewidth+1;
  rc = FillRect(hCanvas,&rect,hbr);
  InvalidateRect(hMainWnd,NULL,FALSE);
  LeaveCriticalSection (&bitmap_section);
  mem_update_i(0,0,vmb_size);
}


void screen_poweroff(void)
{ RECT rect;
  int rc;
  HBRUSH hbr;
  EnterCriticalSection (&bitmap_section);
  hbr = CreateSolidBrush(RGB(127,127,127));
  rect.top=0, rect.left=0, rect.bottom=frameheight+1, rect.right=framewidth+1;
  rc = FillRect(hCanvas,&rect,hbr);
  InvalidateRect(hMainWnd,NULL,FALSE);
  LeaveCriticalSection (&bitmap_section);
  mem_update_i(0,0,vmb_size);
}


void screen_disconnected(void)
/* this function is called when the reading thread disconnects from the virtual bus. */
{ RECT rect;
  int rc;
  HBRUSH hbr;
  if (hCanvas != NULL)
  { EnterCriticalSection (&bitmap_section);
    hbr = CreateSolidBrush(RGB(127,0,0));
    rect.top=0, rect.left=0, rect.bottom=frameheight+1, rect.right=framewidth+1;
    rc = FillRect(hCanvas,&rect,hbr);
    InvalidateRect(hMainWnd,NULL,FALSE);
    LeaveCriticalSection (&bitmap_section);
	mem_update_i(0,0,vmb_size);
  }
  PostMessage(hMainWnd,WM_VMB_DISCONNECT,0,0); /* the disconnect button */
}


void screen_reset(void)
{ screen_poweron();
}

static void init_gpu_canvas(void);



void init_canvas(void)
{	HDC hdc; 
	hdc = GetDC(hMainWnd);
	if (hCanvas!=NULL) DeleteDC(hCanvas); 
	hCanvas = CreateCompatibleDC(hdc);	 
	if (hCanvas==NULL) return;
	InitializeCriticalSection (&bitmap_section);
    if (hBmp!=NULL) DeleteObject(hBmp);
    hBmp = CreateCompatibleBitmap(hdc,framewidth,frameheight);
    if (hBmp==NULL) return;
    SelectObject(hCanvas, hBmp); 
    ReleaseDC(hMainWnd, hdc); 
	vmb_size = frameheight*framewidth*4;
	init_gpu_canvas();
}

static int read_screen(unsigned int offset, int size, unsigned char *buf)
{ unsigned char *p;
  int i=0;
  while (i<size)
  { int s;
    if (size-i<MAXPAYLOAD) s=size-i; else s=MAXPAYLOAD;
    p=screen_get_payload(offset+i,s);
    memmove(buf+i,p,s);
	i=i+s;
  }
  return i;
}

struct inspector_def inspector[4] = {0};

void init_screen(device_info *vmb)
{	init_canvas();
    vmb_size = frameheight*framewidth*4;
	if (vmb->power) screen_poweron(); else screen_poweroff();
	vmb->poweron=screen_poweron;
	vmb->poweroff=screen_poweroff;
	vmb->disconnected=screen_disconnected;
	vmb->reset=screen_reset;
	vmb->terminate=vmb_terminate;
	vmb->put_payload=screen_put_payload;
	vmb->get_payload=screen_get_payload;
	{ RECT rc;
	  rc.top = 0;
	  rc.left = 0;
	  rc.right = (int)(width*zoom);
	  rc.bottom = (int)(height*zoom);
	  AdjustWindowRect(&rc,WS_VRAM,FALSE);
      SetWindowPos(hMainWnd,HWND_TOP,xpos,ypos,rc.right-rc.left,rc.bottom-rc.top,SWP_SHOWWINDOW);
	}
	inspector[0].name="Video RAM";
	inspector[0].size=vmb_size;
	inspector[0].get_mem=read_screen;
	inspector[0].load=screen_get_payload;
	inspector[0].store=screen_put_payload;
    inspector[0].format=hex_format;
	inspector[0].chunk=tetra_chunk;
	inspector[0].de_offset=-1;
	inspector[0].sb_rng=8;
	inspector[0].address=vmb_address;
	inspector[0].num_regs=0;
	inspector[0].regs=NULL;
}


/*
 *     The Mouse Stuff
 *
 */
extern uint64_t vmb_mouse_address;
extern int move_interrupt;
static device_info vmb_mouse = {0};
static unsigned char mouse_mem[2*8];

#define VMB_MOUSEMOVE                  (0x80)
#define VMB_LBUTTONDOWN                (MK_LBUTTON | MK_SHIFT)
#define VMB_LBUTTONUP                  (MK_LBUTTON)
#define VMB_LBUTTONDBLCLK              (MK_LBUTTON | MK_SHIFT | MK_CONTROL)
#define VMB_RBUTTONDOWN                (MK_RBUTTON | MK_SHIFT)
#define VMB_RBUTTONUP                  (MK_RBUTTON)
#define VMB_RBUTTONDBLCLK              (MK_RBUTTON | MK_SHIFT | MK_CONTROL)
#define VMB_MBUTTONDOWN                (MK_MBUTTON | MK_SHIFT)
#define VMB_MBUTTONUP                  (MK_MBUTTON)
#define VMB_MBUTTONDBLCLK              (MK_MBUTTON | MK_SHIFT | MK_CONTROL)


void mouse_poweron(void)
{ memset(mouse_mem,0,sizeof(mouse_mem));
  mem_update_i(1,0,0x10);
}

unsigned char *mouse_get_payload(unsigned int offset,int size)
{ 
   return mouse_mem+offset;
}

void mouse_set_position(WPARAM wParam, LPARAM lParam, int event)
{   int xPos, yPos;
	xPos = (int)(LOWORD(lParam)/zoom); 
	yPos = (int)(HIWORD(lParam)/zoom); 
	mouse_mem[12] = (xPos>>8)&0xFF;
	mouse_mem[13] = (xPos)&0xFF;
	mouse_mem[14] = (yPos>>8)&0xFF;
	mouse_mem[15] = (yPos)&0xFF;
	if (event != VMB_MOUSEMOVE || move_interrupt)
	{  
		/* The values of wParam indicate which buttons are down when
		the event happens:
		0x01 = MK_LBUTTON The left mouse button is down. 
	    	0x10 = MK_MBUTTON The middle mouse button is down. 
		0x02 = MK_RBUTTON The right mouse button is down. 
		0x04 = MK_SHIFT The SHIFT key is down. 
		0x08 = MK_CONTROL The CTRL key is down.   
		*/
		mouse_mem[1] = (unsigned char)(wParam&0x1F);
		/* The eventbits use MK_LBUTTON, MK_MBUTTON, and MK_RBUTTON
		   to indicate the button. Bit 0x04 (MK_SHIFT) is on for a
		   down event, and 0 for an up event. If the down event is
		   a double-click, the bit 0x08 (MK_CONTROL) is set in addition.
		   A Mouse Move event is indicated by bit 0x80.
		 */
		mouse_mem[3] = event;	
		/* next we keep a copy of the position that is not changed
		   by subsequent mouse moves 
		*/
		mouse_mem[4] = mouse_mem[12];
		mouse_mem[5] = mouse_mem[13];
		mouse_mem[6] = mouse_mem[14];
		mouse_mem[7] = mouse_mem[15];
		vmb_raise_interrupt(&vmb_mouse, interrupt_no);
		vmb_debugi(VMB_DEBUG_PROGRESS,"Mouse Event %X",event);
	}
  mem_update_i(1,0,0x10);
}

static int read_mouse(unsigned int offset, int size, unsigned char *buf)
{ if (offset>0x10) return 0;
  if (offset+size>0x10) size =0x10-offset;
  memmove(buf,mouse_mem+offset,size);
  return size;
}

struct register_def mouse_regs[] = {
	{"Buttons",  0,2,wyde_chunk,hex_format},
	{"Events",   2,2,wyde_chunk,hex_format},
    {"X",        4,2,wyde_chunk,unsigned_format},
    {"Y",        6,2,wyde_chunk,unsigned_format},
    {"Current X",0xC,2,wyde_chunk,unsigned_format},
    {"Current Y",0xE,2,wyde_chunk,unsigned_format},
	{0}};

void init_mouse(void)
{	vmb_mouse.id = 1;
	vmb_mouse.get_payload=mouse_get_payload;
	vmb_mouse.poweron = mouse_poweron;
	vmb_mouse.reset = mouse_poweron;
	vmb_mouse.terminate = NULL;
	inspector[1].name = "Mouse";
	inspector[1].size=0x10;
	inspector[1].get_mem=read_mouse;
	inspector[1].load=mouse_get_payload;
	inspector[1].store=NULL;
	inspector[1].format=unsigned_format;
	inspector[1].chunk=wyde_chunk;
	inspector[1].de_offset=-1;
	inspector[1].address=0;
	inspector[1].sb_rng=8;
	inspector[1].num_regs= 6;
	inspector[1].regs =mouse_regs;
}


/*
 *     The GPU Stuff
 *
 */
extern uint64_t vmb_gpu_address;
static device_info vmb_gpu = {0};
extern int gpu_interrupt;
#define GPU_REGS 19
static struct register_def gpu_regs[GPU_REGS+1] = {
	/* name, nr,offset,size, chunk,format */
	{"Command",        0x00,1,byte_chunk,unsigned_format},
	{"Aux Command",    0x01,3,byte_chunk,hex_format},
    {"Secondary X",    0x04,2,wyde_chunk,unsigned_format},
    {"Secondary Y",    0x06,2,wyde_chunk,unsigned_format},
    {"Width",          0x08,2,wyde_chunk,unsigned_format},
    {"Height",         0x0a,2,wyde_chunk,unsigned_format},
    {"X",              0x0c,2,wyde_chunk,unsigned_format},
    {"Y",              0x0e,2,wyde_chunk,unsigned_format},
    {"BLT Adress",     0x10,8,octa_chunk,hex_format},
    {"Text Backgrnd",  0x18,4,tetra_chunk,hex_format},
    {"Text Color",     0x1c,4,tetra_chunk,hex_format},
    {"Fill Color",     0x20,4,tetra_chunk,hex_format},
    {"Line Color",     0x24,4,tetra_chunk,hex_format},
    {"Text Width",     0x28,2,wyde_chunk,unsigned_format},
    {"Text Height",    0x2a,2,wyde_chunk,unsigned_format},
    {"Frame Width",    0x30,2,wyde_chunk,unsigned_format},
    {"Frame Height",   0x32,2,wyde_chunk,unsigned_format},
    {"Screen Width",   0x34,2,wyde_chunk,unsigned_format},
    {"Screen Height",  0x36,2,wyde_chunk,unsigned_format},
	{0}};

/*
GPU Commands and memmory layout
1 Layout
First OCTA:
gpu_mem[0] command 
gpu_mem[1] command aux HI
gpu_mem[2] command aux MI
gpu_mem[3] command aux LO
gpu_mem[4] x secondary HI
gpu_mem[5] x secondary LO
gpu_mem[6] y secondary HI
gpu_mem[7] y secondary LO
Second OCTA
gpu_mem[8] width HI
gpu_mem[9] width LO
gpu_mem[a] height HI
gpu_mem[b] height LO
gpu_mem[c] x position HI
gpu_mem[d] x position LO
gpu_mem[e] y position HI
gpu_mem[f] y position LO
Third OCTA
gpu_mem[10] blt address
gpu_mem[11] 
gpu_mem[12] 
gpu_mem[13] 
gpu_mem[14] 
gpu_mem[15] 
gpu_mem[16] 
gpu_mem[17] blt address
Fourth OCTA
gpu_mem[18] text background color aux
gpu_mem[19] text background color Red
gpu_mem[1a] text background color Green
gpu_mem[1b] text background color Blue
gpu_mem[1c] text color aux
gpu_mem[1d] text color Red
gpu_mem[1e] text color Green
gpu_mem[1f] text color Blue
Fifth OCTA
gpu_mem[20] fill color aux
gpu_mem[21] fill color Red
gpu_mem[22] fill color Green
gpu_mem[23] fill color Blue
gpu_mem[24] line color aux
gpu_mem[25] line color Red
gpu_mem[26] line color Green
gpu_mem[27] line color Blue
Sixth OCTA
gpu_mem[28] text width HI
gpu_mem[29] text width LO
gpu_mem[2a] text heigth HI
gpu_mem[2b] text height LO
gpu_mem[2c] 
gpu_mem[2d] 
gpu_mem[2e] 
gpu_mem[2f] 
Seventh OCTA
gpu_mem[30] frame width HI
gpu_mem[31] frame width LO
gpu_mem[32] frame height HI
gpu_mem[33] frame height LO
gpu_mem[34] width HI
gpu_mem[35] width LO
gpu_mem[36] height HI
gpu_mem[37] height LO

The first 4 Byte (0-4) contain the command. writing any of these
bytes will trigger the execution of the command.
The command may be split into a command number (byte 0) and some
auxiliar information (1-3).

Most commands require a position. the coordinates are taken
from Bytes c,d (x) and e,f (y).  If appropriate, executing of a command
will update these values to make the two values work like a cursor.

Bit Block transfers from/to non graphic memory need a physical
target address. The address goes in the third OCTA.

Most commands require, in addition, color information (Drawing of Text,
Lines and Rectangles) this information commes in the fourth and fifth OCTA.

Commands that need additional x,y information would primarily use byte 4 to 7.
Additional x,y and w,h information can be provided in the second OCTA.

The sixth and seventh OCTA are read only. They provide information
on the size of the system font, the frame size and the visible part of it.

2 Commands
2.0 command == 0 Do Nothing
2.1 command == 1 Write Character
write the character contained in command aux LO (ASCII) to the position
given in x and y using the system Font. update x and y for the next character.
Handle newline (0x0A) by moving x,y to the beginning of the next line and 
cariage return (0x0D) by moving x,y to the beginning of the current line.
Scroll up the screen, if a newline happens to be in the last line of the screen.
2.2 command == 2 Draw Rectangle
Draw a filled Rectangle using the fill color, using the x,y position as top left
and the secondary x,y as bottom right. No update of the position occurs.
2.3 command == 3 Draw Line
Draw a line using the line color from the x,y position to the secondary x,y
The secondary x,y will then replace the position x,y to support drawing joint lines.
If command aux is zero, the default line width is used (usually 1 pixel). If command aux
is not zero, it will be used as the line width in pixel.
2.4 command == 4 Bit Block Transfer
Transfers a rectangular block of pixels. The x,y position is the top left coordinate of 
the destination. the secondary x,y is the top left coordinate of the source. 
the width and heigth values of the second OCTA define the destination rectangle.
clipping (if needed) occurs on the destination rectangle.
The command aux value is used for the raster mode used in a
windows BitBlt Function. the following values are defined:
 SRCCOPY             0xCC0020  dest = source                   
 SRCPAINT            0xEE0086  dest = source OR dest           
 SRCAND              0x8800C6  dest = source AND dest          
 SRCINVERT           0x660046  dest = source XOR dest          
 SRCERASE            0x440328  dest = source AND (NOT dest )   
 NOTSRCCOPY          0x330008  dest = (NOT source)             
 NOTSRCERASE         0x1100A6  dest = (NOT src) AND (NOT dest) 
 MERGECOPY           0xC000CA  dest = (source AND pattern)     
 MERGEPAINT          0xBB0226  dest = (NOT source) OR dest     
 PATCOPY             0xF00021  dest = pattern                  
 PATPAINT            0xFB0A09  dest = DPSnoo                   
 PATINVERT           0x5A0049  dest = pattern XOR dest         
 DSTINVERT           0x550009  dest = (NOT dest)               
 BLACKNESS           0x000042  dest = BLACK                    
 WHITENESS           0xFF0062  dest = WHITE                    
*/

#define GPU_MEM_SIZE (7*8)
static unsigned char gpu_mem[GPU_MEM_SIZE];

static int read_gpu(unsigned int offset, int size, unsigned char *buf)
{ if (offset>GPU_MEM_SIZE) return 0;
  if (offset+size>GPU_MEM_SIZE) size =GPU_MEM_SIZE-offset;
  memmove(buf,gpu_mem+offset,size);
  return size;
}

#define GPU_STATUS		(gpu_mem[0])
#define GPU_IDLE		0
#define GPU_COMMAND		(gpu_mem[0])
#define GPU_NOP			0x00
#define GPU_WRITE_CHAR		0x01
#define GPU_RECTANGLE		0x02
#define GPU_LINE		0x03
#define GPU_BLT			0x04
#define GPU_BLT_IN		0x05
#define GPU_BLT_OUT		0x06
#define GPU_DIB			0x07



#define GPU_COMMAND_AUX_LO	(gpu_mem[3])	
#define GPU_COMMAND_AUX		((gpu_mem[1]<<16)|(gpu_mem[2]<<8)|(gpu_mem[3]))
#define GPU_X_2			((gpu_mem[0x4]<<8)|gpu_mem[0x5])
#define GPU_Y_2			((gpu_mem[0x6]<<8)|gpu_mem[0x7])

#define GPU_W			((unsigned)((gpu_mem[8]<<8)|gpu_mem[9]))
#define GPU_H			((unsigned)((gpu_mem[0xa]<<8)|gpu_mem[0xb]))
#define GPU_X				((unsigned)((gpu_mem[0xc]<<8)|gpu_mem[0xd]))
#define GPU_Y				((unsigned)((gpu_mem[0xe]<<8)|gpu_mem[0xf]))
#define SET_16(i,v)		((gpu_mem[i]=(unsigned char)((v)>>8)),(gpu_mem[(i)+1]=(unsigned char)(v)))
#define SET_GPU_X(x)	SET_16(0xc,x)
#define SET_GPU_Y(y)	SET_16(0xe,y)

#define GPU_R_TEXT_BK		(gpu_mem[0x19])
#define GPU_G_TEXT_BK		(gpu_mem[0x1a])
#define GPU_B_TEXT_BK		(gpu_mem[0x1b])
#define GPU_R_TEXT		(gpu_mem[0x1d])
#define GPU_G_TEXT		(gpu_mem[0x1e])
#define GPU_B_TEXT		(gpu_mem[0x1f])

#define GPU_R_FILL		(gpu_mem[0x21])
#define GPU_G_FILL		(gpu_mem[0x22])
#define GPU_B_FILL		(gpu_mem[0x23])
#define GPU_R_LINE		(gpu_mem[0x25])
#define GPU_G_LINE		(gpu_mem[0x26])
#define GPU_B_LINE		(gpu_mem[0x27])

#define SET_FONT_W(w)	SET_16(0x28,w)
#define SET_FONT_H(w)	SET_16(0x2a,w)

#define SET_FRAME_W(w)	SET_16(0x30,w)
#define SET_FRAME_H(w)	SET_16(0x32,w)
#define SET_SCREEN_W(w)	SET_16(0x34,w)
#define SET_SCREEN_H(w)	SET_16(0x36,w)



static HFONT hgpu_font=0;

static int gpu_char_width, gpu_char_ascent, gpu_char_descent, gpu_char_height;
static int gpu_text_height;

void init_gpu_memory(void)
{ memset(gpu_mem,0,sizeof(gpu_mem));
  GPU_R_TEXT = GPU_G_TEXT = GPU_B_TEXT = 255; /* Text color white */
  GPU_R_LINE = GPU_G_LINE = GPU_B_LINE = 255; /* Line color white */
  GPU_R_FILL = GPU_G_FILL = GPU_B_FILL = 255; /* Fill color white */
  SET_FONT_W(gpu_char_width);
  SET_FONT_H(gpu_char_height);
  SET_FRAME_W(framewidth);	
  SET_FRAME_H(frameheight);
  SET_SCREEN_W(width);	
  SET_SCREEN_H(height);

  mem_update_i(2,0,GPU_MEM_SIZE);
}
void gpu_poweron(void)
{ init_gpu_memory();
}
unsigned char *gpu_get_payload(unsigned int offset,int size)
{  return gpu_mem+offset;
}

void gpu_put_payload(unsigned int offset,int size, unsigned char *payload)
{ int x,y,w,h,dw,dh;
  y=x=w=h=dw=dh=0;
  memmove(gpu_mem+offset,payload, size);
  if (offset>0 || GPU_COMMAND == GPU_NOP) 
  {	mem_update_i(2,offset,size);
	return;
  }
  EnterCriticalSection (&bitmap_section);
  switch (GPU_COMMAND)
  { case GPU_WRITE_CHAR: 
		vmb_debugi(VMB_DEBUG_PROGRESS,"Writing Character 0x%2x",GPU_COMMAND_AUX_LO);
		if (GPU_COMMAND_AUX_LO == '\n')
		{ int newline;
		  SET_GPU_X(0);
		  newline = GPU_Y+gpu_text_height;
		  if (newline+gpu_text_height>height)
		  { int d = newline+gpu_text_height-height;
		    BitBlt(hCanvas,0,0,width,height-d,hCanvas,0,d,SRCCOPY);
		    BitBlt(hCanvas,0,height-d,width,d,hCanvas,0,height-d,SRCINVERT);
			newline = newline-d;
			w=width;
			h=height;
		  }
		  SET_GPU_Y(newline);
		}
		else if (GPU_COMMAND_AUX_LO == '\r')
		{ SET_GPU_X(0);
		}
		else
		{ SetTextAlign(hCanvas,TA_TOP|TA_LEFT);
		  y = GPU_Y;
		  h = gpu_text_height;
		  x = GPU_X;
		  w = gpu_char_width;
		  SetTextColor(hCanvas,RGB(GPU_R_TEXT,GPU_G_TEXT,GPU_B_TEXT));
		  SetBkColor(hCanvas,RGB(GPU_R_TEXT_BK,GPU_G_TEXT_BK,GPU_B_TEXT_BK));
		  ExtTextOut(hCanvas,GPU_X,GPU_Y,0,NULL,&(GPU_COMMAND_AUX_LO),1,NULL);
		  SET_GPU_X(x+w);
		}
	    if (size<0x10) size=0x10;
		GPU_STATUS=GPU_IDLE;
        break;
	case GPU_RECTANGLE:
		 vmb_debug(VMB_DEBUG_PROGRESS,"Writing Rectangle");
	    { HBRUSH hold, hnew;
		  hnew = CreateSolidBrush(RGB(GPU_R_FILL,GPU_G_FILL,GPU_B_FILL)); 
		  hold = SelectObject(hCanvas, hnew);
		  x=GPU_X_2;
		  y=GPU_Y_2;
		  w=GPU_W;
		  h=GPU_H;
		  PatBlt(hCanvas, x,y, w, h ,PATCOPY);  
		  SelectObject(hCanvas, hold);
		  DeleteObject(hnew);
		}
		GPU_STATUS=GPU_IDLE;
		break;
	case GPU_LINE:
		 vmb_debug(VMB_DEBUG_PROGRESS,"Writing Line");
	    { int lwidth;
		  HPEN hold, hnew;
		  if (GPU_COMMAND_AUX==0) lwidth=1;
		  else lwidth=GPU_COMMAND_AUX;
		  hnew = CreatePen(PS_SOLID, lwidth, RGB(GPU_R_LINE,GPU_G_LINE,GPU_B_LINE)); 
		  hold = SelectObject(hCanvas, hnew);
   		  MoveToEx(hCanvas,GPU_X,GPU_Y,NULL);
		  LineTo(hCanvas,GPU_X_2,GPU_Y_2);
  		  SelectObject(hCanvas, hold);
		  DeleteObject(hnew);
		  x=GPU_X;
		  y=GPU_Y;
		  w=GPU_X_2-GPU_X;
		  h=GPU_Y_2-GPU_Y;
		  if (w<0) { x=x+w; w=-w; }
		  if (h<0) { y=y+h; h=-h; }
		  lwidth=(lwidth+1)/2;
		  x = x-lwidth;
		  y = y-lwidth;
		  w= w+2*lwidth;
		  h= h+2*lwidth;
		  SET_GPU_X(GPU_X_2);
		  SET_GPU_Y(GPU_Y_2);
		  if (size<0x10) size=0x10;
		}
		GPU_STATUS=GPU_IDLE;
		break;
	case GPU_BLT:
		vmb_debug(VMB_DEBUG_PROGRESS,"BitBlockTransfer within GPU");
		BitBlt(hCanvas,GPU_X,GPU_Y,GPU_W,GPU_H,hCanvas,GPU_X_2,GPU_Y_2,GPU_COMMAND_AUX);
		y = GPU_Y;
		h = GPU_H;
		x = GPU_X;
		w = GPU_W;
		GPU_STATUS=GPU_IDLE;
		break;
	case GPU_BLT_IN:
		vmb_debug(VMB_DEBUG_PROGRESS,"BitBlockTransfer to GPU");
		LeaveCriticalSection (&bitmap_section);
		PostMessage(hMainWnd,WM_VMB_OTHER,0,0);
		mem_update_i(2,offset,size);
		return;
	case GPU_BLT_OUT:
		vmb_debug(VMB_DEBUG_PROGRESS,"BitBlockTransfer from GPU");
		LeaveCriticalSection (&bitmap_section);
	    PostMessage(hMainWnd,WM_VMB_OTHER+1,0,0);
		mem_update_i(2,offset,size);
        return;
	case GPU_DIB:
		vmb_debug(VMB_DEBUG_PROGRESS,"DIB Transfer to GPU");
		LeaveCriticalSection (&bitmap_section);
		PostMessage(hMainWnd,WM_VMB_OTHER+2,0,0);
		mem_update_i(2,offset,size);
		return;
  }

  if (x<0) x=0;
  if (y<0) y=0;
  if (w>framewidth) w=framewidth;
  if (h>frameheight) h=frameheight;
  { RECT rect;
    rect.top=(int)(y*zoom);
    rect.left=(int)(x*zoom);
    rect.bottom=(int)((y+h+1)*zoom);
    rect.right=(int)((x+w+1)*zoom);
    InvalidateRect(hMainWnd,&rect,FALSE);
  }
  LeaveCriticalSection (&bitmap_section);  
  if (w>0 && h>0)
    mem_update_i(0,(y*framewidth+x)*4,(h*framewidth+w)*4);
  mem_update_i(2,offset,size);
}


static void init_gpu_canvas(void)
{ 
  TEXTMETRIC tm;
  LOGFONT lgpu_font={0};
  if (hgpu_font!=0) 
	  DeleteObject(hgpu_font);
  lgpu_font.lfHeight=fontheight;
  lgpu_font.lfWidth=fontwidth;
  lgpu_font.lfCharSet = ANSI_CHARSET;
  lgpu_font.lfPitchAndFamily=FF_DONTCARE|FIXED_PITCH;
  hgpu_font = CreateFontIndirect(&lgpu_font);  
  SelectObject(hCanvas,hgpu_font);
  GetTextMetrics(hCanvas,&tm);
  gpu_char_width= tm.tmAveCharWidth;
  gpu_char_ascent= tm.tmAscent;
  gpu_char_descent=tm.tmDescent;
  gpu_char_height=tm.tmHeight;
  gpu_text_height=gpu_char_height+tm.tmExternalLeading;

  SelectObject(hCanvas, GetStockObject(WHITE_BRUSH));
  SelectObject(hCanvas, GetStockObject(WHITE_PEN));
}



void init_gpu(void)
{	vmb_gpu.id = 2;
	vmb_gpu.get_payload=gpu_get_payload;
	vmb_gpu.put_payload=gpu_put_payload;
	vmb_gpu.poweron = gpu_poweron;
	vmb_gpu.reset= gpu_poweron;
	vmb_gpu.terminate = NULL;
    memset(gpu_mem,0,sizeof(gpu_mem));
    inspector[2].name = "GPU";
    inspector[2].size=GPU_MEM_SIZE;
	inspector[2].get_mem=read_gpu;
	inspector[2].load=gpu_get_payload;
	inspector[2].store=gpu_put_payload;
	inspector[2].format=hex_format;
	inspector[2].chunk=byte_chunk;
	inspector[2].de_offset=-1;
    inspector[2].address=0;
	inspector[2].sb_rng=8;
    inspector[2].num_regs= GPU_REGS;
    inspector[2].regs =gpu_regs;
}



/*
 *     The Common Stuff
 *
 */


void connect_all(void)
{ 	vmb_connect(&vmb,host,port);
	vmb_register(&vmb,HI32(vmb_address),LO32(vmb_address),vmb_size,0,0,defined,major_version,minor_version);

	vmb_connect(&vmb_mouse,host,port);
	vmb_register(&vmb_mouse,HI32(vmb_mouse_address), LO32(vmb_mouse_address),16,0,0,"mouse",major_version,minor_version);

	vmb_connect(&vmb_gpu,host,port);
	vmb_register(&vmb_gpu,HI32(vmb_gpu_address), LO32(vmb_gpu_address),GPU_MEM_SIZE,0,0,"GPU",major_version,minor_version);
}

void disconnect_all(void)
{  vmb_disconnect(&vmb);
   vmb_disconnect(&vmb_mouse);
   vmb_disconnect(&vmb_gpu);
}


/*
 *     The Windows Stuff
 *
 */

INT_PTR CALLBACK   
SettingsDialogProc( HWND hDlg, UINT message, WPARAM wparam, LPARAM lparam )
{
  switch ( message )
  { case WM_INITDIALOG:
      uint64_to_hex(vmb_address,tmp_option);
      SetDlgItemText(hDlg,IDC_ADDRESS,tmp_option);
	  SetDlgItemInt(hDlg,IDC_FRAMEHEIGHT,frameheight,FALSE);
	  SetDlgItemInt(hDlg,IDC_FRAMEWIDTH,framewidth,FALSE);
	  SetDlgItemInt(hDlg,IDC_HEIGHT,height,FALSE);
	  SetDlgItemInt(hDlg,IDC_WIDTH,width,FALSE);

	  uint64_to_hex(vmb_mouse_address,tmp_option);
      SetDlgItemText(hDlg,IDC_ADDRESS_MOUSE,tmp_option);
	  SetDlgItemInt(hDlg,IDC_INTERRUPT,interrupt_no,FALSE);
	  CheckDlgButton(hDlg,IDC_MOUSEMOVE,move_interrupt?BST_CHECKED:BST_UNCHECKED);

      uint64_to_hex(vmb_gpu_address,tmp_option);
      SetDlgItemText(hDlg,IDC_ADDRESS_GPU,tmp_option);
	  SetDlgItemInt(hDlg,IDC_GPU_INTERRUPT,gpu_interrupt,FALSE);
	  
	  return TRUE;
   case WM_SYSCOMMAND:
      if( wparam == SC_CLOSE ) 
      { EndDialog(hDlg, TRUE);
        return TRUE;
      }
      break;
    case WM_COMMAND:
      if( wparam == IDOK )
      { int newframewidth, newframeheight;
	    int newwidth, newheight;
	    GetDlgItemText(hDlg,IDC_ADDRESS,tmp_option,MAXTMPOPTION);
        vmb_address = strtouint64(tmp_option); 
		inspector[0].address=vmb_address;
		newframeheight=GetDlgItemInt(hDlg,IDC_FRAMEHEIGHT,FALSE,FALSE);
		newframewidth=GetDlgItemInt(hDlg,IDC_FRAMEWIDTH,FALSE,FALSE);
		newheight=GetDlgItemInt(hDlg,IDC_HEIGHT,FALSE,FALSE);
		newwidth=GetDlgItemInt(hDlg,IDC_WIDTH,FALSE,FALSE);

		if(newframeheight!=frameheight || newframewidth!=framewidth)
		{   frameheight=newframeheight;
			framewidth=newframewidth;
			init_canvas();
			init_gpu_memory();
		}
		if (newwidth!=width || newheight!=height)
		{ RECT rc;
		  width=newwidth;
		  height=newheight;
		  init_gpu_memory();
		  rc.top = 0;
		  rc.left = 0;
		  rc.right = (int)(width*zoom);
		  rc.bottom = (int)(height*zoom);
		  AdjustWindowRect(&rc,WS_VRAM,FALSE);
          SetWindowPos(hMainWnd,HWND_TOP,xpos,ypos,rc.right-rc.left,rc.bottom-rc.top,SWP_SHOWWINDOW);
		}
	    GetDlgItemText(hDlg,IDC_ADDRESS_MOUSE,tmp_option,MAXTMPOPTION);
        vmb_mouse_address = strtouint64(tmp_option); 
		interrupt_no=GetDlgItemInt(hDlg,IDC_INTERRUPT,FALSE,FALSE);
		move_interrupt = (IsDlgButtonChecked(hDlg,IDC_MOUSEMOVE)==BST_CHECKED);

	    GetDlgItemText(hDlg,IDC_ADDRESS_GPU,tmp_option,MAXTMPOPTION);
        vmb_gpu_address = strtouint64(tmp_option); 
		gpu_interrupt=GetDlgItemInt(hDlg,IDC_GPU_INTERRUPT,FALSE,FALSE);
     }
      if (wparam == IDOK || wparam == IDCANCEL)
      { EndDialog(hDlg, TRUE);
        return TRUE;
      }
      break;
  }
  return FALSE;
}

void set_zoom(double z)
{ RECT rc;
  zoom = z;
  rc.top = 0;
  rc.left = 0;
  rc.right = (int)(width*zoom);
  rc.bottom = (int)(height*zoom);
  AdjustWindowRect(&rc,WS_VRAM,FALSE);
  SetWindowPos(hMainWnd,HWND_TOP,xpos,ypos,rc.right-rc.left,rc.bottom-rc.top,SWP_SHOWWINDOW);
}


LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{ switch (message) 
  {    case WM_NCHITTEST:
		return (DefWindowProc(hWnd, message, wParam, lParam));                 
        /* return HTCAPTION; */
    case WM_VMB_ON: /* On */
		return 0;
	case WM_VMB_OFF: /* Off */
		return 0;
	case WM_VMB_CONNECT: /* Connected */
		if (ModifyMenu(hMenu,ID_CONNECT, MF_BYCOMMAND|MF_STRING,ID_CONNECT,"Disconnect"))
			DrawMenuBar(hMainWnd);
 		return 0;
	case WM_VMB_DISCONNECT: /* Disconnected */
		if (ModifyMenu(hMenu,ID_CONNECT, MF_BYCOMMAND|MF_STRING,ID_CONNECT,"Connect..."))
			DrawMenuBar(hMainWnd);
		return 0;
	case WM_VMB_OTHER: /* called for GPU_BLT_IN */
	    { int x,y,w,h,i,k,n,size,d;
		  static unsigned char sector_buffer[256*8];
          data_address da ={sector_buffer,0,0,256*8,STATUS_INVALID};
		  RECT rect;

		  vmb_debug(VMB_DEBUG_INFO,"Incomming Bit Block Transfer");
		  x = GPU_X;
		  y = GPU_Y;
		  w = GPU_W; i = 0;  /* below we have 0<=i<w and 0<=k<h */
		  h = GPU_H; k = 0;
		  size = w*h*4;
		  da.address_hi = chartoint(gpu_mem+0x10);
          da.address_lo = chartoint(gpu_mem+0x14);
		  d = 0x100*8;
          while (size>0)
		  { if (d>size) d = size;
			da.size = d;
		    vmb_load(&vmb_gpu, &da);
		    vmb_wait_for_valid(&vmb_gpu, &da);
			EnterCriticalSection (&bitmap_section);
            for (n=0;n<da.size;n=n+4)
			{ put_one_pixel(x+i,y+k,da.data+n);
#if 0
			  /* not shure what these two lines should accomplish */
			  if (!(*(da.data+n)|*(da.data+n+1)|*(da.data+n+2)|*(da.data+n+3)))
				  i= i+k&0x80000000;
#endif
			  i++;
			  if (i >= w)
			  { i=0; k++;
			  }
			}
            LeaveCriticalSection (&bitmap_section);
			size=size-d;
			if (da.address_lo+d<da.address_lo) da.address_hi++;
			da.address_lo += d;
		  }
		  vmb_raise_interrupt(&vmb_gpu, gpu_interrupt);
		  GPU_STATUS=GPU_IDLE;
		  rect.top = (int)(y*zoom);
		  rect.bottom = (int)((y+h)*zoom);
		  rect.left = (int)(x*zoom);
		  rect.right = (int)((x+w)*zoom);  
		  EnterCriticalSection (&bitmap_section);
          InvalidateRect(hMainWnd,&rect,FALSE);
          LeaveCriticalSection (&bitmap_section);
		  if (w>0 && h>0)
            mem_update_i(0,(y*framewidth+x)*4,(h*framewidth+w)*4);
		  return 0;
		}
	case WM_VMB_OTHER+1: /* called for GPU_BLT_OUT */
			    { int x,y,w,h,i,k,n,size,d;
		  static unsigned char sector_buffer[256*8];
          data_address da ={sector_buffer,0,0,256*8,STATUS_INVALID};

		  vmb_debug(VMB_DEBUG_INFO,"Outgoing Bit Block Transfer");
		  x = GPU_X;
		  y = GPU_Y;
		  w = GPU_W; i = 0;  /* below we have 0<=i<w and 0<=k<h */
		  h = GPU_H; k = 0;
		  size = w*h*4;
		  da.address_hi = chartoint(gpu_mem+0x10);
          da.address_lo = chartoint(gpu_mem+0x14);
		  d = 0x100*8;
          while (size>0)
		  { if (d>size) d = size;
			da.size = d;
			EnterCriticalSection (&bitmap_section);
            for (n=0;n<da.size;n=n+4)
			{ get_one_pixel(x+i,y+k,da.data+n);
			  i++;
			  if (i >= w)
			  { i=0; k++;
			  }
			}
            LeaveCriticalSection (&bitmap_section);
		    vmb_store(&vmb_gpu, &da);
			size=size-d;
			if (da.address_lo+d<da.address_lo) da.address_hi++;
			da.address_lo += d;
		  }
		  vmb_raise_interrupt(&vmb_gpu, gpu_interrupt);
		  GPU_STATUS=GPU_IDLE;
		  return 0;
		}

	case WM_VMB_OTHER+2: /* called for GPU_DIB */
	    { int x,y,w,h,d;
		  unsigned char *buf=NULL;
          data_address da ={NULL,0,0,256*8,STATUS_INVALID};
		  BITMAPFILEHEADER   *fh=NULL;
		  BITMAPINFOHEADER   *pbmih=NULL;
	      BITMAPINFO *pbmi=NULL;
	      unsigned char *lpbits=NULL;
	      int size;
	      HBITMAP hb;
	      unsigned char *signature;
		  RECT rect;
#define BLKSIZE (256*8)
		  vmb_debug(VMB_DEBUG_INFO,"Incomming DIB Transfer");
          d=BLKSIZE;
	      buf = (unsigned char *)malloc(d);
	      if (buf == NULL) return 0;
		  da.address_hi = chartoint(gpu_mem+0x10);
          da.address_lo = chartoint(gpu_mem+0x14);
		  da.size = d;
		  da.data = buf;
		  da.status = STATUS_INVALID;
		  vmb_load(&vmb_gpu, &da);
		  vmb_wait_for_valid(&vmb_gpu, &da);
		  if (da.address_lo+d<da.address_lo) da.address_hi++;
			da.address_lo += d;

          fh = (BITMAPFILEHEADER *)(buf + 0);
	      signature = (unsigned char *)&fh->bfType;
	      if (signature[0] != 'B' || signature[1]!= 'M') 
		  { free(buf);
		  	vmb_debug(VMB_DEBUG_WARN,"No DIB detected");
		    return 0; 
		  }
	      /* Bitmap Info Header */
	      pbmi = (BITMAPINFO *)(buf + sizeof(BITMAPFILEHEADER));
	      pbmih = &(pbmi->bmiHeader);
		  w = pbmi->bmiHeader.biWidth;
		  h = pbmi->bmiHeader.biHeight; 
	      size = pbmi->bmiHeader.biSizeImage;
	      if (pbmi->bmiHeader.biCompression == BI_RGB)
	        size = ((w*pbmi->bmiHeader.biBitCount +31)/ 32)*4*h;
	      size = size + fh->bfOffBits;
	      if (size > d)
	      { unsigned char * tmp=(unsigned char *)realloc(buf, size);
		    if (tmp == NULL) { 
				free(buf); 
			    vmb_debug(VMB_DEBUG_ERROR,"DIB too big. Out of memory");
				return 0;
			}
            buf=tmp;
		    fh = (BITMAPFILEHEADER *)(buf + 0);
		    pbmi = (BITMAPINFO *)(buf + sizeof(BITMAPFILEHEADER));
		    pbmih = &(pbmi->bmiHeader);
			da.data=buf+d;
			size=size-d;

            while (size>0)
		    { if (d>size) d = size;
			  da.size = d;
		      vmb_load(&vmb_gpu, &da);
		      vmb_wait_for_valid(&vmb_gpu, &da);
			  size=size-d;
			  da.data=da.data+d;
			  if (da.address_lo+d<da.address_lo) da.address_hi++;
			  da.address_lo += d;
              da.status = STATUS_INVALID;
		    }
		  }
	      lpbits = buf + fh->bfOffBits;
		  x = GPU_X;
		  y = GPU_Y;
		  EnterCriticalSection (&bitmap_section);
		  hb= CreateDIBitmap(hCanvas, pbmih, CBM_INIT, lpbits, pbmi, DIB_RGB_COLORS);
 	      StretchDIBits(hCanvas, x,y, w,h,0, 0, w, h, lpbits, pbmi, DIB_RGB_COLORS, SRCCOPY);
          LeaveCriticalSection (&bitmap_section);
		  DeleteObject(hb);
		  vmb_raise_interrupt(&vmb_gpu, gpu_interrupt);
		  GPU_STATUS=GPU_IDLE;
		  rect.top = (int)(y*zoom);
		  rect.bottom = (int)((y+h)*zoom);
		  rect.left = (int)(x*zoom);
		  rect.right = (int)((x+w)*zoom);  
		  EnterCriticalSection (&bitmap_section);
          InvalidateRect(hMainWnd,&rect,FALSE);
          LeaveCriticalSection (&bitmap_section);
		  if (w>0 && h>0)
            mem_update_i(0,(y*framewidth+x)*4,(h*framewidth+w)*4);
		  /* free(buf); */
		  return 0;
		}


	case WM_CREATE:
		return (DefWindowProc(hWnd, message, wParam, lParam));
    case WM_COMMAND:
      if (HIWORD(wParam)==0) /* Menu */
        switch(LOWORD(wParam))
	    { case ID_CONNECT:
	      if (!vmb.connected)
		  { if (DialogBox(hInst,MAKEINTRESOURCE(IDD_CONNECT),hWnd,ConnectDialogProc))
	  	    {  connect_all();
		       SendMessage(hMainWnd,WM_VMB_CONNECT,0,0); /* the connect button */
		    }
	      }
	      else
		    disconnect_all();
	      return 0;
		case ID_ZOOM_50:
			set_zoom(0.5);
		    CheckMenuItem(hMenu,ID_ZOOM_50,MF_BYCOMMAND|MF_CHECKED);
		    CheckMenuItem(hMenu,ID_ZOOM_100,MF_BYCOMMAND|MF_UNCHECKED);
		    CheckMenuItem(hMenu,ID_ZOOM_200,MF_BYCOMMAND|MF_UNCHECKED);
		    CheckMenuItem(hMenu,ID_ZOOM_400,MF_BYCOMMAND|MF_UNCHECKED);
		    CheckMenuItem(hMenu,ID_ZOOM_800,MF_BYCOMMAND|MF_UNCHECKED);
           return 0;
		case ID_ZOOM_100:
			set_zoom(1.0);
		    CheckMenuItem(hMenu,ID_ZOOM_50,MF_BYCOMMAND|MF_UNCHECKED);
		    CheckMenuItem(hMenu,ID_ZOOM_100,MF_BYCOMMAND|MF_CHECKED);
		    CheckMenuItem(hMenu,ID_ZOOM_200,MF_BYCOMMAND|MF_UNCHECKED);
		    CheckMenuItem(hMenu,ID_ZOOM_400,MF_BYCOMMAND|MF_UNCHECKED);
		    CheckMenuItem(hMenu,ID_ZOOM_800,MF_BYCOMMAND|MF_UNCHECKED);
           return 0;
		case ID_ZOOM_200:
			set_zoom(2.0);
		    CheckMenuItem(hMenu,ID_ZOOM_50,MF_BYCOMMAND|MF_UNCHECKED);
		    CheckMenuItem(hMenu,ID_ZOOM_100,MF_BYCOMMAND|MF_UNCHECKED);
		    CheckMenuItem(hMenu,ID_ZOOM_200,MF_BYCOMMAND|MF_CHECKED);
		    CheckMenuItem(hMenu,ID_ZOOM_400,MF_BYCOMMAND|MF_UNCHECKED);
		    CheckMenuItem(hMenu,ID_ZOOM_800,MF_BYCOMMAND|MF_UNCHECKED);
           return 0;
		case ID_ZOOM_400:
			set_zoom(4.0);
		    CheckMenuItem(hMenu,ID_ZOOM_50,MF_BYCOMMAND|MF_UNCHECKED);
		    CheckMenuItem(hMenu,ID_ZOOM_100,MF_BYCOMMAND|MF_UNCHECKED);
		    CheckMenuItem(hMenu,ID_ZOOM_200,MF_BYCOMMAND|MF_UNCHECKED);
		    CheckMenuItem(hMenu,ID_ZOOM_400,MF_BYCOMMAND|MF_CHECKED);
		    CheckMenuItem(hMenu,ID_ZOOM_800,MF_BYCOMMAND|MF_UNCHECKED);
           return 0;
		case ID_ZOOM_800:
			set_zoom(8.0);
		    CheckMenuItem(hMenu,ID_ZOOM_50,MF_BYCOMMAND|MF_UNCHECKED);
		    CheckMenuItem(hMenu,ID_ZOOM_100,MF_BYCOMMAND|MF_UNCHECKED);
		    CheckMenuItem(hMenu,ID_ZOOM_200,MF_BYCOMMAND|MF_UNCHECKED);
		    CheckMenuItem(hMenu,ID_ZOOM_400,MF_BYCOMMAND|MF_UNCHECKED);
		    CheckMenuItem(hMenu,ID_ZOOM_800,MF_BYCOMMAND|MF_CHECKED);
           return 0;
	    }
      break;

	case WM_PAINT:
	{ PAINTSTRUCT ps;
	  BOOL rc;
	  DWORD dw;
	  int src_left, src_top,src_right,src_bottom;
	  EnterCriticalSection (&bitmap_section);
      BeginPaint(hWnd, &ps); 
	  src_left = (int)floor(ps.rcPaint.left/zoom);
	  src_right = (int)ceil(ps.rcPaint.right/zoom);
	  src_top = (int)floor(ps.rcPaint.top/zoom);
	  src_bottom = (int)ceil(ps.rcPaint.bottom/zoom);
	  if (zoom<1.0)
	    SetStretchBltMode(ps.hdc,HALFTONE);
	  else
	    SetStretchBltMode(ps.hdc,COLORONCOLOR);
	  rc = StretchBlt(ps.hdc, 
		          (int)(src_left*zoom),(int)(src_top*zoom),
				  (int)((src_right-src_left)*zoom), (int)((src_bottom-src_top)*zoom),
                  hCanvas, 
				  src_left,src_top, 
				  src_right-src_left, src_bottom-src_top,
				  SRCCOPY);
      if (!rc)
	    dw = GetLastError();
	  EndPaint(hWnd, &ps); 
	  LeaveCriticalSection (&bitmap_section);
    }
    return 0;

	case WM_MOUSEMOVE:
		mouse_set_position(wParam, lParam,VMB_MOUSEMOVE);
		return (DefWindowProc(hWnd, message, wParam, lParam));                 
	case WM_LBUTTONDOWN:
		mouse_set_position(wParam, lParam,VMB_LBUTTONDOWN);
		return (DefWindowProc(hWnd, message, wParam, lParam));                 
	case WM_LBUTTONUP: 
		mouse_set_position(wParam, lParam,VMB_LBUTTONUP);
		return (DefWindowProc(hWnd, message, wParam, lParam));                 
	case WM_LBUTTONDBLCLK:      
		mouse_set_position(wParam, lParam,VMB_LBUTTONDBLCLK);
		return (DefWindowProc(hWnd, message, wParam, lParam));                 
	case WM_RBUTTONDOWN:          
		mouse_set_position(wParam, lParam,VMB_RBUTTONDOWN);
		return (DefWindowProc(hWnd, message, wParam, lParam));                 
	case WM_RBUTTONUP:             
		mouse_set_position(wParam, lParam,VMB_RBUTTONUP);
		return (DefWindowProc(hWnd, message, wParam, lParam));                 
	case WM_RBUTTONDBLCLK:          
		mouse_set_position(wParam, lParam,VMB_RBUTTONDBLCLK);
		return (DefWindowProc(hWnd, message, wParam, lParam));                 
	case WM_MBUTTONDOWN:        
		mouse_set_position(wParam, lParam,VMB_MBUTTONDOWN);
		return (DefWindowProc(hWnd, message, wParam, lParam));                 
	case WM_MBUTTONUP:             
		mouse_set_position(wParam, lParam,VMB_MBUTTONUP);
		return (DefWindowProc(hWnd, message, wParam, lParam));                 
	case WM_MBUTTONDBLCLK:     
		mouse_set_position(wParam, lParam,VMB_MBUTTONDBLCLK);
		return (DefWindowProc(hWnd, message, wParam, lParam));                 
	
	case WM_DESTROY:
		SelectObject(hCanvas,GetStockObject(SYSTEM_FONT));
		DeleteObject(hgpu_font);
		if (hBmp!=NULL) DeleteObject(hBmp);
		if (hCanvas != NULL) 
		{ DeleteDC(hCanvas); 
		  hCanvas=NULL;
		  DeleteCriticalSection(&bitmap_section);
		}
	   break;
  }
 return (OptWndProc(hWnd, message, wParam, lParam));
}


HINSTANCE hInst;
HWND hMainWnd;
HBITMAP hBmp=NULL;
HMENU hMenu;
HBITMAP hon,hoff,hconnect;
device_info vmb = {0};


BOOL InitInstance(HINSTANCE hInstance)
{
  WNDCLASSEX wcex;
  BITMAP bm;
  int r;

#define MAX_LOADSTRING 100		
  static TCHAR szClassName[MAX_LOADSTRING];
  hInst = hInstance; 

  r = LoadString(hInstance, IDS_CLASS, szClassName, MAX_LOADSTRING);
  if (r==0)
  { r = GetLastError();
    vmb_debugi(VMB_DEBUG_FATAL,"Unable to load class name (%X)",r);
	vmb_fatal_error(__LINE__,"Unable to load class name");
  }
  ZeroMemory(&wcex, sizeof(wcex));
	wcex.cbSize = sizeof(WNDCLASSEX); 
	wcex.style			= CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS ;
	wcex.lpfnWndProc	= (WNDPROC)WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON));
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= NULL; /*(HBRUSH)(COLOR_WINDOW+1);*/
	wcex.lpszMenuName	= NULL;
	wcex.lpszClassName = szClassName;
	wcex.hIconSm		= LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON));
/*	wcex.hIconSm		= LoadIcon(NULL, IDI_APPLICATION);
*/
	if (!RegisterClassEx(&wcex)) return FALSE;

	if (hBmp)
		GetObject(hBmp, sizeof(bm), &bm);
	else
		bm.bmWidth=bm.bmHeight=CW_USEDEFAULT;

    hMainWnd = CreateWindow(szClassName, title ,WS_VRAM,
                            CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
	                        NULL, NULL, hInstance, NULL);
   return TRUE;
}




int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
	HACCEL hAccelTable;
    MSG msg;
    vmb_message_hook = win32_message;
	vmb_debug_hook = win32_log;
	vmb_error_init_hook = win32_error_init;

	hMenu = LoadMenu(hInstance,MAKEINTRESOURCE(IDR_MENU));
	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDR_ACCELERATOR));
    InitCommonControls();
	if (!InitInstance (hInstance)) return FALSE;
	init_layout(0);
	win32_param_init();
    init_screen(&vmb);
    init_mouse();
    init_gpu();
	if (minimized)CloseWindow(hMainWnd); 
	ShowWindow(hMainWnd,SW_SHOWDEFAULT);
	UpdateWindow(hMainWnd);
	vmb_begin();
	connect_all();
    SendMessage(hMainWnd,WM_VMB_CONNECT,0,0); /* the connect button */
	if (vmb_verbose_flag) vmb_debug_mask=0; 
	CheckMenuItem(hMenu,ID_DEBUG,MF_BYCOMMAND|(vmb_debug_flag?MF_CHECKED:MF_UNCHECKED));
	CheckMenuItem(hMenu,ID_VERBOSE,MF_BYCOMMAND|(vmb_debug_mask==0?MF_CHECKED:MF_UNCHECKED));

	while (GetMessage(&msg, NULL, 0, 0)) 
	  if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg) &&
		  !do_subwindow_msg(&msg) 
		  ) 
	  { TranslateMessage(&msg);
	    DispatchMessage(&msg);
	  }

	disconnect_all();
	vmb_end();
	return (int)msg.wParam;
}

