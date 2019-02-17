#include <windows.h>
#include "edit.h"
#include "winmain.h"
#include "indent.h"

#define STATIC_BUILD
#include "../scintilla/include/scintilla.h"
extern sptr_t ed_send(unsigned int msg,uptr_t wparam,sptr_t lparam);


int max_label_indent=8; /*maximum characters for the label column */
int max_arg_indent=16; /*maximum characters for the arguments column */
int max_opcode_indent=8; /* maximum characters for the opcode column */
static int label_size=8; /*measured characters for the label column */
static int arg_size=16; /*measured characters for the arguments column */
static int op_size=7;   /* size of the op column */
int use_tab_indent=1; /* whether to use tabs or spaces */
int use_crlf=1; /* Use CR LF to end a line */
unsigned char line_comment; /* character to use for line comments */

static unsigned char *buffer=NULL; /* buffer to hold entire text */
static unsigned int buffer_size=0;

static unsigned char *get_buffer(int file_no,double extra_size)
{ unsigned int len, extra;
  set_edit_file(file_no);
  len= (int)ed_send(SCI_GETLENGTH,0,0);
  extra = (int)(len*extra_size);
  if (extra<128)
    extra=128;
  if (buffer==NULL) buffer=malloc(len+extra+1);
  else buffer=realloc(buffer,len+extra+1);
  if (buffer==NULL) return NULL;
  ed_send(SCI_GETTEXT,len+1,(sptr_t)(buffer+extra));
  buffer_size=len+extra;
  return buffer+extra;
}



static int put_buffer(int file_no)
{ if (buffer==NULL)
    return 0;
  set_edit_file(file_no);
  ed_send(SCI_SETTEXT,0,(sptr_t)buffer);
  free(buffer);
  buffer=NULL;
  return 1;
}

#define isletter(c) (isalpha(c)||c=='_'||c==':'||(unsigned int)(c)>126)
#define isblank(c)  (isspace(c) && ! ((c)=='\n'))

/*sizes and index of various parts of the line */
static int slabel, iop, sop, iarg, sarg, icomment, strailing, has_cr, sline, tabsize;

static int argsize(unsigned char *line);

static int scan_line(unsigned char *line)
/* return 0 on end of file, 1 on end of line */
{ int i=0;
  slabel=iop=sop=iarg=sarg=icomment=strailing=has_cr=sline=0;
  if (isletter(line[i])||isdigit(line[i])) /* label */
  { while (!isspace(line[i]) && line[i]!=0) i++;
	slabel=i;
  }
  while (isblank(line[i])) i++;
  if (isletter(line[i])||isdigit(line[i]))
  { iop=i;
    while (isletter(line[i])||isdigit(line[i])) i++; /* opcode */
    sop=i-iop;
    while (isblank(line[i])) i++;
    iarg=i;
	sarg=argsize(line+iarg);
	i = i+sarg;
    while (isblank(line[i])) i++;
  }  
  icomment=i;
  while(line[i]!='\n' && line[i]!=0) i++;
  if (line[i]==0) 
	sline=i;
  else 
  { sline=i+1;
    if (line[i-1]=='\r') { has_cr=1; i--; }
  }
  while (i>0 && isspace(line[i-1]) && line[i-1]!='\n') i--;
  strailing = sline-i;
  if (line[sline]==0) return 0;
  return 1;
}


static int argsize(unsigned char *line)
/* scan argument starting at , return size */
#define EOA if (line[i]==0) return i
#define NEXT i++; EOA
{ int i=0; 
  EOA;
  while (1) {
	  if (line[i]=='\'') { NEXT; NEXT; NEXT; } /* character constant */
      else if (line[i]=='\"')  /* string constant */
	  { do { NEXT; } while (line[i]!='\"');
	    NEXT;
      }
      else if (isspace(line[i])) return i;
	  else if (line[i]==';') /* line extension */
	  { scan_line(line+i+1); return i+1+iarg+sarg; }
	  else NEXT;
    }
}

static void measure_indent(unsigned char *buffer)
/* Find out what the minimum values for labels and args in buffer are. */
{ int i, eol;
  if (buffer==NULL) return;
  label_size=0;
  arg_size=0;
  op_size=0;
  i=0;
  do
  { eol=scan_line(buffer+i);
    i = i+sline;
    if (slabel>label_size) label_size=slabel;
	if (sop>op_size) op_size=sop;
	if (sarg>arg_size) arg_size=sarg;
  } while (eol!=0);
}


static unsigned char *pretty_print(unsigned char *out, /* output buffer */
				 unsigned char *in,  /*input buffer */
				 int stab, /* size of a tab */
				 int slabel, /* size of the label column in characters, multible of stab*/
				 int sop,  /* size of the op column in characters, multible of stab*/			
				 int sarg,  /* size of the arg column in characters, multible of stab*/
				 unsigned char tab) /* tab character */ 
/* input and output buffer may overlap. out <= in
   in must point to the start of a line.
   pretty printing stops when out==in 
   the return value is out when out==in else it is NULL
*/
#define CRLF	  if (use_crlf && out<in) *out++='\r'; *out++=*in++
#define TRIM      while (isblank(*(out-1))) out--; 
#define EOF       if (*in==0) {TRIM; *out=0; return NULL; }
#define EOL			if (*in=='\n') { TRIM; CRLF;  continue; }
#define OOM       if (out>=in) return out
#define COPY      do { *out++=*in++; pos++; OOM; EOF;} while (!isspace(*in)) 
#define COPYTONL    do { *out++=*in++; OOM; EOF;} while (*in!='\n'); TRIM; CRLF;
#define SPACE     while (isblank(*in)) in++ 
#define SKIPTO(i) pos=pos -pos%stab; do { *out++=tab; pos=pos+stab; OOM; } while (pos<(i))
#define COPYTO(t) do { *out++=*in++; pos++; OOM; EOF;} while (in < t);

{ int iarg=slabel+sop;
  int icomment=iarg+sarg;
  unsigned char *t;
  while (out<in)
  { int pos=0; /* position in line out */
    EOF;
	if (isletter(*in)||isdigit(*in)) COPY; /* label */
	else if (!isspace(*in)) { COPYTONL; continue; } /*comment*/
	SPACE;
	EOF;
	EOL;
	SKIPTO(slabel); 
	if (isletter(*in)||isdigit(*in))
		do { 
			*out++=*in++; 
			pos++; 
			OOM; 
			EOF;
		} while (!isspace(*in)); 
			//COPY; /* op */
	else { COPYTONL; continue; } /* indented comment */
    SPACE;
	EOF;
	EOL;
	SKIPTO(iarg);
    t =in+argsize(in);
	COPYTO(t);
	SPACE;
	EOF;
	EOL;
	SKIPTO(icomment);
	COPYTONL;
  }
  return out;
}



void indent(int file_no)
/* pretty print the given file */
{ unsigned char  *eob;
  unsigned char *in;
  double ratio,extra_size=0.25;
  int try = 3;
  while (try-->0)
  { in=get_buffer(file_no,extra_size);
    if (in==NULL) return;
    measure_indent(in);
    if (label_size>max_label_indent) label_size=max_label_indent;  
    if (arg_size>max_arg_indent) arg_size=max_arg_indent;
    if (op_size>max_opcode_indent) op_size=max_opcode_indent;
    if (use_tab_indent) tabsize=tabwidth;
    else tabsize=1;
    label_size=tabsize*((label_size+tabsize)/tabsize);
    arg_size=tabsize*((arg_size+tabsize)/tabsize);
    op_size=tabsize*((op_size+tabsize)/tabsize);
    eob=pretty_print(buffer,in,tabsize,label_size,op_size,arg_size,use_tab_indent?'\t':' ');
    if (eob==NULL) 
	{ put_buffer(file_no);
	  return;
	}
	else
	{ ratio=((double)buffer_size)/(eob-buffer);
	  if (ratio<1.0) ratio=1.0;
	  if (ratio>2.0) ratio=2.0;
      put_buffer(file_no);
	  extra_size=ratio*extra_size;
	}
  }
}

