#include <windows.h>
#include "resource.h"
#include "winde.h"
#include "dedit.h"
#include "inspect.h"
#include "winopt.h"

/* DataEditor */
typedef struct dataedit
{  HWND hWnd;   /* the handle to the data edit dialog window */
   inspector_def *insp; /* the inspector connected to */
   char * reg_name; /* register name, if NULL this is memory */
   int reg_offset; /* offset to devices base address */
   uint64_t address; /* use instead of name and offset if name==NULL */
   unsigned char mem[8]; /* up to one octa */
   int size; /* 1,2,4, or 8  number of byte to edit */
   enum mem_fmt format; /* current format */
   enum chunk_fmt chunk; /* current chunk */
} dataedit;

static dataedit *new_dataedit(void)
{ dataedit *de;
  de = (dataedit*)malloc(sizeof(dataedit));
  if (de == NULL)
  { win32_error(__LINE__,"Out of Memory for data editor");
    return NULL;
  }
  memset(de,0,sizeof(dataedit));
  return de;
}

static void show_item(dataedit *de,int IDC,int offset, int size)
{ char str[22]; 
  if (offset>=de->size) return;
  chunk_to_str(str, de->mem+offset,de->format,size,0);
  SetDlgItemText(de->hWnd,IDC,str);
  SendMessage(GetDlgItem(de->hWnd,IDC),EM_SETMODIFY,0,0); 
}

static void show_edit_mem(dataedit *de)
{ if (de->chunk==byte_chunk)
  {	show_item(de,IDC_EDITBYTE0,0,1);
    show_item(de,IDC_EDITBYTE1,1,1);
    show_item(de,IDC_EDITBYTE2,2,1);
    show_item(de,IDC_EDITBYTE3,3,1);
    show_item(de,IDC_EDITBYTE4,4,1);
    show_item(de,IDC_EDITBYTE5,5,1);
    show_item(de,IDC_EDITBYTE6,6,1);
    show_item(de,IDC_EDITBYTE7,7,1);
  } else if (de->chunk==wyde_chunk)
  {	show_item(de,IDC_EDITBYTE0,0,2);
    show_item(de,IDC_EDITBYTE1,2,2);
    show_item(de,IDC_EDITBYTE2,4,2);
    show_item(de,IDC_EDITBYTE3,6,2);
 } else if (de->chunk==tetra_chunk)
  {	show_item(de,IDC_EDITBYTE0,0,4);
    show_item(de,IDC_EDITBYTE1,4,4);
  } else if (de->chunk==octa_chunk)
    show_item(de,IDC_EDITBYTE0,0,8);
}
static uint64_t str_to_u64(char *str)
{ uint64_t u=0;
  while (isspace(*str)) str++;
  while (*str!=0 && isdigit(*str))
  { u=u*10+((*str)-'0');
    str++;
  }
  return u;
}

static void str_to_chunk(char *str, unsigned char *buf, enum mem_fmt fmt, int chunk_size)
/* reads the data from str using format and chunk size.
   puts results into buffer.
*/
{ uint64_t u;
         
  if (fmt==float_format)
  { u=f64_from_str(str);
    if (chunk_size<8)
	  u=f32_from_f64(u);
  }
  else if (fmt==signed_format)
	u= (uint64_t)_atoi64(str);
  else if (fmt==unsigned_format)
    u= str_to_u64(str);
  else if (fmt==hex_format)
	u= hex_to_uint64(str);
  else if (fmt==ascii_format)
    u=(*str)&0xFF;

  while (chunk_size>0)
  { chunk_size--;
	*buf = (unsigned char)((u>>(8*chunk_size))&0xFF);
	buf++;
  }
}


static void put_item(dataedit *de, int IDC, int offset, int size)
{ char str[22]; 
  if (de->size>offset && SendMessage(GetDlgItem(de->hWnd,IDC),EM_GETMODIFY,0,0)) 
  { GetDlgItemText(de->hWnd,IDC,str,22);
	str_to_chunk(str, de->mem+offset,de->format,size);
  }
}

static void put_edit_mem(dataedit *de)
{ if (de->chunk==byte_chunk)
  {	put_item(de,IDC_EDITBYTE0,0,1);
	put_item(de,IDC_EDITBYTE1,1,1);
	put_item(de,IDC_EDITBYTE2,2,1);
	put_item(de,IDC_EDITBYTE3,3,1);
	put_item(de,IDC_EDITBYTE4,4,1);
	put_item(de,IDC_EDITBYTE5,5,1);
	put_item(de,IDC_EDITBYTE6,6,1);
	put_item(de,IDC_EDITBYTE7,7,1);
  }
  else if (de->chunk==wyde_chunk)
  {	put_item(de,IDC_EDITBYTE0,0,2);
	put_item(de,IDC_EDITBYTE1,2,2);
	put_item(de,IDC_EDITBYTE2,4,2);
	put_item(de,IDC_EDITBYTE3,6,2);
  }
  else if (de->chunk==tetra_chunk)
  {	put_item(de,IDC_EDITBYTE0,0,4);
	put_item(de,IDC_EDITBYTE1,4,4);
  }
  else if (de->chunk==octa_chunk)
	put_item(de,IDC_EDITBYTE0,0,8);
}

static int left_width; /* left position where the edit windowd start */
static int top_height; /* top position where the edit windowd start */
static int min_w, min_h;

static void show_edit_windows(dataedit *de)
{ HWND h = de->hWnd;
  enum chunk_fmt chunk=de->chunk;
  int size=de->size;
  int chunk_size=1<<de->chunk;
  int x;
  int chunk_w= chunk_len(de->format,chunk_size)*fixed_char_width+4*border_size;
  x = left_width;
  min_w=x;
  SetWindowPos(GetDlgItem(h,IDC_EDITBYTE0),HWND_TOP,x,top_height,
	  chunk_w,fixed_line_height,size>0?SWP_SHOWWINDOW:SWP_HIDEWINDOW);
  x+=chunk_w+separator_width;
  if (size>0) min_w=x;
  size -=chunk_size;
  SetWindowPos(GetDlgItem(h,IDC_EDITBYTE1),HWND_TOP,x,top_height,
	  chunk_w,fixed_line_height,size>0?SWP_SHOWWINDOW:SWP_HIDEWINDOW);
  x+=chunk_w+separator_width;
  if (size>0) min_w=x;
  size -=chunk_size;
  SetWindowPos(GetDlgItem(h,IDC_EDITBYTE2),HWND_TOP,x,top_height,
	  chunk_w,fixed_line_height,size>0?SWP_SHOWWINDOW:SWP_HIDEWINDOW);
  x+=chunk_w+separator_width;
  if (size>0) min_w=x;
  size -=chunk_size;
  SetWindowPos(GetDlgItem(h,IDC_EDITBYTE3),HWND_TOP,x,top_height,
	  chunk_w,fixed_line_height,size>0?SWP_SHOWWINDOW:SWP_HIDEWINDOW);
  x+=chunk_w+separator_width;
  if (size>0) min_w=x;
  size -=chunk_size;
  SetWindowPos(GetDlgItem(h,IDC_EDITBYTE4),HWND_TOP,x,top_height,
	  chunk_w,fixed_line_height,size>0?SWP_SHOWWINDOW:SWP_HIDEWINDOW);
  x+=chunk_w+separator_width;
  if (size>0) min_w=x;
  size -=chunk_size;
  SetWindowPos(GetDlgItem(h,IDC_EDITBYTE5),HWND_TOP,x,top_height,
	  chunk_w,fixed_line_height,size>0?SWP_SHOWWINDOW:SWP_HIDEWINDOW);
  x+=chunk_w+separator_width;
  if (size>0) min_w=x;
  size -=chunk_size;
  SetWindowPos(GetDlgItem(h,IDC_EDITBYTE6),HWND_TOP,x,top_height,
	  chunk_w,fixed_line_height,size>0?SWP_SHOWWINDOW:SWP_HIDEWINDOW);
  x+=chunk_w+separator_width;
  if (size>0) min_w=x;
  size -=chunk_size;
  SetWindowPos(GetDlgItem(h,IDC_EDITBYTE7),HWND_TOP,x,top_height,
	  chunk_w,fixed_line_height,size>0?SWP_SHOWWINDOW:SWP_HIDEWINDOW);
  x+=chunk_w+separator_width;
  if (size>0) min_w=x;
  size -=chunk_size;
  show_edit_mem(de);
}

static void set_edit_chunk(dataedit *de, enum chunk_fmt chunk);
static void set_edit_format(dataedit *de, enum mem_fmt format)
{ enum chunk_fmt cf;
  cf=de->chunk;
  put_edit_mem(de);
  if (format==ascii_format) 
    cf=byte_chunk;
  else if (format==float_format && de->size<4) format=de->format;
  else if (format==float_format && de->chunk<tetra_chunk) cf=tetra_chunk; 
  if (cf!=de->chunk)
  { de->chunk=cf; set_chunk(de->hWnd, de->chunk);}
  de->format=format;
  set_format(de->hWnd,de->format);
}

static void set_edit_chunk(dataedit *de, enum chunk_fmt chunk)
{ enum mem_fmt mf;
  put_edit_mem(de);
  mf=de->format;
  if ((1<<chunk) > de->size) chunk=de->chunk;
  else if (de->format==ascii_format && chunk!=byte_chunk) mf=hex_format;
  else if (de->format==float_format && chunk<tetra_chunk) mf=hex_format;
  if (mf!=de->format)
  {clear_format(de->hWnd,de->format); de->format=mf; set_format(de->hWnd,de->format);}
  de->chunk=chunk;
  set_chunk(de->hWnd, de->chunk);
}

	
static void show_edit_name(dataedit *de)
{ int name_w;
  char *str;
	if (de->insp==NULL)
  {	str= "Disconnected";
    name_w=12;
  }
  else if (de->reg_name!=NULL)
  { str=de->reg_name;
    name_w=(int)strlen(de->reg_name);
  }
  else
  { static char value[22]; /* big enough for the largest 8Byte integer */
    uint64_to_hex(de->address,value);
	str=value;
	name_w=2*8+2;
  }
   SetWindowPos(GetDlgItem(de->hWnd,IDC_NAME),HWND_TOP,separator_width,top_height,
			name_w*fixed_char_width,fixed_line_height,SWP_SHOWWINDOW); 
   SetDlgItemText(de->hWnd,IDC_NAME,str);
   left_width=name_w*fixed_char_width+2*separator_width;
   min_h=top_height+fixed_line_height+separator_height;
}

/* the API for the data editor */

void de_update(HWND hDlg)
/* call after changes to the inspector or to the data */
{ 
  dataedit *de = (dataedit*)(LONG_PTR)GetWindowLongPtr(hDlg,DWLP_USER);
  if (de->insp==NULL || de->insp->de_size==0)
  { de->reg_name=NULL;
	de->reg_offset=0;
	set_edit_format(de,hex_format);
	set_edit_chunk(de,byte_chunk);
	de->size=0;
	de->address=0;
  }
  else if (de->insp->regs!=NULL)
  {	struct register_def *r = &(de->insp->regs[de->insp->de_offset]);
    static char empty[1]={0};
	de->reg_name=r->name;
	if (de->reg_name==NULL) de->reg_name=empty;
  	de->size=r->size;
  	set_edit_format(de, r->format);
    set_edit_chunk(de,r->chunk);
	de->reg_offset=r->offset;
    if (de->size>0) de->insp->get_mem(r->offset,r->size,de->mem);
  }
  else
  {	de->reg_name=NULL;
  	de->address=de->insp->address+de->insp->de_offset;
    de->size=1<<de->insp->chunk;
  	set_edit_format(de, de->insp->format);
    set_edit_chunk(de, de->insp->chunk);
	de->reg_offset=0;
    if (de->size>0) de->insp->get_mem(de->insp->de_offset,de->size,de->mem);
  }
  show_edit_name(de);
  show_edit_windows(de);
}

void de_connect(HWND hDlg, inspector_def *insp)
/* call to associate a new inspector with the editor.
   call with NULL to disconnect the current editor */
{ dataedit *de = (dataedit*)(LONG_PTR)GetWindowLongPtr(hDlg,DWLP_USER);
  if (de==NULL) return;
  if (de->insp==insp) return;
  if (de->insp!=NULL) de_disconnect(de->insp);
  de->insp=insp;
  if (insp==NULL) de->size=0;
}

void de_save(HWND hDlg)
/* call to save the content of the editor back to memory.
   This is the same as pressing the store button */
{ dataedit *de = (dataedit*)(LONG_PTR)GetWindowLongPtr(hDlg,DWLP_USER);
  if (de->insp!=NULL && de->insp->store !=NULL && de->insp->de_size>0)
  { put_edit_mem(de);
    if (de->reg_name!=NULL)
      de->insp->store(de->reg_offset,de->size,de->mem); 
	else
	  de->insp->store((int)(de->address-de->insp->address),de->size,de->mem); 
  }
}

static void set_de_font(HWND hDlg)
{		SendDlgItemMessage(hDlg,IDC_NAME,WM_SETFONT,(WPARAM)hFixedFont,0);

     	SendDlgItemMessage(hDlg,IDC_EDITBYTE0,WM_SETFONT,(WPARAM)hFixedFont,0);
		SendDlgItemMessage(hDlg,IDC_EDITBYTE1,WM_SETFONT,(WPARAM)hFixedFont,0);
		SendDlgItemMessage(hDlg,IDC_EDITBYTE2,WM_SETFONT,(WPARAM)hFixedFont,0);
		SendDlgItemMessage(hDlg,IDC_EDITBYTE3,WM_SETFONT,(WPARAM)hFixedFont,0);
		SendDlgItemMessage(hDlg,IDC_EDITBYTE4,WM_SETFONT,(WPARAM)hFixedFont,0);
		SendDlgItemMessage(hDlg,IDC_EDITBYTE5,WM_SETFONT,(WPARAM)hFixedFont,0);
		SendDlgItemMessage(hDlg,IDC_EDITBYTE6,WM_SETFONT,(WPARAM)hFixedFont,0);
		SendDlgItemMessage(hDlg,IDC_EDITBYTE7,WM_SETFONT,(WPARAM)hFixedFont,0);
}

void change_de_font(HWND hDlg)
{	dataedit *de =(dataedit*)(LONG_PTR)GetWindowLongPtr(hDlg,DWLP_USER);
    set_de_font(hDlg);
    show_edit_name(de);
    show_edit_windows(de);
}

static INT_PTR CALLBACK   
DataEditDialogProc( HWND hDlg, UINT message, WPARAM wparam, LPARAM lparam )
{
  switch ( message )
  { case WM_INITDIALOG :
      { RECT rect;
	    dataedit *de;
		register_subwindow(hDlg);
		de = new_dataedit();
		if (de==NULL) 
		{ DestroyWindow(hDlg);
		  return FALSE;
		}
	    SetWindowLongPtr(hDlg,DWLP_USER,(LONG)(LONG_PTR)de);
	    de->hWnd=hDlg;
		set_format(hDlg,de->format);
		set_chunk(hDlg,de->chunk);
		GetWindowRect(GetDlgItem(hDlg,IDC_CHUNK),&rect);
		top_height=-rect.top;
		GetWindowRect(GetDlgItem(hDlg,IDC_CHECK_BYTE),&rect);
		top_height = top_height+separator_height*2+rect.bottom;
	    SetFocus(GetDlgItem(hDlg,IDC_LOAD));
		set_de_font(hDlg);
        de_update(hDlg);
	  }
	  return FALSE;
	case WM_CLOSE:
	  DestroyDataEdit(0);
	  return FALSE;
	case WM_DESTROY:
	  unregister_subwindow(hDlg);
	  free((dataedit*)(LONG_PTR)GetWindowLongPtr(hDlg,DWLP_USER));
      return FALSE;
	case DE_CONNECT:
	     de_connect(hDlg,(inspector_def *)lparam);
	  /* fall through to DE_UPDATE */
	case DE_UPDATE:
		de_update(hDlg);
	  return FALSE;
	case DE_SAVE:
	  de_save(hDlg);
	  return FALSE;
	case WM_COMMAND: 
      if (HIWORD(wparam) == BN_CLICKED) 
	  { dataedit *de = (dataedit*)(LONG_PTR)GetWindowLongPtr(hDlg,DWLP_USER);
	    enum chunk_fmt cf;
		enum mem_fmt mf;
        if (LOWORD(wparam) ==IDC_LOAD)   /* User has hit the Load key.*/
		{ if (de->insp!=NULL && de->insp->load!=NULL && de->insp->de_size>0) 
		  { if (de->reg_name!=NULL)
 			  memmove(de->mem,de->insp->load(de->reg_offset,de->size),de->size);
		    else
			  memmove(de->mem,de->insp->load((int)(de->address-de->insp->address),de->size),de->size);
		    show_edit_mem(de);
		  }
		}
		else if (LOWORD(wparam) ==IDC_STORE)   /* User has hit the store key.*/
		  de_save(hDlg);

	    else if ((mf=get_format(hDlg,LOWORD(wparam)))!=undefined_format)
		{ set_edit_format(de,mf);
		  show_edit_windows(de);
		}
	    else if ((cf=get_chunk(hDlg,LOWORD(wparam)))!=undefined_chunk)
		{ set_edit_chunk(de,cf);
		  show_edit_windows(de);
		}
	  }
	  return FALSE;
	case WM_SIZE:
		InvalidateRect(hDlg,NULL,TRUE);
		return FALSE;
	case WM_GETMINMAXINFO:
	{ MINMAXINFO *p = (MINMAXINFO *)lparam;
	  p->ptMinTrackSize.x = min_w;
      p->ptMinTrackSize.y = min_h;
	  p->ptMaxTrackSize.x=p->ptMinTrackSize.x;
	  p->ptMaxTrackSize.y=p->ptMinTrackSize.y;
	}
	return 0;

 }
  return FALSE;
}


HWND CreateDataEdit(HINSTANCE hInst, HWND hWnd)
/* call this to create an data editor child window. */
{ return CreateDialog(hInst,MAKEINTRESOURCE(IDD_DATAEDIT),hWnd,DataEditDialogProc);
}