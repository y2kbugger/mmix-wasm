extern void create_breakpoints(void);
extern int set_file_breakpoint(int file_no, int line_no, int bits, int mask);
extern void add_loc_breakpoint(octa loc);
extern void del_file_breakpoint(int file_no, int line_no, int bits);
extern void remove_file_breakpoints(int file_no);
extern void remove_loc_breakpoints(int file_no);
extern void set_break_at_Main(void);
extern void sync_breakpoints(void);
/* remove all breakpoints for this file */
extern HWND hBreakpoints;
extern HWND hBreakList;
extern int break_at_symbol(int file_no, char *symbol);
/* get associations of files, lines, and locations */
extern void add_line_loc(int file_no,int line_no,octa loc);
extern void mark_all_file_breakpoints(int file_no);
extern void sweep_all_file_breakpoints(int file_no);
extern void show_breakpoints(void);
extern int BreakpointContextMenuHandler(int x, int y);