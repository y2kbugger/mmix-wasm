#include <windows.h>
#include <stdio.h>
#include "winopt.h"
#include "option.h"
#pragma warning(disable : 4996)

#include "resource.h"


void option_window(HWND hText)
{ option_spec *p;
  int i;
  char line[1000];
  i=0;
  p= options;
  SendMessage(hText,EM_REPLACESEL,(WPARAM)FALSE,(LPARAM)   
	          "#The following options can be used on the commandline\r\n"
			  "#or in configuration files.\r\n\r\n");
  while(p->description!=NULL)
  { if (p->shortopt)
    {  sprintf(line,"-%c  ",p->shortopt);
       SendMessage(hText,EM_REPLACESEL,(WPARAM)FALSE,(LPARAM)line);
    }
	sprintf(line,"--%s",p->longopt);
    SendMessage(hText,EM_REPLACESEL,(WPARAM)FALSE,(LPARAM)line);
    if (p->arg_name != NULL)
	{  sprintf(line," <%s>", p->arg_name);
	   SendMessage(hText,EM_REPLACESEL,(WPARAM)FALSE,(LPARAM)line);
	}
    sprintf(line,"\r\n    %s ",p->description);
    SendMessage(hText,EM_REPLACESEL,(WPARAM)FALSE,(LPARAM)line);

    if (p->default_str != NULL)
	{  sprintf(line,"\t(default = %s)",p->default_str);
       SendMessage(hText,EM_REPLACESEL,(WPARAM)FALSE,(LPARAM)line);
	}
    sprintf(line,"\r\n\r\n");
    SendMessage(hText,EM_REPLACESEL,(WPARAM)FALSE,(LPARAM)line);

    p++;
  }
}



BOOL APIENTRY   
ConfigurationDialogProc( HWND hDlg, UINT message, WPARAM wparam, LPARAM lparam )
{
  switch ( message )
  { case WM_INITDIALOG:
      SendDlgItemMessage(hDlg,IDC_CONFIGURATION,CB_SETCURSEL,0,0);  
      option_window(GetDlgItem(hDlg,IDC_CONFIGURATION));
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