
#include <windows.h>
#include "winopt.h"

#define SUBWINDOW_MAX 100
static int  subwindow_num=0;
static HWND subwindow[SUBWINDOW_MAX] = {0};

void register_subwindow(HWND h)
{ if (subwindow_num<SUBWINDOW_MAX)
  { subwindow[subwindow_num]=h;
	subwindow_num++;
  }
  else
    win32_error(__LINE__,"Too many subwindows, limited functionality");
}

void unregister_subwindow(HWND h)
{ int i;
  for (i=0; i< subwindow_num;i++)
    if (subwindow[i]==h)
	{ subwindow_num--;
	  while (i<subwindow_num)
	  { subwindow[i]=subwindow[i+1];
	    i++;
	  }
      return;
	}
    win32_error(__LINE__,"Unregistering an unregistered window");
}

BOOL do_subwindow_msg(MSG *msg)
{ int i;
  for (i=0; i<subwindow_num;i++)
	  if (IsDialogMessage(subwindow[i], msg)) 
		  return TRUE;
  return FALSE;
}

/* In WinMain call like this:
	while (GetMessage(&msg, NULL, 0, 0)) 
	  if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg) &&
		  !do_subwindow_msg(&msg) 
		  ) 
	  { TranslateMessage(&msg);
	    DispatchMessage(&msg);
	  }


	  and creating a Dialog do
	  hDlg=CreateDialog(...);
	  register_subwindow(hDlg);

	  and unregister befor destroying

	  unregister_subwindow(hDlg);
      DestroyWindow(hDlg);
*/