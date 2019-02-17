#include <winsock2.h>
#include <windows.h>
#include <Mmsystem.h>
#include <afxres.h>
#include "vmb.h"
#include "bus-arith.h"
#include "resource.h"
#include "winopt.h"
#include "opt.h"
#include "param.h"
#include "inspect.h"
#include "option.h"

extern HWND createPlayerWindow(HWND hParent);

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

/* from wimp3.c */
extern void stop_sound(void);
extern void start_sound_server(void);

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{ switch (message) 
  {  
  case WM_VMB_OTHER+1: /* Disk bussy */
	return 0;
  case WM_CREATE: 
#if 0
	{ static INITCOMMONCONTROLSEX comctl;
	  BOOL b;
	  memset(&comctl,0,sizeof(comctl));
	  comctl.dwSize=sizeof(comctl);
	  comctl.dwICC = ICC_BAR_CLASSES ;
    
	  b=InitCommonControlsEx(&comctl);
	  if (!b) 
		  win32_error("non common controls");
	}
#endif
	// check there is a device 
	if( waveOutGetNumDevs() < 1 )
		win32_ferror(__LINE__,"No audio output devices found.",NULL);
	hpower = CreateWindow("STATIC",NULL,WS_CHILD|WS_VISIBLE|SS_BITMAP|SS_REALSIZEIMAGE,5,155,32,32,hWnd,NULL,hInst,0);
    SendMessage(hpower,STM_SETIMAGE,(WPARAM) IMAGE_BITMAP,(LPARAM)hoff);
	addExtraDebug=createPlayerWindow;
	start_sound_server();
    return 0;
	case WM_CLOSE:
    case WM_DESTROY:
		stop_sound();
		break;


      stop_sound();

  default:
    return (OptWndProc(hWnd, message, wParam, lParam));
  }
 return (OptWndProc(hWnd, message, wParam, lParam));
}

