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
extern int fontheight;

option_spec options[] = {
/* description short long kind default handler */
{"the host where the bus is located", 'h',   "host",    "host",          str_arg, "localhost", {&host}},
{"the port where the bus is located",   'p', "port",    "port",          int_arg, "9002", {&port}},
{"the x position of the window",        'x', "x",       "x position",    int_arg, "0", {&xpos}},
{"the y position of the window",        'y', "y",       "y position",    int_arg, "0", {&ypos}},
{"the height of the window font",        'F', "fontheight",       "height of font",    int_arg, "14", {&fontheight}},
{"start with a minimized window",        'm', "minimized",       "minimizedflag",    on_arg, NULL, {&minimized}},
{"to generate debug output",            'd', "debug",   "debugflag",     fun_arg, NULL, {&vmb_debug_on}},
{"make debugging verbose",   'v', "verbose",    "verbose debugging", on_arg, NULL, {&vmb_verbose_flag}},
{"set the debug mask",                  'M', "debugmask", "hide debug output",   int_arg, "0xFFF0", {&vmb_debug_mask}},
{"to define a name for conditionals",   'D', "define",  "conditional",   str_arg, NULL, {&defined}},
{"address where the resource is located",'a', "address", "hex address",  uint64_arg, "0x8000000000000000", {&vmb_address}},
{"size of address range in octas",      's', "size",    "size in octas", int_arg, "1", {&vmb_size}},
{"interrupt send by device",            'i', "interrupt", "interrupt number", int_arg, "8", {&interrupt_no}},
{"to disable interrupts",               'Q', "disableinterrupt",   "false",       on_arg, NULL, {&disable_interrupt}},
{"filename for input file",             'f', "file",    "file name",     str_arg, NULL, {&vmb_filename}},
{"filename for a configuration file",    'c', "config", "file",          fun_arg, NULL, {do_option_configfile}},
{"to print usage information",           '?', "help",   NULL,            fun_arg, NULL,{usage}},
{NULL}
};
