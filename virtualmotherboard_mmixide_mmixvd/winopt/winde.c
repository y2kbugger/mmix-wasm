#include <windows.h>
#include "winopt.h"
#include "dedit.h"
#include "winde.h"



#define MAXDATAEDIT 4
static HWND hDataEdit[MAXDATAEDIT] = {0};
static ATOM hDataEditClass=0;
HINSTANCE hDataEditInstance=0;
HWND hDataEditParent=0;

#define DEDIT_STYLE (WS_OVERLAPPED|WS_SYSMENU|WS_CAPTION|WS_VISIBLE|WS_SIZEBOX)

static LRESULT CALLBACK DataEditProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{ static int min_w, min_h;
  switch ( message )
  { case WM_CREATE :
    { RECT rect;
	  int x,y;
	  HWND hDataEdit = CreateDataEdit(hDataEditInstance,hWnd);
	  SetWindowLongPtr(hWnd,GWLP_USERDATA,(LONG)(LONG_PTR)hDataEdit);
      GetWindowRect(hDataEdit,&rect);
	  x=rect.left;
	  y=rect.top;
	  AdjustWindowRect(&rect,DEDIT_STYLE,FALSE);
	  min_w = rect.right-rect.left;
	  min_h = rect.bottom-rect.top;
	  if (y>screen_height-min_h) y=screen_height-min_h;
	  if (x>screen_width-min_w) x=screen_width-min_w;
	  SetWindowPos(hWnd,HWND_TOP,x,y,min_w,min_h,SWP_NOZORDER|SWP_SHOWWINDOW);
      UpdateWindow(hWnd);
	}
  	 return 0;
	case WM_CLOSE:
	{ int id;
	  HWND child = (HWND)(LONG_PTR)GetWindowLongPtr(hWnd,GWLP_USERDATA);
	  de_connect(child,NULL);
      for (id=0;id<MAXDATAEDIT;id++)
	    if (hDataEdit[id]==child)
           hDataEdit[id]=NULL;
      }
	  DestroyWindow(hWnd);
	  return 0;
	case WM_GETMINMAXINFO:
	{ MINMAXINFO *p = (MINMAXINFO *)lParam;
	  p->ptMinTrackSize.x = min_w;
      p->ptMinTrackSize.y = min_h;
	  p->ptMaxTrackSize.x=p->ptMinTrackSize.x;
	  p->ptMaxTrackSize.y=p->ptMinTrackSize.y;
	}
	return 0;
 }
  return DefWindowProc(hWnd, message, wParam, lParam);
}

static ATOM RegisterDataEditClass(HINSTANCE hInst)
{ WNDCLASSEX wcex;
  ZeroMemory(&wcex, sizeof(wcex));
	wcex.cbSize = sizeof(WNDCLASSEX); 
	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= (WNDPROC)DataEditProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInst;
	wcex.hIcon			= NULL;
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= NULL; /*(HBRUSH)(COLOR_WINDOW+1);*/
	wcex.lpszMenuName	= NULL;
	wcex.lpszClassName = "DATAEDITCLASS";
	wcex.hIconSm		= NULL;

	return RegisterClassEx(&wcex);
}

HWND GetDataEdit(int id, HWND hMemory)
{ HWND h;
  RECT rect;
  if (id <0 || id>=MAXDATAEDIT) 
  { win32_error(__LINE__,"ID out of range in GetDataEdit");
    id = 0;
  }
  if (hDataEdit[id]!=NULL)
    return hDataEdit[id];
  if (hDataEditClass==0)
    hDataEditClass=RegisterDataEditClass(hDataEditInstance);
  if (hDataEditParent!=NULL)
    GetWindowRect(hDataEditParent,&rect);
  else
    rect.left=rect.bottom=CW_USEDEFAULT;
  h = CreateWindow("DATAEDITCLASS","Data Editor",DEDIT_STYLE ,
			rect.left,rect.bottom,CW_USEDEFAULT,CW_USEDEFAULT,
		  hDataEditParent,NULL,hDataEditInstance,0);
  if (h!=NULL)
    hDataEdit[id]=(HWND)(LONG_PTR)GetWindowLongPtr(h,GWLP_USERDATA);
  if (hDataEdit[id]==NULL)
    win32_fatal_error(__LINE__,"Unable to create Data Editor");
  return hDataEdit[id];
}

void DestroyDataEdit(int id)
{ if (hDataEdit[id]!=NULL)
  { DestroyWindow(GetParent(hDataEdit[id]));
    hDataEdit[id]=NULL;
  }
}


