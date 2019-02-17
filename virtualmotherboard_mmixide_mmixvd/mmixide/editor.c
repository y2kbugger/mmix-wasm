#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#pragma warning(disable : 4996)
#include "resource.h"
#include "winmain.h"
#include "winopt.h"
#include "splitter.h"
#include "info.h"
#include "debug.h"
#include "breakpoints.h"
#include "mmixdata.h"
#include "edit.h"
#include "editor.h"
#define STATIC_BUILD
#include "../scintilla/include/scintilla.h"
#include "../scintilla/include/scilexer.h"



int edit_file_no = -1; /* the file currently in the editor */

HWND hEdit=NULL;
HIMAGELIST hFileMarkers=NULL;
static HWND hSCe=NULL;
static HWND hTabs=NULL;
static int tabh = 40;
static LRESULT CALLBACK EditorProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

int register_editor(HINSTANCE hInstance)
{
  WNDCLASSEX wcex;
  ZeroMemory(&wcex, sizeof(wcex));
	wcex.cbSize = sizeof(WNDCLASSEX); 
	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= (WNDPROC)EditorProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= NULL;
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	=  (HBRUSH)(COLOR_APPWORKSPACE+1);
	wcex.lpszMenuName	= NULL;
	wcex.lpszClassName = "MMIXEDITORCLASS";
	wcex.hIconSm		= NULL;

	return RegisterClassEx(&wcex);
}


static void create_edit(void)
{  
   sp_create_options(1,0,0.8,0,NULL);
   hEdit = CreateWindow("MMIXEDITORCLASS", NULL ,
				WS_CHILD|WS_VISIBLE|WS_TABSTOP|WS_CLIPCHILDREN,
                CW_USEDEFAULT, CW_USEDEFAULT,CW_USEDEFAULT, CW_USEDEFAULT,
	            hSplitter, NULL, hInst, NULL);
}



SciFnDirect ed_fn;
sptr_t ed_ptr;


LONG_PTR ed_send(unsigned int msg,ULONG_PTR wparam,LONG_PTR lparam)
{  if (hSCe==NULL) return 0;
	return ed_fn(ed_ptr,msg,wparam,lparam);
  // Scintilla_DirectFunction used only in findreplace.c and print.c
}


int ed_setfocus(void)
{ if (hSCe==NULL) return 0;
  return SetFocus(hSCe)!=NULL;
}

int  ed_operation(unsigned int op)
{ return (int)ed_send(op,0,0);
}

char * ed_get_instruction(void)
/* return pointer to a string containing the instruction in the current line
   or NULL if no such instrcution is found */
{ int i, n = (int)ed_send(SCI_GETCURLINE,0,0);
  char *p,*q;
  static char op[7];
  p = malloc(n+1);
  if (p==NULL)
  { win32_error(__LINE__, "Unable to allocate memory");
    return NULL;
  }
  ed_send(SCI_GETCURLINE,n,(LONG_PTR)p);
  p[n]=0;
  q=p;
  while (!isspace(*q) && *q!=0) q++; /*skip label */
  while (isspace(*q)) q++; /*skip white space */
  i=0;
  while (!isspace(*q) && *q!=0 && i<6)
  { op[i]=*q;
    i++;
    q++;
  }
  free(p);
  if (i==0) return NULL;
  op[i]=0;
  return op;
}


void ed_tab_select(int file_no);
#define ED_BLOCKSIZE 0x800
#if 0
static void ed_read_file(void)
{ FILE *fp;
  fp = fopen(file2fullname(edit_file_no), "rb");
  if (fp) {
    char data[ED_BLOCKSIZE];
    size_t len;
    while ((len = fread(data, 1, ED_BLOCKSIZE, fp))>0)
      ed_send(SCI_ADDTEXT, len, (sptr_t)data);
	fclose(fp); 
	file2dirty(edit_file_no)=0;
    ed_send(SCI_SETUNDOCOLLECTION, 1,0);
    ed_send(SCI_SETSAVEPOINT,0,0);
    ed_send(SCI_GOTOPOS,0,0);
	set_lineno_width();
  }
  else
  { ide_status("Unable to open file");
  }
  file2reading(edit_file_no)=0;
}
#else
static void ed_read_file(void)
{ HANDLE fh;
  fh = CreateFile(file2fullname(edit_file_no),
	  GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
  if (fh==INVALID_HANDLE_VALUE) 
  { ide_status("Unable to open file");
  }
  else {
    char data[ED_BLOCKSIZE];
    DWORD len;
	file_time[edit_file_no]=ftime(file2fullname(edit_file_no));
	ed_send(SCI_SETTEXT,0,(sptr_t)"");
    while (ReadFile(fh,data,ED_BLOCKSIZE,&len,NULL) && len>0)
      ed_send(SCI_ADDTEXT, len, (sptr_t)data);
	CloseHandle(fh);
	file2dirty(edit_file_no)=0;
    ed_send(SCI_SETUNDOCOLLECTION, 1,0);
    ed_send(SCI_SETSAVEPOINT,0,0);
    ed_send(SCI_GOTOPOS,0,0);
	set_lineno_width();
  }
  file2reading(edit_file_no)=0;
}
#endif

static void check_diskfile_change(void)
{ time_t dtime;
  char *full_mms_name;
  if (edit_file_no<0) return;
  full_mms_name=file2fullname(edit_file_no);
  if (full_mms_name==NULL) return;
  dtime=ftime(full_mms_name);
  if (dtime>file_time[edit_file_no])
  { int dirty = (int)ed_send(SCI_GETMODIFY,0,0);
    int decision;
    char msg[200];
    if (dirty)
		sprintf(msg, "File (time %ld) has changed on disk (time %ld).\nDiscard changes and reload?",
			(long)file_time[edit_file_no],(long)dtime);
    else
		sprintf(msg, "File (time %ld) has changed on disk (time %ld).\n Reload?",
			(long)file_time[edit_file_no],(long)dtime);
    decision= MessageBox(hMainWnd, msg, full_mms_name, MB_YESNO|MB_ICONWARNING);
	if (decision == IDYES)
	{ ed_read_file();
	}
	else
	{ file_time[edit_file_no]=dtime;
	}
  }
}


void set_edit_file(int file_no)
{ if (hEdit==NULL) new_edit();
  if (file_no<0) return;
  if (file_no==edit_file_no && !(fullname[edit_file_no]!=NULL && file2reading(edit_file_no))) return;
  ide_clear_error_marker();
  clear_stop_marker();
  ed_refresh_breaks();
  if (edit_file_no>=0)
  {  file2dirty(edit_file_no)=(int)ed_send(SCI_GETMODIFY,0,0);
     curPos[edit_file_no]=(int)ed_send(SCI_GETCURRENTPOS,0,0);
     curAnchor[edit_file_no]=(int)ed_send(SCI_GETANCHOR,0,0);
     firstLine[edit_file_no]=(int)ed_send(SCI_GETFIRSTVISIBLELINE,0,0);
  }
  edit_file_no = file_no;
  SetWindowText(hMainWnd,unique_name(edit_file_no));
  ed_send(SCI_SETDOCPOINTER,0,(LONG_PTR)doc[edit_file_no]);
  if (fullname[edit_file_no]!=NULL)
  { if (file2reading(edit_file_no)) ed_read_file();
    else check_diskfile_change();
  }
  set_tabwidth();
  set_text_style();
  set_whitespace(show_whitespace);
  ed_send(SCI_SETCODEPAGE,codepage,0);
  ed_send(SCI_SETFIRSTVISIBLELINE,firstLine[edit_file_no],0);
  ed_send(SCI_SETSEL,curPos[edit_file_no],curAnchor[edit_file_no]);
  ed_tab_select(edit_file_no);
  update_symtab();
}


void ed_set_break(int file_no, int line_no, unsigned char bits)
{ if (file_no!=edit_file_no) set_edit_file(file_no);
  ed_send(SCI_MARKERDELETE,line_no-1,-1);
  ed_send(SCI_MARKERADDSET,line_no-1,bits); /* lines start with zero */
  /* possibly stitch back to the old edit_file_no ? */
}


static int all_saved=1;
static int all_cancel=0;
static int current_file_no=-1;


static void save_file_if_needed(int file_no)
{ if (!all_saved) return;
  if (!file_dirty(file_no)) return;
  set_edit_file(file_no);
  if (!ed_save_changes(all_cancel)) all_saved=0;
}

int ed_save_all(int cancel)
{ current_file_no = edit_file_no;
  all_saved=1;
  all_cancel=cancel;
  for_all_files(save_file_if_needed);
  set_edit_file(current_file_no);
  return all_saved;
}


void ed_toggle_break(int bit)
{ int position = (int)ed_send(SCI_GETCURRENTPOS,0,0);
  int line = (int)ed_send(SCI_LINEFROMPOSITION,position,0);
  int markers = (int)ed_send(SCI_MARKERGET,line,0);
  if (markers & bit)
  { int i;
	del_file_breakpoint(edit_file_no,line+1, bit);
    for (i=0;i<4;i++)
     if (bit&(1<<i)) ed_send(SCI_MARKERDELETE,line,i);
  }
  else
  { set_file_breakpoint(edit_file_no,line+1, bit,bit);
    ed_send(SCI_MARKERADDSET,line,bit);
  }
}

static void init_filemarkers(void)
{ HICON hIcon;
  if (hFileMarkers!=NULL) return;
  hFileMarkers=ImageList_Create(16,16,ILC_COLOR|ILC_MASK,1,1);
  hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_LOAD_FILE));
  ImageList_AddIcon(hFileMarkers, hIcon);
}

void init_edit(HINSTANCE hInstance)
{	Scintilla_RegisterClasses(hInstance);
	Scintilla_LinkLexers();
    register_editor(hInst);	
    init_filemarkers();
}



static LRESULT CALLBACK EditorProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{ 	
  switch (message) 
  { case WM_CREATE:
         hSCe = CreateWindowEx(WS_EX_LEFT,"Scintilla","Editor",WS_CHILD|WS_VISIBLE|WS_TABSTOP|WS_CLIPCHILDREN,
		       0,0,0,0,hWnd,NULL,hInst,NULL);
         ed_fn = (SciFnDirect)SendMessage(hSCe,SCI_GETDIRECTFUNCTION,0,0);
         ed_ptr= (sptr_t)SendMessage(hSCe,SCI_GETDIRECTPOINTER,0,0);
		 hTabs = CreateWindowEx(WS_EX_LEFT,WC_TABCONTROL,"EditorTabs",
			           WS_CHILD|WS_TABSTOP|WS_CLIPCHILDREN|WS_CLIPSIBLINGS,
		               0,0,0,0,hWnd,NULL,hInst,NULL);
		 TabCtrl_SetImageList(hTabs,hFileMarkers);
         SendMessage(hTabs,WM_SETFONT,(WPARAM)hGUIFont,0);
		 DragAcceptFiles(hWnd,TRUE);
	     return 0;
    case WM_SIZE:
		if (wParam==SIZE_RESTORED || SIZE_MAXIMIZED)
		{ if (TabCtrl_GetItemCount(hTabs)>1)
		  {  RECT r;
		     r.top=r.left=0;
		     r.bottom=HIWORD(lParam);
		     r.right=LOWORD(lParam);
		     TabCtrl_AdjustRect(hTabs,FALSE,&r);
			 ShowWindow(hTabs,SW_SHOW);
		     MoveWindow(hTabs,0,0,LOWORD(lParam),HIWORD(lParam), TRUE);
             MoveWindow(hSCe,r.left,r.top,r.right-r.left,r.bottom-r.top, TRUE);
		   }
		   else
		   { ShowWindow(hTabs,SW_HIDE);
		     MoveWindow(hSCe,0,0,LOWORD(lParam),HIWORD(lParam), TRUE);
		   }
		}
		break;
	case WM_GETMINMAXINFO:
	{ MINMAXINFO *p = (MINMAXINFO *)lParam;
	  p->ptMinTrackSize.x  = 200;
      p->ptMinTrackSize.y = 50;
	  p->ptMaxTrackSize.x=p->ptMinTrackSize.x;
	  p->ptMaxTrackSize.y=p->ptMinTrackSize.y;
	}
	return 0;
	case WM_DROPFILES:
	  { HDROP hDrop = (HDROP)wParam;
	    char name[MAX_PATH+1];
		DragQueryFile(hDrop,0,name,MAX_PATH);
		set_edit_file(filename2file(name));
		DragFinish(hDrop);
	  }
	  return 0;
	case WM_CLOSE:
		ed_close();		  
		return 0;
	case WM_DESTROY:
		ed_close_all(0);		  
		hEdit=NULL;
		hSCe=NULL;
		hTabs=NULL;
		break;
    case WM_NOTIFY:
	  { NMHDR *n = (NMHDR*)lParam;
	    if (n->hwndFrom==hTabs)
		{ if (n->code == TCN_SELCHANGE)
		  { int index = TabCtrl_GetCurSel(hTabs);
 			TCITEM tie;
			tie.mask=TCIF_PARAM;
			if (TabCtrl_GetItem(hTabs,index,&tie))
			  set_edit_file((int)tie.lParam);
		  }
		}
	    else if (n->hwndFrom=hSCe)
	    {  if (n->code == SCN_MARGINCLICK)
		  { struct SCNotification *sn=(struct SCNotification*)n;
		    int line, brkp;
		    line = (int)ed_send(SCI_LINEFROMPOSITION,sn->position,0);
		    brkp = (int)ed_send(SCI_MARKERGET,line,0);
		    brkp &= 1<<MMIX_BREAKX_MARKER;
            if (brkp)
		    { del_file_breakpoint(edit_file_no,line+1,exec_bit);
		      ed_send(SCI_MARKERDELETE,line,MMIX_BREAKX_MARKER);
		    }
		    else
		    { set_file_breakpoint(edit_file_no,line+1,exec_bit,exec_bit);
		      ed_send(SCI_MARKERADD,line,MMIX_BREAKX_MARKER);
		    }
		  }
		  else if(n->code==SCN_MODIFIED)
		  { struct SCNotification *sn=(struct SCNotification*)n;
		    if (sn->linesAdded!=0) clear_stop_marker();
		  }
	    }
	  return 0;
	case WM_COMMAND:
		{ HWND hControl = (HWND)lParam;
	      if (hControl==hSCe && HIWORD(wParam)==SCEN_SETFOCUS)
		  { check_diskfile_change();
		    /* update_symtab(); */
		  }
		}
	  }
	}
  return (DefWindowProc(hWnd, message, wParam, lParam));
}



void ed_zoom(int fontsize)
{ ed_send(SCI_SETZOOM,fontsize-DEFAULT_FONT_SIZE,0);
  set_lineno_width();
}

void ed_set_ascii(void)
{ ed_send(SCI_SETCODEPAGE,0,0);
  codepage=0;
}

void ed_set_utf8(void)
{ ed_send(SCI_SETCODEPAGE,SC_CP_UTF8,0);
  codepage=SC_CP_UTF8;
}



#define MMIXAL_COLORS (SCE_MMIXAL_INCLUDE+1)
COLORREF syntax_color[MMIXAL_COLORS] =
{       
  RGB(0x00,0x00,0x00),		//SCE_MMIXAL_LEADWS 0
  RGB(0x00,0xB0,0x00),		//SCE_MMIXAL_COMMENT 1
  RGB(0x00,0x00,0x00),		//SCE_MMIXAL_LABEL 2
  RGB(0x00,0x00,0x00),		//SCE_MMIXAL_OPCODE 3
  RGB(0x00,0x00,0x00),		//SCE_MMIXAL_OPCODE_PRE 4
  RGB(0x80,0x00,0x80),		//SCE_MMIXAL_OPCODE_VALID 5
  RGB(0xFF,0x00,0x00),		//SCE_MMIXAL_OPCODE_UNKNOWN 6
  RGB(0x00,0x00,0x00),		//SCE_MMIXAL_OPCODE_POST 7
  RGB(0x00,0x00,0x00),		//SCE_MMIXAL_OPERANDS 8
  RGB(0x00,0x00,0x00),		//SCE_MMIXAL_NUMBER 9
  RGB(0x00,0x00,0x00),		//SCE_MMIXAL_REF 10
  RGB(0x00,0x00,0x00),		//SCE_MMIXAL_CHAR 11
  RGB(0x00,0x00,0x00),		//SCE_MMIXAL_STRING 12
  RGB(0x00,0x00,0xFF),		//SCE_MMIXAL_REGISTER 13
  RGB(0x00,0x00,0x00),		//SCE_MMIXAL_HEX 14
  RGB(0x00,0x00,0x00),		//SCE_MMIXAL_OPERATOR 15
  RGB(0x00,0x80,0xFF),		//SCE_MMIXAL_SYMBOL 16
  RGB(0x00,0x00,0x00)		//SCE_MMIXAL_INCLUDE 17
};


static void mms_style(void)
/* configure the MMIXAL lexer */
{  int stylebits;  
   ed_send(SCI_SETLEXER,SCLEX_MMIXAL,0);
   stylebits= (int)ed_send(SCI_GETSTYLEBITSNEEDED,0,0);
   ed_send(SCI_SETSTYLEBITS,stylebits,0);
   
   ed_send(SCI_SETKEYWORDS,0,(sptr_t)mmix_opcodes);
   ed_send(SCI_STYLESETFORE,SCE_MMIXAL_OPCODE_VALID,syntax_color[SCE_MMIXAL_OPCODE_VALID]);
   ed_send(SCI_STYLESETFORE,SCE_MMIXAL_OPCODE_UNKNOWN,syntax_color[SCE_MMIXAL_OPCODE_UNKNOWN]);

   ed_send(SCI_SETKEYWORDS,1,(sptr_t)mmix_special_registers);
   ed_send(SCI_STYLESETFORE,SCE_MMIXAL_REGISTER,syntax_color[SCE_MMIXAL_REGISTER]);

   ed_send(SCI_SETKEYWORDS,2,(sptr_t)mmix_predefined);
   ed_send(SCI_STYLESETFORE,SCE_MMIXAL_SYMBOL,syntax_color[SCE_MMIXAL_SYMBOL]);

   ed_send(SCI_STYLESETFORE,SCE_MMIXAL_COMMENT,syntax_color[SCE_MMIXAL_COMMENT]);
}

static void txt_style(void)
/* configure the NULL lexer */
{  int stylebits;  
   ed_send(SCI_SETLEXER,SCLEX_NULL,0);
   stylebits= (int)ed_send(SCI_GETSTYLEBITSNEEDED,0,0);
   ed_send(SCI_SETSTYLEBITS,stylebits,0);
   ed_send(SCI_STYLESETFORE,0,RGB(0x0,0x0,0x0));
   ed_send(SCI_STYLESETBACK,0,RGB(0xFF,0xFF,0xFF));
   ed_send(SCI_COLOURISE,0,-1);
}

void set_lineno_width(void)
{ int width;
  if (show_line_no)
  { char zeros[]= "00000";
    char *number;
    int lines;
    lines=(int)ed_send(SCI_GETLINECOUNT,0,0);
    if (lines<100) number= zeros+2;
    else if (lines<1000) number = zeros+1;
    else number=zeros;
    width =(int) ed_send(SCI_TEXTWIDTH,MMIX_LINE_MARGIN,(sptr_t)number);
  }
  else
    width = 0;
  ed_send(SCI_SETMARGINWIDTHN,MMIX_LINE_MARGIN,width);
}

static int profile_digits=2;
unsigned int max_profile_data=0;
static unsigned int limit_profile_data=100;

void reset_profile_width(void)
{ profile_digits=2;
  max_profile_data=0;
  limit_profile_data=100;
  set_profile_width();
}


void set_profile_width(void)
{ int width;
static int old_width=-1;
#define MAX_PROFILE_DIGITS 11
  static char zeroes[MAX_PROFILE_DIGITS+1]="00000000000";
  if (show_profile)
#if 0  
  width =(int) ed_send(SCI_TEXTWIDTH,MMIX_PROFILE_MARGIN,(sptr_t)"000000");
#else
  { while (max_profile_data>=limit_profile_data && profile_digits<MAX_PROFILE_DIGITS)
    { limit_profile_data=limit_profile_data*10;
      profile_digits++;
    }
	width =separator_width+(int) ed_send(SCI_TEXTWIDTH,MMIX_PROFILE_MARGIN,(sptr_t)(zeroes+sizeof(zeroes)-1-profile_digits));
  }
#endif
  else
    width = 0;
#if 0
  ed_send(SCI_SETMARGINWIDTHN,MMIX_PROFILE_MARGIN,width);
#else
  if (width!=old_width)
  { ed_send(SCI_SETMARGINWIDTHN,MMIX_PROFILE_MARGIN,width);
    old_width=width;
  }
#endif
}

#define STYLE_PROFILE (STYLE_LASTPREDEFINED+1)


static void show_profile_range(int from, int to)
{ int line_no;
  int *p,*freq;
  p=malloc(sizeof(int)*(to-from+1));
  if (p==NULL)
  { win32_error(__LINE__, "Out of memory");
    return;
  }
  memset(p,0xFF,sizeof(int)*(to-from+1)); /* initialize with -1 */
  freq=p-from; /* now we have freq[from..to] */
  line2freq(edit_file_no,from,to,freq);
#if 1
  set_profile_width();
#endif
  for (line_no=from;line_no<=to; line_no++)
  { char number[21];
	if (freq[line_no]<=0) continue;
    sprintf(number,"%d",freq[line_no]);
	ed_send(SCI_MARGINSETTEXT,line_no, (sptr_t)number);
    ed_send(SCI_MARGINSETSTYLE,line_no, STYLE_PROFILE);
  }
  free(p);
}

void update_profile(void)
{ int from, to;
  if (!show_profile) return;

  from = (int)ed_send(SCI_GETFIRSTVISIBLELINE,0,0);
  to= from+(int)ed_send(SCI_LINESONSCREEN,0,0);
  show_profile_range(from,to);
}


void display_profile(void)
{   if (!show_profile) return;
	show_profile_range(0,(int)ed_send(SCI_GETLINECOUNT,0,0));
}

void clear_profile(void)
{ clear_profile_data();
  ed_send(SCI_MARGINTEXTCLEARALL,0,0);
  reset_profile_width();
}

void set_tabwidth(void)
{   ed_send(SCI_SETTABWIDTH,tabwidth,0);
}



#include "Icons\brkexec.c"
#include "Icons\brkread.c"
#include "Icons\brkwrite.c"
#include "Icons\brktrace.c"

void new_edit(void)
{ 
  if (hEdit==NULL) create_edit();
  /* configure the style */
   ed_send(SCI_STYLESETFONT,STYLE_DEFAULT,(sptr_t)"Courier New");
   ed_send(SCI_STYLESETSIZE,STYLE_DEFAULT,(sptr_t)DEFAULT_FONT_SIZE);
   ed_send(SCI_STYLESETWEIGHT,STYLE_DEFAULT, SC_WEIGHT_BOLD);
   ed_send(SCI_SETSCROLLWIDTH,80*fixed_char_width,0);
   ed_send(SCI_SETVISIBLEPOLICY,CARET_SLOP|CARET_STRICT,5);
   ed_send(SCI_SETYCARETPOLICY,CARET_SLOP|CARET_STRICT|CARET_EVEN,5);
   
   set_text_style();
   /* configure margins and markers */
   /* line numbers */
   ed_send(SCI_SETMARGINTYPEN,MMIX_LINE_MARGIN,SC_MARGIN_NUMBER);
   set_lineno_width();

   /* profile margin */
   ed_send(SCI_SETMARGINTYPEN,MMIX_PROFILE_MARGIN,SC_MARGIN_RTEXT);
   ed_send(SCI_SETMARGINMASKN,MMIX_PROFILE_MARGIN,0);
   ed_send(SCI_STYLESETFORE,STYLE_PROFILE, RGB(0x00,0x00,0xD0));
   ed_send(SCI_STYLESETBACK,STYLE_PROFILE, ed_send(SCI_STYLEGETBACK,STYLE_LINENUMBER,0));

   set_profile_width();
   update_profile();

   /* errors */
   ed_send(SCI_MARKERDEFINE, MMIX_ERROR_MARKER,SC_MARK_BACKGROUND);
   ed_send(SCI_MARKERSETBACK, MMIX_ERROR_MARKER,RGB(0xFF,0xA0,0xA0));
   ed_send(SCI_MARKERSETFORE, MMIX_ERROR_MARKER,RGB(0,0,0));

   /* break points */
   ed_send(SCI_SETMARGINTYPEN,MMIX_BREAK_MARGIN,SC_MARGIN_SYMBOL);
   ed_send(SCI_SETMARGINWIDTHN,MMIX_BREAK_MARGIN,16);
   ed_send(SCI_SETMARGINSENSITIVEN,MMIX_BREAK_MARGIN,1);
   ed_send(SCI_SETMARGINMASKN,MMIX_BREAK_MARGIN,
	   (1<<MMIX_BREAKX_MARKER)|(1<<MMIX_BREAKR_MARKER)|
	   (1<<MMIX_BREAKW_MARKER)|(1<<MMIX_BREAKT_MARKER)	   
	   );

   ed_send(SCI_RGBAIMAGESETWIDTH,brkexec.width,0);
   ed_send(SCI_RGBAIMAGESETHEIGHT,brkexec.height,0);
   ed_send(SCI_MARKERDEFINERGBAIMAGE,MMIX_BREAKX_MARKER,(LONG_PTR)brkexec.pixel_data);

   ed_send(SCI_RGBAIMAGESETWIDTH,brktrace.width,0);
   ed_send(SCI_RGBAIMAGESETHEIGHT,brktrace.height,0);
   ed_send(SCI_MARKERDEFINERGBAIMAGE,MMIX_BREAKT_MARKER,(LONG_PTR)brktrace.pixel_data);

   ed_send(SCI_RGBAIMAGESETWIDTH,brkread.width,0);
   ed_send(SCI_RGBAIMAGESETHEIGHT,brkread.height,0);
   ed_send(SCI_MARKERDEFINERGBAIMAGE,MMIX_BREAKR_MARKER,(LONG_PTR)brkread.pixel_data);

   ed_send(SCI_RGBAIMAGESETWIDTH,brkwrite.width,0);
   ed_send(SCI_RGBAIMAGESETHEIGHT,brkwrite.height,0);
   ed_send(SCI_MARKERDEFINERGBAIMAGE,MMIX_BREAKW_MARKER,(LONG_PTR)brkwrite.pixel_data);

   /* tracing */
   ed_send(SCI_SETMARGINTYPEN,MMIX_TRACE_MARGIN,SC_MARGIN_BACK);
   ed_send(SCI_SETMARGINWIDTHN,MMIX_TRACE_MARGIN,16);
   ed_send(SCI_SETMARGINMASKN,MMIX_TRACE_MARGIN,(1<<MMIX_TRACE_MARKER));
   ed_send(SCI_MARKERDEFINE, MMIX_TRACE_MARKER,SC_MARK_ARROW);
   ed_send(SCI_MARKERSETBACK, MMIX_TRACE_MARKER,RGB(0xFF,0x00,0x00));
   ed_send(SCI_MARKERSETFORE, MMIX_TRACE_MARKER,RGB(0xFF,0,0));


   /* kommand keys for the edit windows*/
   ed_send(SCI_ASSIGNCMDKEY,VK_OEM_PLUS+(SCMOD_CTRL<<16),SCI_ZOOMIN);
   ed_send(SCI_ASSIGNCMDKEY,VK_OEM_MINUS+(SCMOD_CTRL<<16),SCI_ZOOMOUT);
   ed_send(SCI_ASSIGNCMDKEY,'/'+(SCMOD_CTRL<<16),SCI_SETZOOM);
  /* modify delete line keys */
   ed_send(SCI_ASSIGNCMDKEY,SCK_DELETE+(SCMOD_CTRL<<16),SCI_DELLINERIGHT);
   ed_send(SCI_ASSIGNCMDKEY,SCK_DELETE+(SCMOD_SHIFT<<16),SCI_LINEDELETE);
   ed_send(SCI_ASSIGNCMDKEY,SCK_BACK+(SCMOD_CTRL<<16),SCI_DELLINELEFT);


   /* set standatd encoding */
   SendMessage(hMainWnd,WM_COMMAND,(WPARAM)ID_ENCODING_ASCII,(LPARAM)0);
}


void ed_new(void)
{ int file_no;
  if (hEdit==NULL) new_edit();
  file_no = filename2file(NULL);
  set_edit_file(file_no);
/*	ed_send(SCI_SETSAVEPOINT,0,0);
	ed_send(SCI_CANCEL,0,0);
*/
}

int ed_close(void)
/* return 1 if file was closed zero otherwise */
{ int file_no;
  if (!ed_save_changes(1)) return 0;
  close_file(edit_file_no);
  edit_file_no=-1; /* no edit file yet */
  file_no=get_inuse_file();
  if (file_no>=0) set_edit_file(file_no);
  else ed_new();
  return 1;
}

static int aux_cancel;
static void aux_close_file(int file_no)
{ set_edit_file(file_no);
  if (!ed_save_changes(aux_cancel)) return;
  close_file(edit_file_no);
}

int ed_close_all(int cancel)
/* return 1 if all files could be closed zero otherwise */
{ int file_no;
  aux_cancel=cancel;
  for_all_files(aux_close_file);
  edit_file_no=-1; /* no edit file yet */
  file_no=get_inuse_file();
  if (file_no>=0) 
  { int decission;
	set_edit_file(file_no);
    decission= MessageBox(hMainWnd, "There are unsaved files. Exit anyway?","Unsaved Files", MB_YESNO|MB_ICONWARNING);
	if (decission == IDYES)
	{   for_all_files(close_file);
		return 1;
	}
    else return 0;
  }
  else
  { ed_new();
    return 1;
  }
}



int ed_open(void)
/* opens a file as a new document */
{ 	char name[MAX_PATH+1] = "\0";
	OPENFILENAME ofn={0};
	ofn.lStructSize= sizeof(OPENFILENAME);
	ofn.hwndOwner = hMainWnd;
	ofn.hInstance = hInst;
	ofn.lpstrFile = name;
	ofn.nMaxFile = MAX_PATH;
	ofn.lpstrFilter = "MMIX (.mms)\0*.mms\0" "All Files (*.*)\0*.*\0\0";
	ofn.lpstrCustomFilter = NULL;
	ofn.nMaxCustFilter = 0;
	ofn.nFilterIndex = 1;
	ofn.lpstrTitle = "Open File";
	ofn.Flags = OFN_HIDEREADONLY;
    if (GetOpenFileName(&ofn)) 
      return filename2file(name);
	else
      return -1;
}

#if 0
static void ed_write_file(void)
{	FILE *fp;
    char *name = fullname[edit_file_no];
    if (name==NULL || name[0]==0) return;
    fp = fopen(name, "wb");
	if (fp==NULL)
       win32_ferror(__LINE__,"Unable to save file (%s)\r\n",name);
	else {
		int i;
		struct Sci_TextRange tr;
		char data[ED_BLOCKSIZE+1];
		int len = (int)ed_send(SCI_GETLENGTH,0,0);
	    tr.lpstrText = data;
		for (i = 0; i < len; i += ED_BLOCKSIZE) 
		{ int next = len - i;
		  if (next > ED_BLOCKSIZE) next = ED_BLOCKSIZE;
	      tr.chrg.cpMin = i;
	      tr.chrg.cpMax = i+next;
	      ed_send(SCI_GETTEXTRANGE, 0,(sptr_t)&tr);
		  fwrite(data, next, 1, fp);
		}
		fclose(fp);
		ed_send(SCI_SETSAVEPOINT,0,0);
	}
}
#else
static int ed_write_file(void)
{ HANDLE fh;
    int ok=1;
    char *name = fullname[edit_file_no];
    if (name==NULL || name[0]==0) return 0;
    fh = CreateFile(name,
	  GENERIC_WRITE,FILE_SHARE_WRITE,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
    if (fh==INVALID_HANDLE_VALUE) 
	{  win32_ferror(__LINE__,"Unable to save file (%s)\r\n",name);
	   return 0;
	}
	else {
		int i;
		struct Sci_TextRange tr;
		char data[ED_BLOCKSIZE+1];
		int len;
		len= (int)ed_send(SCI_GETLENGTH,0,0);
	    tr.lpstrText = data;
		for (i = 0; i < len; i += ED_BLOCKSIZE) 
		{ DWORD nextw;
		  DWORD next = len - i;
		  if (next > ED_BLOCKSIZE) next = ED_BLOCKSIZE;
	      tr.chrg.cpMin = i;
	      tr.chrg.cpMax = i+next;
	      ed_send(SCI_GETTEXTRANGE, 0,(sptr_t)&tr);
		  if (!WriteFile(fh,data,next,&nextw,NULL) || nextw<next)
		  { win32_ferror(__LINE__,"Unable to write file (%s)\r\n",name);
		    ok=0;
		    break;
		  }
		}
		CloseHandle(fh);
	    file_time[edit_file_no]=ftime(name);
		ed_send(SCI_SETSAVEPOINT,0,0);
    }
	return ok;
}
#endif

int ed_save(void)
{ if (hEdit==NULL) return 0;
  if (file2fullname(edit_file_no)==NULL)
    return ed_save_as();
  else
	return ed_write_file();
}

int ed_save_as(void)
{	char *name=NULL;
	char asname[MAX_PATH+1] = "\0";
	OPENFILENAME ofn ={0};
	if (hEdit==NULL) return 0;
	if (edit_file_no>=0)
	  name = file2fullname(edit_file_no);
	if (name!=NULL)
	  strncpy(asname,name ,MAX_PATH);
	else
	  strncpy(asname,unique_name(edit_file_no),MAX_PATH);
	ofn.lStructSize= sizeof(ofn);
	ofn.lpstrFile = asname;
	ofn.hwndOwner = hMainWnd;
	ofn.hInstance = hInst;
	ofn.lpstrFile = asname;
	ofn.lpstrFilter = "MMIX (.mms)\0*.mms\0" "All Files (*.*)\0*.*\0\0";
	ofn.nMaxFile = MAX_PATH;
	ofn.lpstrTitle = "Save File";
	ofn.lpstrDefExt = "mms";
	ofn.Flags = OFN_HIDEREADONLY|OFN_OVERWRITEPROMPT;

	if (GetSaveFileName(&ofn))
	{  file_set_name(edit_file_no,asname);
	   ed_write_file();
	   file2reading(edit_file_no)=0;
	   SetWindowText(hMainWnd,unique_name(edit_file_no));
	  //InvalidateRect(hEdit, NULL, NULL);
	   return 1;
	}
	else
	  return 0;
}

int ed_save_changes(int cancel)
{ int dirty;
  if (hEdit==NULL) return 1;
  dirty = (int)ed_send(SCI_GETMODIFY,0,0);
  if (dirty)
  {	 
     if (autosave)
	 {  return ed_save();
	 }
	 else 
	 { int decision= MessageBox(hMainWnd, "Save changes ?", unique_name(edit_file_no), cancel?MB_YESNOCANCEL:MB_YESNO);
	   if (decision == IDYES)
	     return ed_save();
	   else if (decision == IDCANCEL)
	     return 0;
	 }
  }
  return 1;
}



static int previous_error_line_no=-1;


void ide_clear_error_marker(void)
{ ed_send(SCI_MARKERDELETEALL,MMIX_ERROR_MARKER,0);
  previous_error_line_no=-1;
}

void ed_mark_error(int file_no, int line_no)
{ 
  set_edit_file(file_no);
  if (previous_error_line_no>=0)
	ed_send(SCI_MARKERDELETE, previous_error_line_no-1, MMIX_ERROR_MARKER);
  if (line_no>=0) {
    ed_send(SCI_MARKERADD, line_no-1, MMIX_ERROR_MARKER);
    ed_send(SCI_ENSUREVISIBLEENFORCEPOLICY,line_no-1,0);  
    /* ed_send(SCI_GOTOLINE,line_no-1,0);  not needed */
 }
  previous_error_line_no=line_no;
}
static int previous_mmix_line_no = -1;

void clear_stop_marker(void)
{ if (previous_mmix_line_no > 0)
  {  ed_send(SCI_MARKERDELETEALL,MMIX_TRACE_MARKER,0);  
     previous_mmix_line_no= -1;
  }
}

void show_stop_marker(int file_no, int line_no)
{ set_edit_file(file_no);
  if (previous_mmix_line_no > 0)
	PostMessage(hSCe,SCI_MARKERDELETE,previous_mmix_line_no-1,MMIX_TRACE_MARKER);
  if (line_no>0) {
    PostMessage(hSCe,SCI_MARKERADD, line_no-1, MMIX_TRACE_MARKER);
	PostMessage(hSCe,SCI_ENSUREVISIBLEENFORCEPOLICY,line_no-1,0); 
//	PostMessage(hSCe,SCI_GOTOLINE,line_no-1,0); /* first line is 0 */
  }
  previous_mmix_line_no=line_no;
}

void ide_clear_mmix_line(void)
{  ed_send(SCI_MARKERDELETEALL,MMIX_TRACE_MARKER,0);
   previous_mmix_line_no=-1;
}

int show_whitespace=0;
int syntax_highlighting = 0;
int codepage=0;

void set_whitespace(int ws)
{ show_whitespace=ws;
	if (show_whitespace)
		  {  ed_send(SCI_SETVIEWWS,SCWS_VISIBLEALWAYS,0);
		     ed_send(SCI_SETVIEWEOL,1,0);
		  }
  		  else
		  {  ed_send(SCI_SETVIEWWS,SCWS_INVISIBLE,0);
		     ed_send(SCI_SETVIEWEOL,0,0);
		  }
}

void ed_refresh_breaks(void)
/*mark all breakpoint belonging to the edit_file,
  iterate over the markers 
   - set the corresponding break bits 
   - create a new breakpoint if it does not exist.
   - remove the mark if it does exist
  sweep (remove) all breakpoints that still have marks 
*/
{ int line=-1;
  if (hEdit==NULL) return; 
  mark_all_file_breakpoints(edit_file_no);
  while ((line = (int)ed_send(SCI_MARKERNEXT,line+1,
	                           (1<<MMIX_BREAKX_MARKER)|(1<<MMIX_BREAKT_MARKER)|
	                           (1<<MMIX_BREAKR_MARKER)|(1<<MMIX_BREAKW_MARKER)
							   ))>=0)
  { int markers = (int)ed_send(SCI_MARKERGET,line,0);
    int bits =0;

    if (markers & (1<<MMIX_BREAKX_MARKER)) bits|=exec_bit;
    if (markers & (1<<MMIX_BREAKR_MARKER)) bits|=read_bit;
    if (markers & (1<<MMIX_BREAKW_MARKER)) bits|=write_bit;
    if (markers & (1<<MMIX_BREAKT_MARKER)) bits|=trace_bit;
	if (bits!=0)
      set_file_breakpoint(edit_file_no,line+1,bits,0);
  }
  sweep_all_file_breakpoints(edit_file_no);
}

void  ed_show_line(int line_no)
{  ed_send(SCI_ENSUREVISIBLEENFORCEPOLICY,line_no-1,0); /* scroll into view */
   ed_send(SCI_GOTOLINE,line_no-1,0); /* position cursor */
   SetFocus(hSCe);
}

static is_mms_file(void)
{ char *name=file2shortname(edit_file_no);
  size_t n; 
  if (name==NULL) return 1;
  n = strlen(name);
  if (n>4 && strncmp(name+n-4,".mms",4)==0) return 1;
  if (n>4 && strncmp(name+n-4,".MMS",4)==0) return 1;
  return 0;
}


void set_text_style(void)
{ if (syntax_highlighting && is_mms_file())
     mms_style();
  else
     txt_style();
}

void *ed_create_document(void)
{ return (void *)ed_send(SCI_CREATEDOCUMENT,0,0);
}

void ed_release_document(int file_no)
{ if (doc[file_no]!=NULL) 
  {	ed_send(SCI_RELEASEDOCUMENT,0,(LONG_PTR)doc[file_no]);
    doc[file_no]=NULL;
  }
}




static int find_tab(int file_no)
/* searches the tab control for the given file_no. Returns the index if found, -1 otherwise. */
{ int count = TabCtrl_GetItemCount(hTabs);
  int index;
  for (index=0; index<count;index++)
  { TCITEM tie;
    tie.mask=TCIF_PARAM;
	if (TabCtrl_GetItem(hTabs,index,&tie) && tie.lParam==file_no) return index;
  }
  return -1;
}

static void resize_tab(void)
{ RECT r;
  GetWindowRect(hEdit,&r);
  SendMessage(hEdit,WM_SIZE,SIZE_RESTORED,MAKELONG( r.right-r.left,r.bottom-r.top));
}

void ed_tab_select(int file_no)
{  int index = find_tab(file_no);
   if (index>=0)
	 TabCtrl_SetCurSel(hTabs,index);
   else
	 ed_add_tab(file_no);
}

void ed_remove_tab(int file_no)
{ int index = find_tab(file_no);
  if (index>=0)
  {  TabCtrl_DeleteItem (hTabs, index);
     if (TabCtrl_GetItemCount(hTabs)==1) resize_tab();
  }
}


int ed_show_tab(int file_no)
{ TCITEM tie;
  int index;
  if (hTabs==NULL) return -1;
  if (file_no<0) return -1;
  index = find_tab(file_no);
  if (index>=0)
  { tie.mask = TCIF_TEXT|TCIF_PARAM|TCIF_IMAGE;
    tie.pszText = unique_name(file_no);
    tie.lParam=file_no;
	tie.iImage= (file2loading(file_no))?0:-1;
    TabCtrl_SetItem (hTabs, index, &tie);
  }
  return index;
}

int ed_add_tab(int file_no)
{ int index;
  if (hTabs==NULL) return -1;
  if (file_no<0) return -1;
  index = ed_show_tab(file_no);
  if (index <0)
  { TCITEM tie;
    index = TabCtrl_GetItemCount(hTabs);
    tie.mask = TCIF_TEXT|TCIF_PARAM|TCIF_IMAGE;
    tie.pszText = unique_name(file_no);
    tie.lParam=file_no;
	if (file2loading(file_no)) tie.iImage=0; else tie.iImage=-1;
    TabCtrl_InsertItem (hTabs, index, &tie);
	if (index==1) resize_tab();
	TabCtrl_SetCurSel (hTabs, index);
  }
  return index;
}
