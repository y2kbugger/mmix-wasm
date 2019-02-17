#include <windows.h>
#include <commctrl.h>
#include "vmb.h"
#include "resource.h"
#include "winopt.h"
#include "inspect.h"
#include "winlog.h"
#include "option.h"


static HWND hMemory=NULL;
static HWND hDebug=NULL; /* debug output goes to this window, if not NULL */
static HWND hFilter=NULL; 
static HWND hExtraDebug=NULL;
extern HWND (*addExtraDebug)(HWND hDebug)=NULL;
static int extraTabCtrl=-1;


static void show_filter(void)
{ CheckDlgButton(hFilter,IDC_HIDE_FATAL,(vmb_debug_mask&VMB_DEBUG_FATAL)?BST_CHECKED:BST_UNCHECKED);
  CheckDlgButton(hFilter,IDC_HIDE_ERROR,(vmb_debug_mask&VMB_DEBUG_ERROR)?BST_CHECKED:BST_UNCHECKED);
  CheckDlgButton(hFilter,IDC_HIDE_WARN,(vmb_debug_mask&VMB_DEBUG_WARN)?BST_CHECKED:BST_UNCHECKED);
  CheckDlgButton(hFilter,IDC_HIDE_NOTIFY,(vmb_debug_mask&VMB_DEBUG_NOTIFY)?BST_CHECKED:BST_UNCHECKED);
  CheckDlgButton(hFilter,IDC_HIDE_PROGRESS,(vmb_debug_mask&VMB_DEBUG_PROGRESS)?BST_CHECKED:BST_UNCHECKED);
  CheckDlgButton(hFilter,IDC_HIDE_INFO,(vmb_debug_mask&VMB_DEBUG_INFO)?BST_CHECKED:BST_UNCHECKED);
  CheckDlgButton(hFilter,IDC_HIDE_MSG,(vmb_debug_mask&VMB_DEBUG_MSG)?BST_CHECKED:BST_UNCHECKED);
  CheckDlgButton(hFilter,IDC_HIDE_PAYLOAD,(vmb_debug_mask&VMB_DEBUG_PAYLOAD)?BST_CHECKED:BST_UNCHECKED);
  CheckDlgButton(hFilter,IDC_HIDE_ALL,vmb_debug_mask==0?BST_UNCHECKED:
		  ((vmb_debug_mask&0xFF)==0xFF?BST_CHECKED:BST_INDETERMINATE));
}


static void resize_dialog(HWND hDlg,int w, int h)
{ MoveWindow(GetDlgItem(hDlg,IDC_TAB_DEBUG),5,5,w-10,h-10,TRUE); 
  if (hLog!=NULL) MoveWindow(hLog,10,30,w-20,h-40,TRUE); 
  MoveWindow(hMemory,10,30,w-20,h-40,TRUE);
  MoveWindow(hFilter,10,30,w-20,h-40,TRUE);
  if (hExtraDebug!=NULL)  MoveWindow(hExtraDebug,10,30,w-20,h-40,TRUE);
}


static INT_PTR CALLBACK FilterProc(HWND hDlg,UINT uMsg,WPARAM wparam,LPARAM lParam)
{ switch ( uMsg )
  { case WM_COMMAND: 
	if (HIWORD(wparam) == BN_CLICKED) 
     { int flag = 0;
	   switch (LOWORD(wparam)) 
	   { case IDC_HIDE_FATAL: flag = VMB_DEBUG_FATAL; break;
		 case IDC_HIDE_ERROR: flag = VMB_DEBUG_ERROR; break;	
		 case IDC_HIDE_WARN: flag = VMB_DEBUG_WARN; break;	
		 case IDC_HIDE_NOTIFY: flag = VMB_DEBUG_NOTIFY; break;	
		 case IDC_HIDE_PROGRESS: flag = VMB_DEBUG_PROGRESS; break;	
		 case IDC_HIDE_INFO: flag = VMB_DEBUG_INFO; break;	
		 case IDC_HIDE_MSG: flag = VMB_DEBUG_MSG; break;	
		 case IDC_HIDE_PAYLOAD: flag = VMB_DEBUG_PAYLOAD; break;	
		 case IDC_HIDE_ALL:
		 { LRESULT bs; 
		   bs = SendDlgItemMessage(hDlg, IDC_HIDE_ALL, BM_GETSTATE, 0, 0);
		   if (bs & BST_CHECKED) vmb_debug_mask = 0xFFFF;
		   else if (bs & BST_INDETERMINATE) vmb_debug_mask = VMB_DEBUG_DEFAULT;
		   else vmb_debug_mask = 0;
		   show_filter();
	       return FALSE;
		 }
	   }
       /* Retrieve the state of the check box. */
       if (SendDlgItemMessage(hDlg, LOWORD(wparam), BM_GETSTATE, 0, 0)&BST_CHECKED)
             vmb_debug_mask |= flag;
		   else
             vmb_debug_mask &= ~flag;
       return FALSE; 
	 }
     break;
  }
return FALSE;
}
	
static int cur_insp=-1;

static INT_PTR CALLBACK   
DebugDialogProc( HWND hDlg, UINT message, WPARAM wparam, LPARAM lparam )
{ static int minw, minh;
  switch ( message )
  { case WM_INITDIALOG :
  	hDebug=hDlg;
	register_subwindow(hDebug);
	hMemory=CreateMemoryDialog(hInst,hDlg);
    register_subwindow(hMemory);
	hLog = CreateLog(hDlg,hInst);
	hFilter=CreateDialog(hInst,MAKEINTRESOURCE(IDD_FILTER),hDlg,FilterProc);
    register_subwindow(hFilter);
	{ TCITEM tie;
	  RECT rect;
	  int i=0;
      tie.mask = TCIF_TEXT;
      tie.iImage = -1;
 	  tie.pszText = "Debug";
	  TabCtrl_InsertItem (GetDlgItem (hDlg, IDC_TAB_DEBUG), 0, &tie);
 	  tie.pszText = "Filter";
	  TabCtrl_InsertItem (GetDlgItem (hDlg, IDC_TAB_DEBUG), 1, &tie);
	  while (inspector[i].name!=NULL)
	  { tie.pszText = inspector[i].name;
	    TabCtrl_InsertItem (GetDlgItem (hDlg, IDC_TAB_DEBUG), i+2, &tie);
		i++;
	  }
	  if (addExtraDebug!=NULL) hExtraDebug=addExtraDebug(hDlg);
	  if (hExtraDebug!=NULL) 
	  { register_subwindow(hExtraDebug);
		GetWindowText(hExtraDebug,tmp_option,MAXTMPOPTION);
	    tie.pszText = tmp_option;  
	    TabCtrl_InsertItem (GetDlgItem (hDlg, IDC_TAB_DEBUG), i+2, &tie);
		extraTabCtrl=i+2;
	  }
	  TabCtrl_SetCurSel (GetDlgItem (hDlg, IDC_TAB_DEBUG), 0);
	  ShowWindow(hLog,SW_SHOW);
	  GetWindowRect(hDlg,&rect);
      minw = rect.right-rect.left;
	  minh = rect.bottom-rect.top;
	  if (hExtraDebug!=NULL)
	  {	GetWindowRect(hExtraDebug,&rect);
       if (minw < rect.right-rect.left+30) minw = rect.right-rect.left+30;
       if (minh < rect.bottom-rect.top+40) minh = rect.bottom-rect.top+40;
	  }
      SetWindowPos(hDlg,HWND_TOP,0,0,minw,minh,SWP_NOMOVE);
	  GetClientRect(hDlg,&rect);
      resize_dialog(hDlg,rect.right-rect.left,rect.bottom-rect.top);
	}
    return FALSE; /* no keyboard focus on the debug window */
  case WM_NOTIFY:
	{ NMHDR *p;
      p = (NMHDR*)lparam;
      if (p->code == TCN_SELCHANGE) 
      { int i = TabCtrl_GetCurSel (GetDlgItem (hDlg, IDC_TAB_DEBUG));
	    ShowWindow(hLog,i==0?SW_SHOW:SW_HIDE);
        ShowWindow(hFilter,i==1?SW_SHOW:SW_HIDE);
		ShowWindow(hMemory,(i!=extraTabCtrl && i>1)?SW_SHOW:SW_HIDE); 
        ShowWindow(hExtraDebug,i==extraTabCtrl?SW_SHOW:SW_HIDE);
	    if (i==1) show_filter();
		else if (i>1 && i!=extraTabCtrl)
		{ cur_insp=i-2;
		  SetInspector(hMemory, &inspector[cur_insp]);
		}
      }
	}
    break;
     case WM_COMMAND:
	  if (wparam ==IDOK)
	  {  /* User has hit the ENTER key and I forward it to the memory dialog
		     It should go there directly, but I dont know how and why.*/
		 return SendMessage(hMemory, message, wparam, lparam );
	  }
	  return FALSE;
    case WM_SYSCOMMAND:
      if( wparam == SC_CLOSE ) 
      { unregister_subwindow(hFilter);
	    DestroyWindow(hFilter);
		hFilter=NULL;
		unregister_subwindow(hMemory);
		DestroyWindow(hMemory);
		hMemory=NULL;
        if (hExtraDebug!=NULL)
		{ unregister_subwindow(hExtraDebug);
		  DestroyWindow(hExtraDebug);
		  hExtraDebug=NULL;
		} 
		/* hLog is destroyed by its own window procedure */
		vmb_debug_flag=0;
	    CheckMenuItem(hMenu,ID_DEBUG,MF_BYCOMMAND|MF_UNCHECKED);
		unregister_subwindow(hDebug);
		hDebug = NULL;
		EndDialog(hDlg, TRUE);
        return TRUE;
      }
      break;
    case WM_SIZE: 
	  resize_dialog(hDlg,LOWORD(lparam),HIWORD(lparam));
      return TRUE;
	case WM_GETMINMAXINFO:
		{ MINMAXINFO *p = (MINMAXINFO *)lparam;
		  p->ptMinTrackSize.x = minw;
          p->ptMinTrackSize.y = minh;
		}
		return 0;
		  /* change color of edit contols like trace */
  case WM_CTLCOLOREDIT :
  case WM_CTLCOLORSTATIC:
		{ HDC hdc = (HDC)wparam;
		  HWND hc = (HWND)lparam;
		  if (hc==hLog)
		  { SetTextColor(hdc,RGB(0,0,0));
		    SetBkColor(hdc,GetSysColor(COLOR_BTNFACE));
		    return (LRESULT)GetSysColorBrush(COLOR_BTNFACE);
		  }
		  else
			 break;
		}
  }
  return FALSE;
}

/* extern functions */

void win32_error_init(int on)
{ if (on)
  { if (hDebug==NULL)
      hDebug=CreateDialog(hInst,MAKEINTRESOURCE(IDD_DEBUG),hMainWnd,DebugDialogProc);
	SetWindowText(hDebug,defined);
	CheckMenuItem(hMenu,ID_DEBUG,MF_BYCOMMAND|MF_CHECKED);
  }
  else
  { if (hDebug!=NULL)
      SendMessage(hDebug,WM_SYSCOMMAND,SC_CLOSE,0);
	CheckMenuItem(hMenu,ID_DEBUG,MF_BYCOMMAND|MF_UNCHECKED);
  }
}

void mem_update(unsigned int offset, int size)
{ if (cur_insp<0) return;
  MemoryDialogUpdate(&inspector[cur_insp], offset, size);
}

void mem_update_i(int i, unsigned int offset, int size)
{ if (i!=cur_insp) return;
  MemoryDialogUpdate(&inspector[cur_insp], offset, size);
}