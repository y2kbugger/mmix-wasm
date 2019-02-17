
int major_version=2, minor_version=1;
char version[]="$Revision: 664 $ $Date: 2018-12-10 18:10:14 +0100 (Mon, 10 Dec 2018) $";
char title[]="VMB Button";
char howto[]="The button device can be configured to send interrupts\n"
             "on button up or button down events.";
#ifdef WIN32

#include <winsock2.h>
#include <windows.h>
#include <winuser.h>
#include <afxres.h>
#include "vmb.h"
#include "resource.h"
#include "opt.h"
#include "winopt.h"
#include "param.h"
#include "inspect.h"
#include "option.h"

HBITMAP hOn, hOff, hOnTemplate, hOffTemplate;
HDC hCanvas = NULL;
int color;
int upinterrupt;
int pushbutton;
int pushstate;
int enable_interrupts;
char *label=NULL;

#define fontheight 14

void select_font(char *label)
{
  HFONT hfont;
  LOGFONT lgpu_font={0};
  lgpu_font.lfHeight=fontheight;
  lgpu_font.lfWidth=0;
  lgpu_font.lfCharSet = ANSI_CHARSET;
  lgpu_font.lfQuality= ANTIALIASED_QUALITY;
  strcpy(lgpu_font.lfFaceName,"Arial");
  lgpu_font.lfPitchAndFamily=FF_MODERN;
  hfont = CreateFontIndirectA(&lgpu_font);  
  SelectObject(hCanvas,hfont);
}

unsigned char blend_color(int c, int w)
{  int n;
   w = (w-0x7F) * 2 ;
    n = c+w;
	if (n>0xFF) n = 0xFF;
	if (n<0) n=0;
	return n;
}

#define bmRGB(r,g,b) ((b)|((g)<<8)|((r)<<16))
#define bmB(c) ((unsigned char)((c)&0xFF))
#define bmG(c) ((unsigned char)(((c)>>8)&0xFF))
#define bmR(c) ((unsigned char)(((c)>>16)&0xFF))


void color_bitmap(HBITMAP hBmp, HBITMAP hTemplate, int color, char *label)
{ BITMAP bm,bmT;
  HBITMAP hold;
  int i,n;
  unsigned char r, g, b;
  LONG *p, *q;
  RECT rect;
  HBRUSH hbr;

  r = (color>>16)&0xFF;
  g = (color>>8)&0xFF;;
  b =  color&0xFF;;

  hold=SelectObject(hCanvas, hBmp); 
  hbr = CreateSolidBrush(RGB(r,g,b));
  rect.top=0, rect.left=0, rect.bottom=height+1, rect.right=width+1;
  FillRect(hCanvas,&rect,hbr);
  if (label!=NULL && label[0]!=0)
  { if (r+g+b> 3*0x100/2)
       SetTextColor(hCanvas,RGB(0,0,0));
    else
       SetTextColor(hCanvas,RGB(0xFF,0xFF,0xFF));
    SetBkColor(hCanvas,RGB(r,g,b));
	SetTextAlign(hCanvas,TA_CENTER|TA_BOTTOM);
	ExtTextOut(hCanvas,width/2,(height+fontheight)/2,ETO_CLIPPED|ETO_OPAQUE,&rect,
		label,(UINT)strlen(label),NULL);
  }

  GetObject(hBmp, sizeof(bm), &bm);
  GetObject(hTemplate, sizeof(bmT), &bmT);
  n = bm.bmHeight*bm.bmWidth;
  p = (LONG *)bmT.bmBits;
  q = (LONG *)bm.bmBits;
  for(i=0;i<n;i++)
  { int nr,ng,nb;
    nr = blend_color(bmR(*q),bmR(*p));
    ng = blend_color(bmG(*q),bmG(*p));
    nb = blend_color(bmB(*q),bmB(*p));
	*q = bmRGB(nr,ng,nb);
	p++;
	q++;
  }
  SelectObject(hCanvas, hold); 
}

void color_bitmaps(int color, char *label)
{ color_bitmap(hOn,hOnTemplate,color,label);
  color_bitmap(hOff,hOffTemplate,color,label);
}

INT_PTR CALLBACK   
SettingsDialogProc( HWND hDlg, UINT message, WPARAM wparam, LPARAM lparam )
{
  switch ( message )
  { case WM_INITDIALOG:
      uint64_to_hex(vmb_address,tmp_option);
      SetDlgItemText(hDlg,IDC_ADDRESS,tmp_option);
	  SetDlgItemInt(hDlg,IDC_INTERRUPT,interrupt_no,FALSE);
	  SetDlgItemInt(hDlg,IDC_UPINTERRUPT,upinterrupt,FALSE);
	  SetDlgItemInt(hDlg,IDC_COLOR,color,FALSE);
	  SetDlgItemText(hDlg,IDC_LABEL,label?label:"");
	  CheckDlgButton(hDlg,IDC_INTENABLE,enable_interrupts&1?BST_CHECKED:BST_UNCHECKED);
	  CheckDlgButton(hDlg,IDC_UPINTENABLE,enable_interrupts&2?BST_CHECKED:BST_UNCHECKED);
	  CheckDlgButton(hDlg,IDC_PUSHBUTTON,pushbutton?BST_CHECKED:BST_UNCHECKED);
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
		GetDlgItemText(hDlg,IDC_LABEL,tmp_option,MAXTMPOPTION);
		set_option(&label,tmp_option);
		interrupt_no =GetDlgItemInt(hDlg,IDC_INTERRUPT,NULL,FALSE);
		upinterrupt =GetDlgItemInt(hDlg,IDC_UPINTERRUPT,NULL,FALSE);
		color =GetDlgItemInt(hDlg,IDC_COLOR,NULL,FALSE);
	    color_bitmaps(color,label);
		if (pushstate) hBmp=hOn; else hBmp=hOff;
		InvalidateRect(hMainWnd,NULL,FALSE);
		pushbutton=IsDlgButtonChecked(hDlg,IDC_PUSHBUTTON)==BST_CHECKED;
		if (IsDlgButtonChecked(hDlg,IDC_INTENABLE)==BST_CHECKED)
			enable_interrupts|=1;
		else
			enable_interrupts&=~1;
		if (IsDlgButtonChecked(hDlg,IDC_UPINTENABLE)==BST_CHECKED)
			enable_interrupts|=2;
		else
			enable_interrupts&=~2;
      }
	  if (HIWORD(wparam) == STN_CLICKED && LOWORD(wparam)== IDC_COLORCHOOSE)
	  { CHOOSECOLOR cc;                 // common dialog box structure 
        static COLORREF acrCustClr[16]; // array of custom colors 
	    // Initialize CHOOSECOLOR 
        ZeroMemory(&cc, sizeof(cc));
        cc.lStructSize = sizeof(cc);
        cc.hwndOwner = hDlg;
        cc.lpCustColors = (LPDWORD) acrCustClr;
		{int c;
		 c = GetDlgItemInt(hDlg,IDC_COLOR,NULL,FALSE);
		 cc.rgbResult = RGB(bmR(c),bmG(c),bmB(c));
		}
        cc.Flags = CC_FULLOPEN | CC_RGBINIT;
        if (ChooseColor(&cc)==TRUE) {
			int c;
			c = bmRGB(GetRValue(cc.rgbResult),GetGValue(cc.rgbResult),GetBValue(cc.rgbResult));
		    SetDlgItemInt(hDlg,IDC_COLOR,c,FALSE);
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

static int top, left;

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{ 
  switch (message) 
  {  
 case WM_VMB_ON: /* Power On */
 	return 0;
  case WM_VMB_OFF: /* Power Off */
	return 0;
  case WM_VMB_CONNECT: /* Connected */
	if (ModifyMenu(hMenu,ID_CONNECT, MF_BYCOMMAND|MF_STRING,ID_CONNECT,"Disconnect"))
	  DrawMenuBar(hMainWnd);
 	return 0;
  case WM_VMB_DISCONNECT: /* Disconnected */
	if (ModifyMenu(hMenu,ID_CONNECT, MF_BYCOMMAND|MF_STRING,ID_CONNECT,"Connect..."))
	  DrawMenuBar(hMainWnd);
	return 0;
  case WM_CREATE: 
	hpower = NULL;
	hOffTemplate = (HBITMAP)LoadImage(hInst, MAKEINTRESOURCE(IDB_BITMAPOFF), 
		                            IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
	hOnTemplate = (HBITMAP)LoadImage(hInst, MAKEINTRESOURCE(IDB_BITMAPON), 
		                            IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);

	{ HDC hdc; 
	  BITMAP bm;
	  GetObject(hOnTemplate, sizeof(bm), &bm);
	  width=bm.bmWidth;
	  height=bm.bmHeight;
	  if (bm.bmBitsPixel!=32)
        vmb_fatal_error(__LINE__, "Requires 32 bit Button Bitmap");
	  hdc = GetDC(hWnd);
	  hCanvas = CreateCompatibleDC(hdc);	 
	  hOn = (HBITMAP)LoadImage(hInst, MAKEINTRESOURCE(IDB_BITMAPON), 
		                            IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
      hOff = (HBITMAP)LoadImage(hInst, MAKEINTRESOURCE(IDB_BITMAPOFF), 
		                            IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
	  select_font(label);
	}
    return 0;
 
  case WM_NCLBUTTONDOWN:
  case WM_NCLBUTTONDBLCLK:
  {   int dx, dy;
	  WINDOWPLACEMENT wndpl;
      wndpl.length=sizeof(WINDOWPLACEMENT);
      if(GetWindowPlacement(hWnd,&wndpl))
	  { xpos = wndpl.rcNormalPosition.left;   // horizontal position 
        ypos = wndpl.rcNormalPosition.top;   // vertical position 
	  }
	  dx = xpos + 32 - LOWORD(lParam); 
      dy = ypos +32 - HIWORD(lParam);
      if (dx*dx+dy*dy>25*25) break;
	  if (pushbutton && pushstate)
	  { hBmp=hOff;
	    pushstate = 0;
	    vmb_debug(VMB_DEBUG_PROGRESS,"Button UP");
	    if (enable_interrupts&2)
    	  vmb_raise_interrupt(&vmb, upinterrupt);
	  }
	  else
	  { hBmp=hOn;
	    pushstate = 1;
	    vmb_debug(VMB_DEBUG_PROGRESS,"Button DOWN");
	    if (enable_interrupts&1)
    	  vmb_raise_interrupt(&vmb, interrupt_no);
	  }
	  InvalidateRect(hWnd,NULL,FALSE);
	  return 0;
  }
  case WM_WINDOWPOSCHANGED:
	  top = ((WINDOWPOS *)lParam)->y;
	  left = ((WINDOWPOS *)lParam)->x;
	  break;
  case WM_NCLBUTTONUP:
  {   int dx, dy;
	  dx = left + 32 - LOWORD(lParam); 
      dy = top +32 - HIWORD(lParam);
      if (dx*dx+dy*dy>20*20) break;
	  if (pushbutton) return 0;
	  hBmp=hOff;
	  pushstate = 0;
  	  vmb_debug(VMB_DEBUG_PROGRESS,"Button UP");
	  InvalidateRect(hWnd,NULL,FALSE);
	  if (enable_interrupts&2)
	    vmb_raise_interrupt(&vmb, upinterrupt);
	  return 0;
  }
  }
 return (OptWndProc(hWnd, message, wParam, lParam));
}


void init_device(device_info *vmb)
{ BITMAP bm;
  pushstate = 0;
  color_bitmaps(color,label);
  hBmp = hOff;
  GetObject(hBmp, sizeof(bm), &bm);
  SetWindowPos(hMainWnd,HWND_TOP,0,0,width,height,
		  SWP_NOMOVE|SWP_NOOWNERZORDER|SWP_SHOWWINDOW);
  vmb->poweron=vmb_poweron;
  vmb->poweroff=vmb_poweroff;
  vmb->disconnected=vmb_disconnected;
  vmb->terminate=vmb_terminate;
}

#else
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <termio.h>
#include <string.h>
#include "vmb.h"
#include "resource.h"
#include "param.h"
#include "option.h"


device_info vmb = {0};


int color;
int upinterrupt;
int pushbutton;
int pushstate;
int enable_interrupts;
char *label=NULL;

void process_input(unsigned char c) 
{ if (c!='\n') return;
  if (pushbutton && pushstate)
  { pushstate = 0;
    vmb_debug(VMB_DEBUG_PROGRESS,"Button UP");
    if (enable_interrupts&2)
   	  vmb_raise_interrupt(&vmb, upinterrupt);
  }
  else
  { pushstate = 1;
    vmb_debug(VMB_DEBUG_PROGRESS,"Button DOWN");
    if (enable_interrupts&1)
  	  vmb_raise_interrupt(&vmb, interrupt_no);
  }
}

void button_terminate(void)
{ close(0);
#if 0
  vmb_terminate();
  vmb_debug(VMB_DEBUG_PROGRESS,"Disconnecting");
  vmb_disconnect(&vmb);
  vmb_debug(VMB_DEBUG_PROGRESS,"Exiting");
  exit(0);
#endif
}

static struct termios *tio=NULL;

static void prepare_input(void)
{ static struct termios oldtio;
  struct termios newtio;
  
  tcgetattr(0,&oldtio); /* save current port settings */
  tio = &oldtio;

  newtio=oldtio;
  //memset(&newtio, 0, sizeof(newtio));

  //newtio.c_cflag = CS8 | CLOCAL | CREAD; /* input modes */
  //newtio.c_iflag = 0; /* IGNBRK; */ /* control modes */
  //newtio.c_oflag = 0;      /* output modes */

  /* set input mode (non-canonical, no echo,...) */
  //newtio.c_lflag = 0;     /* local modes */
  newtio.c_lflag &= ~ICANON;
 
  /* control characters */
  newtio.c_cc[VTIME]    = 20;   /* inter-character timerin 1/10 s unused */
  newtio.c_cc[VMIN]     = 0;   /* blocking read until 0 chars received */

  tcflush(0, TCIFLUSH);
  tcsetattr(0,TCSANOW,&newtio);
}

void init_device(device_info *vmb)
{  prepare_input();
   vmb->poweron=vmb_poweron;
   vmb->poweroff=vmb_poweroff;
   vmb->disconnected=vmb_disconnected;
   vmb->reset=vmb_reset;
   vmb->terminate=button_terminate;
   vmb_size=0;
   vmb_address=0;
   pushstate=0;
}

int main(int argc, char *argv[])
{
  param_init(argc, argv);
  vmb_debugs(VMB_DEBUG_INFO, "%s ",vmb_program_name);
  vmb_debugs(VMB_DEBUG_INFO, "%s ", version);
  vmb_debugs(VMB_DEBUG_INFO, "host: %s ",host);
  vmb_debugi(VMB_DEBUG_INFO, "port: %d ",port);
  init_device(&vmb);
  vmb_debugi(VMB_DEBUG_INFO, "interrupt: %d",interrupt_no);
  
  vmb_connect(&vmb,host,port); 

  vmb_register(&vmb,HI32(vmb_address),LO32(vmb_address),vmb_size,
               0, 0, vmb_program_name,major_version,minor_version);

  vmb_debug(VMB_DEBUG_INFO, "Reading characters");
  while (vmb.connected)
  { unsigned char c;
    int i;
    while ((i = read(0,&c,1))==0 && vmb.connected)
      continue;
    if (i < 0)
    { vmb_error(__LINE__,"Read Error");
      break;
    }
    else if (i>0 && c=='\n')
    { vmb_debugi(VMB_DEBUG_INFO, "got %02X",c&0xFF);
      process_input(c);
    }
  }
  vmb_disconnect(&vmb);
  return 0;
}

#endif
