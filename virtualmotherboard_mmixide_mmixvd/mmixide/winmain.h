extern HINSTANCE hInst;
extern HWND	  hMainWnd, hError;
extern HMENU  hMenu;
extern HMENU  hMemContextMenu, hRegContextMenu, hSymContextMenu, hBrkContextMenu; 
extern HWND	  hSplitter;
extern HWND   hButtonBar;
extern HWND	  hEdit;

extern int major_version, minor_version;
extern char version[];
extern char title[];
extern char *program_name;

extern void ide_status(char *message);
extern void ide_help(char *topic);
extern void ide_add_error(char *message, int file_no, int line_no);
extern void ide_clear_error_marker(void);
extern void ide_clear_error_list(void);

extern int assemble_if_needed(int file_no);
extern int assemble_all_needed(void);
extern int execute_commands(void);
extern void new_errorlist(void);

extern void new_edit(void);

extern void clear_stop_marker(void);
extern void set_edit_file(int file_no);
extern void show_stop_marker(int file_no, int line_no);
extern void set_lineno_width(void);
extern void set_profile_width(void);
extern void reset_profile_width(void);
extern unsigned int max_profile_data;
extern void update_profile(void);
extern void display_profile(void);
extern void set_tabwidth(void);
#define WM_MMIX_STOPPED (WM_USER+1)
#define WM_MMIX_RESET (WM_USER+2)
#define WM_MMIX_LOAD (WM_USER+3)
#define WM_MMIX_INTERACT (WM_USER+4)

#define MMIX_LINE_MARGIN 0
#define MMIX_PROFILE_MARGIN 1
#define MMIX_BREAK_MARGIN 2
#define MMIX_TRACE_MARGIN 3

/* this must match the definition of exec_bit, write_bit, read_bit and trace_bit in mmix-internals.h */
#define MMIX_BREAKX_MARKER 0
#define MMIX_BREAKW_MARKER 1
#define MMIX_BREAKR_MARKER 2
#define MMIX_BREAKT_MARKER 3

#define MMIX_TRACE_MARKER 4
#define MMIX_ERROR_MARKER 5

/* packing file and line in an LPARAM */
#define item_data(file_no, line_no) ((LPARAM)(((line_no)<<8)|((file_no)&0xff)))
#define item_line_no(data) ((int)((data)>>8))
#define item_file_no(data) ((int)((data)&0xff))
extern COLORREF syntax_color[];
extern void set_text_style(void);
extern int syntax_highlighting;
extern int show_whitespace;
extern int codepage;


extern void win32_message(char *msg);
extern void win32_error(int line, char *message);
extern void win32_ferror(int line, char *format, char *str);
extern void win32_fatal_error(int line, char *message);
#ifdef VMB
extern void  ide_exit_ignore(int returncode);
#endif