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


INT_PTR CALLBACK   
SettingsDialogProc( HWND hDlg, UINT message, WPARAM wparam, LPARAM lparam )
{
  switch ( message )
  { case WM_INITDIALOG:
      uint64_to_hex(vmb_address,tmp_option);
      SetDlgItemText(hDlg,IDC_ADDRESS,tmp_option);
	  SetDlgItemInt(hDlg,IDC_RAMSIZE,vmb_size,FALSE);
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
		vmb_size=GetDlgItemInt(hDlg,IDC_RAMSIZE,NULL,FALSE);
      }
      if (wparam == IDOK || wparam == IDCANCEL)
      { EndDialog(hDlg, TRUE);
        return TRUE;
      }
      break;
  }
  return FALSE;
}


LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{ 
  switch (message) 
  {  
  default:
    return (OptWndProc(hWnd, message, wParam, lParam));
  }
 return (DefWindowProc(hWnd, message, wParam, lParam));
}

