/*
    Copyright 2012 Wladimir Danilov
    
    w.danilov@googlemail.com

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



	mspoptions.c

	Configuration options for MSP430 simulator

*/

#include "mspshared.h"

option_spec options[] = {
/* description short long kind default handler */
{"the host where the bus is located", 'h',   "host",    "host",          str_arg, "localhost", {&host}},
{"the port where the bus is located",   'p', "port",    "port",          int_arg, "9002", {&port}},
{"translate the lower RAM addresses to extRAM",'t', "translate", "translate flag",  on_arg, "0", {&translate_ram_to_upper}},
{"start interactive mode",'i', "interactive", "interactive flag",  on_arg, "1", {&interactiveMode}},
{"execution start address", 's', "exstart", "hex address",  uint64_arg, "0x0000000000000000", {&executionStartAddress}},
{"to generate debug output",            'd', "debug",   "debug flag",     fun_arg, NULL, {&do_option_debug}},
{"make debugging verbose",   'v', "verbose",    "verbose debugging", on_arg, NULL, {&vmb_verbose_flag}},
{"set the debug mask",                  'M', "debugmask", "hide debug output",   int_arg, "0xFFF0", {&vmb_debug_mask}},
{"to define a name for conditionals",   'D', "define",  "conditional",   str_arg, NULL, {&defined}},
{"filename for a configuration file",    'c', "config", "file",          fun_arg, NULL, {do_option_configfile}},
{"to print usage information",           '?', "help",   NULL,            fun_arg, NULL,{usage}},
{NULL}
};