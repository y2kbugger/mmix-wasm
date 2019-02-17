#include <windows.h>
#include "resource.h"
#include "option.h"
#include "vmb.h"

extern char *host;
extern int port;
int auto_connect=0;

INT_PTR CALLBACK    
ConnectDialogProc( HWND hDlg, UINT message, WPARAM wparam, LPARAM lparam )
{

  switch ( message )
  { case WM_INITDIALOG:
      SetDlgItemText(hDlg,IDC_THE_SERVER,host);
      SetDlgItemInt(hDlg,IDC_THE_PORT,port,FALSE);
	  CheckDlgButton(hDlg,IDC_CHECK_AUTOCONNECT,auto_connect?BST_CHECKED:BST_UNCHECKED);
      return TRUE;
   case WM_SYSCOMMAND:
      if( wparam == SC_CLOSE ) 
      { EndDialog(hDlg, FALSE);
        return TRUE;
      }
      break;
    case WM_COMMAND:
      if( wparam == IDOK )
        { 
	      GetDlgItemText(hDlg,IDC_THE_SERVER,tmp_option,MAXTMPOPTION);
		  set_option(&host, tmp_option);
          port = GetDlgItemInt(hDlg,IDC_THE_PORT,NULL,FALSE);
		  auto_connect=IsDlgButtonChecked(hDlg,IDC_CHECK_AUTOCONNECT);
		  EndDialog(hDlg, TRUE);
          return TRUE;
      }
      else if( wparam == IDCANCEL )
      {
        EndDialog(hDlg, FALSE);
        return TRUE;
      }
     break;
  }
  return FALSE;
}
