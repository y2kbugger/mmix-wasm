#include <winsock2.h>
#include <windows.h>
#include <afxres.h>
#include <time.h>
#include "vmb.h"
#include "bus-arith.h"
#include "resource.h"
#include "winopt.h"
#include "opt.h"
#include "param.h"
#include "option.h"
#include "inspect.h"
#include "timer.h"
#pragma warning(disable : 4996)

/* variables to operate the timer */
static uint64_t T0;   /* global base time T0, filetime at midnight of system start */
static SYSTEMTIME now; /* current host time information */


void timer_set_T0(void)
{ SYSTEMTIME midnight;
  FILETIME fnow;
  ULARGE_INTEGER ulmidnight;
  GetLocalTime(&midnight);
  midnight.wHour=midnight.wMinute=midnight.wSecond=midnight.wMilliseconds=0;
  SystemTimeToFileTime(&midnight, &fnow); 
  ulmidnight.HighPart = fnow.dwHighDateTime;
  ulmidnight.LowPart = fnow.dwLowDateTime;
  T0 = ulmidnight.QuadPart;
}

unsigned int timer_since_T0(void)
/* set now and return time in ms since T0 
   we make shure that the current host time and the time stored in now
   never differ by more than 1 hour, by calling this function once every hour.
*/
{   FILETIME fnow;
    ULARGE_INTEGER ul;
    GetLocalTime(&now);
    SystemTimeToFileTime(&now, &fnow);  
    ul.HighPart = fnow.dwHighDateTime;
    ul.LowPart = fnow.dwLowDateTime;
    return ((unsigned int)((ul.QuadPart-T0)/10000))%(24*60*60*1000);
}

void timer_get_DateTime(void) 
/* set the first two octas of the timer from host time */
{   unsigned int ms;
    ms = timer_get_now();
	SETYEAR(now.wYear);
	MONTH = (unsigned char)(now.wMonth-1);
	DAY = (unsigned char)now.wDay;
	WEEKDAY = (unsigned char)now.wDayOfWeek;
	HOUR = (unsigned char)now.wHour;
	MIN = (unsigned char)now.wMinute;
	SEC = (unsigned char)now.wSecond;
	SETMILLISEC(ms);
}


void timer_stop(void)
/* cancel a running timer */
{ SendMessage(hMainWnd,WM_VMB_OTHER+1,0,0); /* Stop the timer */  
}

void timer_set(unsigned int delay)
{ SendMessage(hMainWnd,WM_VMB_OTHER+2,0,(LPARAM)delay); /* Start the timer */
}

void timer_terminate(void)
{
  PostMessage(hMainWnd,WM_CLOSE,0,0);
}


void update_display(void);

INT_PTR CALLBACK   
SettingsDialogProc( HWND hDlg, UINT message, WPARAM wparam, LPARAM lparam )
{
  switch ( message )
  { case WM_INITDIALOG:
      uint64_to_hex(vmb_address,tmp_option);
      SetDlgItemText(hDlg,IDC_ADDRESS,tmp_option);
	  SetDlgItemInt(hDlg,IDC_INTERRUPT,interrupt_no,FALSE);
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
        inspector[1].address=vmb_address;
		interrupt_no = GetDlgItemInt(hDlg,IDC_INTERRUPT,NULL,FALSE);
      }
      if (wparam == IDOK || wparam == IDCANCEL)
      { EndDialog(hDlg, TRUE);
        return TRUE;
      }
      break;
  }
  return FALSE;
}


static HFONT hTimeFont;
static HBITMAP hblink,hold;

static HWND hTime;
void update_display(void)
{ static char timestr[20] = {0};
  static unsigned int last_dt=0;
  if (last_dt!=dt)
	  { sprintf(timestr,"%6.3f",dt/1000.0);
	    SetWindowText(hTime,timestr);
		last_dt=dt;
	  }
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{ static HDC BitmapDC ;
  static HBITMAP hold;
  switch (message) 
  {  
   case WM_VMB_OFF: /* Power Off */
	if (tt!=0) 
	{ KillTimer(hMainWnd,1); 
	}
	break;
  case WM_VMB_DISCONNECT: /* Disconnected */
	if (tt!=0) 
	{ KillTimer(hMainWnd,1);  
	}
	break;
  case WM_VMB_OTHER+1: /* Stop Timer */
    KillTimer(hMainWnd,1); 
	update_display();
    return 0;
  case WM_VMB_OTHER+2: /* Start Timer */
	SetTimer(hMainWnd,1,(UINT)lParam,NULL);
	return 0;
  case WM_TIMER: /* Timer expired */
	  if (wParam == 1) /* the Real Timer */
	  { KillTimer(hMainWnd,1); 
	    timer_signal();
	    if (ti==0)
	      vmb_debugi(VMB_DEBUG_INFO,"Timer signaled at %u",timer_get_now());
#define BLINKTIME 200
	    if (ti>2*BLINKTIME || ti==0)
	    {	    
	      hold = (HBITMAP)SendMessage(hpower,STM_SETIMAGE,(WPARAM) IMAGE_BITMAP,(LPARAM)hblink);
	      SetTimer(hMainWnd,2,BLINKTIME,NULL);	 
	    }
	  } 
	  else if (wParam == 2) /* the Blink Timer */
	  {  SendMessage(hpower,STM_SETIMAGE,(WPARAM) IMAGE_BITMAP,(LPARAM)hold);
	     KillTimer(hMainWnd,2); 
	  }
	  else if (wParam == 3) /* the Hour Timer */
            timer_get_now();
	  return 0;
  case WM_CREATE: 
	hpower = CreateWindow("STATIC",NULL,WS_CHILD|WS_VISIBLE|SS_BITMAP|SS_REALSIZEIMAGE,10,20,32,32,hWnd,NULL,hInst,0);
    SendMessage(hpower,STM_SETIMAGE,(WPARAM) IMAGE_BITMAP,(LPARAM)hoff);

	hTime = CreateWindow( 
                "STATIC",     // predefined class 
                NULL,       // no window title 
                WS_CHILD | WS_VISIBLE | SS_RIGHT,
                70, 15, 200, 40, 
                hWnd,       // parent window 
                (HMENU)2,
                hInst, 
                NULL);                // pointer not needed 
    hTimeFont = CreateFont(40,0,0,0,0,FALSE,FALSE,FALSE,ANSI_CHARSET,
                    OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,FIXED_PITCH|FF_MODERN,"Arial");
    SendMessage(hTime, WM_SETFONT, (WPARAM)hTimeFont, 0); 
    dt=0; 
	SetWindowText(hTime,"0.000");
    hblink = (HBITMAP)LoadImage(hInst, MAKEINTRESOURCE(IDB_BLINK), 
				IMAGE_BITMAP, 32, 32, LR_CREATEDIBSECTION);

	{ int hour = 60*60*1000;
	  if (hour > USER_TIMER_MAXIMUM)
		  hour = USER_TIMER_MAXIMUM;
	  SetTimer(hMainWnd,3,hour,NULL); /* every hour */
	}
	return 0;
  case WM_DESTROY:
	KillTimer(hMainWnd,1);
	KillTimer(hMainWnd,2);
	KillTimer(hMainWnd,3);
	DeleteObject(hTimeFont);
    break;
  case WM_CTLCOLORSTATIC:
	 { HDC hDC = (HDC) wParam;   // handle to display context 
 	   SetBkColor(hDC,0);
	   SetTextColor(hDC,RGB(35,135,0));
	 }
	 return (LRESULT)GetStockObject(BLACK_BRUSH);
  }
 return (OptWndProc(hWnd, message, wParam, lParam));
}



