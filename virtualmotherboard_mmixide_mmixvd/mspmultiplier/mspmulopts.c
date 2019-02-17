#include "mspmulshared.h"

option_spec options[] = {
/* description short long kind default handler */
{"the host where the bus is located", 'h',   "host",    "host",          str_arg, "localhost", {&host}},
{"the port where the bus is located",   'p', "port",    "port",          int_arg, "9002", {&port}},
{"address where the resource is located",'a', "address", "hex address",  uint64_arg, "0x0000000000000130", {&vmb_address}},
{"to generate debug output",            'd', "debug",   "debug flag",     fun_arg, NULL, {&do_option_debug}},
{"make debugging verbose",   'v', "verbose",    "verbose debugging", on_arg, NULL, {&vmb_verbose_flag}},
{"set the debug mask",                  'M', "debugmask", "hide debug output",   int_arg, "0xFFF0", {&vmb_debug_mask}},
{"to define a name for conditionals",   'D', "define",  "conditional",   str_arg, NULL, {&defined}},
{"filename for a configuration file",    'c', "config", "file",          fun_arg, NULL, {do_option_configfile}},
{"to print usage information",           '?', "help",   NULL,            fun_arg, NULL,{usage}},
{NULL}
};