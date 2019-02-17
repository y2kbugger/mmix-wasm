#include <windows.h>
#include <stdio.h>
#include <htmlhelp.h> 
#include "resource.h"
#include "winmain.h"
#include "winopt.h"
#ifdef VMB
#include "option.h"
#else
#define MAXTMPOPTION 1024
static char  tmp_option[MAXTMPOPTION+1];
#endif
#include "runoptions.h"

char *run_args=NULL;
char *stdin_file=NULL;



static void browse_stdin(HWND hDlg)
/* opens a file as a new document */
{ 	OPENFILENAME ofn={0};
	ofn.lStructSize= sizeof(OPENFILENAME);
	ofn.hwndOwner = hMainWnd;
	ofn.hInstance = hInst;
	ofn.lpstrFile = tmp_option;
	ofn.nMaxFile = MAXTMPOPTION;
	ofn.lpstrFilter = NULL;
	ofn.lpstrCustomFilter = NULL;
	ofn.nMaxCustFilter = 0;
	ofn.nFilterIndex = 0;
	ofn.lpstrTitle = "Open File";
	ofn.Flags = OFN_FILEMUSTEXIST;
    if (GetOpenFileName(&ofn))
	  SetDlgItemText(hDlg,IDC_FAKE_STDIN,tmp_option);
}

INT_PTR CALLBACK 
OptionRunDialogProc( HWND hDlg, UINT message, WPARAM wparam, LPARAM lparam )
{
  switch ( message )
  { case WM_INITDIALOG:
      if (run_args!=NULL) SetDlgItemText(hDlg,IDC_ARGS,run_args);
	  if (stdin_file!=NULL) SetDlgItemText(hDlg,IDC_FAKE_STDIN,stdin_file);
      return TRUE;
    case WM_SYSCOMMAND:
      if( wparam == SC_CLOSE ) 
      { EndDialog(hDlg, TRUE);
        return TRUE;
      }
      break;
    case WM_COMMAND:
      if( wparam == IDOK )
	  { GetDlgItemText(hDlg,IDC_ARGS,tmp_option,MAXTMPOPTION);
	    set_option(&run_args,tmp_option);
	    GetDlgItemText(hDlg,IDC_FAKE_STDIN,tmp_option,MAXTMPOPTION);
	    set_option(&stdin_file,tmp_option);
		EndDialog(hDlg, TRUE);
        return TRUE;
      } 
	  else if (wparam==IDCANCEL)
	  {	 EndDialog(hDlg, TRUE);
        return TRUE;
	  }
	  else if (wparam==IDC_BROWSE_STDIN)
	  {  browse_stdin(hDlg);
	  }
     break;
  case WM_HELP:
    ide_help("help\\options\\run.html");
    return TRUE;

  }
  return FALSE;
}
