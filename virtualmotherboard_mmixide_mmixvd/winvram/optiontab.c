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

#include "option.h"
#include "param.h"
#include "vmb.h"

int framewidth,frameheight, fontwidth, fontheight;
double zoom;
uint64_t vmb_mouse_address, vmb_gpu_address;
int move_interrupt, gpu_interrupt;

option_spec options[] = {
/* description short long kind default handler */
{"the host where the bus is located",   'h',   "host",    "host",          str_arg, "localhost", {&host}},
{"the port where the bus is located",   'p', "port",    "port",          int_arg, "9002", {&port}},
{"to generate debug output",            'd', "debug",   "debugflag",     fun_arg, NULL, {&vmb_debug_on}},
{"make debugging verbose",   'v', "verbose",    "verbose debugging", on_arg, NULL, {&vmb_verbose_flag}},
{"set the debug mask",                  'M', "debugmask", "hide debug output",   int_arg, "0xFFF0", {&vmb_debug_mask}},
{"to define a name for conditionals",   'D', "define",  "conditional",   str_arg, NULL, {&defined}},
{"address where the vram is located",'a', "address", "start address",   uint64_arg, "0x0002000000000000", {&vmb_address}},
{"address where the mouse is located",'m', "mouseaddress", "start address",   uint64_arg, "0x0001000000000010", {&vmb_mouse_address}},
{"address where the GPU is located",'g', "gpuaddress", "start address",   uint64_arg, "0x0001000000000020", {&vmb_gpu_address}},
{"interrupt send by the mouse",            'i', "interrupt", "interrupt number", int_arg, "19", {&interrupt_no}},
{"interrupt send by the gpu",            'I', "gpuinterrupt", "interrupt number", int_arg, "20", {&gpu_interrupt}},
{"whether mouse moves generate interrupts",  'v', "moveinterrupt", "mouse move interrupt flag", on_arg, NULL, {&move_interrupt}},
{"the visible width",                   'w', "width",    "visible width",int_arg, "640", {&width}},
{"the visible height",                  'h', "height",    "visible height",int_arg, "480", {&height}},
{"the frame width",                     'q', "fwidth",    "frame width",int_arg, "640", {&framewidth}},
{"the frame height",                    'r', "fheight",    "frame height",int_arg, "480", {&frameheight}},
{"the font width",                      's', "fontwidth",    "font width",int_arg, "10", {&fontwidth}},
{"the font height",                     't', "fontheight",    "font height",int_arg, "20", {&fontheight}},
{"the zoom factor",                      'z', "zoom",    "zoom factor",double_arg, "1", {&zoom}},
{"filename for a configuration file",    'c', "config", "file",          fun_arg, NULL, {do_option_configfile}},
{"to print usage information",           '?', "help",   NULL,            fun_arg, NULL,{usage}},
{NULL}
};
