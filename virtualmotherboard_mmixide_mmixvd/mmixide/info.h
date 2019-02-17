
#include "mmixlib.h"
#include "libmmixal.h"

#define MAX_FILES 0x100
extern char *fullname[MAX_FILES+1]; /* the full filenames */
extern char *shortname[MAX_FILES+1]; /* pointers to the tail of the full name */
extern char *command[MAX_FILES+1];
extern char has_debug_info[MAX_FILES+1];
extern char needs_assembly[MAX_FILES+1];
#ifdef VMB
extern char needs_image[MAX_FILES+1];
#define file2image(file_no) (needs_image[file_no])
#endif
extern char execute[MAX_FILES+1];
extern char needs_loading[MAX_FILES+1];
extern char doc_dirty[MAX_FILES+1];
extern char needs_reading[MAX_FILES+1];
extern void *doc[MAX_FILES+1]; /* pointer to scintilla documents */
extern int curPos[MAX_FILES+1];
extern int curAnchor[MAX_FILES+1];
extern int firstLine[MAX_FILES+1];
extern time_t file_time[MAX_FILES+1]; 
#define file2shortname(file_no) (shortname[file_no])
#define file2fullname(file_no)  (fullname[file_no])
#define file2debuginfo(file_no) (has_debug_info[file_no])
#define file2assembly(file_no) (needs_assembly[file_no])
#define file2execute(file_no) (execute[file_no])
#define file2loading(file_no) (needs_loading[file_no])
#define file2dirty(file_no) (doc_dirty[file_no])
#define file2reading(file_no) (needs_reading[file_no])

extern int file_dirty(int file_no);

extern char *unique_name(int file_no);
/* return a unique short name for the file */

extern void *file2symbols(int file_no);
/* return symbol trie for file actually returns a trie_node* */

extern int filename2file(char *filename);
/* return file_no for this file, allocate fullname as needed */
extern char *file2filename(int file_no);
/* return filename for this file */

extern void file_set_name(int file_no, char *filename);
/* function to compute full and short name and set them */

extern void set_file(int file_no, char *filename);
/* rearrange data to associate file_no with filename, 
   allocate fullname if needed, 
   */

extern void clear_file_info(int file_no);
/* remove all data about file */

extern void clear_all_info(void);
/* remove all data */

extern void for_all_loc(int file_no, int line_no, void f(octa loc));
/* iterate f over all locations belonging to this file and line */

/* retrieve information about locations */
#define loc2bkpt(loc) (mem_find(loc)->bkpt)
#define loc2freq(loc) (mem_find(loc)->freq)
#define loc2file(loc) (mem_find(loc)->file_no)
#define loc2line(loc) (mem_find(loc)->line_no)

extern void for_all_files(void f(int i));
/* set all file names in the listbox h */
extern void update_symtab(void);
/* set all symbols in the symbol table*/
extern void symtab_add_file(int file_no,trie_node *t);
/* add a symbol table for this file */
extern void close_file(int file_no);
/* remove a file from the database */
extern int get_inuse_file(void);
/* return a file that is in use */
extern void line2freq(int file_no,int from, int to, unsigned int *freq);
/* returns the frequency count for this line  or -1 if none found*/
extern void clear_profile_data(void);
/* resets profile information */
extern void mem_iterator(void f(octa loc, mem_tetra *dat));
/* iterate f over all mem tetras */

extern time_t ftime(char *file_name);