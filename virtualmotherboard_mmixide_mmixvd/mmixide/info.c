#include <stdio.h>
#include <windows.h>
#include <commctrl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "error.h"
#include "mmixlib.h"
#define STATIC_BUILD
#include "../scintilla/include/scintilla.h"
#include "mmixrun.h"
#include "winmain.h"
#include "symtab.h"
#include "editor.h"
#include "breakpoints.h"
#include "info.h"
#pragma warning(disable : 4996)

/* we maintain files as file numbers in the range 0 to 255
*/



char *fullname[MAX_FILES+1] = {NULL};	/* the full filenames */
char *shortname[MAX_FILES+1] = {NULL};	/* pointers to the tail of the fullname */
extern char *command[MAX_FILES+1]={NULL};
extern char execute[MAX_FILES+1]={0};
static trie_node* symbols[MAX_FILES+1] = {NULL}; /* pointer to pruned symbol table */
void *doc[MAX_FILES+1] = {NULL};		/* pointer to scintilla documents */
int curPos[MAX_FILES+1] ={0};
int curAnchor[MAX_FILES+1] ={0};
int firstLine[MAX_FILES+1] ={0};
char doc_dirty[MAX_FILES+1] ={0};		/* records whether the doc is dirty, but not for the edit_file */
char has_debug_info[MAX_FILES+1] ={0};	/* is debug information available */
char needs_assembly[MAX_FILES+1] ={0};			/* does this file need needs_assembly */
#ifdef VMB
char needs_image[MAX_FILES+1] ={0};			/* does this file need needs_immage file */
#endif
char needs_loading[MAX_FILES+1] ={0};			/* does this file need needs_loading */
char needs_reading[MAX_FILES+1] ={0};   /* reading a file with a full filename can be delayed until displayed for the first time */
static int next_file_no=0;				/* all used file numbers are below next_file_no */
static int count_file_no=0;				/* number of used file numbers */
time_t file_time[MAX_FILES+1] ={0};   /* filetime of disk file */

extern void free_tree(trie_node *root);


/* file numbers get allocated together with documents 
   setting a filename must be followed by reading the file
   the dirty flag is valid only if the file is currently not in the editor
   the fullname and the short name are set and freed together, they are not required.
   unique_name provides a uniqe short name for all files
   file_no must always point to a valid file number 0<=file_no<=255
*/
#define available(file_no) (doc[file_no]==NULL)
#define inuse(file_no) (!available(file_no))



int file_dirty(int file_no)
{ if (file_no==edit_file_no)
    return (int)ed_operation(SCI_GETMODIFY);
  else
	return file2dirty(file_no);
}


static int alloc_file_no(void)
/* return -1 on failure, otherwise an available file number 
   files without fullname or document are available
*/
{ int file_no;
  if (count_file_no<MAX_FILES)
  { for(file_no=0;file_no<MAX_FILES;file_no++)
      if(available(file_no))
	  { count_file_no++;
        doc[file_no]=ed_create_document();
		fullname[file_no]=shortname[file_no]=NULL;
		needs_reading[file_no]=0;
		symbols[file_no]=NULL;
        doc_dirty[file_no]=0;
        has_debug_info[file_no]=0;
        needs_assembly[file_no]=0;
#ifdef VMB
        needs_image[file_no]=0;
#endif
		execute[file_no]=0;
        needs_loading[file_no]=0;
		file_time[file_no]=0;
		if (file_no>=next_file_no) next_file_no = file_no+1;
        return file_no;
      }
  }
  win32_error(__LINE__,"Too many files");
  return -1; /* we should never get here */
}

static void release_file_no(int file_no)
{ if (file_no<0) return;
  ed_remove_tab(file_no);
  remove_file_breakpoints(file_no);
  if (fullname[file_no]!=NULL)
    free(fullname[file_no]);
  fullname[file_no]=shortname[file_no]=NULL;
  needs_reading[file_no]=0;
  ed_release_document(file_no);
  // symbols[file_no]=NULL; needs freeing 
  count_file_no--;
  while(next_file_no>0 && available(next_file_no-1)) next_file_no--;
}

static int is_unique_shortname(int file_no)
/* return true if the short filename is unique */
{  int i;
   for (i=0; i<next_file_no; i++)
	   if (inuse(i) && i!=file_no && 
		    ( (shortname[i]==NULL && shortname[file_no]==NULL) ||
		      (shortname[i]!=NULL && shortname[file_no]!=NULL && strcmp(shortname[i],shortname[file_no])==0)
		    )
		  )
		  return 0;
   return 1;
}

char *unique_name(int file_no)
{ 
  static char noname[]="Unnamed";
  char *name = file2shortname(file_no);
  if (name==NULL) name=noname;
  if(!is_unique_shortname(file_no) && file_no>0)
  { static char str[64+7];
    sprintf_s(str,64,"%.64s-%d",name,file_no);
	name = str;
  }
  return name;
}

static char *full_filename(char *filename, char **tail)
/* compute and allocate the full filename for the given filename set tail to the shortname*/
{ static char name[MAX_PATH+2], *head;
  int n;
  if (filename==NULL) return NULL;
  n = GetFullPathName(filename,MAX_PATH,name,tail);
  if (n<=0)
  { win32_error(__LINE__,"Illegal file name");
    return NULL;
  }
  head =malloc(n+1);
  if (head ==NULL)
  { win32_error(__LINE__,"Out of memory");
    return NULL;
  }
  if (n>MAX_PATH)
    GetFullPathName(filename,n,head,tail);
  else
  { strncpy(head,name,n+1);
    *tail = head +(*tail-name);
  }
  return head;
}


static int find_file(char *name)
{ int i;
  for(i=0;i<next_file_no;i++)
    if (fullname[i]!=NULL && strcmp(fullname[i],name)==0) return i;
  return -1;
}

static void set_filename(int file_no,char *head, char * tail)
{ static int first_name=1;
  fullname[file_no]=head;
  shortname[file_no]=tail;
  needs_reading[file_no]=1;
  if (first_name)
  { file2loading(file_no)=1;
	file2assembly(file_no)=1;
	first_name=0;
  }
}

int filename2file(char *filename)
/* return file_no for this file, allocate file_no if needed */
{ int file_no;
  char *head, *tail;
  head = full_filename(filename, &tail);
  if (head==NULL)
  { file_no = alloc_file_no();
    return file_no;
  }
  file_no=find_file(head);
  if (file_no>=0) 
  { free(head);
	return file_no;
  }
  /* at this point we might reuse file number 0 if it is unnamed, in use, and not dirty */
  if (fullname[0]==NULL && doc[0]!=NULL && !file_dirty(0))
  {  file_no=0;
     ed_remove_tab(file_no); /* remove a possibly existing tab for this file */
  }
  else
      file_no = alloc_file_no();
  set_filename(file_no,head,tail);
  return file_no;
}

char *file2filename(int file_no)
{ 
  if (file_no<0 || file_no>=MAX_FILES) 
	   return NULL;
   else
	   return fullname[file_no];
}

void file_set_name(int file_no, char *filename)
/* function to compute full and short name and set them */
{ 
  char *head, *tail;
  head = full_filename(filename, &tail);
  if (fullname[file_no]!=NULL) free(fullname[file_no]);
  fullname[file_no]=NULL;
  shortname[file_no]=NULL;
  needs_reading[file_no]=0;
  set_filename(file_no,head,tail);
  ed_add_tab(file_no);
}


static void clear_symbols(int file_no)
{ 
  free_tree(symbols[file_no]);
  symbols[file_no]=NULL;
  has_debug_info[file_no]=0;
}


/* auxiliar variable and function for clear_mem_file */
static int clear_file_no=-1;

static void aux_clear_mem_file(octa loc, mem_tetra *dat)
{ if (dat->file_no==clear_file_no)
  { dat->file_no=-1;
    dat->line_no=0; 
  }
}

static void clear_mem_file(int file_no)
/* removes information for this file */
{ clear_file_no=file_no;
  mem_iterator(aux_clear_mem_file);
}



void clear_file_info(int file_no)
/* remove all data about file */
{   remove_loc_breakpoints(file_no);
    clear_mem_file(file_no);
    clear_symbols(file_no);
}

void clear_all_info(void)
/* remove all data */
{ int file_no;
  for (file_no=0;file_no<next_file_no;file_no++)
    clear_file_info(file_no);
}

static mem_iterator_aux(mem_node *p,void f(octa loc, mem_tetra *dat))
{ int j;
  if (p->left) mem_iterator_aux(p->left,f);
  for (j=0;j<512;j++) 
		f(incr(p->loc,4*j),p->dat+j);
  if (p->right) mem_iterator_aux(p->right,f);
}

void mem_iterator(void f(octa loc, mem_tetra *dat))
{ mem_iterator_aux(mem_root,f);
}


void file_line_loc(mem_node *p,int file_no, int line_no, void f(octa loc))
{ int j;
  if (p->left) file_line_loc(p->left,file_no,line_no,f);
  for (j=0;j<512;j++) 
	if (p->dat[j].file_no==file_no && p->dat[j].line_no==line_no)
		f(incr(p->loc,4*j));
  if (p->right) file_line_loc(p->right,file_no,line_no,f);

}


void for_all_files(void f(int i))
/* iterate over all files in use */
{ int file_no;
  for (file_no=0; file_no<next_file_no;file_no++)
  { if (inuse(file_no))
		f(file_no);
  }
}

/* auxiliar variables and function for line2freq */
static int freq_file_no=-1;
static int freq_from=0;
static int freq_to=0;
static int *freq_freq=NULL;

static void get_max_freq(octa loc, mem_tetra *dat)
{ if (dat->line_no>=freq_from+1 && dat->line_no<=freq_to+1 &&
	  dat->file_no==freq_file_no &&
      (int)(dat->freq)>freq_freq[dat->line_no-1]) 
  { freq_freq[dat->line_no-1]=dat->freq;
    if (dat->freq>max_profile_data) 
		max_profile_data=dat->freq;
  }
}

void line2freq(int file_no,int from, int to, unsigned int *freq)
/* stores frequency counts in freq[from..to] */
{ 
  freq_file_no=file_no;
  freq_from=from;
  freq_to=to;
  freq_freq=freq;
  mem_iterator(get_max_freq);

}

static void aux_clear_profile(octa loc, mem_tetra *dat)
{ dat->freq=0;
}
void clear_profile_data(void)
{  mem_iterator(aux_clear_profile);
   max_profile_data=0;   
}


void symtab_add_file(int file_no,trie_node *t)
{ 
  clear_symbols(file_no);
  symbols[file_no]=t;
  has_debug_info[file_no] = 1;
  update_symtab();
}

void *file2symbols(int file_no)
{ if (file_no<0 || file_no>=next_file_no) return NULL;
  return (void*)symbols[file_no];
}

void close_file(int file_no)
/* remove a file from the database */
{ release_file_no(file_no);
  clear_symbols(file_no);
}

int get_inuse_file(void)
/* return a used file number or -1 if none exists */
{ int file_no;
  if (count_file_no>0)
    for(file_no=0;file_no<next_file_no;file_no++)
      if(inuse(file_no)) return file_no;
  return -1;
}


time_t ftime(char *file_name)
{ struct _stat buf;
	_stat(file_name,&buf);
	return buf.st_mtime;
}


