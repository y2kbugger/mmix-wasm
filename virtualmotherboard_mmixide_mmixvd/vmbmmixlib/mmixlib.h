/* auxiliar functions provided by the library  but without a prototype here*/
#ifndef _MMIX_SIM_
extern void free_tree(trie_node *root);

#endif

#ifndef _MMIXAL_
extern void free_file_info(void);
/* reset the file_info and ybyte2file_no information */
extern int ybyte2file_no[256]; /* mapping internal to external files */
extern mem_tetra* mem_find(octa addr);
extern mem_node* new_mem(void);
extern void mmputchars(unsigned char *buf,int size,octa addr);
extern int mmgetchars(unsigned char *buf, int size, octa addr, int stop);
extern device_info vmb;
extern void show_line(void);
extern void show_stats(bool verbose);
extern void scan_option(char *arg, /* command-line argument (without the `\.-') */
                 bool usage); /* should we exit with usage note if unrecognized? */
extern void print_hex(octa o);
extern void print_int(octa o);
extern void print_string(octa o);
extern void show_breaks(mem_node *p);
#endif



/* function to return a symbol table back to free storage */


extern void mmix_exit(int returncode);

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
extern int	mmix_interact(void);      
/* mmix user interaction */
extern int mmix_fetch_instruction(void);
/* called to fetch one instruction returns zero on error */
extern int mmix_resume(void);
/* called instead of mmix_fetch_insruction after a RESUME */
extern int mmix_perform_instruction(void); 
/* called to execute one instruction */
extern void mmix_trace(void);
/* tests for tracing and outputs a trace of the instruction */
extern void mmix_exit(int returncode);
/* call to cause a running mmix simulator to return*/
extern void mmix_dynamic_trap(void);
/* check for dynamic traps */
extern void mmix_profile(void);
/* print the profile */

extern int mmix_printf(char *format,...);
/* printf replacement */
extern int mmix_fputc(int c, FILE *f);
/* fputc replacement */


#if 0

extern int cur_file; /* index of the current file in |filename| */
extern int line_no; /* current position in the file */
extern bool line_listed; /* have we listed the buffer contents? */


extern int err_count; /* this many errors were found */
extern char *src_file_name; /* name of the \MMIXAL\ input file */
extern Char *filename[257];
extern char obj_file_name[FILENAME_MAX+1]; /* name of the binary output file */
extern char listing_name[FILENAME_MAX+1]; /* name of the optional listing file */
extern FILE *src_file, *obj_file, *listing_file;
extern int expanding; /* are we expanding instructions when base address fail? */
extern int buf_size; /* maximum number of characters per line of input */
extern void flush_listing_line(char *s);
extern int mmixal(void);

extern int mmo_ptr;

extern int mmix_status; 

extern char full_mmo_name[];
extern void mmix_status(int status);



#endif


/* functions that mmixal provides */

extern int mmixal(char *mms_name, char *mmo_name, char *mml_name, int x_option, int b_option);
/* returns zero on success, 
           negative values on fatal errors, 
		   otherwise the number of errors found in the input
*/

/* functions that need to be provided externaly for mmixal */
extern int filename2file(char *filename, char c);
/* return file_no for this file */

extern void mmixal_error(char *message, int file_no, int line_no, int status);
/* report an error in the given file and line. 
   status is 1 for warnings, 0 for errors, -1 for fatal errors.
 */

extern void add_line_loc(int file_no, int line_no, octa loc);
/* report the association of a file/line with a location */

extern int mmoimg_main(int argc, char *argv[]);