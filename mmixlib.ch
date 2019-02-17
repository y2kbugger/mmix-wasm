% This is the change file that extracts from mmix-sim.w 
% the various .c files that make up the mmix library

Types and preprocessor macros go into libtype.h

@x
@* Basics. To get started, we define a type that provides semantic sugar.
@y
@ @(libtype.h@>=
@h
#define _LIBTYPE_H_
@<Preprocessor macros@>@;
@<Type declarations@>@;

@* Basics. To get started, we define a type that provides semantic sugar.
@z

@x
@ @<Sub...@>=
void print_hex @,@,@[ARGS((octa))@];@+@t}\6{@>
@y
@ @(libprint.c@>=
#include <stdio.h>
#include <setjmp.h>
#include "libconfig.h"
#include <time.h>
#include "libtype.h"
#include "libglobals.h"
#include "mmixlib.h"
#include "libarith.h"
#include "libimport.h"

void print_hex @,@,@[ARGS((octa))@];@+@t}\6{@>
@z


The external definitions of mmix-arith
should also go into a header file.

@x
@<Sub...@>=
extern octa zero_octa; /* |zero_octa.h=zero_octa.l=0| */
@y
@(libarith.h@>=
extern octa zero_octa; /* |zero_octa.h=zero_octa.l=0| */
@z

Fatal errors need to be handled but error messages moves to libconfig.h

@x
@d panic(m) {@+fprintf(stderr,"Panic: %s!\n",m);@+exit(-2);@+}
@y
@d panic(m) {@+MMIX_ERROR("Panic: %s!\n",m);@+longjmp(mmix_exit,-2);@+}
@z

@x
@<Sub...@>=
void print_int @,@,@[ARGS((octa))@];@+@t}\6{@>
@y
@(libprint.c@>=
void print_int @,@,@[ARGS((octa))@];@+@t}\6{@>
@z

The tet field of the mem_tetra may be eliminated.

@x
  tetra tet; /* the tetrabyte of simulated memory */
@y
#ifdef MMIX_MEM_TET
  tetra tet; /* the tetrabyte of simulated memory */
#endif
@z

@x
@<Sub...@>=
mem_node* new_mem @,@,@[ARGS((void))@];@+@t}\6{@>
@y
@(libmem.c@>=
#include <stdlib.h>
#include <stdio.h>
#include <setjmp.h>
#include "libconfig.h"
#include <time.h>
#include "libtype.h"
#include "libglobals.h"
#include "mmixlib.h"
#include "libimport.h"






mem_node* new_mem @,@,@[ARGS((void))@];@+@t}\6{@>
@z

We distinguish between different levels of initialization:
persistent data (initialized once when we load the library),
then data that needs to be initialized each time we start the
simulator, 
and finally data that is initialized each time we boot the machine.

@x
@<Initialize...@>=
mem_root=new_mem();
mem_root->loc.h=0x40000000;
last_mem=mem_root;
@y
@<Set up persistent data@>=
mem_root=new_mem();
mem_root->loc.h=0x40000000;
last_mem=mem_root;
@z

Handling the simulation of memory goes into its own file.

@x
@<Sub...@>=
mem_tetra* mem_find @,@,@[ARGS((octa))@];@+@t}\6{@>
@y
@(libmem.c@>=
mem_tetra* mem_find @,@,@[ARGS((octa))@];@+@t}\6{@>
@z

Defining the macro mm conflicts with other macros when using MS Visual C

@x
@d mm 0x98 /* the escape code of \.{mmo} format */
@y
@d mmo_esc 0x98 /* the escape code of \.{mmo} format */
@z

Loading the object file will go into its own file. 

@x
@<Initialize everything@>=
mmo_file=fopen(mmo_file_name,"rb");
@y
@<Load object file@>=
mmo_file=fopen(mmo_file_name,"rb");
@z

How to print errors is defined in libconfig.h and
exit needs to be replaced by a longjmp.

@x
    fprintf(stderr,"Can't open the object file %s or %s!\n",
@.Can't open...@>
               mmo_file_name,alt_name);
    exit(-3);
@y
   MMIX_ERROR("Can't open the object file %s!\n",mmo_file_name);
   longjmp(mmix_exit,-3);
@z


these global varaibles go into libload

@x
@ @<Glob...@>=
FILE *mmo_file; /* the input file */
int postamble; /* have we encountered |lop_post|? */
int byte_count; /* index of the next-to-be-read byte */
byte buf[4]; /* the most recently read bytes */
int yzbytes; /* the two least significant bytes */
int delta; /* difference for relative fixup */
tetra tet; /* |buf| bytes packed big-endianwise */
@y
@ @<Load globals@>=
static FILE *mmo_file; /* the input file */
static int postamble; /* have we encountered |lop_post|? */
static int byte_count; /* index of the next-to-be-read byte */
static byte buf[4]; /* the most recently read bytes */
static int yzbytes; /* the two least significant bytes */
static int delta; /* difference for relative fixup */
static tetra tet; /* |buf| bytes packed big-endianwise */@z
@z

MMIX_ERROR again.

@x
     fprintf(stderr,"Bad object file! (Try running MMOtype.)\n");
@.Bad object file@>
     exit(-4);
@y   
     MMIX_ERROR("%s","Bad object file! (Try running MMOtype.)\n");
     longjmp(mmix_exit,-4);
@z

The next function goes into libload.

@x
@<Sub...@>=
void read_tet @,@,@[ARGS((void))@];@+@t}\6{@>
@y
@(libload.c@>=
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <setjmp.h>
#include "libconfig.h"
#include <time.h>
#include "libtype.h"
#include "libglobals.h"
#include "mmixlib.h"
#include "libarith.h"
#include "libname.h"
#include "libimport.h"


@<Load globals@>

@<Loading subroutines@>@;

@ @<Loading subroutines@>=
void read_tet @,@,@[ARGS((void))@];@+@t}\6{@>
@z

Also to libload.

@x
@ @<Sub...@>=
byte read_byte @,@,@[ARGS((void))@];@+@t}\6{@>
@y
@ @<Loading subroutines@>=
byte read_byte @,@,@[ARGS((void))@];@+@t}\6{@>
@z

Replace mm by mmo_esc.

@x
if (buf[0]!=mm || buf[1]!=lop_pre) mmo_err;
@y
if (buf[0]!=mmo_esc || buf[1]!=lop_pre) mmo_err;
@z

And again.

@x
 loop:@+if (buf[0]==mm) switch (buf[1]) {
@y
 loop:@+if (buf[0]==mmo_esc) switch (buf[1]) {
@z

The macro mmo_load becomes a function,
which uses MMIX_LDT and MMIX_STT as defined in libconfig.h.

line 940
@x
@d mmo_load(loc,val) ll=mem_find(loc), ll->tet^=val
@y
@d mmo_load(loc,val) ll=load_mem_tetra(loc,val)

@(libload.c@>=
static mem_tetra *load_mem_tetra(octa loc, tetra val)  
{ octa x; 
  mem_tetra *ll=mem_find(loc);           
  MMIX_LDT(x,loc);     
  x.l = x.l^val;             
  if (!MMIX_STT(x,loc))  
  panic("Unable to store mmo file to RAM");
  return ll;
}

@ This function is used next.
@z

Before we increment the line number,
we reset the frequency and possibly record file, line, and loc.

@x
    ll->file_no=cur_file;
    ll->line_no=cur_line;
    cur_line++;
@y
#ifdef MMIX_LOAD_LINE_LOC
    MMIX_LOAD_LINE_LOC(cur_file,cur_line,cur_loc);
#else
    ll->file_no=cur_file;
    ll->line_no=cur_line;
#endif
    ll->freq=0;
    cur_line++;
@z


We add a variable for the current frequency.
@x
int cur_line; /* the current position in |cur_file|, if nonzero */
@y
int cur_line; /* the current position in |cur_file|, if nonzero */
tetra cur_freq; /* the current frequency, if nonzero */
@z



The next lines of code complete loading the object file.

@x
@ @<Initialize...@>=
cur_loc.h=cur_loc.l=0;
@y
@ @<Load object file@>=
cur_loc.h=cur_loc.l=0;
@z

@x
cur_line=0;
@y
cur_freq=cur_line=0;
@z

mmo files use local file numbers.
As we might load multiple files,
we have to map the file number stored in
the mmo file as the ybyte to the 
filenumbers used inside the library.
Default conversion functions are defined
in libname.c.

First the case of known files.

@x
case lop_file:@+if (file_info[ybyte].name) {
   if (zbyte) mmo_err;
   cur_file=ybyte;
@y
case lop_file:
   if (ybyte2file[ybyte]>=0) {
   if (zbyte) mmo_err;
   cur_file=ybyte2file[ybyte];
@z

Now we handle new files.

@x
 }@+else {
   if (!zbyte) mmo_err;
   file_info[ybyte].name=(char*)calloc(4*zbyte+1,1);
   if (!file_info[ybyte].name) {
     fprintf(stderr,"No room to store the file name!\n");@+exit(-5);
@.No room...@>
   }
   cur_file=ybyte;
   for (j=zbyte,p=file_info[ybyte].name; j>0; j--,p+=4) {
     read_tet();
     *p=buf[0];@+*(p+1)=buf[1];@+*(p+2)=buf[2];@+*(p+3)=buf[3];
   }
 }
@y
 }@+else {
   if (!zbyte) mmo_err;
   if (4*zbyte+1>FILENAME_MAX) mmo_err;
   for (j=zbyte,p=filename; j>0; j--,p+=4) {
     read_tet();
     *p=buf[0];@+*(p+1)=buf[1];@+*(p+2)=buf[2];@+*(p+3)=buf[3];
   }
   cur_file=filename2file(filename);
   ybyte2file[ybyte]=cur_file;
 }
@z

mm was replaced by mmo_esc.

@x
   if (buf[0]==mm) {
@y
   if (buf[0]==mmo_esc) {
@z

We load the postamble into the beginning
of segment~3, also known as \.{Stack\_Segment}).
The stack segment is set up to be used with an unsave instruction.
On the stack, we have, the local registers (argc and argv) and the 
value of rL, then the global 
registers and the special registers rB, rD, rE, rH, rJ, rM, rR, rP, rW, rX, rY, and rZ,
followed by rG and rA packed into eight byte.

@x
@<Load the postamble@>=
aux.h=0x60000000;@+ aux.l=0x18;
ll=mem_find(aux);
(ll-1)->tet=2; /* this will ultimately set |rL=2| */
(ll-5)->tet=argc; /* and $\$0=|argc|$ */
(ll-4)->tet=0x40000000;
(ll-3)->tet=0x8; /* and $\$1=\.{Pool\_Segment}+8$ */
G=zbyte;@+ L=0;@+ O=0;
for (j=G+G;j<256+256;j++,ll++,aux.l+=4) read_tet(), ll->tet=tet;
inst_ptr.h=(ll-2)->tet, inst_ptr.l=(ll-1)->tet; /* \.{Main} */
(ll+2*12)->tet=G<<24;
g[255]=incr(aux,12*8); /* we will \.{UNSAVE} from here, to get going */
@y
@<Load the postamble@>=
{ octa x;
  aux.h=0x60000000;
  x.h=0;@+x.l=0;@+aux.l=0x00;
  if (!MMIX_STO(x,aux)) /* $\$0=|argc|$ */
     panic("Unable to store mmo file to RAM");
  x.h=0x40000000;@+x.l=0x8;@+aux.l=0x08;
  if (!MMIX_STO(x,aux)) /* and $\$1=\.{Pool\_Segment}+8$ */
     panic("Unable to store mmo file to RAM");
  x.h=0;@+x.l=2;@+aux.l=0x10;
  if (!MMIX_STO(x,aux)) /* this will ultimately set |rL=2| */
     panic("Unable to store mmo file to RAM");
  G=zbyte;@+ L=0;@+ O=0;
  aux.l=0x18;
  for (j=G;j<256;j++,aux.l+=8) 
  { read_tet(); x.h=tet;
    read_tet(), x.l=tet;
    if (!MMIX_STO(x,aux))
       panic("Unable to store mmo file to RAM");
  }
  aux=incr(aux,12*8); /* we can |UNSAVE| from here, to get going */
#ifdef MMIX_BOOT
  loc.h=0x80000000; loc.l=0;
  g[rWW] = x;  /* last octa stored is address of \.{Main} */
  g[rBB] = aux;
  g[rXX].h = 0; g[rXX].l = ((tetra)UNSAVE<<24)+255; /* \.{UNSAVE} \$255 */
  rzz = 1;
#else
  loc.h=0x80000000; loc.l=0;
  inst_ptr = x;  /* last octa stored is address of \.{Main} */
  g[255] = aux;
  g[rXX].h = 0; g[rXX].l = ((tetra)UNSAVE<<24)+255; /* \.{UNSAVE} \$255 */
  rzz = 1;
/*  RESUME 0 will not work for x = 0
    inst_ptr = x;
    g[255] = aux;
    rzz = 0;  pretend \.{RESUME} 0 */
#endif
  x.h=G<<24; x.l=0 /* rA */; 
  if (!MMIX_STO(x,aux))
     panic("Unable to store mmo file to RAM");
  G=g[rG].l; /* restore G to rG because it was changed above */
}
@z

The source line buffer is allocated once.


@x
@<Initialize...@>=
if (buf_size<72) buf_size=72;
@y
@<Set up persistent data@>=
if (buf_size<=0) buf_size=256;
@z

The display of source lines gets its own file.

@x
@<Sub...@>=
void make_map @,@,@[ARGS((void))@];@+@t}\6{@>
@y
@(libshowline.c@>=
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <setjmp.h>
#include "libconfig.h"
#include <time.h>
#include "libtype.h"
#include "libglobals.h"
#include "mmixlib.h"
#include "libimport.h"


void make_map @,@,@[ARGS((void))@];@+@t}\6{@>
@z

these include files are needed only in libshowline.c

@x
@<Preprocessor macros@>=
@y
@<Showline macros@>=
@z

@x
@<Sub...@>=
void print_line @,@,@[ARGS((int))@];@+@t}\6{@>
void print_line(k)
  int k;
@y
@(libshowline.c@>=
void print_line(int k)
@z


@x
@ @<Preprocessor macros@>=
@y
@ @<Showline macros@>=
@z

@x
@<Sub...@>=
void show_line @,@,@[ARGS((void))@];@+@t}\6{@>
@y
@(libshowline.c@>=
void show_line @,@,@[ARGS((void))@];@+@t}\6{@>
@z

Printing the profile has its own file.

@x
@<Sub...@>=
void print_freqs @,@,@[ARGS((mem_node*))@];@+@t}\6{@>
@y
@(libprofile.c@>=
#include <stdio.h>
#include <setjmp.h>
#include "libconfig.h"
#include <time.h>
#include "libtype.h"
#include "libglobals.h"
#include "mmixlib.h"
#include "libarith.h"
#include "libimport.h"

void print_freqs @,@,@[ARGS((mem_node*))@];@+@t}\6{@>
@z

We might need ll as a local variable here.

@x
  octa cur_loc;
@y
  octa cur_loc;
  MMIX_LOCAL_LL
@z

we use MMIX_FETCH.

@x
 loc_implied: printf("%10d. %08x%08x: %08x (%s)\n",
      p->dat[j].freq, cur_loc.h, cur_loc.l, p->dat[j].tet,
      info[p->dat[j].tet>>24].name);
@y
 loc_implied:
 { tetra inst; 
   MMIX_FETCH(inst,cur_loc);
   printf("%10d. %08x%08x: %08x (%s)\n",
      p->dat[j].freq, cur_loc.h, cur_loc.l, inst,
      info[inst>>24].name);
 }
@z

For the mmixlib, we split performing the
instruction in three parts: 
resuming,fetching, and executing.
here we do only the resuming.

@x
@<Perform one instruction@>=
{
  if (resuming) loc=incr(inst_ptr,-4), inst=g[rX].l;
  else @<Fetch the next instruction@>;
@y
@<Perform one instruction@>=
{
  if (resuming)
  { loc=incr(inst_ptr,-4), inst=g[rzz?rXX:rX].l;
    if (rzz==0) /* RESUME 0 */
    { if ((loc.h&sign_bit) != (inst_ptr.h&sign_bit))
      { resuming = false;
        goto protection_violation;
      }
      @<Check for security violation@>
    }
  }
@z

This restriction is no longer necessary.

@x
  if (loc.h>=0x20000000) goto privileged_inst;
@y
@z

The next two lines are not a propper part of
performing an instruction and move to a separate 
function respectively to the main loop.
@x
  @<Trace the current instruction, if requested@>;
  if (resuming && op!=RESUME) resuming=false;
@y
@z

@x
int rop; /* ropcode of a resumed instruction */
@y
int rop; /* ropcode of a resumed instruction */
int rzz; /* Z field of a resumed instruction */
@z

We turn |breakpoint| into an int to provide more information on the
kind of breakpoint. Instead of setting |breakpoint| to |true|, we 
(usually) store the respective bit of the |bkpt| field in the |mem_tetra| 
that caused the break. In case of a SWYM instruction, we set it to 
the YZ value shifted by 8 bit to the left. 
In all other cases, we set the |trace_bit|.

@x
bool breakpoint; /* should we pause after the current instruction? */
bool tracing; /* should we trace the current instruction? */
@y
int breakpoint=0; /* what caused the pause after the current instruction? */
bool tracing; /* should we trace the current instruction? */
@z


@x
bool interacting; /* are we in interactive mode? */
@y
bool interacting=false; /* are we in interactive mode? */
#ifdef MMIX_TRAP
bool show_operating_system = false; /* do we show negative addresses */
#endif
@z

We make some more variables global.

@x
@ @<Local...@>=
register mmix_opcode op; /* operation code of the current instruction */
register int xx,yy,zz,yz; /* operand fields of the current instruction */
register tetra f; /* properties of the current |op| */
@y
@ @<Glob...@>=
mmix_opcode op; /* operation code of the current instruction */
int xx,yy,zz,yz; /* operand fields of the current instruction */
tetra f; /* properties of the current |op| */

@ @<Local...@>=
@z

and p is no longer needed in performing an instruction.

@x
register char *p; /* current place in a string */
@y
@z

loading the instruction is postponed

@x
  inst=ll->tet;
@y
@z

When we hit an execute breakpoint, we set the exec bit in |breakpoint|.
We keep the current frequency for tracing.
@x
  ll->freq++;
  if (ll->bkpt&exec_bit) breakpoint=true;
  tracing=breakpoint||(ll->bkpt&trace_bit)||(ll->freq<=trace_threshold);
@y
  cur_freq=++(ll->freq);
  if (ll->bkpt&exec_bit) breakpoint|=exec_bit;
  if ((ll->bkpt&trace_bit)||(ll->freq<=trace_threshold)) breakpoint|=trace_bit;
  tracing=(breakpoint!=0);
@z


now before incrementing the instruction pointer we load the instruction.
@x
  inst_ptr=incr(inst_ptr,4);
@y
  @<Check for security violation@>
  inst=0; /* default TRAP 0,Halt,0 */
  if(!MMIX_FETCH(inst,loc)) 
    goto page_fault;
  inst_ptr=incr(inst_ptr,4);
  if ((inst_ptr.h&sign_bit) && !(loc.h&sign_bit))
    goto protection_violation;
@z

We change how to display certain instructions.

@x
{"RESUME",0x00,0,0,5,"{%#b} -> %#z"},@|
@y
{"RESUME",0x00,0,0,5,"{%#b}, $255 = %x, -> %#z"},@|
@z

@x
{"SYNC",0x01,0,0,1,""},@|
{"SWYM",0x00,0,0,1,""},@|
@y
{"SYNC",0x01,0,0,1,"%z"},@|
{"SWYM",0x01,0,0,1,"%r"},@|
@z

L,G, and O are made global.

@x
@ @<Local...@>=
register int G,L,O; /* accessible copies of key registers */
@y
@ @<Glob...@>=
int G=255,L=0,O=0; /* accessible copies of key registers */
@z


Some initialization needs to be done once others at each reboot.

@x
@<Initialize...@>=
g[rK]=neg_one;
g[rN].h=(VERSION<<24)+(SUBVERSION<<16)+(SUBSUBVERSION<<8);
g[rN].l=ABSTIME; /* see comment and warning above */
g[rT].h=0x80000005;
g[rTT].h=0x80000006;
g[rV].h=0x369c2004;
if (lring_size<256) lring_size=256;
lring_mask=lring_size-1;
if (lring_size&lring_mask)
  panic("The number of local registers must be a power of 2");
@.The number of local...@>
l=(octa*)calloc(lring_size,sizeof(octa));
if (!l) panic("No room for the local registers");
@.No room...@>
cur_round=ROUND_NEAR;
@y
@<Set up persistent data@>=
if (lring_size<256) lring_size=256;
lring_mask=lring_size-1;
if (lring_size&lring_mask) 
  panic("The number of local registers must be a power of 2");
l=(octa*)calloc(lring_size,sizeof(octa));
if (!l)  panic("No room for the local registers");

@ @<Initialize...@>=
sclock.l=sclock.h=0;
profile_started=false;
halted=false;
stdin_buf_start=stdin_buf_end=stdin_buf;
good_guesses=bad_guesses=0;
profiling=false;
interrupt=false;

@ @<Boot the machine@>=
memset(l,0,lring_size*sizeof(octa));
memset(g,0,sizeof(g));
L=O=S=0;
G=g[rG].l=255;
#ifdef MMIX_BOOT
g[rK] = zero_octa;
#else
g[rK] = neg_one;
#endif
g[rN].h=(VERSION<<24)+(SUBVERSION<<16)+(SUBSUBVERSION<<8);
g[rN].l=ABSTIME; /* see comment and warning above */
g[rT].h=0x80000000;g[rT].l=0x00000000;
g[rTT].h=0x80000000;g[rTT].l=0x00000000;
g[rV].h=0x12340D00;
g[rV].l=0x00002000;
cur_round=ROUND_NEAR;
@z

@x
  if (((S-O-L)&lring_mask)==0) stack_store();
@y
  if (((S-O-L)&lring_mask)==0) stack_store(l[S&lring_mask]);
@z

@x
@d test_store_bkpt(ll) if ((ll)->bkpt&write_bit) breakpoint=tracing=true
@y
@d test_store_bkpt(ll) if ((ll)->bkpt&write_bit) breakpoint|=write_bit,tracing=true
@z


stack_store must implement the rC register.

@x
@<Sub...@>=
void stack_store @,@,@[ARGS((void))@];@+@t}\6{@>
void stack_store()
{
  register mem_tetra *ll=mem_find(g[rS]);
  register int k=S&lring_mask;
  ll->tet=l[k].h;@+test_store_bkpt(ll);
  (ll+1)->tet=l[k].l;@+test_store_bkpt(ll+1);
  if (stack_tracing) {
    tracing=true;
    if (cur_line) show_line();
    printf("             M8[#%08x%08x]=l[%d]=#%08x%08x, rS+=8\n",
              g[rS].h,g[rS].l,k,l[k].h,l[k].l);
  }
  g[rS]=incr(g[rS],8),  S++;
}
@y
@<Stack store@>=
void stack_store @,@,@[ARGS((octa))@];@+@t}\6{@>
void stack_store(x)
  octa x;
{ unsigned int pw_bit, new_pw_bit;
  mem_tetra *ll;
  pw_bit=g[rQ].h&PW_BIT;
  new_pw_bit=new_Q.h&PW_BIT;
  if(!MMIX_STO(x,g[rS])) /* implementing the rC register */
  {   /* set CP_BIT */
      g[rQ].l |= CP_BIT;
      new_Q.l |= CP_BIT;
      if (g[rC].l&0x02)  /* Write bit */
      {  int s;
         octa address, base, offset,mask;
         mask.h=mask.l=0xFFFFFFFF;
         g[rQ].h &=~PW_BIT;  /* restore PW_BIT */
         new_Q.h &=~PW_BIT;
         g[rQ].h |= pw_bit;
         new_Q.h |= new_pw_bit;
         s    = (g[rV].h>>8)&0xFF;  /* extract the page size from rV */
         mask = shift_left(mask,s);
         offset.h = g[rS].h&~mask.h,offset.l = g[rS].l&~mask.l;
         mask.h &= 0x0000FFFF;     /* reduce mask to 48 bits */
         base.h = g[rC].h&mask.h,base.l = g[rC].l&mask.l;
         address.h=base.h|offset.h,address.l=base.l|offset.l;
         MMIX_STO(x,address);
      }
  }
  ll=mem_find(g[rS]);
  test_store_bkpt(ll);
  test_store_bkpt(ll+1);
  mmix_stack_trace("             M8[#%08x%08x]=#%08x%08x, rS+=8\n",
              g[rS].h,g[rS].l,x.h,x.l);
  g[rS]=incr(g[rS],8),  S++;
}

@ @<Sub...@>=
@<Stack store@>@;
@z

@x
@d test_load_bkpt(ll) if ((ll)->bkpt&read_bit) breakpoint=tracing=true
@y
@d test_load_bkpt(ll) if ((ll)->bkpt&read_bit) breakpoint|=read_bit,tracing=true
@z

Same with stack load.
@x
@<Sub...@>=
void stack_load @,@,@[ARGS((void))@];@+@t}\6{@>
@y
@<Sub...@>=
@<Stack load@>@;

@ @<Stack load@>=
void stack_load @,@,@[ARGS((void))@];@+@t}\6{@>
@z

@x
  l[k].h=ll->tet;@+test_load_bkpt(ll);
  l[k].l=(ll+1)->tet;@+test_load_bkpt(ll+1);
@y
  test_load_bkpt(ll);@+test_load_bkpt(ll+1);
  MMIX_LDO(l[k],g[rS]);
@z

showing lines is part of main.

@x
  if (stack_tracing) {
    tracing=true;
    if (cur_line) show_line();
    printf("             rS-=8, l[%d]=M8[#%08x%08x]=#%08x%08x\n",
              k,g[rS].h,g[rS].l,l[k].h,l[k].l);
  }
@y
  mmix_stack_trace("             rS-=8, l[%d]=M8[#%08x%08x]=#%08x%08x\n",
              k,g[rS].h,g[rS].l,l[k].h,l[k].l);
@z


@x
@<Sub...@>=
int register_truth @,@,@[ARGS((octa,mmix_opcode))@];@+@t}\6{@>
@y
@<Sub...@>=
@<Register truth@>@;

@ @<Register truth@>=
int register_truth @,@,@[ARGS((octa,mmix_opcode))@];@+@t}\6{@>
@z

Make sure we do not branch from a positive to a negative address.
@x
   inst_ptr=z;
@y
   if ((z.h&sign_bit) && !(loc.h&sign_bit))
   goto protection_violation;
   inst_ptr=z;
@z

@x
   if (g[rI].l<=2 && g[rI].l && g[rI].h==0) tracing=breakpoint=true;
@y
   if (g[rI].l<=2 && g[rI].l && g[rI].h==0) 
     g[rQ].l |= IN_BIT, new_Q.l |= IN_BIT;
@z


Loading is supposed to use the functions from libconfig

@x
case LDB: case LDBI: case LDBU: case LDBUI:@/
 i=56;@+j=(w.l&0x3)<<3; goto fin_ld;
case LDW: case LDWI: case LDWU: case LDWUI:@/
 i=48;@+j=(w.l&0x2)<<3; goto fin_ld;
case LDT: case LDTI: case LDTU: case LDTUI:@/
 i=32;@+j=0;@+ goto fin_ld;
case LDHT: case LDHTI: i=j=0;
fin_ld: ll=mem_find(w);@+test_load_bkpt(ll);
 x.h=ll->tet;
 x=shift_right(shift_left(x,j),i,op&0x2);
check_ld:@+if (w.h&sign_bit) goto privileged_inst; 
@y
case LDB: case LDBI: case LDBU: case LDBUI:@/
 i=56;@+j=56;@+if(!MMIX_LDB(x,w)) goto page_fault; goto fin_ld;
case LDW: case LDWI: case LDWU: case LDWUI:@/
 i=48;@+j=48;@+if(!MMIX_LDW(x,w)) goto page_fault; goto fin_ld;
case LDT: case LDTI: case LDTU: case LDTUI:@/
 i=32;@+j=32;@+if(!MMIX_LDT(x,w)) goto page_fault; goto fin_ld;
case LDHT: case LDHTI: i=j=0;@+if(!MMIX_LDT(x,w)) goto page_fault;
x.h=x.l;x.l=0;
fin_ld: ll=mem_find(w);@+test_load_bkpt(ll);
 if ((op&0x2)==0) x=shift_right(shift_left(x,i),i,op&0x2);
check_ld:@+ if ((w.h&sign_bit) && !(loc.h&sign_bit)) goto translation_bypassed_inst;
 goto store_x;
page_fault:
 if ((g[rK].h & g[rQ].h) != 0 || (g[rK].l & g[rQ].l) != 0) 
 { x.h=0, x.l=inst;
   y = w;
   z = zero_octa;
   @<Initiate a trap interrupt@>
   inst_ptr=y=g[rTT];
 }
 break;
@z

@x
case LDO: case LDOI: case LDOU: case LDOUI: case LDUNC: case LDUNCI:
 w.l&=-8;@+ ll=mem_find(w);
 test_load_bkpt(ll);@+test_load_bkpt(ll+1);
 x.h=ll->tet;@+ x.l=(ll+1)->tet;
 goto check_ld;
case LDSF: case LDSFI: ll=mem_find(w);@+test_load_bkpt(ll);
 x=load_sf(ll->tet);@+ goto check_ld;
@y
case LDO: case LDOI: case LDOU: case LDOUI: 
 w.l&=-8;@+ ll=mem_find(w);
 test_load_bkpt(ll);@+test_load_bkpt(ll+1);
 if (!MMIX_LDO(x,w)) goto page_fault;
 goto check_ld;
case LDUNC: case LDUNCI:
 w.l&=-8;@+ ll=mem_find(w);
 test_load_bkpt(ll);@+test_load_bkpt(ll+1);
 if (!MMIX_LDO_UNCACHED(x,w)) goto page_fault;
 goto check_ld;
case LDSF: case LDSFI: ll=mem_find(w);@+test_load_bkpt(ll);
 if (!MMIX_LDT(x,w)) goto page_fault;
 x=load_sf(x.l);@+ goto check_ld;
@z

Same for storing.

@x
case STB: case STBI: case STBU: case STBUI:@/
 i=56;@+j=(w.l&0x3)<<3; goto fin_pst;
@y
case STB: case STBI: case STBU: case STBUI:@/
 if ((op&0x2)==0) {
   a=shift_right(shift_left(b,56),56,0);
   if (a.h!=b.h || a.l!=b.l) exc|=V_BIT;
 }
 if ((w.h&sign_bit) && !(loc.h&sign_bit)) goto translation_bypassed_inst;
 if (!MMIX_STB(b,w)) goto page_fault;
 goto fin_st;
@z

@x
case STW: case STWI: case STWU: case STWUI:@/
 i=48;@+j=(w.l&0x2)<<3; goto fin_pst;
@y
case STW: case STWI: case STWU: case STWUI:@/
 if ((op&0x2)==0) {
   a=shift_right(shift_left(b,48),48,0);
   if (a.h!=b.h || a.l!=b.l) exc|=V_BIT;
 }
 if ((w.h&sign_bit) && !(loc.h&sign_bit)) goto translation_bypassed_inst;
 if (!MMIX_STW(b,w)) goto page_fault;
 goto fin_st;
@z

@x
case STT: case STTI: case STTU: case STTUI:@/
 i=32;@+j=0;
fin_pst: ll=mem_find(w);
 if ((op&0x2)==0) {
   a=shift_right(shift_left(b,i),i,0);
   if (a.h!=b.h || a.l!=b.l) exc|=V_BIT;
 }
 ll->tet^=(ll->tet^(b.l<<(i-32-j))) & ((((tetra)-1)<<(i-32))>>j);
 goto fin_st;
@y
case STT: case STTI: case STTU: case STTUI:@/
 if ((op&0x2)==0) {
   a=shift_right(shift_left(b,32),32,0);
   if (a.h!=b.h || a.l!=b.l) exc|=V_BIT;
 }
fin_pst:
 if ((w.h&sign_bit) && !(loc.h&sign_bit)) goto translation_bypassed_inst;
 if (!MMIX_STT(b,w)) goto page_fault;
fin_st:
 ll=mem_find(w); test_store_bkpt(ll);
#ifdef MMIX_MEM_TET
 w.l&=-8;@+ll=mem_find(w);
 a.h=ll->tet;@+ a.l=(ll+1)->tet; /* for trace output */
#endif
 break;
@z


@x
case STSF: case STSFI: ll=mem_find(w);
 ll->tet=store_sf(b);@+exc=exceptions;
 goto fin_st;
@y
case STSF: case STSFI: 
 a.l=b.l= store_sf(b);@+exc=exceptions;
 a.h=b.h=0;
 goto fin_pst;
@z

@x
case STHT: case STHTI: ll=mem_find(w);@+ ll->tet=b.h;
fin_st: test_store_bkpt(ll);
 w.l&=-8;@+ll=mem_find(w);
 a.h=ll->tet;@+ a.l=(ll+1)->tet; /* for trace output */
 goto check_st; 
@y
case STHT: case STHTI: 
  a.l=b.l=b.h;
  b.h=a.h=0;
  goto fin_pst;
@z

@x
case STCO: case STCOI: b.l=xx;
case STO: case STOI: case STOU: case STOUI: case STUNC: case STUNCI:
 w.l&=-8;@+ll=mem_find(w);
 test_store_bkpt(ll);@+ test_store_bkpt(ll+1);
 ll->tet=b.h;@+ (ll+1)->tet=b.l;
check_st:@+if (w.h&sign_bit) goto privileged_inst;
 break;
@y
case STCO: case STCOI: b.l=xx;
case STO: case STOI: case STOU: case STOUI:
 w.l&=-8;
 if ((w.h&sign_bit) && !(loc.h&sign_bit)) goto translation_bypassed_inst;
 if (!MMIX_STO(b,w)) goto page_fault;
 ll=mem_find(w); test_store_bkpt(ll);test_store_bkpt(ll+1);
 break;
case STUNC: case STUNCI:
 w.l&=-8;
 if ((w.h&sign_bit) && !(loc.h&sign_bit)) goto translation_bypassed_inst;
 if (!MMIX_STO_UNCACHED(b,w)) goto page_fault;
 ll=mem_find(w); test_store_bkpt(ll);test_store_bkpt(ll+1);
 break;
@z

@x
case CSWAP: case CSWAPI: w.l&=-8;@+ll=mem_find(w);
 test_load_bkpt(ll);@+test_load_bkpt(ll+1);
 a=g[rP];
 if (ll->tet==a.h && (ll+1)->tet==a.l) {
   x.h=0, x.l=1;
   test_store_bkpt(ll);@+test_store_bkpt(ll+1);
   ll->tet=b.h, (ll+1)->tet=b.l;
   strcpy(rhs,"M8[%#w]=%#b");
 }@+else {
   b.h=ll->tet, b.l=(ll+1)->tet;
   g[rP]=b;
   strcpy(rhs,"rP=%#b");
 }
@y
case CSWAP: case CSWAPI: w.l&=-8;@+ll=mem_find(w);
 test_load_bkpt(ll);@+test_load_bkpt(ll+1);
 if ((w.h&sign_bit) && !(loc.h&sign_bit)) goto  translation_bypassed_inst;
 if (!MMIX_LDO(a,w)) goto page_fault;
 if (g[rP].h==a.h && g[rP].l==a.l) {
   x.h=0, x.l=1;
   test_store_bkpt(ll);@+test_store_bkpt(ll+1);
   if (!MMIX_STO(b,w)) goto page_fault;
   strcpy(rhs,"M8[%#w]=%#b");
 }@+else {
   b=a;
   a = g[rP];
   g[rP]=b;
   x.h=0, x.l=0;
   strcpy(rhs,"rP=%#b");
 }
@z


@x
case GET:@+if (yy!=0 || zz>=32) goto illegal_inst;
  x=g[zz];
  goto store_x;
case PUTI: z.l=yz, yy=0; 
case PUT: if (yy!=0 || xx>=32) goto illegal_inst;
  strcpy(rhs,"%z = %#z");
  if (xx>=8) {
    if (xx<=11 && xx!=8) goto illegal_inst; /* can't change rN, rO, rS */
    if (xx<=18) goto privileged_inst;
    if (xx==rA) @<Get ready to update rA@>@;
    else if (xx==rL) @<Set $L=z=\min(z,L)$@>@;
    else if (xx==rG) @<Get ready to update rG@>;
  }
  g[xx]=z;@+zz=xx;@+break;
@y
case GET:@+if (yy!=0 || zz>=32) goto illegal_inst;
  x=g[zz];
  if (zz==rQ) { 
      new_Q.h = new_Q.l = 0;
  }
  goto store_x;
case PUTI: z.l=yz, yy=0; 
case PUT: if (yy!=0 || xx>=32) goto illegal_inst;
  strcpy(rhs,"%z = %#z");
  if (xx>=8) {
    if (xx==9) goto illegal_inst; /* can't change rN */
    if (xx<=18 && !(loc.h&sign_bit)) goto privileged_inst;
    if (xx==rA) @<Get ready to update rA@>@;
    else if (xx==rL) @<Set $L=z=\min(z,L)$@>@;
    else if (xx==rG) @<Get ready to update rG@>@;
    else if (xx==rQ)
    { new_Q.h |= z.h &~ g[rQ].h;@+
      new_Q.l |= z.l &~ g[rQ].l;
      z.l |= new_Q.l;@+
      z.h |= new_Q.h;@+
    }
  }
  g[xx]=z;@+zz=xx;@+break;
@z

@x
case PUSHGO: case PUSHGOI: inst_ptr=w;@+goto push;
case PUSHJ: case PUSHJB: inst_ptr=z;
push:@+if (xx>=G) {
   xx=L++;
   if (((S-O-L)&lring_mask)==0) stack_store();
 }
@y
case PUSHGO: case PUSHGOI: 
if ((w.h&sign_bit) && !(loc.h&sign_bit))
goto protection_violation;
inst_ptr=w;@+goto push;
case PUSHJ: case PUSHJB: 
if ((z.h&sign_bit) && !(loc.h&sign_bit))
goto protection_violation;  
inst_ptr=z;
push:@+if (xx>=G) {
   xx=L++;
   if (((S-O-L)&lring_mask)==0) stack_store(l[S&lring_mask]);
 }
@z

@x
 y=g[rJ];@+ z.l=yz<<2;@+ inst_ptr=oplus(y,z);
@y
 y=g[rJ];@+ z.l=yz<<2;
 { octa tmp;
   tmp=oplus(y,z);
   if ((tmp.h&sign_bit) && !(loc.h&sign_bit))
   goto protection_violation;  
   inst_ptr = tmp;
}
@z

@x
case SAVE:@+if (xx<G || yy!=0 || zz!=0) goto illegal_inst;
 l[(O+L)&lring_mask].l=L, L++;
 if (((S-O-L)&lring_mask)==0) stack_store();
@y
case SAVE:@+if (xx<G || yy!=0 || zz!=0) goto illegal_inst;
 l[(O+L)&lring_mask].l=L, L++;
 if (((S-O-L)&lring_mask)==0) stack_store(l[S&lring_mask]);
@z

@x
 while (g[rO].l!=g[rS].l) stack_store();
@y
 while (g[rO].l!=g[rS].l) stack_store(l[S&lring_mask]);
@z

@x
@<Store |g[k]| in the register stack...@>=
ll=mem_find(g[rS]);
if (k==rZ+1) x.h=G<<24, x.l=g[rA].l;
else x=g[k];
ll->tet=x.h;@+test_store_bkpt(ll);
(ll+1)->tet=x.l;@+test_store_bkpt(ll+1);
if (stack_tracing) {
  tracing=true;
  if (cur_line) show_line();
  if (k>=32) printf("             M8[#%08x%08x]=g[%d]=#%08x%08x, rS+=8\n",
            g[rS].h,g[rS].l,k,x.h,x.l);
  else printf("             M8[#%08x%08x]=%s=#%08x%08x, rS+=8\n",
            g[rS].h,g[rS].l,k==rZ+1? "(rG,rA)": special_name[k],x.h,x.l);
}
S++, g[rS]=incr(g[rS],8);
@y
@<Store |g[k]| in the register stack...@>=
if (k==rZ+1) x.h=G<<24, x.l=g[rA].l;
else x=g[k];
stack_store(x);
@z

@x
@ @<Load |g[k]| from the register stack@>=
g[rS]=incr(g[rS],-8);
ll=mem_find(g[rS]);
test_load_bkpt(ll);@+test_load_bkpt(ll+1);
if (k==rZ+1) {
  x.l=G=g[rG].l=ll->tet>>24, a.l=g[rA].l=(ll+1)->tet&0x3ffff;
  if (G<32) x.l=G=g[rG].l=32;
}@+else g[k].h=ll->tet, g[k].l=(ll+1)->tet;
if (stack_tracing) {
  tracing=true;
  if (cur_line) show_line();
  if (k>=32) printf("             rS-=8, g[%d]=M8[#%08x%08x]=#%08x%08x\n",
            k,g[rS].h,g[rS].l,ll->tet,(ll+1)->tet);
  else if (k==rZ+1) printf("             (rG,rA)=M8[#%08x%08x]=#%08x%08x\n",
            g[rS].h,g[rS].l,ll->tet,(ll+1)->tet);
  else printf("             rS-=8, %s=M8[#%08x%08x]=#%08x%08x\n",
            special_name[k],g[rS].h,g[rS].l,ll->tet,(ll+1)->tet);
}
@y
@ @<Load |g[k]| from the register stack@>=
g[rS]=incr(g[rS],-8);
ll=mem_find(g[rS]);
test_load_bkpt(ll);@+test_load_bkpt(ll+1);
if (k==rZ+1) { 
  if (!MMIX_LDO(a,g[rS])) { w=g[rS]; goto page_fault; }
  x.l=G=g[rG].l=a.h>>24, a.l=g[rA].l=a.l&0x3ffff;
  if (G<32) { x.l=G=g[rG].l=32;  goto illegal_inst; }
}
else
  if (!MMIX_LDO(g[k],g[rS]))  { w=g[rS]; goto page_fault; }
if (k>=32) mmix_stack_trace("             rS-=8, g[%d]=M8[#%08x%08x]=#%08x%08x\n",
          k,g[rS].h,g[rS].l,g[k].h,g[k].l);
else if (k==rZ+1) mmix_stack_trace("             (rG,rA)=M8[#%08x%08x]=#%08x%08x\n",
          g[rS].h,g[rS].l,a.h,a.l);
else mmix_stack_trace("             rS-=8, %s=M8[#%08x%08x]=#%08x%08x\n",
          special_name[k],g[rS].h,g[rS].l,g[k].h,g[k].l);
@z

@x
@ The cache maintenance instructions don't affect this simulation,
because there are no caches. But if the user has invoked them, we do
provide a bit of information when tracing, indicating the scope of the
instruction.

@<Cases for ind...@>=
case SYNCID: case SYNCIDI: case PREST: case PRESTI:
case SYNCD: case SYNCDI: case PREGO: case PREGOI:
case PRELD: case PRELDI: x=incr(w,xx);@+break;
@y
@ The cache maintenance instructions do affect this simulation.

@<Cases for ind...@>=
case SYNCID: case SYNCIDI:
 MMIX_DELETE_ICACHE(w,xx+1);
 if (loc.h&sign_bit)
   MMIX_DELETE_DCACHE(w,xx+1);
 else
   MMIX_STORE_DCACHE(w,xx+1);
 break;
case PREST: case PRESTI: x=incr(w,xx);@+break;
case SYNCD: case SYNCDI:  
 MMIX_STORE_DCACHE(w,xx+1);
 if (loc.h&sign_bit)
   MMIX_DELETE_DCACHE(w,xx+1);
 break;
case PREGO: case PREGOI:
 MMIX_PRELOAD_ICACHE(w,xx+1);
 break;
case PRELD: case PRELDI:
 MMIX_PRELOAD_DCACHE(w,xx+1);
 x=incr(w,xx);@+break;
@z

@x
case GO: case GOI: x=inst_ptr;@+inst_ptr=w;@+goto store_x;
case JMP: case JMPB: inst_ptr=z;
case SWYM: break;
case SYNC:@+if (xx!=0 || yy!=0 || zz>7) goto illegal_inst;
 if (zz<=3) break;
case LDVTS: case LDVTSI: privileged_inst: strcpy(lhs,"!privileged");
 goto break_inst;
illegal_inst: strcpy(lhs,"!illegal");
break_inst: breakpoint=tracing=true;
 if (!interacting && !interact_after_break) halted=true;
 break;
@y
case GO: case GOI: 
   if ((w.h&sign_bit) && !(loc.h&sign_bit))
   goto protection_violation;  
   x=inst_ptr;@+inst_ptr=w;@+goto store_x;
case JMP: case JMPB: 
   if ((z.h&sign_bit) && !(loc.h&sign_bit))
   goto protection_violation;  
   inst_ptr=z;@+break;
case SYNC:@+if (xx!=0 || yy!=0 || zz>7) goto illegal_inst;
/* should give a privileged instruction interrupt in case zz  >3 */
 else if (zz==4) /* power save mode */
 {  const unsigned int cycle_speed = 1000000; /* cycles per ms */
    const unsigned int max_wait = 100;
    int d, ms;
    if (g[rI].h!=0 || g[rI].l>max_wait*cycle_speed) /* large rI values */
      ms = max_wait;
    else
      ms = g[rI].l/cycle_speed;
    if (ms>0)
    {  MMIX_DELAY(ms,d); 
       g[rI]=incr(g[rI],-(ms-d)*cycle_speed);
     }
     else if (g[rI].l>1000)
        g[rI].l = g[rI].l-1000;
     else if (g[rI].l>100)
        g[rI].l = g[rI].l-100;
     else if (g[rI].l>10)
        g[rI].l = g[rI].l-10;
 }
 else if (zz==5) /* empty write buffer */
   MMIX_WRITE_DCACHE();
 else if (zz==6) /* clear VAT cache */
 {  MMIX_CLEAR_DVTC();
    MMIX_CLEAR_IVTC();
 }
 else if (zz==7) /* clear instruction and data cache */
 { MMIX_CLEAR_DCACHE();
   MMIX_CLEAR_ICACHE();
 }
 break;
case LDVTS: case LDVTSI:   
{ if (!(loc.h&sign_bit)) goto privileged_inst;
  if (w.h&sign_bit) goto illegal_inst;
  x = MMIX_UPDATE_VTC(w);
  goto store_x;
}
break;
case SWYM:
 if ((inst&0xFFFFFF)!=0) 
 {   char buf[256+1];
     int n;
     strcpy(rhs,"$%x,%z");
     z.h=0, z.l=yz;
     x.h=0, x.l=xx;
     tracing= interacting;
     breakpoint=(breakpoint&0xFF) | (yz<<8);
     interrupt=false;
     @<Set |b| from register X@>;
     n=mmgetchars((unsigned char *)buf,256,b,0);
     buf[n]=0;
     if (n>6 && strncmp(buf,"DEBUG ",6)==0) 
     { printf("\n%s!\n",buf+6);
       sprintf(rhs,"rF=#%08X%08X\n",g[rF].h, g[rF].l);
     }
 }
 else
   strcpy(rhs,"");
break;
translation_bypassed_inst: strcpy(lhs,"!negative address in load/store");
g[rQ].h |= N_BIT; new_Q.h |= N_BIT; /* set the n bit */
 goto break_inst;
privileged_inst: strcpy(lhs,"!privileged");
g[rQ].h |= K_BIT; new_Q.h |= K_BIT; /* set the k bit */
 goto break_inst;
illegal_inst: strcpy(lhs,"!illegal");
g[rQ].h |= B_BIT; new_Q.h |= B_BIT; /* set the b bit */
 goto break_inst;
protection_violation: strcpy(lhs,"!protected");
g[rQ].h |= P_BIT; new_Q.h |= P_BIT; /* set the p bit */
 goto break_inst;
security_inst: strcpy(lhs,"!insecure");
break_inst: tracing=true, breakpoint|=trace_bit;
 if (!interacting && !interact_after_break) halted=true;
break;
@z

@x
case TRAP:@+if (xx!=0 || yy>max_sys_call) goto privileged_inst;
@y
#ifndef MMIX_TRAP
case TRAP:@+if (xx!=0 || yy>max_sys_call) goto privileged_inst;
@z

@x
 x=g[255]=g[rBB];@+break;
@y
 x=g[255]=g[rBB];@+break;
#else
case TRAP:@+if (xx==0 && yy<=max_sys_call) 
      { strcpy(rhs,trap_format[yy]);
        a=incr(b,8);
        @<Prepare memory arguments $|ma|={\rm M}[a]$ and $|mb|={\rm M}[b]$ if needed@>;
      }
     else strcpy(rhs, "%#x -> %#y");
 x.h=sign_bit, x.l=inst;
 @<Initiate a trap interrupt@>
 inst_ptr=y=g[rT];
 break;
#endif
@z

This is the code for TRAP 0,Halt,0
@x
if (!zz) halted=breakpoint=true;
@y
if (!zz) halted=true, breakpoint|=trace_bit;
@z

@x
"$255 = Fopen(%!z,M8[%#b]=%#q,M8[%#a]=%p) = %x",
"$255 = Fclose(%!z) = %x",
"$255 = Fread(%!z,M8[%#b]=%#q,M8[%#a]=%p) = %x",
"$255 = Fgets(%!z,M8[%#b]=%#q,M8[%#a]=%p) = %x",
"$255 = Fgetws(%!z,M8[%#b]=%#q,M8[%#a]=%p) = %x",
"$255 = Fwrite(%!z,M8[%#b]=%#q,M8[%#a]=%p) = %x",
"$255 = Fputs(%!z,%#b) = %x",
"$255 = Fputws(%!z,%#b) = %x",
"$255 = Fseek(%!z,%b) = %x",
"$255 = Ftell(%!z) = %x"};
@y
#ifdef MMIX_TRAP
"$255 = Fopen(%!z,M8[%#b]=%#q,M8[%#a]=%p) -> %#y",
"$255 = Fclose(%!z) -> %#y",
"$255 = Fread(%!z,M8[%#b]=%#q,M8[%#a]=%p) -> %#y",
"$255 = Fgets(%!z,M8[%#b]=%#q,M8[%#a]=%p) -> %#y",
"$255 = Fgetws(%!z,M8[%#b]=%#q,M8[%#a]=%p) -> %#y",
"$255 = Fwrite(%!z,M8[%#b]=%#q,M8[%#a]=%p) -> %#y",
"$255 = Fputs(%!z,%#b) -> %#y",
"$255 = Fputws(%!z,%#b) -> %#y",
"$255 = Fseek(%!z,%b) -> %#y",
"$255 = Ftell(%!z) -> %#y"};
#else
"$255 = Fopen(%!z,M8[%#b]=%#q,M8[%#a]=%p) = %x",
"$255 = Fclose(%!z) = %x",
"$255 = Fread(%!z,M8[%#b]=%#q,M8[%#a]=%p) = %x",
"$255 = Fgets(%!z,M8[%#b]=%#q,M8[%#a]=%p) = %x",
"$255 = Fgetws(%!z,M8[%#b]=%#q,M8[%#a]=%p) = %x",
"$255 = Fwrite(%!z,M8[%#b]=%#q,M8[%#a]=%p) = %x",
"$255 = Fputs(%!z,%#b) = %x",
"$255 = Fputws(%!z,%#b) = %x",
"$255 = Fseek(%!z,%b) = %x",
"$255 = Ftell(%!z) = %x"};
#endif
@z



@x
@ @<Prepare memory arguments...@>=
if (arg_count[yy]==3) {
  ll=mem_find(b);@+test_load_bkpt(ll);@+test_load_bkpt(ll+1);
  mb.h=ll->tet, mb.l=(ll+1)->tet;
  ll=mem_find(a);@+test_load_bkpt(ll);@+test_load_bkpt(ll+1);
  ma.h=ll->tet, ma.l=(ll+1)->tet;
}
@y
@ @<Prepare memory arguments...@>=
if (arg_count[yy]==3) {
   MMIX_LDO(mb,b);
   MMIX_LDO(ma,a);
}
@z

@x
@ The input/output operations invoked by \.{TRAP}s are
done by subroutines in an auxiliary program module called {\mc MMIX-IO}.
Here we need only declare those subroutines, and write three primitive
interfaces on which they depend.

@ @<Glob...@>=
@y
@ @(mmix-io.h@>=
@z

@x
@<Sub...@>=
int mmgetchars @,@,@[ARGS((char*,int,octa,int))@];@+@t}\6{@>
int mmgetchars(buf,size,addr,stop)
  char *buf;
  int size;
  octa addr;
  int stop;
{
  register char *p;
  register int m;
  register mem_tetra *ll;
  register tetra x;
  octa a;
  for (p=buf,m=0,a=addr; m<size;) {
    ll=mem_find(a);@+test_load_bkpt(ll);
    x=ll->tet;
    if ((a.l&0x3) || m>size-4) @<Read and store one byte; |return| if done@>@;
    else @<Read and store up to four bytes; |return| if done@>@;
  }
  return size;
}
@y
@(libmmget.c@>=
#include <stdio.h>
#include <setjmp.h>
#include "libconfig.h"
#include <time.h>
#include "libtype.h"
#include "libglobals.h"
#include "mmixlib.h"
#include "libarith.h"
#include "libimport.h"

int mmgetchars(buf,size,addr,stop)
  unsigned char *buf;
  int size;
  octa addr;
  int stop;
{
  register unsigned char *p;
  register int m;
  register mem_tetra *ll;
  octa a;
  for (p=buf,m=0,a=addr; m<size;) {
    ll=mem_find(a);@+test_load_bkpt(ll);
    if ((a.l&0x3) || m>size-4) @<Read and store one byte; |return| if done@>@;
    else @<Read and store up to four bytes; |return| if done@>@;
  }
  return size;
}
@z

@x
@ @<Read and store one byte...@>=
{
  *p=(x>>(8*((~a.l)&0x3)))&0xff;
  if (!*p && stop>=0) {
    if (stop==0) return m;
    if ((a.l&0x1) && *(p-1)=='\0') return m-1;
  }
  p++,m++,a=incr(a,1);
}

@ @<Read and store up to four bytes...@>=
{
  *p=x>>24;
  if (!*p && (stop==0 || (stop>0 && x<0x10000))) return m;
  *(p+1)=(x>>16)&0xff;
  if (!*(p+1) && stop==0) return m+1;
  *(p+2)=(x>>8)&0xff;
  if (!*(p+2) && (stop==0 || (stop>0 && (x&0xffff)==0))) return m+2;
  *(p+3)=x&0xff;
  if (!*(p+3) && stop==0) return m+3;
  p+=4,m+=4,a=incr(a,4);
}
@y
@ @<Read and store one byte...@>=
{ MMIX_LDB(x,a);
    *p=x.l&0xff;
  if (!*p && stop>=0) {
    if (stop==0) return m;
    if ((a.l&0x1) && *(p-1)=='\0') return m-1;
  }
  p++,m++,a=incr(a,1);
}

@ @<Read and store up to four bytes...@>=
{ MMIX_LDT(x,a);
  *p=x.l>>24;
  if (!*p && (stop==0 || (stop>0 && x.l<0x10000))) return m;
  *(p+1)=(x.l>>16)&0xff;
  if (!*(p+1) && stop==0) return m+1;
  *(p+2)=(x.l>>8)&0xff;
  if (!*(p+2) && (stop==0 || (stop>0 && (x.l&0xffff)==0))) return m+2;
  *(p+3)=x.l&0xff;
  if (!*(p+3) && stop==0) return m+3;
  p+=4,m+=4,a=incr(a,4);
}
@z

@x      
@ The subroutine |mmputchars(buf,size,addr)| puts |size| characters
into the simulated memory starting at address |addr|.

@<Sub...@>=
void mmputchars @,@,@[ARGS((unsigned char*,int,octa))@];@+@t}\6{@>
void mmputchars(buf,size,addr)
  unsigned char *buf;
  int size;
  octa addr;
{
  register unsigned char *p;
  register int m;
  register mem_tetra *ll;
  octa a;
  for (p=buf,m=0,a=addr; m<size;) {
    ll=mem_find(a);@+test_store_bkpt(ll);
    if ((a.l&0x3) || m>size-4) @<Load and write one byte@>@;
    else @<Load and write four bytes@>;
  }
}
@y
@ The subroutine |mmputchars(buf,size,addr)| puts |size| characters
into the simulated memory starting at address |addr|.

@(libmmput.c@>=
#include <stdio.h>
#include <setjmp.h>
#include "libconfig.h"
#include <time.h>
#include "libtype.h"
#include "libglobals.h"
#include "mmixlib.h"
#include "libarith.h"
#include "libimport.h"

void mmputchars(unsigned char *buf,int size,octa addr)
{
  register unsigned char *p;
  register int m;
  register mem_tetra *ll;
  octa a;
  for (p=buf,m=0,a=addr; m<size;) {
    ll=mem_find(a);@+test_store_bkpt(ll);
    if ((a.l&0x3) || m>size-4) @<Load and write one byte@>@;
    else @<Load and write four bytes@>;
  }
}
@z

@x
@ @<Load and write one byte@>=
{
  register int s=8*((~a.l)&0x3);
  ll->tet^=(((ll->tet>>s)^*p)&0xff)<<s;
  p++,m++,a=incr(a,1);
}

@ @<Load and write four bytes@>=
{
  ll->tet=(*p<<24)+(*(p+1)<<16)+(*(p+2)<<8)+*(p+3);
  p+=4,m+=4,a=incr(a,4);
}
@y
@ @<Load and write one byte@>=
{ octa x;
  x.l=*p;
  x.h=0;
  MMIX_STB(x,a);
  p++,m++,a=incr(a,1);
}

@ @<Load and write four bytes@>=
{ octa x;
  x.h=0;
  x.l=(*p<<24)+(*(p+1)<<16)+(*(p+2)<<8)+*(p+3);
  MMIX_STT(x,a);
  p+=4,m+=4,a=incr(a,4);
}
@z

The next function is used for mmixware ftraps

@x
@<Sub...@>=
char stdin_chr @,@,@[ARGS((void))@];@+@t}\6{@>
@y
@(libstdin.c@>=
#include <stdlib.h>
#include <stdio.h>
#include <setjmp.h>
#include "libconfig.h"
#include <time.h>
#include "libtype.h"
#include "libglobals.h"
#include "mmixlib.h"
#include "libimport.h"


char stdin_chr @,@,@[ARGS((void))@];@+@t}\6{@>
@z


@x
@ We are finally ready for the last case.
@y
@ We do similar things for a trap interrupt.

Interrupt bits in rQ might be lost if they are set between a \.{GET}
and a~\.{PUT}. Therefore we don't allow \.{PUT} to zero out bits that
have become~1 since the most recently committed \.{GET}.

@<Glob...@>=
octa new_Q; /* when rQ increases in any bit position, so should this */

@ Now we can implement external interrupts.

@<Check for trap interrupt@>=
if (!resuming)
{ 
  MMIX_GET_INTERRUPT;
  if ((g[rK].h & g[rQ].h) != 0 || (g[rK].l & g[rQ].l) != 0) 
  { /*this is a dynamic trap */
    x.h=sign_bit, x.l=inst;
    if (tracing)
      printf("Dynamic TRAP: rQ=%08x%08x rK=%08x%08x\n",
            g[rQ].h, g[rQ].l, g[rK].h, g[rK].l);
    @<Initiate a trap interrupt@>
    inst_ptr=y=g[rTT];
    return true;
  }
}

@ An instruction will not be executed if it violates the basic
security rule of \MMIX: An instruction in a nonnegative location
should not be performed unless all eight of the internal interrupts
have been enabled in the interrupt mask register~rK.
Conversely, an instruction in a negative location should not be performed
if the |P_BIT| is enabled in~rK.

The nonnegative-location case turns on the |S_BIT| of both rK and~rQ\null,
leading to an immediate interrupt.

@<Check for security violation@>=
{
  if (inst_ptr.h&sign_bit)
  { if (g[rK].h&P_BIT) 
    { g[rQ].h |= P_BIT;
      new_Q.h |= P_BIT;
      goto security_inst;
    }
  }
  else
  { if ((g[rK].h&0xff)!=0xff)
    { g[rQ].h |= S_BIT;
      new_Q.h |= S_BIT;
      g[rK].h |= S_BIT;
      goto security_inst;
    }
  }
}


@ Here are the bit codes that affect traps. The first eight
cases apply to the upper half of~rQ the next eight to the lower half.

@d P_BIT (1<<0) /* instruction in privileged location */
@d S_BIT (1<<1) /* security violation */
@d B_BIT (1<<2) /* instruction breaks the rules */
@d K_BIT (1<<3) /* instruction for kernel only */
@d N_BIT (1<<4) /* virtual translation bypassed */
@d PX_BIT (1<<5) /* permission lacking to execute from page */
@d PW_BIT (1<<6) /* permission lacking to write on page */
@d PR_BIT (1<<7) /* permission lacking to read from page */

@d PF_BIT (1<<0) /* power fail */
@d MP_BIT (1<<1) /* memory parity error */
@d NM_BIT (1<<2) /* non existent memory */
@d YY_BIT (1<<3) /* unassigned */
@d RE_BIT (1<<4) /* rebooting */
@d CP_BIT (1<<5) /* page fault */
@d PT_BIT (1<<6) /* page table error */
@d IN_BIT (1<<7) /* interval counter rI reaches zero */

@ We need this:
    
@ @<Initiate a trap interrupt@>=
 g[rWW]=inst_ptr;
 g[rXX]=x;
 g[rYY]=y;
 g[rZZ]=z;
 z.h=0, z.l=zz;
 g[rK].h = g[rK].l = 0;
 g[rBB]=g[255];
 g[255]=g[rJ];

@ We are finally ready for the last case.
@z

@x
case RESUME:@+if (xx || yy || zz) goto illegal_inst;
inst_ptr=z=g[rW];
b=g[rX];
if (!(b.h&sign_bit)) @<Prepare to perform a ropcode@>;
break;
@y
case RESUME:@+if (xx || yy) goto illegal_inst;
rzz=zz;
if ( rzz == 0)
{ if (!(loc.h&sign_bit) && (g[rW].h&sign_bit)) 
  goto protection_violation;
  inst_ptr=z=g[rW];
  b=g[rX];
}
else if ( rzz == 1)
{ 
  if (!(loc.h&sign_bit)) goto privileged_inst;
  inst_ptr=z=g[rWW];
  b=g[rXX];
  g[rK]=g[255];
  x=g[255]=g[rBB];
  @<Check for security violation@>
}
else goto illegal_inst;
if (!(b.h&sign_bit)) @<Prepare to perform a ropcode@>
break;
@z


@x
@d RESUME_AGAIN 0 /* repeat the command in rX as if in location $\rm rW-4$ */
@d RESUME_CONT 1 /* same, but substitute rY and rZ for operands */
@d RESUME_SET 2 /* set register \$X to rZ */
@y
@d RESUME_AGAIN 0 /* repeat the command in rX as if in location $\rm rW-4$ */
@d RESUME_CONT 1 /* same, but substitute rY and rZ for operands */
@d RESUME_SET 2 /* set register \$X to rZ */
@d RESUME_TRANS 3 /* install $\rm(rY,rZ)$ into IT-cache or DT-cache,
        then |RESUME_AGAIN| */
@z

@x
@<Prepare to perform a ropcode@>=
{
  rop=b.h>>24; /* the ropcode is the leading byte of rX */
  switch (rop) {
 case RESUME_CONT:@+if ((1<<(b.l>>28))&0x8f30) goto illegal_inst;
 case RESUME_SET: k=(b.l>>16)&0xff;
   if (k>=L && k<G) goto illegal_inst;
 case RESUME_AGAIN:@+if ((b.l>>24)==RESUME) goto illegal_inst;
   break;
 default: goto illegal_inst;
  }
  resuming=true;
}

@ @<Install special operands when resuming an interrupted operation@>=
if (rop==RESUME_SET) {
    op=ORI;
    y=g[rZ];
    z=zero_octa;
    exc=g[rX].h&0xff00;
    f=X_is_dest_bit;
}@+else { /* |RESUME_CONT| */
  y=g[rY];
  z=g[rZ];
}
@y
@<Prepare to perform a ropcode@>=
{
  rop=b.h>>24; /* the ropcode is the leading byte of rX */
  switch (rop) {
 case RESUME_CONT:@+if ((1<<(b.l>>28))&0x8f30) goto illegal_inst;
 case RESUME_SET: k=(b.l>>16)&0xff;
   if (k>=L && k<G) goto illegal_inst;
 case RESUME_AGAIN:@+if ((b.l>>24)==RESUME) goto illegal_inst;
   break;
 case RESUME_TRANS:@+if (rzz==0) goto illegal_inst;
   break;
 default: goto illegal_inst;
  }
  resuming=true;
}

@ @<Install special operands when resuming an interrupted operation@>=
if (rzz == 0)
{ if (rop==RESUME_SET) {
    op=ORI;
    y=g[rZ];
    z=zero_octa;
    exc=g[rX].h&0xff00;
    f=X_is_dest_bit;
  }@+else if (rop == RESUME_CONT) {
  y=g[rY];
  z=g[rZ];
  }
}
else
{ if (rop==RESUME_SET) {
    op=ORI;
    y=g[rZZ];
    z=zero_octa;
    exc=g[rXX].h&0xff00;
    f=X_is_dest_bit;
  } else if (rop==RESUME_TRANS)
  {  if ((b.l>>24)==SWYM) 
       MMIX_STORE_IVTC(g[rYY], g[rZZ]);
     else
       MMIX_STORE_DVTC(g[rYY], g[rZZ]);
  }@+else if (rop == RESUME_CONT) {
  y=g[rYY];
  z=g[rZZ];
  }
}
@z

@x
  if (g[rI].l<=info[op].oops && g[rI].l && g[rI].h==0) tracing=breakpoint=true;
@y
  if (g[rI].l<=info[op].oops && g[rI].l && g[rI].h==0) 
    g[rQ].l |= IN_BIT, new_Q.l |= IN_BIT; 
@z


@x
  ll=mem_find(loc);
  printf("%10d. %08x%08x: %08x (%s) ",ll->freq,loc.h,loc.l,inst,info[op].name);
@y
  printf("%10d. %08x%08x: %08x (%s) ",cur_freq,loc.h,loc.l,inst,info[inst>>24].name);
@z


@x
@<Print a stream-of-consciousness description of the instruction@>=
if (lhs[0]=='!') printf("%s instruction!\n",lhs+1); /* privileged or illegal */
@y
@<Print a stream-of-consciousness description of the instruction@>=
if (lhs[0]=='!') { printf("%s instruction!\n",lhs+1); /* privileged or illegal */
  lhs[0]='\0';
}
@z

@x
@ @<Sub...@>=
fmt_style style;
char *stream_name[]={"StdIn","StdOut","StdErr"};
@.StdIn@>
@.StdOut@>
@.StdErr@>
@#
void trace_print @,@,@[ARGS((octa))@];@+@t}\6{@>
@y
@ @(libtrace.c@>=
#include <stdio.h>
#include <setjmp.h>
#include <stdarg.h>
#include "libconfig.h"
#include <time.h>
#include "libtype.h"
#include "libglobals.h"
#include "mmixlib.h"
#include "libarith.h"
#include "libimport.h"

void mmix_stack_trace(char *format,...)
{ 
  if (stack_tracing) {
    va_list vargs;
    tracing=true;
    va_start(vargs,format);	
    vprintf(format, vargs);
  }
}


static fmt_style style;
static char *stream_name[]={"StdIn","StdOut","StdErr"};
@.StdIn@>
@.StdOut@>
@.StdErr@>
@#
void trace_print @,@,@[ARGS((octa))@];@+@t}\6{@>
@z
  
  

@x
char switchable_string[48]; /* holds |rhs|; position 0 is ignored */
 /* |switchable_string| must be able to hold any |trap_format| */
@y
char switchable_string[300] ={0}; /* holds |rhs|; position 0 is ignored */
 /* |switchable_string| must be able to hold any debug message */
@z

@x
@ @<Sub...@>=
void show_stats @,@,@[ARGS((bool))@];@+@t}\6{@>
void show_stats(verbose)
  bool verbose;
@y
@ @(libstats.c@>=
#include <stdio.h>
#include <setjmp.h>
#include "libconfig.h"
#include <time.h>
#include "libtype.h"
#include "libglobals.h"
#include "mmixlib.h"
#include "libarith.h"
#include "libimport.h"

void show_stats @,@,@[ARGS((bool))@];@+@t}\6{@>
void show_stats(verbose)
  bool verbose;
@z


@x
@* Running the program. Now we are ready to fit the pieces together into a
working simulator.

@c
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <signal.h>
#include "abstime.h"
@<Preprocessor macros@>@;
@<Type declarations@>@;
@<Global variables@>@;
@<Subroutines@>@;
@#
int main(argc,argv)
  int argc;
  char *argv[];
{
  @<Local registers@>;
  mmix_io_init();
  @<Process the command line@>;
  @<Initialize everything@>;
  @<Load the command line arguments@>;
  @<Get ready to \.{UNSAVE} the initial context@>;
  while (1) {
    if (interrupt && !breakpoint) breakpoint=interacting=true, interrupt=false;
    else {
      breakpoint=false;
      if (interacting) @<Interact with the user@>;
    }
    if (halted) break;
    do @<Perform one instruction@>@;
    while ((!interrupt && !breakpoint) || resuming);
    if (interact_after_break) interacting=true, interact_after_break=false;
  }
 end_simulation:@+if (profiling) @<Print all the frequency counts@>;
  if (interacting || profiling || showing_stats) show_stats(true);
  return g[255].l; /* provide rudimentary feedback for non-interactive runs */
}
@y
@* Making the library. Now we are ready to write the different pieces of
a working simulator to separate files of a library.

@(liblibinit.c@>=
#include <stdlib.h>
#include <stdio.h>
#include <setjmp.h>
#include "libconfig.h"
#include <time.h>
#include "libtype.h"
#include "libglobals.h"
#include "mmixlib.h"
#include "libimport.h"

#ifndef MMIX_TRAP
#include "mmix-io.h"
#endif

int mmix_lib_initialize(void)
{
   @<Set up persistent data@>;
#ifndef MMIX_TRAP
   mmix_io_init();
#endif
   return 0;
}

@ @(libinit.c@>=
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <setjmp.h>
#include "libconfig.h"
#include <time.h>
#include "libtype.h"
#include "libglobals.h"
#include "mmixlib.h"
#include "libarith.h"
#include "libimport.h"

#ifdef WIN32
#include <windows.h>

BOOL CtrlHandler(DWORD fdwCtrlType);
#else
void catchint(int n);
#endif


int mmix_initialize(void)
{ 
  @<Initialize everything@>;
  return 0;
}

@ @(libboot.c@>=
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <setjmp.h>
#include "abstime.h"
#include "libconfig.h"
#include <time.h>
#include "libtype.h"
#include "libglobals.h"
#include "mmixlib.h"
#include "libarith.h"
#include "libimport.h"

extern void dump(mem_node *p);
extern void dump_tet(tetra t);

void mmix_boot(void)
{ 
  @<Boot the machine@>;
  MMIX_CLEAR_DVTC();
  MMIX_CLEAR_IVTC();
  MMIX_CLEAR_ICACHE();
  MMIX_CLEAR_DCACHE();
#ifdef MMIX_BOOT
  loc.h=inst_ptr.h=0x80000000;
  loc.l=inst_ptr.l=0x00000000;
  g[rJ].h=g[rJ].l =0xFFFFFFFF;
  resuming=false;
#else
  @<Get ready to \.{UNSAVE} the initial context@>
#endif
}


@ @(libload.c@>=
int mmix_load_file(char *mmo_file_name)
{ int j; /* miscellaneous indices */
  mem_tetra *ll; /* current place in the simulated memory */
  char *p; /* current place in a string */
  static char filename[FILENAME_MAX];
  static int ybyte2file[256];
  free_file_info();
  for (j=0;j<256;j++) ybyte2file[j]=-1;
  postamble=0;
  if (mmo_file_name!=NULL && mmo_file_name[0]!=0)
  { 
   @<Load object file@>;
   MMIX_WRITE_DCACHE();
   MMIX_CLEAR_ICACHE();
  }
  return 0;
}

@ @(libcommand.c@>=
#include <stdio.h>
#include <string.h>
#include <setjmp.h>
#include "libconfig.h"
#include <time.h>
#include "libtype.h"
#include "libglobals.h"
#include "mmixlib.h"
#include "libarith.h"
#include "libimport.h"

int mmix_commandline(int argc, char *argv[])
{ int k;
  MMIX_LOCAL_LL
  @<Load the command line arguments@>;
  g[rQ].h=g[rQ].l=new_Q.h=new_Q.l=0; /*hide problems with loading the command line*/
  return 0;
}

@ @(libinteract.c@>=
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <setjmp.h>
#include "libconfig.h"
#include <time.h>
#include "libtype.h"
#include "libglobals.h"
#include "mmixlib.h"
#include "libarith.h"
#include "libimport.h"

@<Interact globals@>

static octa scan_hex(char *s, octa offset);

void mmix_interact(void)      
/* return zero to end the simulation */
{ int j,k; /* miscellaneous indices */
  mem_tetra *ll; /* current place in the simulated memory */
  char *p; /* current place in a string */
  @<Interact with the user@>;
  return;
end_simulation:
  halted=true;
}

@ @(libfetch.c@>=
#include <stdio.h>
#include <string.h>
#include <setjmp.h>
#include "libconfig.h"
#include <time.h>
#include "libtype.h"
#include "libglobals.h"
#include "mmixlib.h"
#include "libarith.h"
#include "libimport.h"

int mmix_fetch_instruction(void)
/* return zero if no instruction was loaded */
{ mem_tetra *ll; /* current place in the simulated memory */
  @<Fetch the next instruction@>;
  op=inst>>24;@+xx=(inst>>16)&0xff;@+yy=(inst>>8)&0xff;@+zz=inst&0xff;
  f=info[op].flags;@+yz=inst&0xffff;
  x=y=z=a=b=zero_octa;@+ exc=0;@+ old_L=L;
  return 1;

protection_violation: 
  strcpy(lhs,"!protected");
  g[rQ].h |= P_BIT; new_Q.h |= P_BIT; /* set the p bit */
  return 0;

security_inst: 
  strcpy(lhs,"!insecure");
  return 0;
  
page_fault: 
  strcpy(lhs,"!not fetched");
  return 0;
}

@ @(libperform.c@>=
#include <stdio.h>
#include <string.h>
#ifdef WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif
#include "libconfig.h"
#include <time.h>
#include <setjmp.h>
#include "libtype.h"
#include "libglobals.h"
#include "mmixlib.h"
#include "libarith.h"
#include "mmix-io.h"
#include "libimport.h"

@<Stack store@>@;
@<Stack load@>@;
@<Register truth@>@;

int mmix_perform_instruction(void)  
{ @<Local registers@>;
  @<Perform one instruction@>@;
  return 1;
}

@ @(libtrace.c@>=

void mmix_trace_fetch(void)
{ @<Print the frequency count, the location, and the instruction@>;
}

void mmix_trace_perform(void)
{   char *p; /* current place in a string */
 @<Print a stream-of-consciousness description of the instruction@>;
}

void mmix_trace(void)
{ if (tracing) {
  if (showing_source && cur_line) show_line();
  mmix_trace_fetch();
  mmix_trace_perform();
  if (showing_stats || breakpoint) show_stats(breakpoint);
  just_traced=true;
}@+else if (just_traced) {
  printf(" ...............................................\n");
  just_traced=false;
  shown_line=-gap-1; /* gap will not be filled */
  }
} 

@ @(libdtrap.c@>=
#include <stdio.h>
#include <setjmp.h>
#include "libconfig.h"
#include <time.h>
#include "libtype.h"
#include "libglobals.h"
#include "mmixlib.h"
#include "libimport.h"


bool mmix_dynamic_trap(void)
{  
   @<Check for trap interrupt@>;
   return false;
}

@ @(libprofile.c@>=


void mmix_profile(void)
{
  @<Print all the frequency counts@>;
}


@ @(libfinal.c@>=
#include <stdio.h>
#include <setjmp.h>
#include "libconfig.h"
#include <time.h>
#include "libtype.h"
#include "libglobals.h"
#include "mmixlib.h"
#include "libimport.h"

int mmix_finalize(void)
{ free_file_info();
  return 0;
}

@ @(liblibfinal.c@>=
#include <stdio.h>
#include <setjmp.h>
#include "libconfig.h"
#include <time.h>
#include "libtype.h"
#include "libglobals.h"
#include "mmixlib.h"
#include "libimport.h"

int mmix_lib_finalize(void)
{ return 0;
}


@ @(libglobals.c@>=
#include <stdio.h>
#include <time.h>
#include <setjmp.h>
#include "libconfig.h"
#include "libtype.h"
#include "libglobals.h"
#include "mmixlib.h"

@<Global variables@>@;
jmp_buf mmix_exit;

@ @c
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <signal.h>
#include <setjmp.h>
#include "abstime.h"
#include "libconfig.h"
#include <time.h>
#include "libtype.h"
#include "libglobals.h"
#include "mmixlib.h"


int main(argc,argv)
  int argc;
  char *argv[];
{
  char **boot_cur_arg;
  int boot_argc;
  mmix_lib_initialize();
  g[255].h=0;
  g[255].l=setjmp(mmix_exit);
  if (g[255].l!=0)
   goto end_simulation;
  @<Process the command line@>;
  
  mmix_initialize();

  boot_cur_arg = cur_arg;
  boot_argc = argc;
boot:
  argc = boot_argc;
  cur_arg = boot_cur_arg;
  
  mmix_boot();
  
  mmix_load_file(*cur_arg);
  mmix_commandline(argc, argv);
  breakpoint=0;
  if (interacting) mmix_interact();
  while (true) {
    if (interrupt && !breakpoint) breakpoint|=trace_bit, interacting=true, interrupt=false;
    else
#ifdef MMIX_TRAP 
    if (!(inst_ptr.h&sign_bit) || show_operating_system)
#endif
    { breakpoint=0;
      if (interacting) mmix_interact();
    }
    if (halted) break;
    do   
    { if (!resuming)
        mmix_fetch_instruction();       
      mmix_perform_instruction();
      mmix_trace();
      mmix_dynamic_trap();
      if (resuming && op!=RESUME) resuming=false; 
    } while (resuming || (!interrupt && !breakpoint));
    if (interact_after_break) 
       interacting=true, interact_after_break=false;
    if (g[rQ].l&g[rK].l&RE_BIT)
    { breakpoint|=trace_bit; 
      goto boot;
    }
  }
  end_simulation:@+if (profiling) mmix_profile();
  if (interacting || profiling || showing_stats) show_stats(true);
  mmix_finalize();
  return g[255].l; /* provide rudimentary feedback for non-interactive runs */
}  
@z

mmo_file_name may become a variable not an alias.

@x
@d mmo_file_name *cur_arg
@y
@z

@x
if (!*cur_arg) scan_option("?",true); /* exit with usage note */
argc -= cur_arg-argv; /* this is the |argc| of the user program */
@y
MMIX_NO_FILE;
argc -= (int)(cur_arg-argv); /* this is the |argc| of the user program */
@z

@x
@<Subr...@>=
void scan_option @,@,@[ARGS((char*,bool))@];@+@t}\6{@>
@y
@(libsoption.c@>=
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <setjmp.h>
#include "libconfig.h"
#include <time.h>
#include "libtype.h"
#include "libglobals.h"
#include "mmixlib.h"
#include "mmix-io.h"
#include "libimport.h"




void scan_option @,@,@[ARGS((char*,bool))@];@+@t}\6{@>
@z

@x
 case 'b':@+if (sscanf(arg+1,"%d",&buf_size)!=1) buf_size=0;@+return;
@y
 case 'b':@+if (sscanf(arg+1,"%d",&buf_size)!=1) buf_size=0;@+return;
#ifdef MMIX_TRAP
 case 'O': show_operating_system=true;@+return; 
 case 'o': show_operating_system=false;@+return;
#endif 
 MMIX_OPTIONS
@z

we need to replace all exits.

@x
    exit(-1);
@y
    longjmp(mmix_exit,-1);
@z    

@x
bool interrupt; /* has the user interrupted the simulation recently? */
bool profiling; /* should we print the profile at the end? */
@y
bool interrupt=false; /* has the user interrupted the simulation recently? */
bool profiling=false; /* should we print the profile at the end? */
@z

@x
"S         set current segment to Stack_Segment\n",@|
@y
"S         set current segment to Stack_Segment\n",@|
"N         set current segment to Negative Addresses\n",@|
"O         enable tracing inside the operating system\n",@|
"o         disable tracing inside the operating system\n",@|
@z




@x
@ @<Initialize...@>=
signal(SIGINT,catchint); /* now |catchint| will catch the first interrupt */

@ @<Subr...@>=
void catchint @,@,@[ARGS((int))@];@+@t}\6{@>
void catchint(n)
  int n;
{ if (n!=SIGINT) return;
  interrupt=true;
  signal(SIGINT,catchint); /* now |catchint| will catch the next interrupt */
}
@y
@ @<Initialize...@>=
#ifdef WIN32
SetConsoleCtrlHandler( (PHANDLER_ROUTINE) CtrlHandler, TRUE );
#else
signal(SIGINT,catchint); /* now |catchint| will catch the first interrupt */
#endif

@ @(libinit.c@>=
#ifdef WIN32
BOOL CtrlHandler( DWORD fdwCtrlType ) 
{ MMIX_CTRL_HANDLER;
  interrupt=true;
  if (fdwCtrlType==CTRL_C_EVENT || fdwCtrlType==CTRL_BREAK_EVENT )
  { 
   /* do nothing */
  }
  else
  { 
    interrupt=halted=true;
  }
  return TRUE;
}
#else
void catchint @,@,@[ARGS((int))@];@+@t}\6{@>
void catchint(n)
  int n;
{ MMIX_CTRL_HANDLER;  
  interrupt=true;
  signal(SIGINT,catchint); /* now |catchint| will catch the next interrupt */
}
#endif
@z

@x
 interact: @<Put a new command in |command_buf|@>;
@y
 interact: @<Put a new command in |command_buf|@>;
 MMIX_GET_INTERRUPT;  
@z

@x
  case '\n': case 'n': breakpoint=tracing=true; /* trace one inst and break */
@y
  case '\n': case 'n': breakpoint|=trace_bit,tracing=true; /* trace one inst and break */
@z


@x
  case '-': k=strlen(p);@+if (p[k-1]=='\n') p[k-1]='\0';
@y
  case '-': k=(int)strlen(p);@+if (p[k-1]=='\n') p[k-1]='\0';
@z

@x
  default: what_say: k=strlen(command_buf);
@y
  default: what_say: k=(int)strlen(command_buf);
@z

@x
@ @d command_buf_size 1024 /* make it plenty long, for floating point tests */

@<Glob...@>=
char command_buf[command_buf_size];
FILE *incl_file; /* file of commands included by `\.i' */
char cur_disp_mode='l'; /* |'l'| or |'g'| or |'$'| or |'M'| */
char cur_disp_type='!'; /* |'!'| or |'.'| or |'#'| or |'"'| */
bool cur_disp_set; /* was the last \.{<t>} of the form \.{=<val>}? */
octa cur_disp_addr; /* the |h| half is relevant only in mode |'M'| */
octa cur_seg; /* current segment offset */
char spec_reg_code[]={rA,rB,rC,rD,rE,rF,rG,rH,rI,rJ,rK,rL,rM,
      rN,rO,rP,rQ,rR,rS,rT,rU,rV,rW,rX,rY,rZ};
char spec_regg_code[]={0,rBB,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,rTT,0,0,rWW,rXX,rYY,rZZ};
@y
@ @d command_buf_size 1024 /* make it plenty long, for floating point tests */

@<Interact globals@>=
static char command_buf[command_buf_size];
static FILE *incl_file; /* file of commands included by `\.i' */
static char cur_disp_mode='l'; /* |'l'| or |'g'| or |'$'| or |'M'| */
static char cur_disp_type='!'; /* |'!'| or |'.'| or |'#'| or |'"'| */
static bool cur_disp_set; /* was the last \.{<t>} of the form \.{=<val>}? */
static octa cur_disp_addr; /* the |h| half is relevant only in mode |'M'| */
static octa cur_seg={0,0}; /* current segment offset */
static char spec_reg_code[]={rA,rB,rC,rD,rE,rF,rG,rH,rI,rJ,rK,rL,rM,
      rN,rO,rP,rQ,rR,rS,rT,rU,rV,rW,rX,rY,rZ};
static char spec_regg_code[]={0,rBB,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,rTT,0,0,rWW,rXX,rYY,rZZ};
@z

@x
@ @<Subr...@>=
octa scan_hex @,@,@[ARGS((char*,octa))@];@+@t}\6{@>
@y
@ @(libinteract.c@>=
octa scan_hex @,@,@[ARGS((char*,octa))@];@+@t}\6{@>
@z

@x
 case 'M':@+if (!(cur_disp_addr.h&sign_bit)) {
    ll=mem_find(cur_disp_addr);
    ll->tet=val.h;@+ (ll+1)->tet=val.l;
  }@+break;
@y
 case 'M':
  MMIX_STO(val,cur_disp_addr);
  @+break;
@z

@x
 case 'M':@+if (cur_disp_addr.h&sign_bit) aux=zero_octa;
  else {
    ll=mem_find(cur_disp_addr);
    aux.h=ll->tet;@+ aux.l=(ll+1)->tet;
  }
@y
 case 'M':
    MMIX_LDO(aux,cur_disp_addr);
@z


@x
@ @<Subr...@>=
void print_string @,@,@[ARGS((octa))@];@+@t}\6{@>
@y
@ @(libprint.c@>=
void print_string @,@,@[ARGS((octa))@];@+@t}\6{@>
@z


We allow tracing in the Textsegement and in the negative Segment.

@x
 if (val.h<0x20000000) {
@y
 if (val.h<0x20000000|| val.h>=0x80000000) {
@z

We allow breakpoints at all addresses.

@x
 if (!(val.h&sign_bit)) {
@y
 {
@z

@x
case 'B': show_breaks(mem_root);
@y
case 'B': show_breaks(mem_root);@+goto passit;
#ifdef MMIX_TRAP
case 'N': cur_seg.h=0x80000000;@+goto passit;
case 'O': show_operating_system=true;@+goto passit;
case 'o': show_operating_system=false;@+goto passit;
#endif
@z

@x
@ @<Sub...@>=
void show_breaks @,@,@[ARGS((mem_node*))@];@+@t}\6{@>
@y
@ @(libshowbreaks.c@>=
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <setjmp.h>
#include <time.h>
#include "libconfig.h"
#include "libtype.h"
#include "libglobals.h"
#include "mmixlib.h"
#include "libarith.h"
#include "libimport.h"

void show_breaks @,@,@[ARGS((mem_node*))@];@+@t}\6{@>
@z


@x
@<Load the command line arguments@>=
x.h=0x40000000, x.l=0x8;
loc=incr(x,8*(argc+1));
for (k=0; k<argc; k++,cur_arg++) {
  ll=mem_find(x);
  ll->tet=loc.h, (ll+1)->tet=loc.l;
  ll=mem_find(loc);
  mmputchars((unsigned char *)*cur_arg,strlen(*cur_arg),loc);
  x.l+=8, loc.l+=8+(strlen(*cur_arg)&-8);
}
x.l=0;@+ll=mem_find(x);@+ll->tet=loc.h, (ll+1)->tet=loc.l;
@y
@<Load the command line arguments@>=
x.h=0x60000000, x.l=0x00;
aux.h=0, aux.l=argc;
MMIX_STO(aux,x); /* and $\$0=|argc|$ */
x.h=0x40000000, x.l=0x8;
aux=incr(x,8*(argc+1));
for (k=0; k<argc && *argv!=NULL; k++,argv++) {
  MMIX_STO(aux,x);
  mmputchars((unsigned char *)*argv,(int)strlen(*argv),aux);
  x.l+=8, aux.l+=8+(tetra)(strlen(*argv)&-8);
}
if (argc>0) 
{ x.l=0;
  MMIX_STO(aux,x);
}
@z

we do not support user libraries at #f0

@x
x.h=0, x.l=0xf0;
ll=mem_find(x);
if (ll->tet) inst_ptr=x;
@^subroutine library initialization@>
@^initialization of a user program@>
@y
@z


@x
  exit(0);
@y
  longjmp(mmix_exit,0);
@z


@x
@<Sub...@>=
void dump @,@,@[ARGS((mem_node*))@];@+@t}\6{@>
@y
@(libdump.c@>=
#include <stdio.h>
#include "libconfig.h"
#include <time.h>
#include <setjmp.h>
#include "libtype.h"
#include "libglobals.h"
#include "mmixlib.h"
#include "libarith.h"
#include "libimport.h"

void dump @,@,@[ARGS((mem_node*))@];@+@t}\6{@>
@z

@x
  octa cur_loc;
  if (p->left) dump(p->left);
  for (j=0;j<512;j+=2) if (p->dat[j].tet || p->dat[j+1].tet) {
    cur_loc=incr(p->loc,4*j);
    if (cur_loc.l!=x.l || cur_loc.h!=x.h) {
      if (x.l!=1) dump_tet(0),dump_tet(0);
      dump_tet(cur_loc.h);@+dump_tet(cur_loc.l);@+x=cur_loc;
    }
    dump_tet(p->dat[j].tet);
    dump_tet(p->dat[j+1].tet);
    x=incr(x,8);
  }
@y
  octa cur_loc;
  octa dat;
  MMIX_LOCAL_LL
  if (p->left) dump(p->left);
  for (j=0;j<512;j+=2) {
    cur_loc=incr(p->loc,4*j);
    MMIX_LDO(dat,cur_loc);
    if (dat.h || dat.l) {
      if (cur_loc.l!=x.l || cur_loc.h!=x.h) {
        if (x.l!=1) dump_tet(0),dump_tet(0);
        dump_tet(cur_loc.h);@+dump_tet(cur_loc.l);@+x=cur_loc;
      }
      dump_tet(dat.h);
      dump_tet(dat.l);
      x=incr(x,8);
    }
  }
@z


@x
@ @<Sub...@>=
void dump_tet(t)
@y
@ @(libdump.c@>=
void dump_tet(t)
@z