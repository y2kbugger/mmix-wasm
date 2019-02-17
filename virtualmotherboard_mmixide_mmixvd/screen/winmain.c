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

HWND hwndEdit;
#define FONT_SMALL 11
#define FONT_MEDIUM 14
#define FONT_LARGE 18

int fontheight=FONT_MEDIUM;
static HFONT hfont=0;
static HFONT hCustomfont=0;
static CHOOSEFONT cf;
static LOGFONT lf={0};
void choosefont(void)
{ 
  if (hCustomfont==0) 
  { cf.lStructSize = sizeof(CHOOSEFONT); 
    cf.hwndOwner = (HWND)NULL; 
    cf.hDC = (HDC)NULL; 
    cf.lpLogFont = &lf; 
    cf.iPointSize = 0; 
    cf.Flags = CF_SCREENFONTS|CF_FIXEDPITCHONLY|CF_INITTOLOGFONTSTRUCT; 
    cf.rgbColors = RGB(0,0,0); 
    cf.lCustData = 0L; 
    cf.lpfnHook = (LPCFHOOKPROC)NULL; 
    cf.lpTemplateName = (LPSTR)NULL; 
    cf.hInstance = (HINSTANCE) NULL; 
    cf.lpszStyle = (LPSTR)NULL; 
    cf.nFontType = SCREEN_FONTTYPE; 
    cf.nSizeMin = 0; 
    cf.nSizeMax = 0; 
  }

  if (ChooseFont(&cf))
  { if (hCustomfont!=0)
	  DeleteObject(hCustomfont);
    if (hfont!=0) 
	  DeleteObject(hfont);
	hCustomfont=hfont = CreateFontIndirect(&lf); 
    SendMessage(hwndEdit, WM_SETFONT, (WPARAM) hfont, 0); 
    InvalidateRect(hwndEdit,NULL,FALSE);
  }
} 

void setfont(void)
{ 
  if (hfont!=0) 
	  DeleteObject(hfont);
  lf.lfHeight=-fontheight /* small */;
  lf.lfWidth=0;
  lf.lfOutPrecision=OUT_DEFAULT_PRECIS; /*OUT_STROKE_PRECIS*/
  lf.lfClipPrecision=CLIP_DEFAULT_PRECIS; /*CLIP_STROKE_PRECIS*/
  lf.lfQuality=PROOF_QUALITY; /* ANTIALIASED_QUALITY DRAFT_QUALITY*/;
  lf.lfCharSet = ANSI_CHARSET;
  lf.lfWeight = FW_NORMAL;
  lf.lfPitchAndFamily=FF_MODERN|FIXED_PITCH;
  strncpy(lf.lfFaceName,"Courier New",32);
  hfont = CreateFontIndirect(&lf);  
  SendMessage(hwndEdit, WM_SETFONT, (WPARAM) hfont, 0); 
  InvalidateRect(hwndEdit,NULL,FALSE);
}

INT_PTR CALLBACK   
SettingsDialogProc( HWND hDlg, UINT message, WPARAM wparam, LPARAM lparam )
{
  switch ( message )
  { case WM_INITDIALOG:
      uint64_to_hex(vmb_address,tmp_option);
      SetDlgItemText(hDlg,IDC_ADDRESS,tmp_option);
	  SetDlgItemInt(hDlg,IDC_INTERRUPT,interrupt_no,FALSE);
	  if (hCustomfont!=0)
		  CheckDlgButton(hDlg,IDC_CUSTOM_FONT,BST_CHECKED);
	  else if (fontheight <= FONT_SMALL )
		  CheckDlgButton(hDlg,IDC_SMALL_FONT,BST_CHECKED);
	  else if (fontheight >= FONT_LARGE)
		  CheckDlgButton(hDlg,IDC_LARGE_FONT,BST_CHECKED);
	  else
		  CheckDlgButton(hDlg,IDC_MEDIUM_FONT,BST_CHECKED);
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
		if (IsDlgButtonChecked(hDlg,IDC_CUSTOM_FONT))
			choosefont();
		else if (IsDlgButtonChecked(hDlg,IDC_SMALL_FONT)) fontheight=FONT_SMALL,setfont();
		else if (IsDlgButtonChecked(hDlg,IDC_MEDIUM_FONT)) fontheight=FONT_MEDIUM,setfont();
		else fontheight=FONT_LARGE,setfont();
      }
      if (wparam == IDOK || wparam == IDCANCEL)
      { EndDialog(hDlg, TRUE);
        return TRUE;
      }
      break;
  }
  return FALSE;
}

extern void process_input(unsigned char c);
static int hexoutput=0;


LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{ switch (message) 
  {  
  case WM_VMB_ON: /* Power On */
    SendMessage(hpower,STM_SETIMAGE,(WPARAM) IMAGE_BITMAP,(LPARAM)hon);
	SendMessage(hwndEdit, WM_SETTEXT, 0, (LPARAM) ""); 
	return 0;
  case WM_VMB_OFF: /* Power Off */
    SendMessage(hpower,STM_SETIMAGE,(WPARAM) IMAGE_BITMAP,(LPARAM)hoff);
	SendMessage(hwndEdit, WM_SETTEXT, 0, (LPARAM) "No Power"); 
	return 0;
 case WM_VMB_RESET: /* Reset */
	SendMessage(hwndEdit, WM_SETTEXT, 0, (LPARAM) ""); 
	return 0;
 case WM_VMB_OTHER+2: /* Got character to display */
	 { char str[4];
	   static int crlf=0; /* got cr output crlf */
       LRESULT n;
	   char c = (char)wParam;
       n = SendMessage(hwndEdit,EM_GETLINECOUNT,0,0);
       if (n>100)
       { n = SendMessage(hwndEdit,EM_LINELENGTH,0,0);
         SendMessage(hwndEdit,EM_SETSEL,0,n+2);
         SendMessage(hwndEdit,EM_REPLACESEL,(WPARAM)FALSE,(LPARAM)"");
       }
	   if (hexoutput)
       {
#define hexdigit(d) ((d)<10?(d)+'0':(d)-10+'A')
	    str[0]=hexdigit((c>>4)&0xF); str[1]=hexdigit(c&0xF); str[2]=' '; str[3]=0; 
	   }
	   else if (c=='\r')
       { crlf=1; str[0]='\r'; str[1]='\n'; str[2]=0; } 
       else if (crlf && c=='\n')
       { crlf=0; 
         str[0]=0;
	   } 
       else if (c=='\n')
       { str[0]='\r'; str[1]='\n'; str[2]=0; crlf=0; } 
       else if (c=='\b')/* backspace */
       { n = SendMessage(hwndEdit, WM_GETTEXTLENGTH,0,0);
	     if (n>0)
		 { SendMessage(hwndEdit, EM_SETSEL,n-1,n);
           SendMessage(hwndEdit, EM_REPLACESEL, 0,(LPARAM)""); 
           SendMessage(hwndEdit, EM_SCROLLCARET, 0, 0); 
		 }
         crlf=0; 
		 return 0;
	   } 
       else
       { str[0]=c; str[1]=0; crlf=0; 
	   }
       n = SendMessage(hwndEdit, WM_GETTEXTLENGTH,0,0);
       SendMessage(hwndEdit, EM_SETSEL,n,n);
       SendMessage(hwndEdit, EM_REPLACESEL, 0,(LPARAM)str); 
       SendMessage(hwndEdit, EM_SCROLLCARET, 0, 0); 
	 }
	 return 0;
  case WM_CREATE: 
	hpower = CreateWindow("STATIC",NULL,WS_CHILD|WS_VISIBLE|SS_BITMAP|SS_REALSIZEIMAGE,393,330,32,32,hWnd,(HMENU)1,hInst,0);
	SendMessage(hpower,STM_SETIMAGE,(WPARAM) IMAGE_BITMAP,(LPARAM)hoff);
    hwndEdit = CreateWindow( 
                "EDIT",     // predefined class 
                NULL,       // no window title 
                WS_CHILD | WS_VISIBLE | 
                    ES_NOHIDESEL | ES_LEFT | ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL
					/* | WS_VSCROLL */, 
                25, 25, 400, 300, 
                hWnd,       // parent window 
                (HMENU)2,
                hInst, 
                NULL);                // pointer not needed 
	 setfont();
             // Add text to the window. 
     SendMessage(hwndEdit, WM_SETTEXT, 0, 
                (LPARAM)"");  
    return 0;
  case WM_PAINT:
    { PAINTSTRUCT ps;
      HDC hdc;
      hdc = BeginPaint (hWnd, &ps);
      if (hBmp)
      {	HDC memdc = CreateCompatibleDC(NULL);
        HBITMAP h = (HBITMAP)SelectObject(memdc, hBmp);
        BITMAP bm;
        GetObject(hBmp, sizeof(bm), &bm);
        BitBlt(hdc, 0, 0, bm.bmWidth, bm.bmHeight, memdc, 0, 0, SRCCOPY);
        SelectObject(memdc, h);
      }
      EndPaint (hWnd, &ps);
    }
    return 0;
  case WM_NCRBUTTONDOWN: /* right Mouse Button -> Context Menu */
    TrackPopupMenu(GetSubMenu(hMenu,0),TPM_LEFTALIGN|TPM_TOPALIGN,
		   LOWORD(lParam),HIWORD(lParam),0 ,hWnd,NULL);
    return 0;
  case WM_COMMAND:
	if (HIWORD(wParam)==0) /* Menu */
      switch(LOWORD(wParam))
	{ 
      case ID_HEXOUTPUT:
	    hexoutput = !hexoutput;
	    CheckMenuItem( hMenu,ID_HEXOUTPUT,
		  MF_BYCOMMAND|(hexoutput?MF_CHECKED:MF_UNCHECKED));
/*	  DrawMenuBar(hMainWnd); */

	  return 0; 
	}
 }
 return (OptWndProc(hWnd, message, wParam, lParam));
}
