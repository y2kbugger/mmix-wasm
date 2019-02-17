/* option.h
 * This File is part of the MMIX Hardware Project 
 *
 *	Copyright (c) 2005 Martin Ruckert (mailto:ruckertm@acm.org)
 * 
 * The MMIX Hardware is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * The MMIX hardware is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with the MMIX Hardware; if not, write to the
 * Free Software Foundation, Inc., 
 * 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef OPTION_H
#define OPTION_H

#include <stdio.h>

#ifdef WIN32
#include <windows.h>
typedef INT8 int8_t;
typedef INT16 int16_t;
typedef INT32 int32_t;
typedef INT64 int64_t;
typedef UINT8 uint8_t;
typedef UINT16 uint16_t;
typedef UINT32 uint32_t;
typedef UINT64 uint64_t;
#else
#include <stdint.h>
#endif



/* this is what you get: */
/* strings with the program path  and program name */
extern char *programpath;
extern char *programhelpfile;
extern char *vmb_program_name;
/* extern char *program_name; */
extern char *defined;
/* NULL, or a string that is used to identify conditionals
   that will not be skipped in the configuration file 
   if NULL, the programname will be used instead.
   programname and defined can be changed with set_option (see below).
*/


/* a string to store temporarily an option */
#define MAXTMPOPTION 1024
extern char tmp_option[MAXTMPOPTION+1];

extern void set_option(char **option, char *str);
/* deallocate *option if necessary, allocate if necesarry, and fill with the given string */

extern void option_usage(FILE *out);
/*write an explanation of the options to out */

extern void do_program(char * arg);
/* initialize program dependent variables */

extern void parse_commandline(int argc, char **argv);
/* parse the commandline handling options and arguments 
   short options consist of a "-" followed by the option character
   several short options might be combined -a -b -c is the same as -abc
   in this case only the last of these short oprions might have an argument string
   long options consist of a "--" followed by the option string
   "-" and "--" are not options.
   options may take an optional/required argument that follows the option
   separated by a space.
   in the case of long options only, a single "=" can be used to separate the
   option from the following argument
*/
#if 0
extern void parse_commandstr(char *p);
/* like parse_commandline takes all the information from one string */
#endif
extern int do_option_debug(char *dummy);
/* switch debugging on immediately */
extern int do_option_configfile(char *filename);
/* same for a configuration file (options only) 
   the file with the given filename is opened and read.
   empty/blank lines are ignored
   lines starting with a # character are comments and ignored.
   all other lines should start with a keyword and be followed
   by an optional value. The keywords are exactly the strings valid
   as long options. the semantics of a keyword line is the same as giving
   that same option on the command line with the rest of the line as argument.
   reading of configuration files can be recursive.
   the special string #FILE# is repaced by the configuration file
   the special string #PATH# is replaced by the path to the configuration file.
   do_option_configfile can be used as a handler to handle config-file options
   (see below).
*/
extern int write_configfile(char *filename);
/* writes a configfile that can be read by parse_configfile */

extern void option_defaults(void);
/* extract the defaults from the options table and set them.
   on_args have an implicit default 0 and off_args have an implicit default 1
   all other options need an explicit string as default otherwise no
   default is set. (that is the programm is responsible for setting defaults
   otherwise.)
*/

extern FILE *vmb_fopen(char *filename, char *mode);
/* fopen(filename,mode) look in the configPATH and programpath before giving up 
*/
extern char *vmb_get_cwd(void); 
/* set and get the current working directory */


/* this is what you must provide: */

/* from error.h/error.c */
extern void vmb_fatal_error(int line, char *message);
/* a function to call if something goes wrong (should not return)*/
/* a function to call if something goes wrong (should not return)*/
extern void vmb_debug(int level, char *msg);
/* a function to call to display messages */
extern void vmb_error(int line, char *message);
/* a function to call to display errormessages */

/* the argument handler */
extern void do_argument(int pos, char *arg);
/* the agument 0 is the program name it is found in the corresponding
   variable, arument 1, if it exists, it the defined name for conditionals,
   it id found in the variable "defined".
   All the other arguments for pos=2,3,... argc passed to this function
*/
extern int do_define(char *arg);
/* a function called on the very first agument unless this first argument starts with '-' 
   it is typically used to define an alternative program name for use in configuartion file
   conditionals.
*/

/* the option description */

typedef enum {
  fun_arg, /* call a specified function with an argument */
  str_arg, /* store the argument as string */
  int_arg, /* store the argument as int */
  double_arg, /* store the argument as a double */
  tgl_arg, /* toggle the argument, an int, between 0 and 1 */
  on_arg,  /* set the argument, an int, to 1 */
  off_arg, /* set the argument, an int, to 0 */
  inc_arg,  /* increment the argument, an int */
  uint64_arg /* store the argument as unsigned int with 64 bit uint64_t*/
} option_type;  /* see below */


typedef
struct {
 char *description;      /* the human readable description of the option */
 char shortopt;          /* the character to indicate this option */
 char *longopt;          /* the name used as long option name */
 char *arg_name;         /* the name of the argument (only for description) */
 option_type kind;       /* the option/argument type */
 char *default_str;      /* string containing the deault value */
 union {                 /* the handler for this option */
   void *v;              /* dummy for internal use */
   char **str;           /* where to store a string */
   int *i;               /* where to store an int */
   double *d;			 /* where to store a double */
   uint64_t *u;          /* where to store an uint64_t */
   int (*f)(char *arg); } handler; /* what function to call */
} option_spec;  /* see below */


extern option_spec options[];
/* table describing the options. terminated with an entry where the
   description is NULL.
   Each entry in the table describes an option
   we best describe it by example:

   option_spec options = {

   {"set input file to <file>",'i',"input","file",str_arg,"in.txt",{&s}},

    This option can be used as ... -i "string" or --input "string"
    the option_usage function will output something like:
     -i --input <file> set input file to <file> default=in.txt 

    option_defaults will set the variable s to "in.txt"
    and giving the option on the commandline (or in a configuration file)
    will change s to whatever value was given there.
   
   {"set priority to <number>",'p',"priority","number",int_arg,"8000",{&i}},

    This option can be used as ... -p 123 or --priority 4567
    the option_usage function will output something like:
     -p --priority <number> set priority to <number> default=8000 

    option_defaults will set the variable i to 8000
    and giving the option on the commandline (or in a configuration file)
    will change i to whatever value was given there.

   {"set zoom to <float>",'z',"zoom","float",double_arg,"1.0",{&zoom}},

    This option can be used as ... -z 1.5 or --zoom 4
    the option_usage function will output something like:
     -z --zoom <float> set zoom to <float> default=1.0

    option_defaults will set the variable zoom to 1.0
    and giving the option on the commandline (or in a configuration file)
    will change zoom to whatever value was given there.

   {"turn on verbose mode",'v',"verbose",NULL,on_arg,NULL,{&flag}},

    This option can be used as ... -v or --verbose 
    the option_usage function will output something like:
     -v --verbose turn on verbose mode 

    option_defaults will set the variable flag to 0 (on_arg's are off by 
    default, off_args are on by default, toggle_args and inc_args
    need an explicit default.)
    and giving the option on the commandline (or in a configuration file)
    will change flag to 1 ( off_args are set to 0, toggle_args switch back 
    and forth between 0 and 1, inc_args get incremented each time.)


    {"read configuration from <file>",'f',"config","file",fun_arg,NULL,{parse_configfile}},

    this option can be used as ... -f f.cfg or --config f.cfg
    the option_usage function will output something like:
    -f --config <file> read configuration from <file> 
    option_defaults will not do anything, since no default is given (NULL). if
    a default would be present, it would execute the option as if given on the
    command line (or in a configuration file).
    executing an option of type fun_arg means calling the function given as the handler
    (parse_configfile) in this case with the argument string (if there is one or NULL
    if there is none) the function can do what ever it likes, it should test
    the argument for !=NULL before using it, and it should return 1 if the argument
    was used and 0 if the argument was not used.

   {NULL}
   terminated the table
   }

*/ 

#endif
