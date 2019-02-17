

#ifndef _WINOPT_H_
#define _WINOPT_H_


/* Sybols needed by winopt */

extern HWND hMainWnd;
extern HINSTANCE hInst;
extern HMENU hMenu;
extern int major_version, minor_version;
extern char version[];
extern char title[];
extern char *program_name;
extern char *defined;
extern char *programhelpfile;

extern void win32_message(char *msg);
extern void win32_error_init(int i);
extern void win32_error(int line, char *message);
extern void win32_ferror(int line, char *format, char *str);

extern void win32_log(char *msg);

extern void win32_fatal_error(int line, char *message);
extern void init_layout(int interactive);
extern void set_font_size(int size);
#define DEFAULT_FONT_SIZE 10
extern void change_font_size(int scale);
extern int xpos, ypos; /* Window position */
extern int width,height; /* dimension of main window */


extern void set_option(char **option, char *str);
#define MAXARG 256
extern int mk_argv(char *argv[MAXARG],char *command, int unquote);

/* Symbols provided by winopt */
#include "uint64.h"


/* in winabout.c */
extern INT_PTR CALLBACK    
AboutDialogProc( HWND hDlg, UINT message, WPARAM wparam, LPARAM lparam );

/* in windialog.c */
extern void register_subwindow(HWND h);
extern void unregister_subwindow(HWND h);
extern BOOL do_subwindow_msg(MSG *msg);

 
/* from winxy.c */
extern void set_xypos(HWND hWnd);
extern void get_xypos(void);

extern HBITMAP hBmp;
extern HBITMAP hon,hoff,hconnect;


/* table of key value pairs to store in the registry terminated by a NULL key */

typedef struct {
	char *key;
	void *value;
	int type; } regtable[];

/* these values are used for the type field in the regtable.
   non negative type values between 0 and 31 are used for flags */
#define TYPE_DWORD (-1)
#define TYPE_STRING (-2)
#define KEY_FLAGS ((char *)1)


extern regtable regtab;

/* from winreg.c */
extern void write_regtab(char *program);
extern void read_regtab(char * program);

/* from layout.c */

extern HFONT hFixedFont;
extern HFONT hVarFont;
extern HFONT hGUIFont;
extern int fixed_line_height;
extern int var_line_height;
extern int fixed_char_width;
extern int fixed_char_height;
extern int var_char_height;
extern int separator_width;
extern int separator_height;
extern int border_size;
extern int edge_size;
extern int version_width; /* length of the version string in VarFont */
extern int fontsize;
extern int screen_width;
extern int screen_height;
#define SELECT_COLOR (RGB(0x80,0x80,0xff))

extern int minimized;

extern LRESULT CALLBACK 
OptWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam); /* provided */

/* from winparam.c */
extern void do_program(char * arg);
extern void parse_commandline(int argc, char *argv[]);
extern int do_define(char *arg);

#include <stdio.h>
FILE *vmb_fopen(char *filename, char *mode);
/* open fiename, look in the configPATH and programpath before giving up */


#endif