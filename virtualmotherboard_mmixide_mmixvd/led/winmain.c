#include <windows.h>
#include <commctrl.h>
#include "vmb.h"
#include "bus-arith.h"
#include "resource.h"
#include "winopt.h"
#include "opt.h"
#include "param.h"
#include "inspect.h"
#include "option.h"

extern char *label;
static int labelheight=0;
static int fontheight=0;
extern int vertical;
extern unsigned char led;
extern int nleds; /* number of leds to display */
extern int colors[];
extern char *pictures[];



HBITMAP ledOnB, ledOffB, ledOnW, ledOffW, ledDisconnected;
int ledwidth, ledheight;
int hcolors[8] = {IDC_COLOR0, IDC_COLOR1, IDC_COLOR2, IDC_COLOR3, 
                  IDC_COLOR4, IDC_COLOR5, IDC_COLOR6, IDC_COLOR7};
HBITMAP hOn[8] = {0};
HBITMAP hOff[8] = {0};

LONG * picturebits[8]={0};

static void init_metrics(void)
{ BITMAP bm;
  TEXTMETRIC tm;
  HDC hdc = CreateCompatibleDC(NULL);
  HGDIOBJ oldf, hf = GetStockObject(DEFAULT_GUI_FONT);
  oldf= SelectObject(hdc,hf);
  GetTextMetrics(hdc ,&tm);
  SelectObject(hdc,oldf);
  fontheight = tm.tmHeight;
  GetObject(ledOnB,sizeof(bm),&bm);
  ledwidth=bm.bmWidth;
  ledheight=bm.bmHeight;
}

unsigned char blend_color(unsigned char c, unsigned char b, unsigned char w, unsigned char p)
{  int n;
   n = (int)(b + (c *(w/255.0)*(p/255.0)));
   if (n > 255) return 255;
   else return (unsigned char)n;
}

#define bmRGB(r,g,b) ((b)|((g)<<8)|((r)<<16))
#define bmB(c) ((unsigned char)((c)&0xFF))
#define bmG(c) ((unsigned char)(((c)>>8)&0xFF))
#define bmR(c) ((unsigned char)(((c)>>16)&0xFF))

static void load_pictures(void)
{ int i;
  for (i=0;i<8;i++)
  { if (picturebits[i]!=NULL) free(picturebits[i]);
	picturebits[i] =NULL;
    if (pictures[i]!=NULL && pictures[i]!=0)
	   { HANDLE f =CreateFile(pictures[i],FILE_READ_DATA,FILE_SHARE_READ,
	           NULL,OPEN_EXISTING,FILE_ATTRIBUTE_READONLY,NULL);
	     if (f==INVALID_HANDLE_VALUE)
			 vmb_error2(__LINE__,"Unable to load picture",pictures[i]);
		 else
		 { int size =  GetFileSize(f, NULL);
		   int bmsize = 32*32*sizeof(LONG);
		   DWORD n;
		   BITMAPFILEHEADER bfh;
		   BITMAPINFOHEADER bih;
 		   ReadFile(f,&bfh,sizeof(bfh),&n,NULL);
		   if (bfh.bfType!=0x4D42) /* BM */
			 vmb_error(__LINE__,"picture is not a bitmap file");
		   ReadFile(f,&bih,sizeof(bih),&n,NULL);
		   if (bih.biHeight!=32)
			  vmb_error(__LINE__,"height not 32");
		   if (bih.biWidth!=32)
			  vmb_error(__LINE__,"width not 32");
		   if (bih.biBitCount!=32)
			  vmb_error(__LINE__,"bits/pixel not 32");
		   if (bih.biCompression!=0)
			  vmb_error(__LINE__,"bitmap is compressed");
		   picturebits[i]=malloc(bmsize);
		   if (picturebits[i]==NULL)
		     vmb_error(__LINE__,"Out of Memory for picture");
		   if (bfh.bfOffBits>sizeof(bfh)+sizeof(bih))
		   { int d = bfh.bfOffBits-sizeof(bfh)-sizeof(bih);
		     while (d>=bmsize) 
			 { ReadFile(f,picturebits[i],bmsize,&n,NULL);
			   d = d -bmsize;
			 }
			 if(d>0) 
			   ReadFile(f,picturebits[i],d,&n,NULL);
		   }
		   ReadFile(f,picturebits[i],bmsize,&n,NULL);
		   CloseHandle(f);
		 }
	   }



#if 0
	{  HBITMAP hbm=(HBITMAP)LoadImage(NULL,pictures[i],IMAGE_BITMAP,32,32,
	                LR_LOADFROMFILE|LR_DEFAULTSIZE); 
       if (hbm==0)
		   vmb_error2(__LINE__,"Unable to load picture",pictures[i]);
	   else 

		   /* works simetimes but only consistently in debug mode */
	   { HDC hDC;
		BITMAPINFO bmi;
		int lines;
		bmi.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);
		bmi.bmiHeader.biHeight=32;
		bmi.bmiHeader.biWidth=32;
		bmi.bmiHeader.biPlanes=1;
		bmi.bmiHeader.biClrUsed=0;
		bmi.bmiHeader.biCompression=BI_RGB;
		bmi.bmiHeader.biBitCount=32;
		bmi.bmiHeader.biSizeImage=32*32*sizeof(LONG); /* RGB */
		bmi.bmiHeader.biClrImportant=0;
		bmi.bmiHeader.biXPelsPerMeter=0;
		bmi.bmiHeader.biYPelsPerMeter=0;
	    vmb_debugs(VMB_DEBUG_PROGRESS,"Loaded picture %s",pictures[i]);
		picturebits[i]=malloc(32*32*sizeof(LONG));
		if (picturebits[i]==NULL)
			vmb_error(__LINE__,"Out of Memory for picture");
		hDC = GetDC(hMainWnd);
		lines = GetDIBits(hDC, hbm, 0, 32, picturebits[i], &bmi,DIB_RGB_COLORS);
		if (lines!=32){
		  vmb_error(lines,"loding picture with lines != 32");
          vmb_error(bmi.bmiHeader.biHeight,"height ");
          vmb_error(bmi.bmiHeader.biWidth,"width  ");
		  vmb_error(bmi.bmiHeader.biClrUsed,"crl used ");
		  vmb_error(bmi.bmiHeader.biCompression,"compression ");
		  vmb_error(bmi.bmiHeader.biBitCount,"bit count ");
		  vmb_error(bmi.bmiHeader.biSizeImage,"sizeImage ");
		  vmb_error(bmi.bmiHeader.biSize,"size ");
		}
		ReleaseDC(hMainWnd, hDC);
	   }
	}
#endif
  }
}

static void set_color(BITMAP *bm32, BITMAP *bm32B, BITMAP *bm32W, int color, LONG *pP)
{ int i,n;
  unsigned char r, g, b;
  LONG *p, *pB, *pW, pPL;

  r = (color>>16)&0xFF;
  g = (color>>8)&0xFF;;
  b =  color&0xFF;;
  n = bm32->bmHeight*bm32->bmWidth;
  p = (LONG *)bm32->bmBits;
  pB = (LONG *)bm32B->bmBits;
  pW = (LONG *)bm32W->bmBits;

  for(i=0;i<n;i++)
  { int nr,ng,nb;
	if (pP!=NULL) pPL=*pP; else pPL= 0xFFFFFFFF;
    nr = blend_color(r,bmR(*pB),bmR(*pW),bmR(pPL));
    ng = blend_color(g,bmG(*pB),bmG(*pW),bmG(pPL));
    nb = blend_color(b,bmB(*pB),bmB(*pW),bmB(pPL));
	*p = bmRGB(nr,ng,nb);
	p++; pB++; pW++; 
	if (pP!=NULL) pP++; 
  }
}

static void color_led(int i, int color)
{ BITMAP bm, bmB, bmW;
  
  GetObject(hOn[i], sizeof(bm), &bm);
  GetObject(ledOnB, sizeof(bmB), &bmB);
  GetObject(ledOnW, sizeof(bmW), &bmW);
  set_color(&bm,&bmB,&bmW,color,picturebits[i]);
  GetObject(hOff[i], sizeof(bm), &bm);
  GetObject(ledOffB, sizeof(bmB), &bmB);
  GetObject(ledOffW, sizeof(bmW), &bmW);
  set_color(&bm,&bmB,&bmW,color,picturebits[i]);
  colors[i]=color;
}

static void init_colors(void)
{ int i;
  for (i=0;i<8;i++)
  { 
    hOn[i] = (HBITMAP)LoadImage(hInst, MAKEINTRESOURCE(IDB_ON_BLACK), 
		                            IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
	hOff[i] = (HBITMAP)LoadImage(hInst, MAKEINTRESOURCE(IDB_OFF_BLACK), 
		                            IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
	color_led(i,colors[i]);
  }
}

void choose_color(HWND hDlg, int i)
	  { CHOOSECOLOR cc;                 // common dialog box structure 
        static COLORREF acrCustClr[16]; // array of custom colors 
	    // Initialize CHOOSECOLOR 
        ZeroMemory(&cc, sizeof(cc));
        cc.lStructSize = sizeof(cc);
        cc.hwndOwner = hDlg;
        cc.lpCustColors = (LPDWORD) acrCustClr;
		 cc.rgbResult = RGB(bmR(colors[i]),bmG(colors[i]),bmB(colors[i]));
        cc.Flags = CC_FULLOPEN | CC_RGBINIT;
        if (ChooseColor(&cc)==TRUE) {
		    colors[i] = bmRGB(GetRValue(cc.rgbResult),
				GetGValue(cc.rgbResult),GetBValue(cc.rgbResult));
			color_led(i,colors[i]);
	        SendMessage(GetDlgItem(hDlg,hcolors[i]),STM_SETIMAGE,(WPARAM) IMAGE_BITMAP,(LPARAM)hOn[i]);
		}
	  }


extern HBITMAP hOn[], hOff[];

void paint_leds(HDC dest, unsigned char led)
{ int i;
  HDC src = CreateCompatibleDC(NULL);
  HBITMAP h = (HBITMAP)SelectObject(src, hOn[0]);
  for (i=nleds-1;i>=0;i--) {
	if (!vmb.connected)
      SelectObject(src, ledOffW);
	else if (!vmb.power)
      SelectObject(src, ledOffB);
    else if (led & (1<<i))
      SelectObject(src, hOn[i]);
    else
      SelectObject(src, hOff[i]);
	if (vertical)
	  BitBlt(dest, 
	         0,(nleds-1-i)*ledheight, 
		     ledwidth, ledheight, src, 0, 0, SRCCOPY);
	else
	  BitBlt(dest, 
	         (nleds-1-i)*ledwidth, 0, 
		     ledwidth, ledheight, src, 0, 0, SRCCOPY);
  } 
  SelectObject(src, h);
  DeleteDC(src);
}

void paint_label(HDC dest, char *label)
{ if (label==NULL || label[0]==0)
    return;
  else
  {	int w,h;
    RECT rect;
	HGDIOBJ oldf, hf = GetStockObject(DEFAULT_GUI_FONT);
    oldf= SelectObject(dest,hf);
    SetTextColor(dest,RGB(0xff,0xff,0xff));
    SetBkColor(dest,RGB(0,0,0));
	SetTextAlign(dest,TA_CENTER|TA_TOP);
    if (vertical) w=ledwidth,h=ledheight*nleds;
	else          w=ledwidth*nleds,h=ledheight;
	rect.left=0;
	rect.right=w;
	rect.top=h;
	rect.bottom=h+labelheight;
	ExtTextOut(dest,w/2,h,
		       ETO_CLIPPED|ETO_OPAQUE,&rect,
		       label,(UINT)strlen(label),NULL);
	SelectObject(dest,oldf);
  }
}

void update_display(void)
{  InvalidateRect(hMainWnd,NULL,FALSE); 
}

static void resize_window(void)
{ if (label==NULL || label[0]==0) labelheight=0;
  else labelheight=fontheight;
  if (vertical)
    SetWindowPos(hMainWnd,HWND_TOP,0,0,ledwidth,ledheight*nleds+labelheight,
	SWP_NOMOVE|SWP_NOZORDER|SWP_SHOWWINDOW);
  else
    SetWindowPos(hMainWnd,HWND_TOP,0,0,ledwidth*nleds,ledheight+labelheight,
	SWP_NOMOVE|SWP_NOZORDER|SWP_SHOWWINDOW);
}

INT_PTR CALLBACK  
SettingsDialogProc( HWND hDlg, UINT message, WPARAM wparam, LPARAM lparam )
{

  switch ( message )
  { case WM_INITDIALOG:
      uint64_to_hex(vmb_address,tmp_option);
      SetDlgItemText(hDlg,IDC_ADDRESS,tmp_option);
	  SendMessage(GetDlgItem(hDlg,IDC_COLOR0),STM_SETIMAGE,(WPARAM) IMAGE_BITMAP,(LPARAM)hOn[0]);
	  SendMessage(GetDlgItem(hDlg,IDC_COLOR1),STM_SETIMAGE,(WPARAM) IMAGE_BITMAP,(LPARAM)hOn[1]);
	  SendMessage(GetDlgItem(hDlg,IDC_COLOR2),STM_SETIMAGE,(WPARAM) IMAGE_BITMAP,(LPARAM)hOn[2]);
	  SendMessage(GetDlgItem(hDlg,IDC_COLOR3),STM_SETIMAGE,(WPARAM) IMAGE_BITMAP,(LPARAM)hOn[3]);
	  SendMessage(GetDlgItem(hDlg,IDC_COLOR4),STM_SETIMAGE,(WPARAM) IMAGE_BITMAP,(LPARAM)hOn[4]);
	  SendMessage(GetDlgItem(hDlg,IDC_COLOR5),STM_SETIMAGE,(WPARAM) IMAGE_BITMAP,(LPARAM)hOn[5]);
	  SendMessage(GetDlgItem(hDlg,IDC_COLOR6),STM_SETIMAGE,(WPARAM) IMAGE_BITMAP,(LPARAM)hOn[6]);
	  SendMessage(GetDlgItem(hDlg,IDC_COLOR7),STM_SETIMAGE,(WPARAM) IMAGE_BITMAP,(LPARAM)hOn[7]);
	  SetDlgItemInt(hDlg,IDC_NLEDS,nleds,FALSE);
	  SetDlgItemText(hDlg,IDC_LABEL,label?label:"");
	  SendMessage(GetDlgItem(hDlg,IDC_VERTICAL),BM_SETCHECK,
		  vertical?BST_CHECKED:BST_UNCHECKED,0);
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
	    nleds = GetDlgItemInt(hDlg,IDC_NLEDS,NULL,FALSE);
	    if (nleds>8) nleds=8;
	    if (nleds<1) nleds=1;
        vertical = (BST_CHECKED == SendMessage(GetDlgItem(hDlg,IDC_VERTICAL),
			                        BM_GETCHECK,0,0));
        GetDlgItemText(hDlg,IDC_LABEL,tmp_option,MAXTMPOPTION);
		set_option(&label,tmp_option);
		resize_window();
      }	  
	  if (HIWORD(wparam) == STN_CLICKED)
	  { if (LOWORD(wparam)== IDC_CBUTTON0) choose_color(hDlg, 0);
        else if (LOWORD(wparam)== IDC_CBUTTON1) choose_color(hDlg, 1);
        else if (LOWORD(wparam)== IDC_CBUTTON2) choose_color(hDlg, 2);
        else if (LOWORD(wparam)== IDC_CBUTTON3) choose_color(hDlg, 3);
        else if (LOWORD(wparam)== IDC_CBUTTON4) choose_color(hDlg, 4);
        else if (LOWORD(wparam)== IDC_CBUTTON5) choose_color(hDlg, 5);
        else if (LOWORD(wparam)== IDC_CBUTTON6) choose_color(hDlg, 6);
        else if (LOWORD(wparam)== IDC_CBUTTON7) choose_color(hDlg, 7);
	  }
      if (wparam == IDOK || wparam == IDCANCEL)
      { EndDialog(hDlg, TRUE);
        return TRUE;
      }
      break;
  }
  return FALSE;
}


LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{  switch (message) 
  { case WM_VMB_ON: /* Power On */
	return 0;
    case WM_VMB_OFF: /* Power Off */
	return 0;
    case WM_VMB_RESET: /* Reset */
		load_pictures();
	return 0;
    case WM_VMB_CONNECT: /* Connected */
	if (ModifyMenu(hMenu,ID_CONNECT, MF_BYCOMMAND|MF_STRING,ID_CONNECT,"Disconnect"))
	  DrawMenuBar(hMainWnd);
	  InvalidateRect(hWnd,NULL,FALSE);
 	return 0;
    case WM_VMB_DISCONNECT: /* Disconnected */
	if (ModifyMenu(hMenu,ID_CONNECT, MF_BYCOMMAND|MF_STRING,ID_CONNECT,"Connect..."))
	  DrawMenuBar(hMainWnd);
	  InvalidateRect(hWnd,NULL,FALSE);
	return 0;
  case WM_CREATE: 
	  return 0;
  case WM_PAINT:
    { PAINTSTRUCT ps;
      HDC hdc = BeginPaint (hWnd, &ps);
      paint_leds(hdc,led);
	  paint_label(hdc,label);
      EndPaint (hWnd, &ps);
    }
    return 0;
  }
  return (OptWndProc(hWnd, message, wParam, lParam));
}

HINSTANCE hInst;
HWND hMainWnd;
HBITMAP hBmp=NULL;
HMENU hMenu;
HBITMAP hon,hoff,hconnect;
device_info vmb = {0};




BOOL InitInstance(HINSTANCE hInstance)
{
  WNDCLASSEX wcex;
  int r;

#define MAX_LOADSTRING 100		
  static TCHAR szClassName[MAX_LOADSTRING];
  hInst = hInstance; 

  r = LoadString(hInstance, IDS_CLASS, szClassName, MAX_LOADSTRING);
  if (r==0)
  { r = GetLastError();
    vmb_debugi(VMB_DEBUG_FATAL,"Unable to load class name (%X)",r);
	vmb_fatal_error(__LINE__,"Unable to load class name");
  }
  ZeroMemory(&wcex, sizeof(wcex));
	wcex.cbSize = sizeof(WNDCLASSEX); 
	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= (WNDPROC)WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON));
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= NULL;
	wcex.lpszClassName = szClassName;
	wcex.hIconSm		= LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON));
/*	wcex.hIconSm		= LoadIcon(NULL, IDI_APPLICATION);
*/
	if (!RegisterClassEx(&wcex)) return FALSE;


    hMainWnd = CreateWindow(szClassName, title ,WS_POPUP,
                            xpos, ypos,0, 0,
	                        NULL, NULL, hInstance, NULL);

   return TRUE;
}


int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
	HACCEL hAccelTable;
    MSG msg;
    vmb_message_hook = win32_message;
	vmb_debug_hook = win32_log;
	vmb_error_init_hook = win32_error_init;

	hMenu = LoadMenu(hInstance,MAKEINTRESOURCE(IDR_MENU));
	ledOnB = (HBITMAP)LoadImage(hInstance, MAKEINTRESOURCE(IDB_ON_BLACK), 
		                            IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
	ledOffB= (HBITMAP)LoadImage(hInstance, MAKEINTRESOURCE(IDB_OFF_BLACK), 
		                            IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
	ledOnW= (HBITMAP)LoadImage(hInstance, MAKEINTRESOURCE(IDB_ON_WHITE), 
		                            IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
	ledOffW= (HBITMAP)LoadImage(hInstance, MAKEINTRESOURCE(IDB_OFF_WHITE), 
		                            IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
	ledDisconnected= (HBITMAP)LoadImage(hInstance, MAKEINTRESOURCE(IDB_DISCONNECT), 
		                            IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDR_ACCELERATOR));
    InitCommonControls();
	if (!InitInstance (hInstance)) return FALSE;
	init_layout(0);
	win32_param_init();
	if (nleds<1) nleds=1;
	if (nleds>8) nleds=8;
    init_device(&vmb);
	load_pictures();
	init_colors();
	init_metrics();
	SetWindowPos(hMainWnd,HWND_TOP,xpos,ypos,0,0,SWP_NOSIZE|SWP_NOZORDER|SWP_SHOWWINDOW);
	resize_window();
	if (minimized)CloseWindow(hMainWnd); 
	UpdateWindow(hMainWnd);
	vmb_begin();
 	vmb_connect(&vmb,host,port);
	vmb_register(&vmb,HI32(vmb_address),LO32(vmb_address),vmb_size,0,0,defined,major_version,minor_version);
    SendMessage(hMainWnd,WM_VMB_CONNECT,0,0); /* the connect button */
	if (vmb_verbose_flag) vmb_debug_mask=0; 
	CheckMenuItem(hMenu,ID_DEBUG,MF_BYCOMMAND|(vmb_debug_flag?MF_CHECKED:MF_UNCHECKED));
	CheckMenuItem(hMenu,ID_VERBOSE,MF_BYCOMMAND|(vmb_debug_mask==0?MF_CHECKED:MF_UNCHECKED));

	while (GetMessage(&msg, NULL, 0, 0)) 
	  if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) 
	  { TranslateMessage(&msg);
	    DispatchMessage(&msg);
	  }
	vmb_disconnect(&vmb);
	vmb_end();
	return (int)msg.wParam;
}

