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

#ifdef VMB
#include "option.h"
#include "param.h"

#include "vmb.h"


option_spec options[] = {
/* description short long kind default handler */
#ifdef VMB
{"the host where the bus is located", 'h',   "host",    "host",          str_arg, "localhost", {&host}},
{"the port where the bus is located",   'p', "port",    "port",          int_arg, "9002", {&port}},
#endif
{"the x position of the window",        'x', "x",       "x position",    int_arg, "0", {&xpos}},
{"the y position of the window",        'y', "y",       "y position",    int_arg, "0", {&ypos}},
{"the width of the window",             'W', "width",       "width",    int_arg, "0", {&width}},
{"the height of the window",             'H', "height",       "height",    int_arg, "0", {&height}},
{"start with a minimized window",        'm', "minimized",       "minimizedflag",    on_arg, NULL, {&minimized}},
{"to define a name for conditionals",   'D', "define",  "conditional",   str_arg, NULL, {&defined}},
{"filename for a configuration file",    'c', "config", "file",          fun_arg, NULL, {do_option_configfile}},
{"to print usage information",           '?', "help",   NULL,            fun_arg, NULL,{usage}},
{NULL}
};
#endif