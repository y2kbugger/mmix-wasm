/* Button Bar Code */
#include <windows.h>
#include <commctrl.h>
#include "error.h"
#include "buttonbar.h"


static HINSTANCE hButtonBarInst=NULL;
static const char  bb_class_name[] = "ButtonBarClass";

static HDC hButtonDC;
static HBITMAP hButtonBmp;
static HPEN	hButtonPen;

typedef struct button {
	struct button *next;
	HWND hWnd;
	int command;
	unsigned char group;
	unsigned char id;
	unsigned char active;
	unsigned char visible;
	HICON hImg;
	unsigned int Iwidth,Iheight;
} button;


static button *new_button(void)
{ button *b;
  b = (button*)malloc(sizeof(button));
  if (b==NULL) fatal_error("Out of Memory");
  return b;
}


typedef button *head;


static head *new_head(void)
{ head *h;
  h = (head*)malloc(sizeof(head));
  if (h==NULL) fatal_error("Out of Memory");
  *h=NULL;
  return h;
}


#define bb_list(hWnd) ((head *)(LONG_PTR)GetWindowLongPtr((hWnd),GWLP_USERDATA))

static int bb_id(button *b)
{ int max = 0;
  while (b!=NULL)
  { if (b->id > max) max = b->id;
    b=b->next;
  }
  return max;
}

static int bb_count(button *b)
/* count visible buttons */
{ int count = 0;
  while (b!=NULL)
  { if (b->visible) count++;
    b=b->next;
  }
  return count;
}



static button *add_button(button **list, HWND hButton, int command, unsigned char group, unsigned char id)
{ button *b;
  b=new_button();
  b->hWnd=hButton;
  b->command=command;
  b->active=1;
  b->visible=1;
  b->group=group;
  if (id == 0) id=bb_id(*list)+1;
  b->id = id;
  /* add to list. keep list sorted by group and id */

  while (*list!=NULL)
  { if ((*list)->group > group || 
       ((*list)->group == group && (*list)->id > id))
	   break;
    list = &((*list)->next);
  }
  b->next=*list;
  *list = b;
  return b;
}

static void remove_button(button **list, HWND hButton)
{ 
  while (*list!=NULL)
  { if ((*list)->hWnd==hButton)
    { button *tail = (*list)->next;
      free(*list);
	  *list=tail;
	  return ;
    }
    list = &((*list)->next);
  }
}


static void bb_setsize(HWND hWnd, int x, int y, int w, int h)
/* call if outer dimensions of buttonbar window have changed */
{ button *list=*bb_list(hWnd);
  int bx, by;
  int scale=BB_HEIGHT;
  int count = bb_count(list);
  while (count > (w/scale)*(h/scale) && scale > 1)
	  scale--;

  bx = x;
  by = y;
  while (list!=NULL)
  { if (list->visible) {
	  MoveWindow(list->hWnd,bx,by,scale,scale,TRUE);
      bx=bx+scale;
	  if (bx+scale>w) 
	  { bx = x; by=by+scale;
	  }

    }
	list=list->next;
  }
}

static void bb_resize(HWND hButtonBar)
{ RECT rect;
  GetClientRect(hButtonBar,&rect);
  bb_setsize(hButtonBar, rect.left,rect.top,rect.right-rect.left,rect.bottom-rect.top);
}

static void bb_set_button(button *b, int active, int visible)
{ if (active==0 || active==1) b->active=active;
  if (visible==0 || visible==1)  b->visible=visible;
  ShowWindow(b->hWnd,visible?SW_SHOW:SW_HIDE);
  EnableWindow(b->hWnd,active);
}

void bb_set_group(HWND hButtonBar,unsigned char group, int active, int visible)
{ int x = 0;
  int y = 0;
  button *b=*bb_list(hButtonBar);
  while (b!=NULL)
  { if (b->group==group)
       bb_set_button(b,active,visible);
	b=b->next;
  }
  bb_resize(hButtonBar);
}

void bb_set_id(HWND hButtonBar,unsigned char id, int active, int visible)
{ int x = 0;
  int y = 0;
  button *b=*bb_list(hButtonBar);
  while (b!=NULL)
  { if (b->id==id)
       bb_set_button(b,active,visible);
	b=b->next;
  }
  bb_resize(hButtonBar);
}


static int bb_get_command(button *b,HWND hWnd)
{
  while (b!=NULL)
  { if (b->hWnd==hWnd) return b->command;
	b=b->next;
  }
  return 0;
}
static button *bb_get_button(button *b,HWND hWnd)
{
  while (b!=NULL)
  { if (b->hWnd==hWnd) return b;
	b=b->next;
  }
  return NULL;
}


HWND hTooltips=NULL;

static LRESULT CALLBACK ButtonBarProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{	button *bb_head;
    bb_head = (button *)(LONG_PTR)GetWindowLongPtr(hWnd,GWLP_USERDATA);
	switch (message)
	{
	case WM_CREATE:
		SetWindowLongPtr(hWnd,GWLP_USERDATA,(LONG)(LONG_PTR)new_head()); /* empty list */
        hTooltips=CreateWindowEx(WS_EX_TOPMOST,TOOLTIPS_CLASS,NULL,WS_POPUP|TTS_NOPREFIX|TTS_ALWAYSTIP,
			CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
			hWnd,NULL,hButtonBarInst,NULL);
         hButtonDC = CreateCompatibleDC(GetDC(hWnd)); 
		 hButtonBmp= CreateCompatibleBitmap(GetDC(hWnd),32,32);
		 SelectObject(hButtonDC, hButtonBmp);
		 hButtonPen=CreatePen(PS_SOLID,1,GetSysColor(COLOR_BTNSHADOW));
		 SelectObject(hButtonDC, hButtonPen);
		return 0;
	case WM_SIZE:
		if (wParam==SIZE_RESTORED)
		  bb_setsize(hWnd,0,0,LOWORD(lParam),HIWORD(lParam));
		return 0;
	case WM_PARENTNOTIFY:
        if (LOWORD(wParam)==WM_DESTROY)
		{ remove_button(bb_list(hWnd),(HWND)lParam);
		  bb_resize(hWnd);
		  return 0;
		}
		else if (LOWORD(wParam)==WM_CREATE)
		  return 0;
		else 
		  return SendMessage(GetParent(hWnd),message,wParam,lParam);
	case WM_COMMAND:
		return SendMessage(GetParent(hWnd),WM_COMMAND,bb_get_command(*bb_list(hWnd),(HWND)lParam),0);
	case WM_CLOSE:
		DestroyWindow(hWnd);
		return 0;
	case WM_DESTROY:
		{ head *h = bb_list(hWnd);
		  while (*h!=NULL)
	        DestroyWindow((*h)->hWnd);
		  DestroyWindow(hTooltips);
		  DeleteDC(hButtonDC);
		  DeleteObject(hButtonBmp);
		  DeleteObject(hButtonPen);
		}
		break;
    case WM_DRAWITEM: 
		{ 
		  button *btn;
          LPDRAWITEMSTRUCT lpdis;
		  HBRUSH hBrush;

          lpdis = (LPDRAWITEMSTRUCT) lParam; 
          btn=bb_get_button(*bb_list(hWnd),lpdis->hwndItem);
          if (lpdis->itemState & ODS_SELECTED)  // if selected 
            hBrush=GetSysColorBrush(COLOR_BTNSHADOW);
          else 
            hBrush=GetSysColorBrush(COLOR_BTNFACE);
	      SelectObject(hButtonDC,hBrush);
		  Rectangle(hButtonDC,0,0,32,32);
     	  DrawIconEx(hButtonDC,(32-btn->Iwidth)/2,(32-btn->Iheight)/2,btn->hImg,btn->Iwidth,btn->Iheight,0,NULL,DI_NORMAL);
          StretchBlt( 
                lpdis->hDC,         // destination DC 
                lpdis->rcItem.left, // x upper left 
                lpdis->rcItem.top,  // y upper left 
                // The next two lines specify the width and 
                // height. 
                lpdis->rcItem.right - lpdis->rcItem.left, 
                lpdis->rcItem.bottom - lpdis->rcItem.top, 
                hButtonDC,    // source device context 
                0, 0,      // x and y upper left 
                32,        // source bitmap width 
                32,        // source bitmap height 
                SRCCOPY);  // raster operation 
		}
        return TRUE; 
	default:
		break;
	}
	return DefWindowProc(hWnd, message, wParam, lParam);
}

static ATOM RegisterButtonBarClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= ButtonBarProc;
	wcex.cbClsExtra		= sizeof(LONG_PTR);
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= NULL;
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW); 
	wcex.hbrBackground	= (HBRUSH)(COLOR_MENUBAR + 1 );
	wcex.lpszMenuName	= NULL;
	wcex.lpszClassName	= bb_class_name;
	wcex.hIconSm		= NULL;
	return RegisterClassEx(&wcex);
}

HWND bb_CreateButtonBar(LPCTSTR lpWindowName, DWORD dwStyle, int x, int y, int nWidth, int nHeight,
    HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam)

{ HWND hWnd;
   hWnd = CreateWindow(bb_class_name, lpWindowName, dwStyle,x,y,nWidth, nHeight,
					hWndParent, hMenu, hInstance, lpParam);

   return hWnd;
}

HWND bb_CreateButton(HWND hButtonBar, HICON hImg, int command, unsigned char group, unsigned char id, unsigned char active, unsigned char visible, char *tip)
{ HWND hB;
  button *b;
  head *h;
  TOOLINFO ti;
  ICONINFO Iinfo;

  hB =CreateWindowEx(0,"BUTTON","B1",WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON|BS_OWNERDRAW /*BS_ICON*/ ,
	                  0,0,BB_HEIGHT,BB_HEIGHT,hButtonBar,NULL,hButtonBarInst,MAKELPARAM(0,0));
  //SendMessage(hB,BM_SETIMAGE,IMAGE_ICON,(LPARAM)hImg);
  
  ti.cbSize=sizeof(TOOLINFO);
  ti.uFlags=TTF_SUBCLASS|TTF_IDISHWND;
  ti.hwnd=hButtonBar;
  ti.hinst=hButtonBarInst;
  ti.uId=(UINT_PTR)hB;
  ti.lpszText=tip;
  ti.rect.top=0;
  ti.rect.left=0;
  ti.rect.bottom=0;
  ti.rect.right=0;
  ti.lParam=0;
 
  SendMessage(hTooltips,TTM_ADDTOOL,0,(LPARAM)&ti);
  h = bb_list(hButtonBar);
  b= add_button(h,hB,command, group,id);
  b->hImg=hImg;
  GetIconInfo(hImg, &Iinfo);
  if(Iinfo.hbmColor)
    { BITMAP bmp;
      GetObject(Iinfo.hbmColor, sizeof(bmp), &bmp);
      b->Iwidth = bmp.bmWidth;
      b->Iheight = bmp.bmHeight;
    }
  else if(Iinfo.hbmMask)
    { BITMAP bmp;
      GetObject(Iinfo.hbmMask, sizeof(bmp), &bmp);
      b->Iwidth = bmp.bmWidth;
      b->Iheight = bmp.bmHeight/2;
    }
  bb_set_button(b,active,visible);
  bb_resize(hButtonBar);
  return hB;
}



void bb_init(HINSTANCE hInst)
{ hButtonBarInst = hInst;
  RegisterButtonBarClass(hInst);
}

