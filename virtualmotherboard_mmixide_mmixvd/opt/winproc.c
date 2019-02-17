#include <windows.h>
#include <htmlhelp.h> 
#include <afxres.h>
#include "vmb.h"
#include "opt.h"
#include "winopt.h"
#include "param.h"
#include "option.h"
#include "resource.h"


extern HWND hMainWnd;
extern HINSTANCE hInst;
extern HMENU hMenu;
HWND hpower;
extern INT_PTR CALLBACK    
AboutDialogProc( HWND hDlg, UINT message, WPARAM wparam, LPARAM lparam );

extern INT_PTR CALLBACK    
ConnectDialogProc( HWND hDlg, UINT message, WPARAM wparam, LPARAM lparam );
extern void write_regtab(char *program);


LRESULT CALLBACK OptWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{ 	
  switch (message) 
  {  
  case WM_NCHITTEST:
    return HTCAPTION;
  case WM_VMB_ON: /* Power On */
	if (hpower!=NULL)
      SendMessage(hpower,STM_SETIMAGE,(WPARAM) IMAGE_BITMAP,(LPARAM)hon);
	return 0;
  case WM_VMB_OFF: /* Power Off */
	if (hpower!=NULL)
    SendMessage(hpower,STM_SETIMAGE,(WPARAM) IMAGE_BITMAP,(LPARAM)hoff);
	return 0;
  case WM_VMB_CONNECT: /* Connected */
	if (ModifyMenu(hMenu,ID_CONNECT, MF_BYCOMMAND|MF_STRING,ID_CONNECT,"Disconnect"))
	  DrawMenuBar(hMainWnd);
	if (hpower!=NULL)
	SendMessage(hpower,STM_SETIMAGE,(WPARAM) IMAGE_BITMAP,(LPARAM)hoff);
 	return 0;
  case WM_VMB_DISCONNECT: /* Disconnected */
	if (ModifyMenu(hMenu,ID_CONNECT, MF_BYCOMMAND|MF_STRING,ID_CONNECT,"Connect..."))
	  DrawMenuBar(hMainWnd);
	if (hpower!=NULL)
	   SendMessage(hpower,STM_SETIMAGE,(WPARAM) IMAGE_BITMAP,(LPARAM)hconnect);
	return 0;
  case WM_CREATE: 
	hpower = CreateWindow("STATIC",NULL,WS_CHILD|WS_VISIBLE|SS_BITMAP|SS_REALSIZEIMAGE,10,10,32,32,hWnd,NULL,hInst,0);
	if (hpower!=NULL)
    SendMessage(hpower,STM_SETIMAGE,(WPARAM) IMAGE_BITMAP,(LPARAM)hoff);
    return 0;
  case WM_PAINT:
    { PAINTSTRUCT ps;
      HDC hdc = BeginPaint (hWnd, &ps);
	  if (hBmp)
	  { HDC memdc = CreateCompatibleDC(NULL);
        HBITMAP h = (HBITMAP)SelectObject(memdc, hBmp);
        BITMAP bm;
        GetObject(hBmp, sizeof(bm), &bm);
        BitBlt(hdc, 0, 0, bm.bmWidth, bm.bmHeight, memdc, 0, 0, SRCCOPY);
        SelectObject(memdc, h);
	    DeleteDC(memdc);
	  }
      EndPaint (hWnd, &ps);
    }
    return 0;
  case WM_NCRBUTTONDOWN: /* right Mouse Button -> Context Menu */
	  { SHORT xPos, yPos;
		xPos=LOWORD(lParam);
	    yPos=HIWORD(lParam);
    TrackPopupMenu(GetSubMenu(hMenu,0),TPM_LEFTALIGN|TPM_TOPALIGN|TPM_NONOTIFY|TPM_RIGHTBUTTON,
		   xPos,yPos,0 ,hWnd,NULL);
	  }
    return 0;
  case WM_COMMAND:
    if (HIWORD(wParam)==0) /* Menu */
      switch(LOWORD(wParam))
	{ case ID_EXIT:
	  case ID_FILE_EXIT:
        PostMessage(hMainWnd,WM_CLOSE,0,0);
	    return 0;
	case ID_CONNECT:
	  if (!vmb.connected)
	  { if (DialogBox(hInst,MAKEINTRESOURCE(IDD_CONNECT),hWnd,ConnectDialogProc))
	  	{  vmb_connect(&vmb,host,port);
	       vmb_register(&vmb,HI32(vmb_address), LO32(vmb_address),vmb_size,0,0,defined,major_version,minor_version);
   	       SendMessage(hMainWnd,WM_VMB_CONNECT,0,0); /* the connect button */
		}
	  }
	  else
	    vmb_disconnect(&vmb);
	  return 0;
	case ID_SETTINGS:
	  DialogBox(hInst,MAKEINTRESOURCE(IDD_SETTINGS),hMainWnd,SettingsDialogProc);
	  return 0; 
	case ID_DEBUG:
		if (vmb_debug_flag)
		  vmb_debug_off();
		else 
		  vmb_debug_on();
	  return 0;
	case ID_VERBOSE:
        if (vmb_debug_mask==0) vmb_debug_mask = VMB_DEBUG_DEFAULT; else vmb_debug_mask = 0;
	    CheckMenuItem(hMenu,ID_VERBOSE,MF_BYCOMMAND|(vmb_debug_mask==0?MF_CHECKED:MF_UNCHECKED));
	  return 0;
	case ID_HELP_ABOUT:
	  DialogBox(hInst,MAKEINTRESOURCE(IDD_ABOUT),hWnd,AboutDialogProc);
	  return 0; 
	case ID_HELP_CONFIGURATION:
	  DialogBox(hInst,MAKEINTRESOURCE(IDD_CONFIGURATION),hWnd,ConfigurationDialogProc);
	  return 0; 
	case ID_HELP:
	  HtmlHelp(hWnd,programhelpfile,HH_DISPLAY_TOPIC,(DWORD_PTR)NULL) ;
	  return 0; 
	case ID_MINIMIZE:
	  CloseWindow(hWnd);
	  return 0; 
	}
    return 0;

  case WM_DESTROY:
	set_xypos(hMainWnd);
	write_regtab(defined);
    PostQuitMessage(0);
    return 0;
  default:
    return (DefWindowProc(hWnd, message, wParam, lParam));
  }
 return (DefWindowProc(hWnd, message, wParam, lParam));
}

