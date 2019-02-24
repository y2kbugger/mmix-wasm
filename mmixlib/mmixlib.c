#ifndef _MMIXLIB_H_
#define _MMIXLIB_H_

#include <stdio.h>

/* auxiliar functions provided by the library  but without a prototype here*/
#if 0
extern void free_tree(trie_node *root);

#endif

#ifndef _LIBTYPE_H_

typedef enum{false,true}bool;
typedef unsigned int tetra;
typedef struct{tetra h,l;} octa;
typedef char Char;

typedef enum{
rB,rD,rE,rH,rJ,rM,rR,rBB,
rC,rN,rO,rS,rI,rT,rTT,rK,rQ,rU,rV,rG,rL,
rA,rF,rP,rW,rX,rY,rZ,rWW,rXX,rYY,rZZ}special_reg;

typedef enum{
TRAP,FCMP,FUN,FEQL,FADD,FIX,FSUB,FIXU,
FLOT,FLOTI,FLOTU,FLOTUI,SFLOT,SFLOTI,SFLOTU,SFLOTUI,
FMUL,FCMPE,FUNE,FEQLE,FDIV,FSQRT,FREM,FINT,
MUL,MULI,MULU,MULUI,DIV,DIVI,DIVU,DIVUI,
ADD,ADDI,ADDU,ADDUI,SUB,SUBI,SUBU,SUBUI,
IIADDU,IIADDUI,IVADDU,IVADDUI,VIIIADDU,VIIIADDUI,XVIADDU,XVIADDUI,
CMP,CMPI,CMPU,CMPUI,NEG,NEGI,NEGU,NEGUI,
SL,SLI,SLU,SLUI,SR,SRI,SRU,SRUI,
BN,BNB,BZ,BZB,BP,BPB,BOD,BODB,
BNN,BNNB,BNZ,BNZB,BNP,BNPB,BEV,BEVB,
PBN,PBNB,PBZ,PBZB,PBP,PBPB,PBOD,PBODB,
PBNN,PBNNB,PBNZ,PBNZB,PBNP,PBNPB,PBEV,PBEVB,
CSN,CSNI,CSZ,CSZI,CSP,CSPI,CSOD,CSODI,
CSNN,CSNNI,CSNZ,CSNZI,CSNP,CSNPI,CSEV,CSEVI,
ZSN,ZSNI,ZSZ,ZSZI,ZSP,ZSPI,ZSOD,ZSODI,
ZSNN,ZSNNI,ZSNZ,ZSNZI,ZSNP,ZSNPI,ZSEV,ZSEVI,
LDB,LDBI,LDBU,LDBUI,LDW,LDWI,LDWU,LDWUI,
LDT,LDTI,LDTU,LDTUI,LDO,LDOI,LDOU,LDOUI,
LDSF,LDSFI,LDHT,LDHTI,CSWAP,CSWAPI,LDUNC,LDUNCI,
LDVTS,LDVTSI,PRELD,PRELDI,PREGO,PREGOI,GO,GOI,
STB,STBI,STBU,STBUI,STW,STWI,STWU,STWUI,
STT,STTI,STTU,STTUI,STO,STOI,STOU,STOUI,
STSF,STSFI,STHT,STHTI,STCO,STCOI,STUNC,STUNCI,
SYNCD,SYNCDI,PREST,PRESTI,SYNCID,SYNCIDI,PUSHGO,PUSHGOI,
OR,ORI,ORN,ORNI,NOR,NORI,XOR,XORI,
AND,ANDI,ANDN,ANDNI,NAND,NANDI,NXOR,NXORI,
BDIF,BDIFI,WDIF,WDIFI,TDIF,TDIFI,ODIF,ODIFI,
MUX,MUXI,SADD,SADDI,MOR,MORI,MXOR,MXORI,
SETH,SETMH,SETML,SETL,INCH,INCMH,INCML,INCL,
ORH,ORMH,ORML,ORL,ANDNH,ANDNMH,ANDNML,ANDNL,
JMP,JMPB,PUSHJ,PUSHJB,GETA,GETAB,PUT,PUTI,
POP,RESUME,SAVE,UNSAVE,SYNC,SWYM,GET,TRIP}mmix_opcode;

#define trace_bit (1<<3) 
#define read_bit (1<<2) 
#define write_bit (1<<1) 
#define exec_bit (1<<0)  \

#define RESUME_AGAIN 0
#define RESUME_CONT 1
#define RESUME_SET 2
#define RESUME_TRANS 3 

#define P_BIT (1<<0) 
#define S_BIT (1<<1) 
#define B_BIT (1<<2) 
#define K_BIT (1<<3) 
#define N_BIT (1<<4) 
#define PX_BIT (1<<5) 
#define PW_BIT (1<<6) 
#define PR_BIT (1<<7)  

#define PF_BIT (1<<0) 
#define MP_BIT (1<<1) 
#define NM_BIT (1<<2) 
#define YY_BIT (1<<3) 
#define RE_BIT (1<<4) 
#define CP_BIT (1<<5) 
#define PT_BIT (1<<6) 
#define IN_BIT (1<<7) 

#define X_BIT (1<<8) 
#define Z_BIT (1<<9) 
#define U_BIT (1<<10) 
#define O_BIT (1<<11) 
#define I_BIT (1<<12) 
#define W_BIT (1<<13) 
#define V_BIT (1<<14) 
#define D_BIT (1<<15) 
#define H_BIT (1<<16)  

typedef struct{
#ifndef VMB
  tetra tet;
#endif
  tetra freq;
  unsigned char bkpt;
  unsigned char file_no;
  unsigned short line_no;
} mem_tetra;

typedef struct mem_node_struct{
  octa loc;
  tetra stamp;
  struct mem_node_struct*left,*right;
  mem_tetra dat[512];
} mem_node;

#endif

extern bool show_operating_system;
extern unsigned int tracing_exceptions;
extern int G,L,O;
extern octa g[256];
extern octa*l;
extern int lring_size;
extern int lring_mask;
extern int S;
extern char*special_name[32];
extern octa new_Q;

extern void mmputchars(unsigned char *buf,int size,octa addr);
extern int mmgetchars(unsigned char *buf, int size, octa addr, int stop);


extern FILE*fake_stdin;

extern mem_tetra* mem_find(octa addr);
extern mem_node*mem_root;
extern mem_node* new_mem(void);

extern octa incr(octa y,int delta);
extern void free_file_info(void);
extern void scan_option(char *arg, /* command-line argument (without the `\.-') */
                 bool usage); /* should we exit with usage note if unrecognized? */
extern void print_hex(octa o);
extern void print_int(octa o);
extern void print_string(octa o);





extern void show_line(void);

extern void show_breaks(mem_node *p);




/* functions provided by mmix-lib */
extern int mmix_main(int argc, char *argv[],char *mmo_file_name);
/* the mmix main program */
extern int mmix_lib_initialize(void);
/* call to initialize the library */
extern int mmix_lib_finalize(void);
/* call to clean up, relase memory after the library has been used */
extern int mmix_initialize(void);
/* call to initialize global variables needed for mmix sim to run */
extern int mmix_finalize(void);
/* call to clean up, relase memory after mmix sim has run */
extern void mmix_boot(void);
/* call to bring the simulated mmix cpu into the fresh state after boot/reset */
extern int mmix_load_file(char *mmo_file_name);
/* call to load a file */
extern int mmix_commandline(int argc, char *argv[]);
/* call to provide a commandline to mmix (call after loading files!)*/
extern void	mmix_interact(void);      
/* mmix user interaction */
extern int mmix_fetch_instruction(void);
/* called to fetch one instruction returns zero on error */
extern int mmix_resume(void);
/* called instead of mmix_fetch_insruction after a RESUME */
extern int mmix_perform_instruction(void); 
/* called to execute one instruction */
extern void mmix_trace_fetch(void);
/* Print the frequency count, the location, and the instruction*/
extern void mmix_trace_perform(void);
/* Print a stream-of-consciousness description of the instruction */
extern void mmix_trace(void);
/* tests for tracing and outputs a trace of the instruction */
/* extern void mmix_exit(int returncode); */
/* call to cause a running mmix simulator to return*/
extern bool mmix_dynamic_trap(void);
/* check for dynamic traps return true if switching to trap handler */
extern void mmix_profile(void);
/* print the profile */
extern void show_stats(bool verbose);
/* show statistics */

extern int mmix_printf(FILE *f, char *format,...);
/* printf replacement */
extern int mmix_vprintf(char *format, va_list vargs);
extern int mmix_fputc(int c, FILE *f);
/* fputc replacement */
void mmix_stack_trace(char *format,...);
/* function to do the stack trace */

extern bool halted;
extern int breakpoint;
extern bool interrupt;
extern bool interacting;
extern bool line_listed; /* have we listed the buffer contents? */
extern octa neg_one;
extern bool tracing;
extern bool stack_tracing;
extern octa loc;
extern octa inst_ptr;
extern tetra inst;
extern bool show_operating_system;
extern bool resuming;
extern mmix_opcode op;
extern bool profiling;
extern bool showing_stats;


/* function from mmix-io */
void mmix_fake_stdin(FILE *f);
/* functions that mmixal provides */

extern int mmixal(char *mms_name, char *mmo_name, char *mml_name);
/* returns zero on success, 
           negative values on fatal errors, 
		   otherwise the number of errors found in the input
*/

extern void report_error(char *message, int file_no, int line_no);
/* report an error in the given file and line. 
 */


extern int mmoimg_main(int argc, char *argv[]);

/* global variables of mmixal */
extern char *src_file_name; /* name of the \MMIXAL\ input file */
extern char obj_file_name[FILENAME_MAX+1]; /* name of the binary output file */
extern char listing_name[FILENAME_MAX+1]; /* name of the optional listing file */
extern FILE *src_file, *obj_file, *listing_file;
extern int expanding; /* are we expanding instructions when base address fail? */
extern int buf_size; /* maximum number of characters per line of input */
extern int check_X_BIT; /* whether checking for imprecise floating point constants */
extern int mmixal(char *mms_name, char *mmo_name, char *mml_name);
extern int err_count; /* the error count */
extern void flush_listing_line(char*line);

extern char *file2filename(int file_no);
extern int filename2file(char *filename);
#endif