This file has been written by Felix Albrecht and was modified by Martin Ruckert.

We start by adding a preprocessor definition to disable warnings
about deprecated functions, like strcpy or sprintf, and 
include mmixlib.h at the start of the sybroutines 
to declare the extern library interface.

We prepare for the elimination
of the exit() function, which must be replaced by a return from
the subroutine.


@x
@<Preprocessor definitions@>=
@y
@<Sub...@>=

@ @<Global...@>=
#include <setjmp.h>
jmp_buf mmixal_exit;

@ @<Preprocessor definitions@>=
#ifdef WIN32
#pragma warning(disable : 4996)
#endif
@z

We need to initialize buffers, especially err_buf, very early.

@x
@<Initialize everything@>=
@y
@<Allocate buffers@>=
@z

We change error messages that handle errors in the command line 
(there is none). Errors are no longer written to stderr.

@x
    fprintf(stderr,
       "(say `-b <number>' to increase the length of my input buffer)\n");
@y
    MMXIAL_LINE_TRUNCATED
@z

The filenames in mmixal are considered local.

@x
Char *filename[257];
@y
@z

@x
    if (!filename[filename_count]) {
      filename[filename_count]=(Char*)calloc(FILENAME_MAX+1,sizeof(Char));
      if (!filename[filename_count])
        panic("Capacity exceeded: Out of filename memory");
@.Capacity exceeded...@>
    }
    for (p++,k=0;*p && *p!='\"' && k<FILENAME_MAX; p++,k++)
      filename[filename_count][k]=*p;
@y
    Char filename[FILENAME_MAX+1];
    for (p++,k=0;*p && *p!='\"' && k<FILENAME_MAX; p++,k++)
      filename[k]=*p;
@z

@x
      filename[filename_count][k]='\0';
      for (k=0;strcmp(filename[k],filename[filename_count])!=0;k++);
      if (k==filename_count) {
        if (filename_count==256)
          panic("Capacity exceeded: More than 256 file names");
        filename_count++;
      }
      else free(filename[filename_count]); 
@y
      filename[k]='\0';
      k= filename2file(filename);
      if (k<0) panic("Capacity exceeded: More than 256 file names");
@z







The error reporting goes into a separate file, because we might want
to change it.

@x
@d err(m) {@+report_error(m);@+if (m[0]!='*') goto bypass;@+}
@d derr(m,p) {@+sprintf(err_buf,m,p);
   report_error(err_buf);@+if (err_buf[0]!='*') goto bypass;@+}
@d dderr(m,p,q) {@+sprintf(err_buf,m,p,q);
   report_error(err_buf);@+if (err_buf[0]!='*') goto bypass;@+}
@d panic(m) {@+sprintf(err_buf,"!%s",m);@+report_error(err_buf);@+}
@d dpanic(m,p) {@+err_buf[0]='!';@+sprintf(err_buf+1,m,p);@+
                                          report_error(err_buf);@+}
@y
@d err(m) {@+report_error(m,cur_file,line_no);@+if (m[0]!='*') goto bypass;@+}
@d derr(m,p) {@+sprintf(err_buf,m,p);
   report_error(err_buf,cur_file,line_no);@+if (err_buf[0]!='*') goto bypass;@+}
@d dderr(m,p,q) {@+sprintf(err_buf,m,p,q);
   report_error(err_buf,cur_file,line_no);@+if (err_buf[0]!='*') goto bypass;@+}
@d panic(m) {@+sprintf(err_buf,"!%s",m);@+report_error(err_buf,cur_file,line_no);@+}
@d dpanic(m,p) {@+err_buf[0]='!';@+sprintf(err_buf+1,m,p);@+
                                          report_error(err_buf,cur_file,line_no);@+}
@z


@x
@<Sub...@>=
void report_error @,@,@[ARGS((char*))@];@+@t}\6{@>
void report_error(message)
  char *message;
{
  if (!filename[cur_file]) filename[cur_file]="(nofile)";
  if (message[0]=='*')
    fprintf(stderr,"\"%s\", line %d warning: %s\n",
                 filename[cur_file],line_no,message+1);
  else if (message[0]=='!')
    fprintf(stderr,"\"%s\", line %d fatal error: %s\n",
                 filename[cur_file],line_no,message+1);
  else {
    fprintf(stderr,"\"%s\", line %d: %s!\n",
                 filename[cur_file],line_no,message);
    err_count++;
  }
  if (listing_file) {
    if (!line_listed) flush_listing_line("****************** ");
    if (message[0]=='*') fprintf(listing_file,
            "************ warning: %s\n",message+1);
    else if (message[0]=='!') fprintf(listing_file,
            "******** fatal error: %s!\n",message+1);
    else fprintf(listing_file,
            "********** error: %s!\n",message);
  }
  if (message[0]=='!') exit(-2);
}
@y
@(libalerror.c@>=
#include <stdlib.h>
#include <stdio.h>
#include <setjmp.h>
#include "libname.h"
#define _MMIXAL_

extern int err_count;
extern FILE *listing_file;
extern void flush_listing_line(char*line);
typedef enum{false,true}bool;
extern bool line_listed;
extern jmp_buf mmixal_exit;

void report_error(char *message,int file_no,int line_no)
{ char *filename;
  filename=file2filename(file_no);
  if (!filename) filename="(nofile)";
  if (message[0]=='*')
    fprintf(stderr,"\"%s\", line %d warning: %s\n",
                 filename,line_no,message+1);
  else if (message[0]=='!')
    fprintf(stderr,"\"%s\", line %d fatal error: %s\n",
                 filename,line_no,message+1);
  else {
    fprintf(stderr,"\"%s\", line %d: %s!\n",
                 filename,line_no,message);
    err_count++;
  }
  if (listing_file) {
    if (!line_listed) flush_listing_line("****************** ");
    if (message[0]=='*') fprintf(listing_file,
            "************ warning: %s\n",message+1);
    else if (message[0]=='!') fprintf(listing_file,
            "******** fatal error: %s!\n",message+1);
    else fprintf(listing_file,
            "********** error: %s!\n",message);
  }
  if (message[0]=='!') longjmp(mmixal_exit,-2);
}
@z

in mmo sync we map global file numbers
to local filenumbers that go into the mmo file.
First files that have already been passed. The
local file number is in filename_passed.
@x
    if (filename_passed[cur_file]) mmo_lop(lop_file,cur_file,0);
@y
    if (filename_passed[cur_file]>=0) mmo_lop(lop_file,filename_passed[cur_file],0);
@z

@x
      mmo_lop(lop_file,cur_file,(strlen(filename[cur_file])+3)>>2);
      for (j=0,p=filename[cur_file];*p;p++,j=(j+1)&3) {
        mmo_buf[j]=*p;
        if (j==3) mmo_out();
      }
      if (j) {
        for (;j<4;j++) mmo_buf[j]=0;
        mmo_out();
      }
    filename_passed[cur_file]=1;
@y
	  Char *filename;
      if (filename_count>=256)
        panic("Capacity exceeded: More than 256 file names");
	  filename_passed[cur_file]=filename_count;
	  filename_count++;
	  filename=file2filename(cur_file);
	  if (filename==NULL) panic("Unknown file name");
      mmo_lop(lop_file,filename_passed[cur_file],(strlen(filename)+3)>>2);
      for (j=0,p=filename;*p;p++,j=(j+1)&3) {
        mmo_buf[j]=*p;
        if (j==3) mmo_out();
      }
      if (j) {
        for (;j<4;j++) mmo_buf[j]=0;
        mmo_out();
      }
@z

In the assemble subroutine, we have the
opportunity to gain some information about
the association of lines and locations.

@x
  for (j=0;j<k;j++) {
@y
#ifdef MMIXAL_LINE_LOC
 MMIXAL_LINE_LOC(cur_file, line_no, cur_loc);
#endif
 for (j=0;j<k;j++) {
@z

We allocate tree nodes, node by node, so that we can free them again.

@x
  register trie_node *t=next_trie_node;
  if (t==last_trie_node) {
    t=(trie_node*)calloc(1000,sizeof(trie_node));
    if (!t) panic("Capacity exceeded: Out of trie memory");
@.Capacity exceeded...@>
    last_trie_node=t+1000;
  }
  next_trie_node=t+1;
  return t;
@y
  trie_node *t=(trie_node*)calloc(1,sizeof(trie_node));
  if (!t) panic("Capacity exceeded: Out of trie memory");
  return t;
@z

We need extra information in the symbol table.

@x
typedef struct sym_tab_struct {
@y
typedef struct sym_tab_struct {
  int file_no;
  int line_no;
@z

We allocat single sym nodes to be able to free them.

@x
    p=next_sym_node;
    if (p==last_sym_node) {
      p=(sym_node*)calloc(1000,sizeof(sym_node));
      if (!p) panic("Capacity exceeded: Out of symbol memory");
@.Capacity exceeded...@>
      last_sym_node=p+1000;
    }
    next_sym_node=p+1;
@y
    p=(sym_node*)calloc(1,sizeof(sym_node));
    if (!p) panic("Capacity exceeded: Out of symbol memory");
@z   

Now we start with the rest of the initialization.

@x
@<Init...@>=
@y
@<Initialize everything@>=
@z

The special names are already defined in mmix-sim.w

@x
Char *special_name[32]={"rB","rD","rE","rH","rJ","rM","rR","rBB",
 "rC","rN","rO","rS","rI","rT","rTT","rK","rQ","rU","rV","rG","rL",
 "rA","rF","rP","rW","rX","rY","rZ","rWW","rXX","rYY","rZZ"};
@y
extern Char *special_name[32];
@z

Errors generated at the end of the assembly should use the
normal error reporting function.

@x
  fprintf(stderr,"undefined symbol: %s\n",sym_buf+1);
@.undefined symbol@>
  err_count++;
@y
  sprintf(err_buf,"undefined symbol: %s",sym_buf+1);
  report_error(err_buf,cur_file,line_no);
@z

when a sym_node becomes DEFINED, we record file and line.

@x
  @<Find the symbol table node, |pp|@>;
@y
  @<Find the symbol table node, |pp|@>;
  pp->file_no=cur_file;
  pp->line_no=line_no;  
@z




Some global variables need to be static to
avoid name conflicts in the library.

@x
tetra z,y,x,yz,xyz; /* pieces for assembly */
@y
static tetra z,y,x,yz,xyz; /* pieces for assembly */
@z

The main() program becomes mmixal().

@x
@c
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <time.h>
@#
@<Preprocessor definitions@>@;
@<Type definitions@>@;
@<Global variables@>@;
@<Subroutines@>@;
@#
int main(argc,argv)
  int argc;@+
  char *argv[];
{
  register int j,k; /* all-purpose integers */
  @<Local variables@>;
  @<Process the command line@>;
  @<Initialize everything@>;
  while(1) {
    @<Get the next line of input text, or |break| if the input has ended@>;
    while(1) {
      @<Process the next \MMIXAL\ instruction or comment@>;
      if (!*buf_ptr) break;
    }
    if (listing_file) {
      if (listing_bits) listing_clear();
      else if (!line_listed) flush_listing_line("                   ");
    }
  }
  @<Finish the assembly@>;
}
@y
@c
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#define _MMIXAL_
#include "libconfig.h"
#include "mmixlib.h"

@#
extern int expanding;
extern int buf_size;
extern char*src_file_name;
extern char obj_file_name[FILENAME_MAX+1];
extern char listing_name[FILENAME_MAX+1];

extern void report_error(char * message, int file_no, int line_no);
extern int mmixal(char *mms_name, char *mmo_name, char *mml_name);
@#
int main(argc,argv)
  int argc;@+
  char *argv[];
{
  register int j; /* all-purpose integers */

  @<Process the command line@>;
  return mmixal(src_file_name, obj_file_name, listing_name);
}

@ @(libmmixal.c@>=
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <time.h> 
#include <setjmp.h>
#define _MMIXAL_
#include "libconfig.h"


@#
@h
@<Preprocessor definitions@>@;
@<Type definitions@>@;
@<Global variables@>@;
#include "libimport.h"
#include "libname.h"

extern void report_error(char * message, int file_no, int line_no);
extern jmp_buf mmixal_exit;

@<Subroutines@>@;

@#
int mmixal(char *mms_name, char *mmo_name, char *mml_name)
{
  register int j,k; /* all-purpose integers */
  @<Local variables@>;
  /* instead of processing the commandline */
  err_count=setjmp(mmixal_exit);
  if (err_count!=0){
   prune(trie_root);
   goto clean_up;
  }  
  @<Allocate buffers@>
  if (mms_name==NULL)
    panic("No input file name");
  src_file_name= mms_name;
  if (mmo_name==NULL)
    obj_file_name[0]=0;
  else
    strncpy(obj_file_name,mmo_name,FILENAME_MAX);
  if (mml_name==NULL)
    listing_name[0]=0;
  else
    strncpy(listing_name,mml_name,FILENAME_MAX);
    cur_file=0;
  line_no=0;
  long_warning_given=0;
  cur_loc.h=cur_loc.l=0;
  listing_loc.h=listing_loc.l=0;
  spec_mode= 0;
  spec_mode_loc= 0;
  mmo_ptr=0;
  err_count=0;
  serial_number=0;
  for (j=0;j<256;j++) filename_passed[j]=-1;
  filename_count=0;
  for (j=0;j<10;j++)
  { forward_local[j].link=0;
    backward_local[j].link=0;
  }
  greg= 255;
  lreg= 32;
  @<Initialize everything@>;
  while(1) {
    @<Get the next line of input text, or |break| if the input has ended@>;
    while(1) {
      @<Process the next \MMIXAL\ instruction or comment@>;
      if (!*buf_ptr) break;
    }
    if (listing_file) {
      if (listing_bits) listing_clear();
      else if (!line_listed) flush_listing_line("                   ");
    }
  }
  @<Finish the assembly@>;
}
@z

@x
filename[0]=src_file_name;
filename_count=1;
@y
cur_file=filename2file(src_file_name);
filename_count=0;
@z


We end with return or longjmp instead of exit.

@x
if (err_count) {
  if (err_count>1) fprintf(stderr,"(%d errors were found.)\n",err_count);
  else fprintf(stderr,"(One error was found.)\n");
}
exit(err_count);
@y
  if (err_count>0){
    if (err_count>1) sprintf(err_buf,"(%d errors were found.)",err_count);
    else sprintf(err_buf,"(One error was found.)");
    report_error(err_buf,cur_file,line_no);
  }
clean_up:
  if (listing_file!=NULL) 
  { fclose(listing_file);
    listing_file=NULL;
  }
  if (obj_file!=NULL) 
  { fclose(obj_file);
    obj_file=NULL;
  }
  if (src_file!=NULL) 
  { fclose(src_file);
    src_file=NULL;
  }
  cur_file=0;
  line_no=0;
  long_warning_given=0;
  cur_loc.h=cur_loc.l=0;
  listing_loc.h=listing_loc.l=0;
  spec_mode= 0;
  spec_mode_loc= 0;
  serial_number=0;
  free(buffer); buffer=NULL;
  free(lab_field); lab_field=NULL;
  free(op_field); op_field=NULL;
  free(operand_list); operand_list=NULL;
  free(err_buf);err_buf=NULL;
  free(op_stack); op_stack=NULL;
  free(val_stack);val_stack=NULL;

  filename_count=0;
  
return err_count;
@z


To report undefined local sysmbols we use the
usual error reporting.

@x
  err_count++,fprintf(stderr,"undefined local symbol %dF\n",j);
@y
  { sprintf(err_buf,"undefined local symbol %dF",j);
    report_error(err_buf,cur_file,line_no);
  }
@z
