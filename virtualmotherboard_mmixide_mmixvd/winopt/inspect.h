#ifndef INSPECT_H
#define INSPECT_H
#include "float.h"

enum mem_fmt {hex_format=0, ascii_format=1, unsigned_format=2, signed_format=3,  
   float_format=4, bin_format=5, last_format=5, user_format=6 , undefined_format=7};

enum chunk_fmt {byte_chunk=0, wyde_chunk=1,tetra_chunk=2,octa_chunk=3, 
                last_chunk=3, user_chunk=4, undefined_chunk=5 };

extern char *format_names[];

extern char *chunk_names[];

extern void set_format(HWND hDlg,enum mem_fmt f);
extern void clear_format(HWND hDlg,enum mem_fmt f);
extern enum mem_fmt get_format(HWND hDlg, int ItemID);
extern int chunk_len(enum mem_fmt format, int chunk_size);
extern void set_chunk(HWND hDlg,enum chunk_fmt c);
extern enum chunk_fmt get_chunk(HWND hDlg, int ItemID);
extern int chunk_to_str(char *str, unsigned char *buf, enum mem_fmt fmt, 
						int chunk_size, int column_digits);


/* list registers strictly in order of increasing offsets */
#define REG_OPT_DISABLED 0x1
#define REG_OPT_SEPARATOR 0x2

struct register_def
{ char *name;	/* must not be NULL */
  int offset;   /* must not be negativ */
  int size;		
  enum chunk_fmt chunk;
  enum mem_fmt  format;
  unsigned char options;
};
typedef struct register_def register_def;

struct inspector_def {
	char *name;
	unsigned int size;
    int (*get_mem)(unsigned int offset, int size, unsigned char *buf);
	unsigned char *(*load)(unsigned int offset,int size); /* function to simulate load */
    void (*store)(unsigned int offset,int size, unsigned char *payload); /* same for store */
	enum mem_fmt format;
	enum chunk_fmt chunk;
	int sb_rng; /* number of lines covered by scrollbar */
	int num_regs;      /* number of registers if register_def!=NULL */
	struct register_def *regs; /* NULL if memory */
	/* the rest can be initialized with zero */
	int address_width;   /*size of addess or register names column in characters*/
	uint64_t address;  /* used for memory only */
	HWND hWnd; /* the window where the edit rectangle is displayed */
	unsigned int de_offset; /* offset for the data editor */
	unsigned int de_size; /* number of bytes in data editor 0 if none*/
    unsigned int sb_base; /* offset at base of scrollbar */
    int sb_cur; /* line corresponding to start of page */
    int lines; /* lines per page */
	unsigned int line_range; /* number byte per line */
	int width;  /* width in pixel */
	int height; /* height in pixel */
    int columns; 
	int column_digits; /* number of output characters per column */
    unsigned int mem_base;
	unsigned int mem_size; /* page currently displayed  from offset=mem_base to mem_base+memsize*/
    unsigned char *mem_buf; /* memory buffer */
	unsigned int old_base; /* same for the proviously displayed memory to indicate changes */
    unsigned int old_size;
    unsigned char* old_mem;
	int change_address;
};

typedef struct inspector_def inspector_def;

extern struct inspector_def inspector[];




#ifdef WIN32

extern void update_max_regnames(inspector_def *insp);
/* call this if you change the register names dynamically */

extern HWND CreateMemoryDialog(HINSTANCE hInst,HWND hParent);

/* call this to switch to inspector i */
extern void SetInspector(HWND hWnd, inspector_def * insp);
/* call this function to tell the memory inspector that an update is due */
void MemoryDialogUpdate(inspector_def *insp, unsigned int offset, int size);

extern void mem_update(unsigned int offset, int size);
/* call this function to tell a specific memory inspector that an update is due */
extern void adjust_mem_display(inspector_def *insp);
/* call this function if the number or size of columns is about to change */
extern void resize_memory_dialog(inspector_def *insp);
#else
/* make it a no-op */
#define mem_update(offset, size)

#endif

/* used in winmem when a context menu is asked for */
extern int (*MemContextMenu)(inspector_def *insp, int offset, int x, int y); /* if not NULL the function to handle the context menu for the memory window */  
extern void show_goto_addr(inspector_def *insp, uint64_t goto_addr);
#endif

