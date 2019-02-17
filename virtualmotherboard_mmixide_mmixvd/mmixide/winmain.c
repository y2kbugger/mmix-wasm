#include <windows.h>
#include <commctrl.h>
#include <afxres.h>
#include <htmlhelp.h> 
#include <stdio.h>
#include <process.h>
#include "winopt.h"
#include "inspect.h"
#include "resource.h"
#include "splitter.h"
#include "buttonbar.h"
#include "debug.h"
//#include "option.h"
#include "mmixlib.h"
#include "mmixrun.h"
#ifdef VMB
#include "vmb.h"
#include "mmix-bus.h"
#include "opt.h"
#endif
#include "findreplace.h"
#include "print.h"
#include "info.h"
#include "param.h"
#include "symtab.h"
#include "mmixdata.h"
#include "edit.h"
#include "assembler.h"
#include "sources.h"
#include "debug.h"
#include "breakpoints.h"
#include "editor.h"
#include "winlog.h"
#include "runoptions.h"
#include "indent.h"
#include "mmixwinde.h"
#include "winde.h"
#include "winmain.h"
#define STATIC_BUILD
#include "../scintilla/include/scintilla.h"

#pragma warning(disable : 4996)

int major_version=2, minor_version=0;
char version[]="$Revision: 653 $ $Date: 2018-11-30 17:07:44 +0100 (Fri, 30 Nov 2018) $";
#ifdef VMB
char title[] ="VMB MMIX IDE";
#else
char title[] ="MMIX Visual Debugger";
#endif

/* Button groups for the button bar */
#define BG_FILE 0
#define BG_EDIT 1
#define BG_VIEW 2
#define BG_MMIX 3
#define BG_DEBUG 4
#define BG_HELP 5


HINSTANCE hInst;
HWND	  hMainWnd;
HMENU	  hMenu;
HWND	  hSplitter;
HWND      hButtonBar;
HWND      hStatus;

HWND	  hError=NULL;


int status_width = 100; /* initial width of status window */

void ide_status(char *message)
{ SendMessage(hStatus, WM_SETTEXT,0,(LPARAM)message);
  UpdateWindow(hStatus);
  //SetWindowText(hStatus,message);
}

void ide_help(char *topic)
{ HWND hh;
  hh = HtmlHelp(hMainWnd,programhelpfile,HH_DISPLAY_TOPIC,(DWORD_PTR)topic) ;
}

void ide_add_error(char *message, int file_no, int line_no)
{ int item;
  if (hError==NULL) new_errorlist();
  item = (int) SendMessage(hError,LB_ADDSTRING,0,(LPARAM)message);
  if (item==LB_ERR||item==LB_ERRSPACE) return;
  SendMessage(hError,LB_SETITEMDATA,(WPARAM)item,item_data(file_no, line_no));
}


void ide_clear_error_list(void)
/* clear error markers in current edit file */
{   if (hError==NULL) return;
	SendMessage(hError,LB_RESETCONTENT ,0,0);
	ide_clear_error_marker();
}

#ifdef VMB
extern int auto_connect;
extern device_info vmb;

int ide_connect(void)
{ if (vmb.connected) return 1;
  if (auto_connect)
	init_mmix_bus(host,port,"MMIX IDE");
  else
  { int decision = MessageBox(hMainWnd, "Connect to Motherboard ?","VMB Connect",MB_YESNOCANCEL);
	if (decision == IDYES)
       init_mmix_bus(host,port,"MMIX IDE");
  }
  return vmb.connected;
}
#else
#define ide_connect() true
#endif

static int menu_toggle(int id)
/* toggle the menu id and return its new status */
{ if (GetMenuState(hMenu,id,MF_BYCOMMAND)&MF_CHECKED)
  { CheckMenuItem(hMenu,id,MF_BYCOMMAND|MF_UNCHECKED);
    return 0;
 }
  else
  { CheckMenuItem(hMenu,id,MF_BYCOMMAND|MF_CHECKED);
    return 1;
  }
}



static int load_count=0;

static void count_load_files(int file_no)
{ if (file2loading(file_no)) load_count++;
}
static int check_load_count(void)
{ if (!load_single_file && !missing_app) return 1;
  load_count = 0;
  for_all_files(count_load_files);
  if (load_count==0 && missing_app)
  { int decission;
	decission= MessageBox(hMainWnd, "Debug Option \"Warn if no application\" selected.\r\n"
                         "No files selected for loading. Continue?", "Warning: Missing application", 
						 MB_ICONEXCLAMATION|MB_OKCANCEL);
    if (decission!=IDOK) return 0;
  }
  if (load_count!=1 && load_single_file)
  { MessageBox(hMainWnd, "Source Option \"Load single file\" selected.",
		load_count?"Error: No file selected for loading":"Error: Multiple files selected for loading",
		MB_ICONEXCLAMATION|MB_OK);
	return 0;
  }
  return 1;
}


int ide_prepare_mmix(void)
{  if (mmix_active())
  { MessageBox(hMainWnd,"MMIX is already running.","Error",MB_OK|MB_ICONEXCLAMATION);
    return 0;
  }
  if (!ed_save_all(1)) return 0;
  ed_refresh_breaks(); 
  warning_count=0;
  if (!assemble_all_needed()) return 0;
  set_break_at_Main();
  if (warning_count==0 && auto_close_errors && hError!=NULL) 
  { DestroyWindow(hError); hError=NULL; }
  if (!check_load_count()) return 0;
  if (!execute_commands()) return 0;
#ifdef VMB
  if (!ide_connect()) return 0;
#endif
  bb_set_group(hButtonBar,BG_DEBUG,1,1);
  bb_set_group(hButtonBar,BG_EDIT,0,0);
  return 1;
}

static void change_lb_font(HWND hLB)
{ if (!hLB) return;
  SendMessage(hLB,WM_SETFONT,(WPARAM)hVarFont,0);
  SendMessage(hLB,LB_SETITEMHEIGHT ,0,(LPARAM)var_line_height);
  InvalidateRect(hLB,NULL,TRUE);
}


static void font_has_changed(void)
{ ed_zoom(fontsize);
  change_lb_font(hSymbolTable);
  change_lb_font(hBreakList);
  change_lb_font(hError);
  change_mem_font();
  if (hLog) 
  { SendMessage(hLog,WM_SETFONT,(WPARAM)hFixedFont,0);
    InvalidateRect(hLog,NULL,TRUE);
  }
  mmix_change_de_font();
}


LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{ 	
  switch (message) 
  {     case WM_CREATE:
         SetWindowPos(hWnd,HWND_TOP,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE);
        return 0;
  case WM_KILLFOCUS:
	    break;
  case WM_SETFOCUS:
/* this causes an infinite loop 
	  if (ed_setfocus())  
		  return 0;
*/	 
	 break;
  case WM_SIZE:
		if (wParam==SIZE_RESTORED || SIZE_MAXIMIZED)
		{	if (LOWORD(lParam)<4*version_width) 
		       status_width=LOWORD(lParam)/4;
		    else
				status_width=version_width;
			MoveWindow(hButtonBar, 0,0,LOWORD(lParam)-status_width,BB_HEIGHT, TRUE);
		    MoveWindow(hStatus, LOWORD(lParam)-status_width,0,status_width,BB_HEIGHT, TRUE);
			MoveWindow(hSplitter, 0,BB_HEIGHT+1,LOWORD(lParam),HIWORD(lParam)-BB_HEIGHT-1, TRUE);
		}
		return 1;
  case WM_PARENTNOTIFY:
		if (LOWORD(wParam)==WM_DESTROY)
		{ HWND hChildWnd = (HWND)lParam;
		  if (hChildWnd==hError)
		  {
			hError=NULL;
		  }
		  else if (hChildWnd==hSymbolTable)
		  { CheckMenuItem(hMenu,ID_VIEW_SYMBOLTABLE,MF_BYCOMMAND|MF_UNCHECKED);
		    hSymbolTable=NULL;
		  }
		  else  if (uncheck_memory_view(hChildWnd)) return 1;
		  else  if (uncheck_register_view(hChildWnd)) return 1;
		}
		return 1;

  case WM_MEASUREITEM:
	if (wParam==GetWindowLong(hSymbolTable,GWL_ID))
		return symtab_measureitem((LPMEASUREITEMSTRUCT)lParam);
	else
	    break;
  case WM_DRAWITEM: 
    if (wParam==GetWindowLong(hSymbolTable,GWL_ID))
		return symtab_drawitem((LPDRAWITEMSTRUCT)lParam);
	else
	    break;

  case WM_COMMAND:
    if (lParam==0)
	{ /* if (HIWORD(wParam)==0)  Menu */

#define TOGGLE_VIEW(create,destroy) if (GetMenuState(hMenu,LOWORD(wParam),MF_BYCOMMAND)&MF_CHECKED) {destroy;} else {create;}

		switch(LOWORD(wParam))
	  { case ID_FILE_EXIT:
	      if (ed_close_all(1))
		    DestroyWindow(hWnd);
	      return 0;
	    case ID_FILE_NEW:
		  ed_new();
		  return 0;
	    case ID_FILE_OPEN:
		  set_edit_file(ed_open());
		  return 0;
	    case ID_FILE_CLOSE:
          ed_close();
	      return 0;
	    case ID_FILE_SAVE:
		  ed_save();
		  return 0;
	    case ID_FILE_SAVEAS:
		  ed_save_as();
		  return 0;
	    case ID_FILE_PAGESETUP:
		  page_setup();
		  return 0;
	    case ID_FILE_PRINT:
		  print();
		  return 0;
	    case ID_EDIT_UNDO:
		  ed_operation(SCI_UNDO);
		  return 0;
	    case ID_EDIT_REDO:
		  ed_operation(SCI_REDO);
		  return 0;
	    case ID_EDIT_CUT:
		  ed_operation(SCI_CUT);
		  return 0;
	    case ID_EDIT_COPY:
		  ed_operation(SCI_COPY);
		  return 0;
	    case ID_EDIT_PASTE:
		  ed_operation(SCI_PASTE);
		  return 0;
	    case ID_EDIT_DELETE:
		  ed_operation(SCI_CLEAR);
		  return 0;
	    case ID_EDIT_SELECTALL:
		  ed_operation(SCI_SELECTALL);
		  return 0;
		case ID_EDIT_FIND:
			{ HWND h;
			  h = CreateDialog(hInst,MAKEINTRESOURCE(IDD_FIND),hWnd,FindDialogProc);
			  ShowWindow(h, SW_SHOW); 
			}
			return 0;
		case ID_EDIT_FINDAGAIN:
			find_again();
			return 0;
		case ID_EDIT_REPLACE:
			replace_again();
			return 0;
		case ID_EDIT_PRETTY:
			indent(edit_file_no);
			return 0;
		case ID_MMIX_BREAKT:
			ed_toggle_break(trace_bit);
			return 0;
		case ID_MMIX_BREAKX:
			ed_toggle_break(exec_bit);
			return 0;
		case ID_MMIX_BREAKR:
			ed_toggle_break(read_bit);
			return 0;
		case ID_MMIX_BREAKW:
			ed_toggle_break(write_bit);
			return 0;
		case ID_VIEW_SYMBOLTABLE:
			TOGGLE_VIEW(create_symtab(),DestroyWindow(hSymbolTable));
		    return 0;
  	    case ID_VIEW_TRACE:
			TOGGLE_VIEW(show_trace_window(),DestroyWindow(hLog));
		    return 0;
		case ID_VIEW_BREAKPOINTS:
			TOGGLE_VIEW(create_breakpoints(),DestroyWindow(hBreakpoints));
		    return 0;
	    case ID_VIEW_ZOOMIN:
		  change_font_size(1);
		  font_has_changed();
		  return 0;
	    case ID_VIEW_ZOOMOUT:
		  change_font_size(-1);
		  font_has_changed();
		  return 0;
		case ID_VIEW_WHITESPACE:
          set_whitespace(menu_toggle(ID_VIEW_WHITESPACE));
		  return 0;
		case ID_VIEW_LINENUMBERS:
          show_line_no=menu_toggle(ID_VIEW_LINENUMBERS);
		  set_lineno_width();
		  return 0;
		case ID_VIEW_PROFILE:
          show_profile=menu_toggle(ID_VIEW_PROFILE);
		  set_profile_width();
		  return 0;

		case ID_ENCODING_ASCII:
		  ed_set_ascii();
          CheckMenuItem(hMenu,ID_ENCODING_ASCII,MF_BYCOMMAND|MF_CHECKED);
          CheckMenuItem(hMenu,ID_ENCODING_UTF,MF_BYCOMMAND|MF_UNCHECKED);
          return 0;
		case ID_ENCODING_UTF:
		  ed_set_utf8();
          CheckMenuItem(hMenu,ID_ENCODING_ASCII,MF_BYCOMMAND|MF_UNCHECKED);
          CheckMenuItem(hMenu,ID_ENCODING_UTF,MF_BYCOMMAND|MF_CHECKED);
          return 0;
		case ID_VIEW_SYNTAX:
          syntax_highlighting=menu_toggle(ID_VIEW_SYNTAX);
		  set_text_style();
		  return 0;
		case ID_MMIX_STEP:
		  mmix_continue('s');
		  return 0;
		case ID_MMIX_STEPOVER:
		  mmix_continue('n');
		  return 0;
		case ID_MMIX_STEPOUT:
		  mmix_continue('o');
		  return 0;
	   case ID_MMIX_STOP:
		  mmix_stop();
		  return 0;
		case ID_MMIX_QUIT:
		  mmix_continue('q');
		  return 0;
		case ID_MMIX_DEBUG:
		  if (mmix_active())
		  { mmix_continue('c');
		  }
		  else 
		  { if (!ide_prepare_mmix()) return 0;
		    set_debug_windows();
		    mmix_debug();
		  }
	      return 0; 
		case ID_MMIX_RUN:
		  if (mmix_active()) return 0;
		  if (!ide_prepare_mmix()) return 0;
		  mmix_run();
	      return 0; 

	    case ID_MMIX_ASSEMBLE:
		  if (!ed_save_changes(1)) return 0;
		  ed_refresh_breaks();
		  warning_count=0;
		  if (mmix_assemble(edit_file_no)==0)
		  { 
			if (warning_count==0 && auto_close_errors && hError!=NULL) 
			  {DestroyWindow(hError); hError=NULL; }
			if (file2loading(edit_file_no) && mmix_active() 
#ifdef VMB
			  && vmb.power
#endif
			  )
				MessageBox(hWnd,"mmo file already running! Reset to reload file.", unique_name(edit_file_no),MB_OK|MB_ICONWARNING);
		  }
	      return 0; 
#ifdef VMB
	    case ID_MMIX_CONNECT:
		  if (menu_toggle(ID_MMIX_CONNECT))
		  { if (!vmb.connected)
			ide_connect();
		  }
		  else
		  { vmb_disconnect(&vmb);
		  }
		  return 0;
		  if (vmb.connected) mmix_status(MMIX_CONNECTED);
		  else mmix_status(MMIX_DISCONNECTED);
		  CheckMenuItem(hMenu,ID_MMIX_CONNECT,MF_BYCOMMAND|(vmb.connected?MF_CHECKED:MF_UNCHECKED));
	      return 0;
#endif
	    case ID_MEM_TEXTSEGMENT:
		  if (!ide_connect()) return 0;
		  TOGGLE_VIEW(new_memory_view(0),close_memory_view(0));
		  return 0;
	    case ID_MEM_DATASEGMENT:
		  if (!ide_connect()) return 0;
		  TOGGLE_VIEW(new_memory_view(1),close_memory_view(1));
		  return 0;
	    case ID_MEM_POOLSEGMENT:
		  if (!ide_connect()) return 0;
		  TOGGLE_VIEW(new_memory_view(2),close_memory_view(2));
		  return 0;
	    case ID_MEM_STACKSEGMENT:
		  if (!ide_connect()) return 0;
		  TOGGLE_VIEW(new_memory_view(3),close_memory_view(3));
		  return 0;
	    case ID_MEM_NEGATIVESEGMENT:
		  if (!ide_connect()) return 0;
  		  TOGGLE_VIEW(new_memory_view(4),close_memory_view(4));
		  return 0;
	    case ID_REGISTERS_LOCAL:
  		  TOGGLE_VIEW(new_register_view(REG_LOCAL),close_register_view(REG_LOCAL));
		  return 0;
	    case ID_REGISTERS_GLOBAL:
  		  TOGGLE_VIEW(new_register_view(REG_GLOBAL),close_register_view(REG_GLOBAL));
		  return 0;
	    case ID_REGISTERS_SPECIAL:
  		  TOGGLE_VIEW(new_register_view(REG_SPECIAL),close_register_view(REG_SPECIAL));
		  return 0;
		case ID_REGISTERS_STACK:
		  show_debug_regstack=!show_debug_regstack;
		  CheckMenuItem(hMenu,ID_REGISTERS_STACK,MF_BYCOMMAND|(show_debug_regstack?MF_CHECKED:MF_UNCHECKED));
		  if (show_debug_regstack)
		    new_register_view(REG_LOCAL);
		  regstack_update();
		  return 0;
		case ID_OPTIONS_EDITOR:
		  DialogBox(hInst,MAKEINTRESOURCE(IDD_OPTIONS_EDITOR),hWnd,OptionEditorDialogProc);
		  return 0;
		case ID_OPTIONS_SYMTAB:
		  DialogBox(hInst,MAKEINTRESOURCE(IDD_OPTIONS_SYMTAB),hWnd,OptionSymtabDialogProc);
		  return 0;
		case ID_OPTIONS_ASSEMBLER:
		  DialogBox(hInst,MAKEINTRESOURCE(IDD_OPTIONS_ASSEMBLER),hWnd,OptionAssemblerDialogProc);
		  return 0;
		case ID_OPTIONS_DEBUG:
		  DialogBox(hInst,MAKEINTRESOURCE(IDD_SHOW_DEBUG),hWnd,OptionDebugDialogProc);
		  return 0;
		case ID_OPTIONS_RUN:
		  DialogBox(hInst,MAKEINTRESOURCE(IDD_OPTIONS_RUN),hWnd,OptionRunDialogProc);
		  return 0;
		case ID_OPTIONS_SOURCES:
		  DialogBox(hInst,MAKEINTRESOURCE(IDD_OPTIONS_SOURCES),hWnd,OptionSourcesDialogProc);
		  return 0;
#ifdef VMB
		case ID_OPTIONS_VMB:
		  DialogBox(hInst,MAKEINTRESOURCE(IDD_CONNECT),hWnd,ConnectDialogProc);
		  return 0;
#endif
	    case ID_HELP_ABOUT:
	      DialogBox(hInst,MAKEINTRESOURCE(IDD_ABOUT),hWnd,AboutDialogProc);
	      return 0; 

		case ID_HELP_CONTENT:
#ifdef VMB
		  ide_help("help\\mmixide.html");
#else
		  ide_help("help\\mmixvd.html");
#endif
		  return 0;
#if 0
	   /* see winproc.c for an example */
	   case ID_HELP_CONFIGURATION:
	     DialogBox(hInst,MAKEINTRESOURCE(IDD_CONFIGURATION),hWnd,ConfigurationDialogProc);
	     return 0; 
#endif
	    case ID_HELP_INSTRUCTIONS:
		  ide_help("help\\instructions\\index.html");
		  return 0;	
	  }
	  /* else if (HIWORD(wParam)==1) break; Accellerator */
	}
	else /* Control-defined notification code */
	{ HWND hControl = (HWND)lParam;
	  if (hControl==hError)
	  { int item, line_no, file_no;
		item = (int)SendMessage(hError,LB_GETCURSEL,0,0);
	    if (item!=LB_ERR)
		{ LPARAM data;
		  data = SendMessage(hError,LB_GETITEMDATA,item,0);
		  line_no = item_line_no(data);
		  file_no = item_file_no(data);
		}
		else
		{ line_no = -1;
		  file_no = -1;
		}
		switch (HIWORD(wParam))
		{ case LBN_DBLCLK:
			SetFocus(hEdit);
			return 0;
		  case LBN_SELCHANGE:
			ed_mark_error(file_no,line_no);
			return 0;
		  case   LBN_SELCANCEL:
  			ed_mark_error(-1,-1);
			return 0;
		  case LBN_SETFOCUS:
			  break;
		   case LBN_KILLFOCUS:
			  break;
		  default:
			  return 0;
		}
	  } 
	  else if (hControl==hSymbolTable && HIWORD(wParam)== LBN_DBLCLK)
	  { int item, line_no, file_no;
	    sym_node *sym=NULL;
		item = (int)SendMessage(hSymbolTable,LB_GETCURSEL,0,0);
	    if (item!=LB_ERR)
		  sym = (sym_node *)SendMessage(hSymbolTable,LB_GETITEMDATA,item,0);
		if (sym!=NULL)
		{ file_no=sym->file_no;
		  line_no=sym->line_no;
 		  set_edit_file(file_no);
		  ed_show_line(line_no);
		}
	  }
	}
    return 0;
  case WM_HELP:
	{ HWND hf = GetFocus();
      if (is_inspector(hf))
	    ide_help("help\\debugger\\inspector.html");
      else if (is_dataedit(hf))
	    ide_help("help\\debugger\\inspector.html");
	  else
      { char *op=ed_get_instruction();
        if (op==NULL) 
	      ide_help("help\\instructions\\index.html");
	    else
	    { HWND hh;
	      HH_AKLINK link; 
			  link.cbStruct =     sizeof(HH_AKLINK) ;
              link.fReserved =    FALSE ;
              link.pszKeywords =  op ;
              link.pszUrl =       NULL ;
              link.pszMsgText =   NULL ;
              link.pszMsgTitle =  NULL ;
              link.pszWindow =    NULL ;
              link.fIndexOnFail = TRUE ;
			  HtmlHelp(NULL,programhelpfile, HH_DISPLAY_TOPIC,(DWORD_PTR)NULL);
              hh = HtmlHelp(NULL,programhelpfile, HH_KEYWORD_LOOKUP,(DWORD_PTR)&link);
			}
		  }
	}
    return 0;

  case WM_MMIX_STOPPED:
    if (lParam==-1) /* terminated */   
	{ mmix_status(MMIX_HALTED);
      show_stop_marker(edit_file_no,-1); /* clear stop marker */
	  bb_set_group(hButtonBar,BG_DEBUG,0,0);
	  bb_set_group(hButtonBar,BG_EDIT,1,1);
	  update_symtab();
	}
	else if (lParam==0||item_file_no(lParam)==0xFF)
	{ mmix_status(MMIX_STOPPED);
      show_stop_marker(edit_file_no,-1); /* clear stop marker */
	}
	else
	{ mmix_status(MMIX_STOPPED);
	  show_stop_marker(item_file_no(lParam),item_line_no(lParam));
	}
	return 0;
  case WM_MMIX_INTERACT:
	display_profile();
	memory_update();
	return 0;
  case WM_MMIX_RESET:
	return 0;
  case WM_MMIX_LOAD:
	return 0;
  case WM_CLOSE:
	if (ed_close_all(1))
	  DestroyWindow(hWnd);
	return 0;
  case WM_DESTROY:
	ed_close_all(0);
	set_xypos(hMainWnd);
	write_regtab(defined);
    PostQuitMessage(0);
    return 0;
  case WM_SYSCOMMAND:
	//  	ed_close_all(0);
	break;
  /* change color of edit contols like trace */
  case WM_CTLCOLOREDIT :
  case WM_CTLCOLORSTATIC:
		{ HDC hdc = (HDC)wParam;
		  HWND hc = (HWND)lParam;
		  if (hc==hLog)
		  { SetTextColor(hdc,RGB(0xff,0xff,0xff));
		    SetBkColor(hdc,RGB(0,0,0));
		    return (LRESULT)GetStockObject(BLACK_BRUSH);
		  }
		  else
			 break;
		}
   case WM_CONTEXTMENU:
	   if (SymbolContextMenuHandler(LOWORD(lParam), HIWORD(lParam))) 
		   return 0;
	   break; 
  }
 return (DefWindowProc(hWnd, message, wParam, lParam));
}

static char classname[]="MMIXIDECLASS";

BOOL InitInstance(HINSTANCE hInstance)
{
  WNDCLASSEX wcex;




  ZeroMemory(&wcex, sizeof(wcex));
	wcex.cbSize = sizeof(WNDCLASSEX); 
	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= (WNDPROC)WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance,MAKEINTRESOURCE(IDI_ICON));
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_BTNSHADOW+1);
	wcex.lpszMenuName	= MAKEINTRESOURCE(IDR_MENU);
	wcex.lpszClassName = classname;
	wcex.hIconSm		= LoadIcon(hInstance,MAKEINTRESOURCE(IDI_ICON_SMALL));

	if (!RegisterClassEx(&wcex)) return FALSE;


    hMainWnd = CreateWindow(classname, title ,
				WS_OVERLAPPEDWINDOW,
                CW_USEDEFAULT, CW_USEDEFAULT,CW_USEDEFAULT, CW_USEDEFAULT,
	            NULL, NULL, hInstance, NULL);

   hMenu = GetMenu(hMainWnd);

   return TRUE;
}



int mmo_file_newer(char *full_mms_name)
{ time_t mmsTime, mmoTime;
  mmsTime=ftime(full_mms_name);
  mmoTime=ftime(get_mmo_name(full_mms_name));
  if (mmsTime > mmoTime)
	  return 1;
  else
	  return 0;
}



int assemble_if_needed(int file_no)
{ char *full_mms_name;
  if (file_no<0) return 1;
  full_mms_name=file2fullname(file_no);
  if (full_mms_name==NULL)
  { set_edit_file(file_no);
    if (!ed_save_as()) 
	  return 0;
  }
  if (mmo_file_newer(full_mms_name))
  {	 int decision;
     if (auto_assemble)
    	 decision= IDYES;
	 else	 
		 decision= MessageBox(hMainWnd, "mms file newer than mmo file. Assemble?", unique_name(file_no), MB_YESNOCANCEL);
	 if (decision == IDYES)
	 { int err_count=mmix_assemble(file_no);
	   if (err_count!=0) return 0;
	 } 
	 else if (decision == IDCANCEL)
	   return 0;
  }
  else if (!file2debuginfo(file_no))
  {	 int decision;
     if (auto_assemble)    	 
		 decision= IDYES;
	 else
		 decision= MessageBox(hMainWnd, "No debug information available. Assemble?", unique_name(file_no), MB_YESNOCANCEL);
	 if (decision == IDYES)
	 { int err_count=mmix_assemble(file_no);
	   if (err_count!=0) return 0;
	 } 
	 else if (decision == IDCANCEL)
	   return 0;
  }
  return 1;
}


static int assembly_ok=0;

static void assemble_files(int file_no)
{ if (!assembly_ok) return;
  if (!file2assembly(file_no)) return;
  if (!assemble_if_needed(file_no))
    assembly_ok=0;
}

int assemble_all_needed(void)
{ 
  assembly_ok=1;
  for_all_files(assemble_files); 
  return assembly_ok;
}



#define MAXPROG 512

static int execution_ok=0;
#ifdef VMB
static void create_image_file(int file_no)
{ char *argv[3];
  if (!execution_ok) return;
  if (!needs_image[file_no]) return;
  argv[0]="mmoimg";
  argv[1]=get_mmo_name(file2fullname(file_no));
  argv[2]=NULL;
  mmoimg_main(2,argv);
}
#endif


static void execute_file_command(int file_no)
{ 
#if 0  
  char *argv[MAXARG] = {0};
  char argc;
  char prog[MAXPROG], cmd[MAXPROG];
  char *FilePart;
#else
  char *argv[4];
#endif
  if (!execution_ok) return;
  if (!execute[file_no]) return;
  if (command[file_no]==NULL) return;
#if 0
  strncpy_s(cmd,MAXPROG,command[file_no],MAXPROG);
  argc = mk_argv(argv,cmd,1);
  if (argc<=0)
          return;

  if (SearchPath(NULL,argv[0],".exe",MAXPROG,prog,&FilePart)<=0)
  {	 MessageBox(NULL,"Unable to find command",argv[0],MB_OK);
     execution_ok=0;
  }
  else 
  { argv[0]= prog;
    if (_spawnvp(_P_WAIT,prog,argv)<0)
    { MessageBox(NULL,"Unable to execute command",command[file_no],MB_OK);
      execution_ok=0;
    }
  }
#else
  argv[0]="cmd";
  argv[1]="/K";
  argv[2]=command[file_no];
  argv[3]=NULL;
  _spawnvp(_P_WAIT,argv[0],argv);
#endif
}


int execute_commands(void)
{ 
#ifdef VMB
  execution_ok=1;
  for_all_files(create_image_file);
  if (!execution_ok) return execution_ok;
#endif
  execution_ok=1;
  for_all_files(execute_file_command);
  return execution_ok;
}

void new_errorlist(void)
{   sp_create_options(0,0,0.2,0,hEdit);
	hError = CreateWindow("LISTBOX", "Errorlist" ,
				WS_CHILD|WS_VISIBLE|WS_VSCROLL|LBS_NOTIFY|LBS_NOINTEGRALHEIGHT,
                0,0,0,0,
	            hSplitter, NULL, hInst, NULL);
	SendMessage(hError,WM_SETFONT,(WPARAM)hVarFont,0);
}


void add_button(int iconID, int menuID, int group, int buttonID, char *tip)
{ HANDLE h;
  h = LoadImage(hInst, MAKEINTRESOURCE(iconID),IMAGE_ICON,0,0,LR_DEFAULTCOLOR);
  bb_CreateButton(hButtonBar,h,menuID,group,buttonID,1,1,tip);
}

void add_buttons(void)
{ 
  add_button(IDI_FILE_NEW,ID_FILE_NEW,BG_FILE,0,"New");
  add_button(IDI_FILE_OPEN,ID_FILE_OPEN,BG_FILE,1,"Open");
  add_button(IDI_FILE_SAVE,ID_FILE_SAVE,BG_FILE,2,"Save");
  
  add_button(IDI_EDIT_COPY,ID_EDIT_COPY,BG_EDIT,3,"Copy");
  add_button(IDI_EDIT_PASTE,ID_EDIT_PASTE,BG_EDIT,4,"Paste");
  add_button(IDI_EDIT_CUT,ID_EDIT_CUT,BG_EDIT,5,"Cut");
  add_button(IDI_EDIT_UNDO,ID_EDIT_UNDO,BG_EDIT,6,"Undo");
  add_button(IDI_EDIT_REDO,ID_EDIT_REDO,BG_EDIT,7,"Redo");
  add_button(IDI_VIEW_WHITESPACE,ID_VIEW_WHITESPACE,BG_EDIT,8,"Show Whitespace");

  add_button(IDI_FINDREPLACE,ID_EDIT_FIND,BG_VIEW,9,"Find and Replace");  
  add_button(IDI_VIEW_ZOOMIN,ID_VIEW_ZOOMIN,BG_VIEW,10,"Zoom in");
  add_button(IDI_VIEW_ZOOMOUT,ID_VIEW_ZOOMOUT,BG_VIEW,11,"Zoom out");

  add_button(IDI_BREAKX,ID_MMIX_BREAKX,BG_MMIX,12,"Toggle Execute Breakpoint");
  add_button(IDI_BREAKR,ID_MMIX_BREAKR,BG_MMIX,13,"Toggle Read Breakpoint");
  add_button(IDI_BREAKW,ID_MMIX_BREAKW,BG_MMIX,14,"Toggle Write Breakpoint");
  add_button(IDI_BREAKT,ID_MMIX_BREAKT,BG_MMIX,15,"Toggle Tracepoint");
  add_button(IDI_MMIX_DEBUG,ID_MMIX_DEBUG,BG_MMIX,16,"Debug/Continue");

  add_button(IDI_DEBUG_STEP,ID_MMIX_STEP,BG_DEBUG,17,"Step Instruction");
  add_button(IDI_DEBUG_STEPOVER,ID_MMIX_STEPOVER,BG_DEBUG,18,"Step Over");
  add_button(IDI_DEBUG_STEPOUT,ID_MMIX_STEPOUT,BG_DEBUG,19,"Step Out");
  add_button(IDI_DEBUG_PAUSE,ID_MMIX_STOP,BG_DEBUG,20,"Break Execution");
  add_button(IDI_DEBUG_HALT,ID_MMIX_QUIT,BG_DEBUG,21,"Halt Execution");

  bb_set_group(hButtonBar,BG_DEBUG,0,0);

  add_button(IDI_HELP,ID_HELP_CONTEXT,BG_HELP,22,"Help");


}


static	HMENU hMemMenu=NULL;            // top-level menu 
HMENU hMemContextMenu=NULL;  // shortcut menu 
HMENU hRegContextMenu=NULL;  // shortcut menu 
HMENU hSymContextMenu=NULL;  // shortcut menu 
HMENU hBrkContextMenu=NULL;  // shortcut menu 



int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
    MSG msg;
	HACCEL hAccelTable;
	RECT r;

	hInst = hInstance; 
	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDR_ACCELERATOR));
	sp_init(hInstance);
    bb_init(hInstance);
	if (!InitInstance (hInstance)) return FALSE;
	InitCommonControls(); /* needed for TAB Controls */
	init_layout(0); 
    hMemMenu = LoadMenu(hInst,MAKEINTRESOURCE(IDR_MEMPOPUP));
	if (hMemMenu!= NULL) 
	{ hMemContextMenu = GetSubMenu(hMemMenu, 0); 
	  hRegContextMenu = GetSubMenu(hMemMenu, 1); 
	  hSymContextMenu = GetSubMenu(hMemMenu, 2); 
	  hBrkContextMenu = GetSubMenu(hMemMenu, 3); 
	}
    GetClientRect(hMainWnd,&r);
    hButtonBar = bb_CreateButtonBar("The Button Bar", WS_CHILD|WS_VISIBLE,
		0, 0, r.right-r.left-status_width, BB_HEIGHT , hMainWnd, NULL, hInst, NULL);
	hSplitter = sp_CreateSplitter("The Splitter", WS_CHILD|WS_VISIBLE,
                  0, BB_HEIGHT+1, r.right-r.left, r.bottom-r.top-BB_HEIGHT-1, hMainWnd, NULL, hInst, NULL);
    add_buttons();
    printer_init();
	init_edit(hInstance);
	new_edit();
	mmix_lib_initialize();
	param_init ();
	debug_init();
	set_font_size(fontsize);
	ed_zoom(fontsize);
	CheckMenuItem(hMenu,ID_REGISTERS_STACK,MF_BYCOMMAND|(show_debug_regstack?MF_CHECKED:MF_UNCHECKED));
	CheckMenuItem(hMenu,ID_VIEW_WHITESPACE,MF_BYCOMMAND|(show_whitespace?MF_CHECKED:MF_UNCHECKED));
	set_whitespace(show_whitespace);
	CheckMenuItem(hMenu,ID_VIEW_SYNTAX,MF_BYCOMMAND|(syntax_highlighting?MF_CHECKED:MF_UNCHECKED));
    set_text_style();
	CheckMenuItem(hMenu,ID_VIEW_LINENUMBERS,MF_BYCOMMAND|(show_line_no?MF_CHECKED:MF_UNCHECKED));
    set_lineno_width();
	CheckMenuItem(hMenu,ID_VIEW_PROFILE,MF_BYCOMMAND|(show_profile?MF_CHECKED:MF_UNCHECKED));
    set_profile_width();
    if (programhelpfile==NULL) 
#ifdef VMB
		programhelpfile="mmixide.chm";
#else
		programhelpfile="mmixvd.chm";
#endif
	if (edit_file_no<0) ed_new();
	hStatus = CreateWindow("STATIC", "Status" ,
				WS_CHILD|WS_VISIBLE|SS_RIGHT,
                r.right-r.left-status_width,0, status_width,BB_HEIGHT,
	            hMainWnd, NULL, hInstance, NULL);
	SendMessage(hStatus,WM_SETFONT,(WPARAM)hGUIFont,0);
    ide_status(version);
#ifdef VMB
    vmb_program_name="mmixide";
    vmb_message_hook = win32_message;
    vmb_debug_hook = NULL;
    vmb_error_init_hook = NULL;
    vmb_exit_hook=ide_exit_ignore;
#endif

	SetWindowPos(hMainWnd,HWND_TOP,xpos,ypos,width,height,SWP_SHOWWINDOW|((width&&height)?0:SWP_NOSIZE));

    //ShowWindow(hMainWnd, SW_SHOW);
	if (minimized)CloseWindow(hMainWnd); 
    
	UpdateWindow(hMainWnd);

	while (GetMessage(&msg, NULL, 0, 0)) 
	  if (!TranslateAccelerator(hMainWnd, hAccelTable, &msg) &&
		  !do_subwindow_msg(&msg) ) 
	  { TranslateMessage(&msg);
	    DispatchMessage(&msg);
	  }
	Scintilla_ReleaseResources();
#ifdef VMB
	if (vmb.connected) vmb_atexit();
#endif
	if (hMemMenu!= NULL) DestroyMenu(hMemMenu); 
	return (int)msg.wParam;
}

#ifdef VMB
void  ide_exit_ignore(int returncode)
{ ;
}
#endif

void destroy_log(HWND hLog)
{  CheckMenuItem(hMenu,ID_VIEW_TRACE,MF_BYCOMMAND|MF_UNCHECKED);
}


