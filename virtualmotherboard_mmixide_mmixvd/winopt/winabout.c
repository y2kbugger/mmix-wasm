#include <windows.h>
#include <stdio.h>
#include "winopt.h"
#include "resource.h"

#pragma warning(disable : 4996)

INT_PTR CALLBACK    
AboutDialogProc( HWND hDlg, UINT message, WPARAM wparam, LPARAM lparam )
{ char str[30];
  switch ( message )
  { case WM_INITDIALOG:
		sprintf(str,"Version %d.%d",major_version,minor_version);
		SetDlgItemText(hDlg,IDC_VERSION,str);
		SetDlgItemText(hDlg,IDC_REVISION,version);
		SetDlgItemText (hDlg, IDC_TITLE, title);
      return TRUE;
    case WM_SYSCOMMAND:
      if( wparam == SC_CLOSE ) 
      { EndDialog(hDlg, TRUE);
        return TRUE;
      }
      break;
    case WM_COMMAND:
      if( wparam == IDOK )
      {
        EndDialog(hDlg, TRUE);
        return TRUE;
      }
     break;
  }
  return FALSE;
}

