#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#include "winmain.h"
#include "info.h"
#include "editor.h"
#include "print.h"

#pragma warning(disable : 4996)

#define STATIC_BUILD
#include "../scintilla/include/scintilla.h"
extern sptr_t ed_send(unsigned int msg,uptr_t wparam,sptr_t lparam);
static RECT user_margins={2000,2000,2000,2000};
static POINT papersize={21000,29700};
static HGLOBAL hMode=NULL, hNames=NULL;


void printer_init(void)
{ PAGESETUPDLG psd;    
  ZeroMemory(&psd, sizeof(psd));
  psd.lStructSize = sizeof(psd);
  psd.Flags=PSD_RETURNDEFAULT|PSD_NOWARNING;
  if (PageSetupDlg(&psd)==TRUE) {
	  hMode=psd.hDevMode;
	  hNames=psd.hDevNames;
  }
  else
	  hMode=hNames=NULL;
}

int page_setup(void)
{ PAGESETUPDLG psd;    

ZeroMemory(&psd, sizeof(psd));
psd.lStructSize = sizeof(psd);
psd.hwndOwner   = hMainWnd;
psd.hDevMode    = hMode; 
psd.hDevNames   = hNames; 
psd.Flags       = PSD_INHUNDREDTHSOFMILLIMETERS
 | PSD_MARGINS;
psd.rtMargin.top = user_margins.top;
psd.rtMargin.left = user_margins.left;
psd.rtMargin.right = user_margins.right;
psd.rtMargin.bottom = user_margins.bottom;
psd.ptPaperSize.x = papersize.x;
psd.ptPaperSize.y = papersize.y;

psd.lpfnPagePaintHook = NULL;

if (PageSetupDlg(&psd)==TRUE) {
	user_margins.top=psd.rtMargin.top;
    user_margins.left=psd.rtMargin.left;
    user_margins.right = psd.rtMargin.right;
    user_margins.bottom = psd.rtMargin.bottom;
	papersize.x=psd.ptPaperSize.x;
	papersize.y=psd.ptPaperSize.y;
    hMode=psd.hDevMode;
	hNames=psd.hDevNames;
    // check paper size and margin values here
}
return 1;
}


int print(void)
{ PRINTDLG pd;
  // Initialize PRINTDLG
  ZeroMemory(&pd, sizeof(pd));
  pd.lStructSize = sizeof(pd);
  pd.hwndOwner   = hMainWnd;
  pd.hDevMode    = hMode;     // Don't forget to free or store hDevMode
  pd.hDevNames   = hNames;     // Don't forget to free or store hDevNames
  pd.Flags       = PD_USEDEVMODECOPIESANDCOLLATE | PD_RETURNDC; 
  pd.nCopies     = 1;
  pd.nFromPage   = 1; 
  pd.nToPage     = 0xFFFF; 
  pd.nMinPage    = 1; 
  pd.nMaxPage    = 0xFFFF; 

  if (PrintDlg(&pd)!=TRUE) return 0;
  else
  {	POINT size,resolution;
    RECT printable,margin;
    DOCINFO di;
	struct Sci_RangeToFormat srf;
	int print_from,print_to;
	int page;
	char *name;
    TEXTMETRIC tm;
	int headheight;
#define MAX_NAME 128
	char pagestr[20];
	/* get information about the page */
	hMode=  pd.hDevMode;     
    hNames = pd.hDevNames ;   
	resolution.x = GetDeviceCaps(pd.hDC, LOGPIXELSX);    // dpi in X direction
	resolution.y = GetDeviceCaps(pd.hDC, LOGPIXELSY);    // dpi in Y direction
	size.x = GetDeviceCaps(pd.hDC, PHYSICALWIDTH);   // device units
	size.y = GetDeviceCaps(pd.hDC, PHYSICALHEIGHT);  // device units
	printable.left = GetDeviceCaps(pd.hDC, PHYSICALOFFSETX);
	printable.top = GetDeviceCaps(pd.hDC, PHYSICALOFFSETY);
    printable.right=size.x- GetDeviceCaps(pd.hDC, HORZRES)- printable.left;
    printable.bottom=size.y- GetDeviceCaps(pd.hDC, VERTRES)- printable.top;
    margin.left=MulDiv(user_margins.left,resolution.x,2540);
    margin.right=MulDiv(user_margins.right,resolution.x,2540);
    margin.top=MulDiv(user_margins.top,resolution.y,2540);
    margin.bottom=MulDiv(user_margins.bottom,resolution.y,2540);
	DPtoLP(pd.hDC, (LPPOINT)&margin, 2);
	DPtoLP(pd.hDC, (LPPOINT)&printable, 2);
	DPtoLP(pd.hDC, (LPPOINT)&size, 1);

	GetTextMetrics(pd.hDC,&tm);
	headheight=tm.tmHeight + tm.tmExternalLeading;;

	margin.left=max(margin.left,printable.left);
	margin.right=max(margin.right,printable.right);
	margin.top=max(margin.top,printable.top);
	margin.top=max(margin.top,2*headheight);
	margin.bottom=max(margin.bottom,printable.bottom);
	margin.bottom=max(margin.bottom,2*headheight);

	name = file2shortname(edit_file_no); 

    memset( &di, 0, sizeof(DOCINFO) );
    di.cbSize = sizeof(DOCINFO); 
    di.lpszDocName = file2fullname(edit_file_no); 
    di.lpszOutput = (LPTSTR) NULL; 
    di.lpszDatatype = (LPTSTR) NULL; 
    di.fwType = 0; 
 


	if (pd.Flags&PD_SELECTION)
	{ print_from = (int)ed_send(SCI_GETSELECTIONSTART,0,0);
	  print_to = (int)ed_send(SCI_GETSELECTIONEND,0,0);
	}
	else
	{ print_from=0;
      print_to=(int)ed_send(SCI_GETLENGTH,0,0);
	}

	ed_send(SCI_SETPRINTCOLOURMODE,SC_PRINT_BLACKONWHITE,0);
	ed_send(SCI_SETPRINTWRAPMODE,SC_WRAP_NONE,0);
 	ed_send(SCI_SETPRINTMAGNIFICATION,-3,0);

	srf.chrg.cpMin=0;
	srf.chrg.cpMax=100;
    srf.hdc=srf.hdcTarget=pd.hDC;
	srf.rcPage.top=0;
	srf.rcPage.left=0;
	srf.rcPage.bottom = size.y-printable.top-printable.bottom-1;
	srf.rcPage.right = size.x-printable.left-printable.right-1;
    srf.rc.left=margin.left-printable.left;
	srf.rc.top=margin.top-printable.top;
	srf.rc.right=size.x-margin.right-printable.left;
	srf.rc.bottom=size.y-margin.bottom-printable.top;


    if (StartDoc(pd.hDC, &di) == SP_ERROR)  goto Error; 
    page=1;
 	while(print_from<print_to && page<=pd.nToPage)
	{ int printing = !(pd.Flags&PD_PAGENUMS) || (page>=pd.nFromPage);

	  if (printing)
	  { SetTextAlign(pd.hDC,TA_LEFT|TA_BOTTOM);
	    if (name!=NULL)
        TextOut(pd.hDC,srf.rc.left,srf.rc.top-headheight,name,(int)strlen(name));

	    { HPEN hPen = GetStockObject(BLACK_PEN);
	      HPEN hPenOld = SelectObject(pd.hDC, hPen); 
          MoveToEx(pd.hDC, srf.rc.left, srf.rc.top-headheight/2, NULL); 
          LineTo(pd.hDC, srf.rc.right, srf.rc.top-headheight/2); 
          SelectObject(pd.hDC, hPenOld); 
	    }
        if (StartPage(pd.hDC) <= 0) goto Error; 
	  }

	  srf.chrg.cpMin=print_from;
	  srf.chrg.cpMax=print_to;
	  print_from=(int)ed_send(SCI_FORMATRANGE,printing,(sptr_t)&srf);

      if (printing)
	  { { HPEN hPen = GetStockObject(BLACK_PEN);
	      HPEN hPenOld = SelectObject(pd.hDC, hPen); 
          MoveToEx(pd.hDC, srf.rc.left, srf.rc.bottom+headheight/2, NULL); 
          LineTo(pd.hDC, srf.rc.right, srf.rc.bottom+headheight/2); 
          SelectObject(pd.hDC, hPenOld); 
	     }

	    sprintf(pagestr,"%d",page);
	    SetTextAlign(pd.hDC,TA_RIGHT|TA_TOP);
        TextOut(pd.hDC,srf.rc.right,srf.rc.bottom+headheight,pagestr,(int)strlen(pagestr));

        if (EndPage(pd.hDC) <= 0)  goto Error; 
	  }
	  page++;
	}
	EndDoc(pd.hDC); 
Error:  
    EnableWindow(hMainWnd, TRUE); 
    DeleteDC(pd.hDC);
  return 1;
  }
}


#if 0
    GetTextExtentPoint32(pd.hDC,fullname, (int)strlen(fullname),&szMetric); 
 
    // Compute the starting point for the text-output operation. The 
    // string will be centered horizontally and positioned three lines 
    // down from the top of the page. 
    cWidthPels = GetDeviceCaps(pd.hDC, HORZRES); 

    xLeft = ((cWidthPels / 2) - (szMetric.cx / 2)); 
    yTop = (szMetric.cy * 3); 
 
    // Print the path and filename for the bitmap, centered at the top 
    // of the page. 
 
    TextOut(pd.hDC, xLeft, yTop, fullname, (int)strlen(fullname)); 
 






	

    // Determine whether the user has pressed the Cancel button in the 
    // AbortPrintJob dialog box; if the button has been pressed, call 
    // the AbortDoc function. Otherwise, inform the spooler that the 
    // page is complete. 
 

 
    // Inform the driver that document has ended. 
 
    nError = 
    //if (nError <= 0) 
      //  errhandler("EndDoc", hwnd); 
#endif

