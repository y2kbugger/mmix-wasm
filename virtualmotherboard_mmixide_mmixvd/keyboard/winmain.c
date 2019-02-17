#include <winsock2.h>
#include <windows.h>
#include <afxres.h>
#include "vmb.h"
#include "bus-arith.h"
#include "resource.h"
#include "winopt.h"
#include "param.h"
#include "inspect.h"
#include "option.h"


HBITMAP hBmpActive, hBmpInactive;


INT_PTR CALLBACK   
SettingsDialogProc( HWND hDlg, UINT message, WPARAM wparam, LPARAM lparam )
{
  switch ( message )
  { case WM_INITDIALOG:
	  uint64_to_hex(vmb_address,tmp_option);
      SetDlgItemText(hDlg,IDC_ADDRESS,tmp_option);
	  SetDlgItemInt(hDlg,IDC_INTERRUPT,interrupt_no,FALSE);
      return TRUE;
   case WM_SYSCOMMAND:
      if( wparam == SC_CLOSE ) 
      { EndDialog(hDlg, TRUE);
        return TRUE;
      }
      break;
    case WM_COMMAND:
      if( wparam == IDOK )
      { GetDlgItemText(hDlg,IDC_ADDRESS,tmp_option,MAXTMPOPTION);
	    vmb_address = strtouint64(tmp_option);
		inspector[0].address=vmb_address;
		interrupt_no  = GetDlgItemInt(hDlg,IDC_INTERRUPT,NULL,FALSE);
      }
      if (wparam == IDOK || wparam == IDCANCEL)
      { EndDialog(hDlg, TRUE);
        return TRUE;
      }
      break;
  }
  return FALSE;
}

extern void process_input(unsigned char c, unsigned char extended);
extern void process_input_file(char *filename);


LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{ 
   switch (message) 
  { case WM_SETFOCUS:
      vmb_debug(VMB_DEBUG_PROGRESS, "got focus");
	  hBmp = hBmpActive;
      RedrawWindow(hMainWnd,NULL,NULL,RDW_INVALIDATE);
      break;
    case WM_KILLFOCUS:
	  hBmp = hBmpInactive;
	  RedrawWindow(hMainWnd,NULL,NULL,RDW_INVALIDATE);
	  vmb_debug(VMB_DEBUG_PROGRESS, "lost focus");
	  break;
    case WM_DROPFILES:
	  { HDROP hDrop;
	    char filename[500];
	    hDrop = (HDROP)wParam;
		DragQueryFile(hDrop,0,filename,500);
	    process_input_file(filename);
		DragFinish(hDrop);
	  }
	  return 0;
    case WM_VMB_ON: /* Power On */
	  DragAcceptFiles(hWnd,TRUE);
	  	  if (vmb_filename!=NULL)
	    process_input_file(vmb_filename);
      break;
	case WM_VMB_RESET:
	  if (vmb_filename!=NULL)
	    process_input_file(vmb_filename);
      break;
   return 0;
    case WM_VMB_OFF: /* Power Off */
	  DragAcceptFiles(hWnd,FALSE);
	  break;
   case WM_CHAR:      process_input((unsigned char) wParam,0); 
      return 0;
   case WM_KEYDOWN:
      switch (wParam) 
      { case VK_PRIOR:  case VK_NEXT:
	    case VK_LEFT: case VK_RIGHT: case VK_UP: case VK_DOWN:
        case VK_HOME: case VK_END:
        case VK_INSERT: case VK_DELETE:
		case VK_ESCAPE: case VK_SELECT: case VK_PRINT: case VK_EXECUTE: case VK_SNAPSHOT:
		case VK_HELP:
		case VK_SLEEP:
		case VK_F1: case VK_F2: case VK_F3: case VK_F4:
		case VK_F5: case VK_F6: case VK_F7: case VK_F8:
		case VK_F9: case VK_F10: case VK_F11: case VK_F12:
		case VK_F13: case VK_F14: case VK_F15: case VK_F16:
		case VK_F17: case VK_F18: case VK_F19: case VK_F20:
		case VK_F21: case VK_F22: case VK_F23: case VK_F24:
          process_input((unsigned char) wParam,1); 
          return 0;
		default: 
          break; 
      }      
  }
  return (OptWndProc(hWnd, message, wParam, lParam));
}
