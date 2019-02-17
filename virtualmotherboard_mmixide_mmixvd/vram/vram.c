/*
    Copyright 2005 Martin Ruckert ruckertm@acm.org
    adapded from Andrew Pochinsky (avp@mit.edu)
    virtual frame buffer device for mmix.

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

#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
/* include <assert.h> */

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>



#include "bus-arith.h"
#include "option.h"
#include "param.h"
#include "vmb.h"

device_info vmb = {0};

int major_version=2, minor_version=0;
char version[]="$Revision: 1.9 $ $Date: 2016-02-16 09:49:16 $";

char howto[] =
"\n"
"The program will contact the motherboard at [host:]port\n"
"and register itself with the given start address.\n"
"Then, the program will answer read and write requests from the bus.\n"
"and display the data as colored pixels\n"
"\n"
;



#define window_size_x 640
#define window_size_y 480
#define window_pos_x 100
#define window_pos_y 100


#define C2bits(c,sp) (((((c) &0xff) *sp.cch_scale) >>8) *sp.cch_factor) 
#define RGB2Pixel(r,g,b) (C2bits(r,vga.red) |C2bits(g,vga.green) |C2bits(b,vga.blue) )
#define do_pixel(x,y,r,g,b) XPutPixel(vga.image,x,y,RGB2Pixel(r,g,b) ) 


/* taken from mmix-sim.ch by  Andrew Pochinsky (avp@mit.edu) */

typedef struct
{                                                                    /* 175 */
  unsigned long cch_scale, cch_factor;
} ColorChannel;

typedef struct
{                                                                    /* 178 */
  Display *display;
  GC gc;
  Window win;
  XImage *image;
  ColorChannel red, green, blue;
  XExposeEvent expose;
  int argc;
  char **argv;
} VGA;

typedef struct
{                                                                    /* 207 */
  int x, y;
  int r, g, b;
} PutPixel;

enum
{                                                                    /* 212 */
  vgaHalt,                                                           /* 203 */
  vgaPutPixel,                                                       /* 205 */
  vgaNop,                                                            /* 210 */
};

typedef union
{
  PutPixel pp;                                                       /* 206 */
} Cmd;

typedef struct
{
  pthread_mutex_t lock;
  pthread_cond_t out_c;
  volatile int status;
  int cmd;
  Cmd data;
} CommandPort;


VGA vga; 


                                                            /* 179 */
CommandPort cp;   

ColorChannel mask2channel (unsigned long mask)
{
  unsigned long f;
  ColorChannel ch;

  if (mask == 0)
    vmb_fatal_error (__LINE__, "mask2channel(0)");
  for (f = 1; !(mask & 1); f <<= 1)
    mask >>= 1;
  ch.cch_factor = f;
  ch.cch_scale = mask + 1;
  while (mask & 1)
    mask >>= 1;
  if (mask != 0)
    vmb_fatal_error (__LINE__, "mask2channel(): scattered bits in the mask");
  return ch;
}


void init_monitor (void)
{
  int screen_num;                                                    /* 182 */
  Window root_win;
  XVisualInfo vi_temp;                                               /* 184 */
  XVisualInfo *visual_info;
  int visual_count;
  int best_depth;
  int best_match;
  Visual *visual;
  int i;
  Colormap colormap;                                                 /* 186 */
  XSetWindowAttributes attrib;                                       /* 188 */
  XClassHint *class_hints;                                           /* 190 */
  XSizeHints *size_hints;
  XWMHints *wm_hints;
  XTextProperty windowName;
  XGCValues gc_values;                                               /* 196 */

  if (!XInitThreads ())                                              /* 181 */
    vmb_fatal_error (__LINE__, "Can't enable thread support for X11");
  vga.display = XOpenDisplay (NULL);
  if (vga.display == NULL)
    vmb_fatal_error (__LINE__, "X connection failed");
  screen_num = DefaultScreen (vga.display);
  root_win = RootWindow (vga.display, screen_num);
  vi_temp.screen = screen_num;                                       /* 183 */
  vi_temp.class = TrueColor;
  visual_info = XGetVisualInfo (vga.display, VisualClassMask | VisualScreenMask,
                                &vi_temp, &visual_count);
  if ((visual_info == NULL) || visual_count == 0)
    vmb_fatal_error (__LINE__, "No TrueColor visual found");
  best_match = 0;
  best_depth = visual_info[0].depth;
  visual = visual_info[0].visual;
  for (i = 1; i < visual_count; i++) {
    if (visual_info[i].depth > best_depth) {
      best_match = i;
      best_depth = visual_info[i].depth;
      visual = visual_info[i].visual;
    }
  }
  vga.red = mask2channel (visual_info[best_match].red_mask);         /* 180 */
  vga.green = mask2channel (visual_info[best_match].green_mask);
  vga.blue = mask2channel (visual_info[best_match].blue_mask);
  vga.image = XCreateImage (vga.display, visual, best_depth, ZPixmap,   /* 194 */
                            0, NULL, window_size_x, window_size_y, 32, 0);
  if (!vga.image)
    vmb_fatal_error (__LINE__, "image allocation error");
  vga.image->data = malloc (vga.image->height * vga.image->bytes_per_line);
  if (!vga.image->data) {
    vmb_fatal_error (__LINE__, "image data allocation error");
  }
  else {
    int x, y;
    unsigned long p = RGB2Pixel (0, 0, 0);

    for (y = 0; y < window_size_y; y++)
      for (x = 0; x < window_size_x; x++)
        XPutPixel (vga.image, x, y, p);
  }
  colormap = XCreateColormap (vga.display, root_win, visual, AllocNone);        /* 185 */
  attrib.colormap = colormap;                                        /* 187 */
  attrib.event_mask = ExposureMask;
  attrib.background_pixel = attrib.border_pixel = RGB2Pixel (0, 0, 0);
  vga.win = XCreateWindow (vga.display, root_win,
                           window_pos_x, window_pos_y,
                           window_size_x, window_size_y, 0, best_depth,
                           InputOutput, visual,
                           CWEventMask | CWColormap |
                           CWBackPixel | CWBorderPixel, &attrib);
  if (!(size_hints = XAllocSizeHints ()) ||                          /* 189 */
      !(wm_hints = XAllocWMHints ()) || !(class_hints = XAllocClassHint ()))
    vmb_fatal_error (__LINE__, "Hint allocation failed");
  size_hints->flags = PMinSize | PMaxSize;                           /* 191 */
  size_hints->min_width = window_size_x;
  size_hints->min_height = window_size_y;
  size_hints->max_width = window_size_x;
  size_hints->max_height = window_size_y;
  wm_hints->flags = StateHint | InputHint;                           /* 192 */
  wm_hints->input = True;
  wm_hints->initial_state = NormalState;
  class_hints->res_name = "vram";                                    /* 193 */
  class_hints->res_class = "VRAM";
  if (!XStringListToTextProperty (&class_hints->res_name, 1, &windowName))
    vmb_fatal_error (__LINE__, "property allocation failed");
  XSetWMProperties (vga.display, vga.win, &windowName, NULL,
                    vga.argv, vga.argc, size_hints, wm_hints, class_hints);
  vga.gc = XCreateGC (vga.display, vga.win, 0, &gc_values);          /* 195 */
  XMapWindow (vga.display, vga.win);
  XFlush (vga.display);
  vga.expose.type = Expose;                                          /* 197 */
  vga.expose.display = vga.display;
  vga.expose.window = vga.win;
  vga.expose.count = 0;
  pthread_mutex_lock (&cp.lock);
  if (cp.status != 1)
    vmb_fatal_error (__LINE__, "cp.status should be one now!");
  cp.status = 0;
  pthread_cond_signal (&cp.out_c);
  pthread_mutex_unlock (&cp.lock);
}

int get_cmd (Cmd *data)
{
  int cmd;

  pthread_mutex_lock (&cp.lock);
  if (cp.status == 0) {
    cmd = vgaNop;
  }
  else {
    cmd = cp.cmd;
    *data = cp.data;
    cp.status = 0;
    pthread_cond_signal (&cp.out_c);
  }
  pthread_mutex_unlock (&cp.lock);
  return cmd;
}


void * server (void * _ignore)
{
  int done;

  init_monitor ();
  for (done = 0; !done;) {
    XEvent event;

    XNextEvent (vga.display, &event);
    switch (event.type) {
    case Expose:{                                                   /* 202 */
        Cmd data;
        int cmd;

        switch ((cmd = get_cmd (&data))) {
        case vgaHalt:                                               /* 204 */
          done = 1;
          break;
        case vgaPutPixel:                                           /* 209 */
          do_pixel (data.pp.x, data.pp.y, data.pp.r, data.pp.g, data.pp.b);
          break;
        case vgaNop:                                                /* 211 */
          break;
        default:{
            char buffer[128];

            sprintf (buffer, "server: Unknown command %d\n", cmd);
            vmb_fatal_error (__LINE__, buffer);
          }
        }
        XPutImage (vga.display, vga.win, vga.gc, vga.image,
                   event.xexpose.x, event.xexpose.y,
                   event.xexpose.x, event.xexpose.y,
                   event.xexpose.width, event.xexpose.height);
      } break;
    default:
      break;
    }
  }
  XFreeGC (vga.display, vga.gc);                                     /* 199 */
  XUnmapWindow (vga.display, vga.win);
  XDestroyWindow (vga.display, vga.win);
  XDestroyImage (vga.image);
  XCloseDisplay (vga.display);
  return 0;
}


void send_cmd (int cmd, Cmd *data)
{
  pthread_mutex_lock (&cp.lock);
  while (cp.status != 0)
    pthread_cond_wait (&cp.out_c, &cp.lock);
  cp.status = 1;
  cp.cmd = cmd;
  cp.data = *data;
  pthread_mutex_unlock (&cp.lock);
  XSendEvent (vga.display, vga.win, False, 0, (XEvent *) & vga.expose);
  XFlush (vga.display);
}


void vram_init (int argc, char *argv[])
{
  pthread_attr_t attr;
  pthread_t thread;

  pthread_mutex_init (&cp.lock, NULL);
  pthread_cond_init (&cp.out_c, NULL);
  cp.status = 1;
  cp.cmd = vgaNop;
  pthread_attr_init (&attr);
  pthread_attr_setdetachstate (&attr, PTHREAD_CREATE_DETACHED);
  vga.argc = argc;
  vga.argv = argv;
  if (pthread_create (&thread, &attr, server, NULL) != 0) {
    perror ("server");
    exit (1);
  }
}


void vram_fini (void)
{
  Cmd cmd;

  vga.expose.x = 0;
  vga.expose.y = 0;
  vga.expose.width = 1;
  vga.expose.height = 1;
  send_cmd (vgaHalt, &cmd);
}

void vram_blank(void)
{
   Cmd cmd;
   int x, y;
   unsigned long p = RGB2Pixel (0, 0, 0);
   for (y = 0; y < window_size_y; y++)
     for (x = 0; x < window_size_x; x++)
       XPutPixel (vga.image, x, y, p);
   vga.expose.x = 0;
   vga.expose.y = 0;
   vga.expose.width = window_size_x;
   vga.expose.height = window_size_y;
   send_cmd (vgaNop, &cmd);
}



void vram_write(int x, int y, int r, int g, int b)
{
  Cmd cmd;

  /*  x = (addr >> 2) % window_size_x;
  y = (addr >> 2) / window_size_x;
  r = 0xff & (val >> 16);
  g = 0xff & (val >> 8);
  b = 0xff & val;
  */
  if ((x < 0) || (x >= window_size_x) || (y < 0) || (y >= window_size_y))
    return;
  cmd.pp.x = x;
  cmd.pp.y = y;
  cmd.pp.r = r;
  cmd.pp.g = g;
  cmd.pp.b = b;
  vga.expose.x = x;
  vga.expose.y = y;
  vga.expose.width = 1;
  vga.expose.height = 1;
  send_cmd (vgaPutPixel, &cmd);
}


/* the usual framework */

#define VRAMSIZE (window_size_x*window_size_y*4)

unsigned char vram[VRAMSIZE];


void init_device(void){
	close(0);
        memset(vram,0, VRAMSIZE);
}


unsigned char *vmb_get_payload(unsigned int offset,int size){
  return vram+offset;
}

void show_vram(unsigned int offset,int size)
{  while (size > 0)
    { int x,y,r,g,b;
    x = (offset >> 2) % window_size_x;
    y = (offset >> 2) / window_size_x;
    r =  vram[offset+1];
    g =  vram[offset+2];
    b =  vram[offset+3];
    vmb_debugi(0, "x = %d",x);
    vmb_debugi(0, "y = %d",y);
    vmb_debugx(0, "rgb = %s",vram+offset+1,3);
    vram_write(x,y,r,g,b);
    size = size-4;
    offset = offset +4;
  }
}

void vmb_put_payload(unsigned int offset,int size, unsigned char *payload)
{ int i;
 if (offset+size>VRAMSIZE)
   size = VRAMSIZE-offset;
  memmove(vram+offset,payload,size);
  i = offset & 0x3;
  size = size+i;
  offset = offset-i;
  i = size &0x3;
  if (i!=0)
    size = size +(4-i);
  show_vram(offset,size);

}

void vmb_poweron(void)
{ char *argv[1] = {"vram"};
  memset(vram,0, VRAMSIZE);
  vram_init (1, argv);
}

void vmb_poweroff(void)
{ 
  vram_fini ();
}


void vmb_reset(void)
{ memset(vram,0, VRAMSIZE);
 if (vmb.power)
    vram_blank();
}


int main(int argc, char *argv[])
{
  param_init(argc, argv);
  vmb_debugs(0, "%s ",vmb_program_name);
  vmb_debugs(0, "%s ", version);
  vmb_debugs(0, "host: %s ",host);
  vmb_debugi(0, "port: %d ",port);
  close(0);
  vmb_debugi(0, "address hi: %x",HI32(vmb_address));
  vmb_debugi(0, "address lo: %x",LO32(vmb_address));
  vmb_debugi(0, "size: %x ",vmb_size);

  vmb_connect(&vmb, host,port); 
  vmb_register(&vmb,HI32(vmb_address),LO32(vmb_address),vmb_size,
               0, 0, vmb_program_name,major_version,minor_version);

  vmb_wait_for_disconnect(&vmb);
  vram_fini ();
  return 0;
}

