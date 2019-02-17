#include <windows.h>
#include <string.h>
#include <stdio.h>
#include "resource.h"
#include "winopt.h"
#include "inspect.h"
#include "dedit.h"
#include "winde.h"


#pragma warning(disable : 4996)
int (*MemContextMenu)(inspector_def *insp, int offset, int x, int y)=NULL; /* if not NULL the function to handle the context menu for the memory window */  

char *format_names[]={"Hex","Ascii","Unsigned","Signed","Float"};
char *chunk_names[]={"BYTE","WYDE","TETRA","OCTA",""};
static int top_height, sb_width=0;  

static void invalidate_mem(inspector_def *insp)
{ RECT r;
if (insp->hWnd==NULL) return;
  r.top=top_height-separator_height;
  r.bottom=insp->height;
  r.left=0;
  r.right=insp->width;
  InvalidateRect(insp->hWnd,&r,TRUE);
}

static void invalidate_registers(inspector_def *insp, unsigned int from, unsigned int size)
{ RECT rec;
  int nr, i;
  int first, last, max_size;
  if (insp->hWnd==NULL) return;
  if (insp->regs==NULL) return;
  nr = insp->num_regs;
  if (nr>insp->lines) 
	  nr=insp->lines;
  first=nr;
  last=0;
  max_size=0;
  for (i=0; i<nr;i++)
  {	struct register_def *r;
	r = &insp->regs[insp->sb_cur+i];
	if ((unsigned)r->offset>=from+size || r->offset+(unsigned int)r->size<=from)
	  continue;
    if (i<first)first=i;
	if (i>last) last=i;
	if (max_size<r->size) max_size=r->size;
  }
  if (max_size<=0) return;
  if (last<first) return;
  rec.left=insp->address_width*fixed_char_width+separator_width;
  rec.right=rec.left+(max_size+1)*2*(fixed_char_width+separator_width); /* upper bound */
  rec.top=top_height+first*fixed_line_height;
  rec.bottom=top_height+last*fixed_line_height+fixed_line_height-separator_height;
  InvalidateRect(insp->hWnd,&rec,TRUE);
}

static void sb_range(inspector_def *insp)
/* determine and set scrollbar range as virtual number of insp->lines */
{ SCROLLINFO si;
  unsigned int new_size, new_base; 

  /* determine range */
  if (insp->hWnd==NULL) return;
  if (insp->size==0)
  { new_base=0;
	new_size = 0;
	insp->sb_base=0;
	insp->sb_cur=0;
	insp->sb_rng=0;
  }
  else if (insp->regs!=NULL)
  { insp->sb_rng = insp->num_regs-1;
    if (insp->sb_rng<0) insp->sb_rng=0;
    insp->sb_base =0;
	if (insp->sb_cur>insp->sb_rng-(insp->lines-1)) insp->sb_cur=insp->sb_rng-(insp->lines-1);
	if (insp->sb_cur<0) insp->sb_cur=0;
    new_base = insp->regs[insp->sb_cur].offset;
	new_size = insp->size -new_base;
  }
  else 
  { unsigned int page_range;
    page_range= insp->lines*insp->line_range;
    if (page_range*10<insp->size)
      insp->sb_rng = page_range*10/insp->line_range-1;
    else if (page_range<insp->size)
      insp->sb_rng = (insp->size+insp->line_range-1)/insp->line_range-1;
    else
      insp->sb_rng = (page_range+insp->line_range-1)/insp->line_range-1;
    /* determine address of first line */
	if (insp->sb_cur<0)
	{ if (insp->sb_base > -insp->sb_cur*insp->line_range)
	  { insp->sb_base = insp->sb_base +insp->sb_cur*insp->line_range;
	    insp->sb_cur=0;
	  } 
	  else
	  { insp->sb_base = 0;
	    insp->sb_cur=0;
	  } 
	}
    else if (insp->sb_cur>insp->sb_rng-(insp->lines-1))
	{ int d = insp->sb_cur-insp->sb_rng+(insp->lines-1);
	  if (insp->sb_base+d*insp->line_range + insp->sb_rng*insp->line_range<insp->size)
	    insp->sb_base = insp->sb_base + d*insp->line_range;
	  else if (insp->size> insp->sb_rng*insp->line_range)
		insp->sb_base=insp->size- insp->sb_rng*insp->line_range;
	  else
	    insp->sb_base=0;
	  insp->sb_cur=insp->sb_rng-(insp->lines-1);
	}
    new_base=insp->sb_base+insp->sb_cur*insp->line_range;
	new_size = page_range;
	if (new_size > insp->size -new_base)
	{ new_size = insp->size -new_base;
	}
  }
  /* adjust size */
  if (new_size>insp->mem_size)
  { insp->mem_buf=realloc(insp->mem_buf,new_size);
    if (insp->mem_buf==NULL)
		win32_fatal_error(__LINE__,"Out of memory");
  }
  /* adjust base */

  if (new_base!=insp->mem_base || new_size>insp->mem_size)
  { if (insp->get_mem) 
	  insp->get_mem(new_base,new_size,insp->mem_buf);
    else
      memset(insp->mem_buf,0,new_size);
  }
  insp->mem_size=new_size;
  insp->mem_base=new_base;

  if (insp->sb_rng<insp->lines)
	  ShowScrollBar(GetDlgItem(insp->hWnd,IDC_MEM_SCROLLBAR),SB_CTL,FALSE);
  else
  { si.cbSize=sizeof(si);
    si.fMask=SIF_PAGE|SIF_POS|SIF_RANGE;
    si.nMin=0;
    si.nMax=insp->sb_rng;
    si.nPage=insp->lines;
    si.nPos=insp->sb_cur;
	ShowScrollBar(GetDlgItem(insp->hWnd,IDC_MEM_SCROLLBAR),SB_CTL,TRUE);
    SetScrollInfo(GetDlgItem(insp->hWnd,IDC_MEM_SCROLLBAR),SB_CTL,&si,TRUE);
  }
}



static void sb_move(inspector_def *insp,WPARAM wparam)
{ switch (LOWORD(wparam)) 
	  { case SB_LINEUP:
	      insp->sb_cur--; /* we catch a negative value in sb_range */
		  break;
        case SB_LINEDOWN: 
	      insp->sb_cur++; /* we catch a too big value in sb_range */
		  break;
		case SB_PAGEUP:
		  insp->sb_cur=insp->sb_cur-insp->lines;
		  break;
        case SB_PAGEDOWN: 
		  insp->sb_cur=insp->sb_cur+insp->lines;
		  break;
		case SB_TOP:
		  insp->sb_cur=0;
		  break;
        case SB_BOTTOM: 
		  insp->sb_cur=insp->sb_rng-insp->lines-1;
		  break;
		case SB_THUMBTRACK: /* HIWORD(wparam) is 0 to sb_range */
		   insp->sb_cur = HIWORD(wparam);
          break;
		default:
		case SB_ENDSCROLL:
		case SB_THUMBPOSITION:
		  return;
	  }
	   adjust_mem_display(insp);
}


uint64_t adjust_goto_addr(inspector_def *insp, uint64_t goto_addr)
{  char hexstr[20];
	if (!insp->change_address)
    { if (goto_addr<insp->address)
      { goto_addr=insp->address;
      }
      else if (goto_addr>=insp->address+insp->size)
      { goto_addr=insp->address+(insp->size-1);
      }
    }
    uint64_to_hex(goto_addr,hexstr);
	if (insp->hWnd!=NULL)
      SetDlgItemText(insp->hWnd,IDC_GOTO,hexstr);
	return goto_addr;
}




static void refresh_old_mem(inspector_def *insp)
{ if (insp->old_size<insp->mem_size)
  { insp->old_mem = realloc(insp->old_mem,insp->mem_size);
    if (insp->old_mem==NULL)
		win32_fatal_error(__LINE__,"Out of memory");
  }
  memmove(insp->old_mem,insp->mem_buf,insp->mem_size);
  insp->old_base=insp->mem_base;
  insp->old_size=insp->mem_size;
}


void show_goto_addr(inspector_def *insp, uint64_t goto_addr)
{ goto_addr=adjust_goto_addr(insp,goto_addr);
  if (!insp->change_address || (goto_addr> insp->address && goto_addr-insp->address<INT_MAX))
    insp->sb_base = (unsigned int)(goto_addr-insp->address);
  else /* move base address of inspector */
  { insp->address=goto_addr;
    insp->sb_base=0;
	insp->mem_size=0;
    insp->de_offset=0;
	insp->de_size=0;
  }
  insp->sb_cur = 0;
  adjust_mem_display(insp);
}


void adjust_mem_display(inspector_def *insp)
{ if (insp->hWnd==NULL) return;
  sb_range(insp);
  if (insp->get_mem) 
	insp->get_mem(insp->mem_base,insp->mem_size,insp->mem_buf);
  else if (insp->mem_size>0 && insp->mem_buf!=NULL)
    memset(insp->mem_buf,0,insp->mem_size);
  invalidate_mem(insp);
}

void set_format(HWND hDlg,enum mem_fmt f)
{ int id; 
  switch (f)
  { case ascii_format: id=IDC_FORMAT_ASCII; break;
    case unsigned_format: id=IDC_FORMAT_UINT; break;
    case signed_format: id=IDC_FORMAT_INT; break;
    case float_format: id=IDC_FORMAT_FLOAT; break;
    case bin_format: id=IDC_FORMAT_BIN; break;
	default:
    case hex_format: id=IDC_FORMAT_HEX; break;
  }
  CheckDlgButton(hDlg,id,BST_CHECKED);
}
void clear_format(HWND hDlg,enum mem_fmt f)
{ int id; 
  switch (f)
  { case ascii_format: id=IDC_FORMAT_ASCII; break;
    case unsigned_format: id=IDC_FORMAT_UINT; break;
    case signed_format: id=IDC_FORMAT_INT; break;
    case float_format: id=IDC_FORMAT_FLOAT; break;
    case bin_format: id=IDC_FORMAT_BIN; break;
	default:
    case hex_format: id=IDC_FORMAT_HEX; break;
  }
  CheckDlgButton(hDlg,id,BST_UNCHECKED);
}

enum mem_fmt get_format(HWND hDlg, int ItemID)
{ enum mem_fmt f=undefined_format;
  switch (ItemID)
  { case IDC_FORMAT_ASCII: f= ascii_format; break;
    case IDC_FORMAT_INT: f= signed_format; break;
    case IDC_FORMAT_UINT: f= unsigned_format; break;
    case IDC_FORMAT_HEX: f= hex_format; break;
    case IDC_FORMAT_FLOAT: f= float_format; break;
    case IDC_FORMAT_BIN: f= bin_format; break;
  }
  return f;
}

void set_chunk(HWND hDlg,enum chunk_fmt c)
{ BOOL isbyte, iswyde,istetra,isocta;

  if (c==byte_chunk) isbyte=TRUE, iswyde=istetra=isocta=FALSE;
  else if (c==wyde_chunk) isbyte=iswyde=TRUE, istetra=isocta=FALSE;
  else if (c==tetra_chunk) isbyte=iswyde=istetra=TRUE, isocta=FALSE;
  else if (c==octa_chunk) isbyte=iswyde=istetra=isocta=TRUE;
  CheckDlgButton(hDlg,IDC_CHECK_BYTE,isbyte?BST_CHECKED:BST_UNCHECKED);
  CheckDlgButton(hDlg,IDC_CHECK_WYDE,iswyde?BST_CHECKED:BST_UNCHECKED);
  CheckDlgButton(hDlg,IDC_CHECK_BYTE3,istetra?BST_CHECKED:BST_UNCHECKED);
  CheckDlgButton(hDlg,IDC_CHECK_TETRA,istetra?BST_CHECKED:BST_UNCHECKED);
  CheckDlgButton(hDlg,IDC_CHECK_BYTE5,isocta?BST_CHECKED:BST_UNCHECKED);
  CheckDlgButton(hDlg,IDC_CHECK_BYTE6,isocta?BST_CHECKED:BST_UNCHECKED);
  CheckDlgButton(hDlg,IDC_CHECK_BYTE7,isocta?BST_CHECKED:BST_UNCHECKED);
  CheckDlgButton(hDlg,IDC_CHECK_OCTA,isocta?BST_CHECKED:BST_UNCHECKED);
  SetDlgItemText(hDlg,IDC_CHUNK,chunk_names[c]);
}

  

enum chunk_fmt get_chunk(HWND hDlg, int ItemID)
{ enum chunk_fmt c=undefined_chunk;;
  if (ItemID ==IDC_CHECK_BYTE)
   c= byte_chunk;
  else if (ItemID ==IDC_CHECK_WYDE)
   c= wyde_chunk;
  else if (ItemID ==IDC_CHECK_TETRA)
   c= tetra_chunk;
  else if (ItemID ==IDC_CHECK_OCTA)
   c= octa_chunk;
  return c;
}


int chunk_len(enum mem_fmt f, int chunk_size)
{ int column_digits;
  switch (f)
  { 
    case ascii_format: column_digits=1*chunk_size; break;
	case signed_format:
      if (chunk_size==1) column_digits=4;
      else column_digits=1+5*chunk_size/2;
	  break;
	case unsigned_format:
      if (chunk_size==1) column_digits=3;
      else column_digits=5*chunk_size/2;
	  break;
	case float_format: column_digits=19; break;
	default:
	case hex_format: column_digits=2*chunk_size; break;
  }
  return column_digits;
}

void resize_memory_dialog(inspector_def *insp)
{ 
  int left_margin;

  MoveWindow(GetDlgItem(insp->hWnd,IDC_MEM_SCROLLBAR),
	         insp->width-sb_width,top_height,sb_width,insp->height-top_height,TRUE);


  insp->lines = (insp->height-top_height)/fixed_line_height;
  if (insp->lines<1) insp->lines=1;
  
  insp->column_digits=chunk_len(insp->format,1<<insp->chunk);
  left_margin=insp->address_width*fixed_char_width+separator_width;
  insp->columns = (insp->width-sb_width-left_margin)/(insp->column_digits*fixed_char_width+separator_width);
  if (insp->columns<1)
    insp->columns=1;
  insp->line_range = insp->columns*(1<<insp->chunk);
  adjust_mem_display(insp);
}





/* Color management */

#define COLD RGB(0xFF,0xFF,0xFF)
#define HOT RGB(0xFF,0xA0,0xA0)
#define COLD_DE RGB(0xA0,0xA0,0xFF)
#define HOT_DE RGB(0xFF,0x70,0xFF)


static int different(inspector_def *insp,int offset, int size)
/* offset is relative to mem_base into mem_buf */
{ int i,j;
  i=0;
  j=offset+(insp->mem_base-insp->old_base);
  if (j<0) 
  { i=-j; 
    j=0;
  }
  while (i<size && (unsigned int)j<insp->old_size)
  { if (insp->old_mem[j]!=insp->mem_buf[offset+i]) return 1;
    i++;
	j++;
  }
  return 0;
}

#define GET2(a)   ((unsigned int)(((a)[0]<<8)+(a)[1]))
#define GET4(a)   ((unsigned int)(((a)[0]<<24)+((a)[1]<<16)+((a)[2]<<8)+(a)[3]))
#define GET8(a)   ((uint64_t)((((uint64_t)GET4(a))<<32)+GET4((a)+4)))

int chunk_to_str(char *str, unsigned char *buf, enum mem_fmt fmt, 
						int chunk_size, int column_digits)
/* prints the data from buf to str using format and chunk size 
   tries to use column_digits characters if column_digits>0. 
   returns the number of characters needed 
*/
{ int j;
  int w;
  switch (fmt)
  { default:
    case hex_format:
      for (j=0;j<chunk_size;j++)
	    sprintf(str+j*2,"%02X",buf[j]);
	    w=chunk_size*2;
	  break;
    case ascii_format:
      for (j=0;j<chunk_size;j++)
        if (buf[j]>0x1F && buf[j]<0x7F) str[j]=buf[j]; else str[j]=0x7F;
	  w=chunk_size;
	  break;
	case unsigned_format:
      switch (chunk_size)
      { case 1: w=sprintf(str,"%*u",column_digits,buf[0]); break;
        case 2: w=sprintf(str,"%*u",column_digits,GET2(buf)); break;
        case 4: w=sprintf(str,"%*u",column_digits,GET4(buf)); break;
	    default:
        case 8: w = sprintf(str,"%*llu",column_digits,GET8(buf)); break;
      }
	  break;
	case signed_format:
      switch (chunk_size)
      { case 1: w=sprintf(str,"%*d",column_digits,(int8_t)buf[0]); break;
        case 2: w=sprintf(str,"%*d",column_digits,(int16_t)GET2(buf)); break;
        case 4: w=sprintf(str,"%*d",column_digits,(int32_t)GET4(buf)); break;
	    default:
        case 8: w=sprintf(str,"%*lld",column_digits,(int64_t)GET8(buf)); break;
      }
	  break;
	case float_format:
	  if (chunk_size<8)
	    w=f64_to_str(str,f64_from_f32(GET4(buf)),column_digits);
	  else
	    w=f64_to_str(str,GET8(buf),column_digits); 
	  break;
  }
  if (column_digits>0 && w>column_digits)
  {	memset(str,'*',column_digits);
	w=column_digits;
  }
  str[w]=0;
  return w;
}


void update_max_regnames(inspector_def *insp)
{ int max_regname;
  if (insp->regs==NULL)
    max_regname=19;
  else
  { int i;
    max_regname=4; /* the basic minimum */
    for (i=0;i<insp->num_regs;i++)
    { int n;
      n = (int)strlen(insp->regs[i].name);
	  if (n> max_regname) max_regname=n;
    }
  }
  insp->address_width=max_regname;
}

void display_registers(inspector_def *insp,HDC hdc)
{ 
  int i,k, nr;
  char str[22]; /* big enough for the largest 8-Byte integer */
  RECT rect;
  nr = insp->num_regs;
  if (nr>insp->lines) nr=insp->lines;
  for (i=0;i<nr;i++)
   { enum mem_fmt format;
	 enum chunk_fmt chunk;
	 int y, chunk_size, x;
     struct register_def *r;
	 r = &insp->regs[insp->sb_cur+i];
     y=top_height+i*fixed_line_height;
     x=insp->address_width*fixed_char_width+separator_width;
     SelectObject(hdc, hFixedFont);
     SetBkColor(hdc,GetSysColor(COLOR_BTNFACE));
	 if (r->options&REG_OPT_DISABLED)
	   SetTextColor(hdc,RGB(0,0,0xFF));/* GetSysColor(COLOR_GRAYTEXT)); */
	 else
 	   SetTextColor(hdc,GetSysColor(COLOR_BTNTEXT));
     SetTextAlign(hdc,TA_RIGHT|TA_NOUPDATECP);
	 rect.top=y;
     rect.left=0;
     rect.right=x;
     rect.bottom=y+fixed_line_height-separator_height;
     ExtTextOut(hdc,x,y,
		   ETO_OPAQUE,&rect,r->name,(int)strlen(r->name),NULL);
	 if (r->format==user_format) format=insp->format; else format=r->format;
	 if (r->chunk==user_chunk) chunk=insp->chunk; else chunk=r->chunk;
	 chunk_size=1<<chunk;
	 if (chunk_size>r->size) chunk_size=r->size;
	 x = x+separator_width;
	 for (k=0;k*chunk_size < r->size;k++)
	 { int len;
	   len = chunk_len(format,chunk_size);
	   //if (len>insp->column_digits) len=insp->column_digits;
	   len = chunk_to_str(str, insp->mem_buf+r->offset+k*chunk_size-insp->mem_base, format,chunk_size,len);
	   if (r->options&REG_OPT_DISABLED)
	     SetBkColor(hdc,GetSysColor(COLOR_BTNFACE));
	   else if (different(insp,r->offset+k*chunk_size-insp->mem_base,chunk_size))
	   {  if (insp->de_size>0 && insp->sb_cur+i==insp->de_offset)   SetBkColor(hdc,HOT_DE);  else SetBkColor(hdc,HOT); }
	   else
	   {  if (insp->de_size>0 && insp->sb_cur+i==insp->de_offset)   SetBkColor(hdc,COLD_DE);  else   SetBkColor(hdc,COLD); }
	   SetTextAlign(hdc,TA_RIGHT|TA_NOUPDATECP);
	   rect.top=y;
       rect.left=x;
       rect.right=x+len*fixed_char_width;
       rect.bottom=y+fixed_line_height-separator_height;
       ExtTextOut(hdc,x+len*fixed_char_width,y,
		   ETO_OPAQUE,&rect,str,(int)strlen(str),NULL);
	   x=x+len*fixed_char_width+separator_width;
	 }
   } 
}
void display_address(inspector_def *insp,HDC hdc)
{ int i; 
  uint64_t addr=insp->address+insp->mem_base;
  char str[22]; /* big enough for the largest 8Byte integer */
  
  SelectObject(hdc, hFixedFont);
  SetBkColor(hdc,GetSysColor(COLOR_BTNFACE));
  for (i=0;i<insp->lines && (unsigned int)(i*insp->line_range)<insp->mem_size;i++)
  { uint64_to_hex(addr+i*insp->line_range,str);
    str[18]=':';
	TextOut(hdc,0,top_height+i*fixed_line_height,str,19);
  }
}

void display_memory(inspector_def *insp,HDC hdc)
{ 
  int i,k;
  char str[22]; /* big enough for the largest 8Byte integer */
  RECT r;
  int chunk_size=1<<insp->chunk;
  int columns= insp->columns;
  for (i=0;i<insp->lines && (unsigned int)(i*insp->line_range)<insp->mem_size;i++)
   { int y;
     y=top_height+i*fixed_line_height;
	 for (k=0;k<columns;k++)
	 { int x,l;
	   if ((unsigned int)((i*columns+k+1)*chunk_size)<= insp->mem_size)
	   { unsigned int offset= (i*columns+k)*chunk_size;
		 l=chunk_to_str(str, insp->mem_buf+offset, insp->format,chunk_size, insp->column_digits);
         if (different(insp,offset,chunk_size))
		 { if (offset+chunk_size>insp->de_offset-insp->mem_base && offset< insp->de_offset-insp->mem_base+insp->de_size) SetBkColor(hdc,HOT_DE); else SetBkColor(hdc,HOT); }
	     else
		 { if (offset+chunk_size>insp->de_offset-insp->mem_base && offset< insp->de_offset-insp->mem_base+insp->de_size) SetBkColor(hdc,COLD_DE); else SetBkColor(hdc,COLD); }
	   }
	   else
	   { str[0]=0;
	     l=0;
         SetBkColor(hdc,COLD);
	   }
	   x = insp->address_width*fixed_char_width+separator_width+separator_width+k*(insp->column_digits*fixed_char_width+separator_width);
	   SetTextAlign(hdc,TA_RIGHT|TA_NOUPDATECP);
	   r.top=y;
       r.left=x;
       r.right=x+insp->column_digits*fixed_char_width;
       r.bottom=y+fixed_line_height-separator_height;

       ExtTextOut(hdc,x+insp->column_digits*fixed_char_width,y,
		   ETO_OPAQUE|ETO_CLIPPED,&r,str,l,NULL);
	 }
   } 
}


void display_data(inspector_def *insp,HDC hdc)
{ if (insp->regs!=NULL)
  { /* refresh_old_mem(insp); */
    display_registers(insp,hdc);
  }
  else
  { display_address(insp,hdc);
    /* refresh_old_mem(insp); */
    display_memory(insp,hdc);
  }
}


static int  set_edit_offset(inspector_def *insp,int x, int y)
/* determine de_offset from the position 
   its the offest into memory or the register number
*/
{ int i = (y-top_height)/fixed_line_height;
  if (x<=insp->address_width*fixed_char_width+separator_width) return -1;
  if (i<0 || i >= insp->lines) return -1;
  if (insp->regs!=NULL)
  { if(i+insp->sb_cur>=insp->num_regs) return -1;
      return i+insp->sb_cur;
  }
  else
  { unsigned int offset = insp->mem_base+i*insp->line_range;
    offset+=(1<<insp->chunk)*((x-insp->address_width*fixed_char_width-separator_width)/(insp->column_digits*fixed_char_width+separator_width));
	if (offset>=insp->size) return -1;
    return offset;
  }
}

void SetInspector(HWND hMemory, inspector_def * insp)
{ RECT r;
  SetWindowLongPtr(hMemory,DWLP_USER,(LONG)(LONG_PTR)insp);
  insp->hWnd=hMemory;
  GetClientRect(insp->hWnd,&r);
  insp->width=r.right-r.left;
  insp->height=r.bottom-r.top;
  update_max_regnames(insp);
  resize_memory_dialog(insp);
  if (insp->regs==NULL)
  {
    adjust_goto_addr(insp,insp->address+insp->mem_base);
	ShowWindow(GetDlgItem(insp->hWnd,IDC_GOTO_PROMPT),SW_SHOW);
	ShowWindow(GetDlgItem(insp->hWnd,IDC_GOTO),SW_SHOW);
#if 0
	 RECT r;
	POINT p;
    int xButton;
	GetWindowRect(GetDlgItem(insp->hWnd,IDC_GOTO),&r);
	p.x=r.right;
	p.y=r.top;
    ScreenToClient(insp->hWnd,&p);
	xButton=p.x+fixed_char_width;
#endif
  }
  else
  { ShowWindow(GetDlgItem(insp->hWnd,IDC_GOTO),SW_HIDE);
  	ShowWindow(GetDlgItem(insp->hWnd,IDC_GOTO_PROMPT),SW_HIDE);
  }
  set_format(insp->hWnd,insp->format);
  set_chunk(insp->hWnd,insp->chunk);
}


static INT_PTR CALLBACK   
MemoryDialogProc( HWND hDlg, UINT message, WPARAM wparam, LPARAM lparam )
{
  switch ( message )
  { case WM_INITDIALOG :
      if (sb_width==0)
	  { RECT sbRect;
        GetWindowRect(GetDlgItem(hDlg,IDC_MEM_SCROLLBAR),&sbRect);
        sb_width=sbRect.right-sbRect.left;
		GetWindowRect(GetDlgItem(hDlg,IDC_CHUNK),&sbRect);
		top_height=-sbRect.top;
		GetWindowRect(GetDlgItem(hDlg,IDC_CHECK_BYTE),&sbRect);
		top_height = top_height+separator_height*2+sbRect.bottom;
	  }
	  set_format(hDlg,hex_format);
	  set_chunk(hDlg,octa_chunk);
	  hDataEditInstance=hInst;
	  register_subwindow(hDlg);
	  return FALSE;
	case WM_CLOSE:
		DestroyWindow(hDlg);
		return 0;
	case WM_DESTROY:
		{  inspector_def *insp=(inspector_def *)(LONG_PTR)GetWindowLongPtr(hDlg,DWLP_USER);
		   unregister_subwindow(hDlg);
		   //DestroyDataEdit(0); I think this is not necessary
		   if (insp!=NULL) insp->hWnd=NULL;
		}
	  return FALSE;
	case WM_LBUTTONDBLCLK:
	{ HWND hde;
	  inspector_def *insp=(inspector_def *)(LONG_PTR)GetWindowLongPtr(hDlg,DWLP_USER);
	  if (insp==NULL) return FALSE;
	  if (insp->hWnd!=NULL)
	    InvalidateRect(insp->hWnd,NULL,TRUE);
	  hde=GetDataEdit(0,hDlg);
	  de_connect(hde,insp);
      insp->de_offset=set_edit_offset(insp,LOWORD(lparam),HIWORD(lparam));
	  insp->de_size=1<<insp->chunk;
	  InvalidateRect(insp->hWnd,NULL,FALSE);
	  de_update(hde);
	}
    return FALSE;
	case WM_COMMAND: 
	  if (wparam ==IDOK)
	  {  /* User has hit the ENTER key.*/
		 inspector_def *insp=(inspector_def *)(LONG_PTR)GetWindowLongPtr(hDlg,DWLP_USER);
		if (insp==NULL) return FALSE;
	    if (GetFocus()==GetDlgItem(hDlg,IDC_GOTO))
        { uint64_t goto_addr;
#define   MAXVALUE 32
		  char value[MAXVALUE];
          GetDlgItemText(hDlg,IDC_GOTO,value,MAXVALUE);
    	  goto_addr = strtouint64(value);
		  show_goto_addr(insp, goto_addr);
		 }
	  }
	  else if (HIWORD(wparam) == BN_CLICKED) 
	  { inspector_def *insp=(inspector_def *)(LONG_PTR)GetWindowLongPtr(hDlg,DWLP_USER);
	    enum chunk_fmt cf;
		enum mem_fmt mf;
	    if (insp==NULL) return FALSE;
        if ((cf=get_chunk(hDlg,LOWORD(wparam)))!=undefined_chunk)
		{ insp->chunk=cf;
		  set_chunk(hDlg,cf);
     	  resize_memory_dialog(insp);
		}
        else if ((mf=get_format(hDlg,LOWORD(wparam)))!=undefined_format)
	    { insp->format=mf;
		  if (insp->format==float_format && insp->chunk<tetra_chunk)
		  { insp->chunk=tetra_chunk;
			set_chunk(hDlg,tetra_chunk);
		  } 
     	  resize_memory_dialog(insp);
	    } 
	  }
	  return FALSE;
	case WM_VSCROLL: 
	  { inspector_def *insp=(inspector_def *)(LONG_PTR)GetWindowLongPtr(hDlg,DWLP_USER);
	    if (insp==NULL) return FALSE;
        sb_move(insp,wparam);
	  }
       return TRUE; 
    case SBM_SETRANGE:
		return TRUE;
	case WM_PAINT:
    { inspector_def *insp=(inspector_def *)(LONG_PTR)GetWindowLongPtr(hDlg,DWLP_USER);
	  PAINTSTRUCT ps;
	  HDC hdc = BeginPaint (hDlg, &ps);
	  if (insp!=NULL)
	  { display_data(insp,hdc);
	  }
      EndPaint (hDlg, &ps);
	  return TRUE;
    }    
	case WM_SIZE:
    {   inspector_def *insp=(inspector_def *)(LONG_PTR)GetWindowLongPtr(hDlg,DWLP_USER);
		if (insp!=NULL) 
		{ insp->width=LOWORD(lparam);
		  insp->height=HIWORD(lparam);
		  resize_memory_dialog(insp);
		}
	}
	  break;
	case WM_GETMINMAXINFO:
	{ MINMAXINFO *p = (MINMAXINFO *)lparam;
	  inspector_def *insp=(inspector_def *)(LONG_PTR)GetWindowLongPtr(hDlg,DWLP_USER);
	  if (insp!=NULL)
		  p->ptMinTrackSize.x = insp->address_width*fixed_char_width+separator_width+16*fixed_char_width+2*separator_width+sb_width;
	  else
	      p->ptMinTrackSize.x = 36*fixed_char_width+2*separator_width+sb_width;
      p->ptMinTrackSize.y = top_height+separator_height+fixed_line_height;
	  p->ptMaxTrackSize.x=p->ptMinTrackSize.x;
	  p->ptMaxTrackSize.y=p->ptMinTrackSize.y;
	}
	break;
	case WM_CONTEXTMENU: 
	  if (MemContextMenu!=NULL)
	  { inspector_def *insp=(inspector_def *)(LONG_PTR)GetWindowLongPtr(hDlg,DWLP_USER);
	    if (insp!=NULL)
		{  POINT pt;
		   int offset;
		   pt.x=LOWORD(lparam);
		   pt.y= HIWORD(lparam);
           ScreenToClient(hDlg, &pt); 
		   offset=set_edit_offset(insp,pt.x,pt.y);
		  if (offset>=0 && MemContextMenu(insp,offset,LOWORD(lparam),HIWORD(lparam) )) return TRUE; 
		}
	  }
	  break; 
  }
  return FALSE;
}

HWND CreateMemoryDialog(HINSTANCE hInst,HWND hParent)
{ HWND h;
  h= CreateDialog(hInst,MAKEINTRESOURCE(IDD_MEMORY),hParent,MemoryDialogProc);
  hDataEditParent=hParent;
  return h;
}


void MemoryDialogUpdate(inspector_def *insp, unsigned int offset, int size)
/* called if size byte in memory at offset have changed */
{ if (insp->hWnd==NULL) return;
  if (insp->regs==NULL) adjust_goto_addr(insp,insp->address+insp->mem_base);
  if (offset>=insp->mem_base+insp->mem_size || offset+size<=insp->mem_base) 
    return;
  else
  { refresh_old_mem(insp);
    if (offset<insp->mem_base) { size = size-(insp->mem_base-offset); offset=insp->mem_base; }
	if (offset+size>insp->mem_base+insp->mem_size)  size = insp->mem_base+insp->mem_size-offset;
	if (insp->get_mem) insp->get_mem(offset, size, insp->mem_buf+(offset-insp->mem_base));
	if (insp->regs!=NULL) invalidate_registers(insp, offset, size);
	else invalidate_mem(insp);
  }
}



void de_disconnect(inspector_def *insp)
/* called if a dataedit window disconects from the inspector */
{ if (insp->hWnd!=NULL)
    InvalidateRect(insp->hWnd,NULL,TRUE);
  insp->de_offset=0;
  insp->de_size=0;
}




