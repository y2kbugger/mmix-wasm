#include <winsock2.h>
#include <windows.h>
#include <afxres.h>
#include "vmb.h"
#include "bus-arith.h"
#include "resource.h"
#include "winopt.h"
#include "opt.h"
#include "param.h"
#include "inspect.h"
#include "option.h"


HBITMAP hBmpPinOn,hBmpPinOff;
extern int rinterrupt, winterrupt, rdisable, wdisable;
static int wrequested=0, rrequested=0;
extern char *serial;
extern int unbuffered;
extern int create_pseudo_tty(void);


INT_PTR CALLBACK   
SettingsDialogProc( HWND hDlg, UINT message, WPARAM wparam, LPARAM lparam )
{
  switch ( message )
  { case WM_INITDIALOG:
	  uint64_to_hex(vmb_address,tmp_option);
      SetDlgItemText(hDlg,IDC_ADDRESS,tmp_option);
	  SetDlgItemInt(hDlg,IDC_RINTERRUPT,rinterrupt,FALSE);
	  SetDlgItemInt(hDlg,IDC_WINTERRUPT,winterrupt,FALSE);
      CheckDlgButton(hDlg,IDC_RENABLE,!rdisable?BST_CHECKED:BST_UNCHECKED);
      CheckDlgButton(hDlg,IDC_WENABLE,!wdisable?BST_CHECKED:BST_UNCHECKED);
      CheckDlgButton(hDlg,IDC_BUFFERED,unbuffered?BST_CHECKED:BST_UNCHECKED);
      SetDlgItemText(hDlg,IDC_DEVICENAME,serial);
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
		winterrupt  = GetDlgItemInt(hDlg,IDC_WINTERRUPT,NULL,FALSE);
		rinterrupt  = GetDlgItemInt(hDlg,IDC_RINTERRUPT,NULL,FALSE);
		wdisable=!IsDlgButtonChecked(hDlg,IDC_WENABLE);
		rdisable=!IsDlgButtonChecked(hDlg,IDC_RENABLE);
	    unbuffered=IsDlgButtonChecked(hDlg,IDC_BUFFERED);
		GetDlgItemText(hDlg,IDC_DEVICENAME,tmp_option,MAXTMPOPTION);
		if (strncmp(serial,tmp_option,MAXTMPOPTION)!=0)
		{ set_option(&serial, tmp_option);
		  create_pseudo_tty();
		}
      }
      if (wparam == IDOK || wparam == IDCANCEL)
      { EndDialog(hDlg, TRUE);
        return TRUE;
      }
      break;
  }
  return FALSE;
}
extern void process_input_file(char *filename);

extern int pins, npins;

void display_pins(void)
{ RECT Rect;
  Rect.top=18;
  Rect.bottom=27;
  Rect.left=25;
  Rect.right=25+25*npins+9;
  InvalidateRect(hMainWnd,&Rect,FALSE);
}

void paint_pins(HDC memdc, HDC hdc)
{ int i;
  for (i=0;i<npins;i++)  
  { if ((pins>>i)&1)
	  SelectObject(memdc, hBmpPinOn);
    else
	  SelectObject(memdc, hBmpPinOff);
    BitBlt(hdc, 25+25*i, 18, 9, 9, memdc, 0, 0, SRCCOPY);
  }
}


LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{ switch (message) 
  { case WM_CREATE: 
	  hpower = CreateWindow("STATIC",NULL,WS_CHILD|WS_VISIBLE|SS_BITMAP|SS_REALSIZEIMAGE,145,80,32,32,hWnd,NULL,hInst,0);
      SendMessage(hpower,STM_SETIMAGE,(WPARAM) IMAGE_BITMAP,(LPARAM)hoff);
	  hBmpPinOn= (HBITMAP)LoadImage(hInst, MAKEINTRESOURCE(IDB_PIN_ON), 
	              IMAGE_BITMAP, 9, 9, LR_CREATEDIBSECTION);
	  hBmpPinOff= (HBITMAP)LoadImage(hInst, MAKEINTRESOURCE(IDB_PIN_OFF), 
				IMAGE_BITMAP, 9, 9, LR_CREATEDIBSECTION);

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

        paint_pins(memdc, hdc);

        SelectObject(memdc, h);
	    DeleteDC(memdc);
	  }
      EndPaint (hWnd, &ps);
    }
    return 0;
  }
  return (OptWndProc(hWnd, message, wParam, lParam));
}