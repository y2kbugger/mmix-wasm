#include <stdio.h>
#include <windows.h>
#include <commctrl.h>
#include "resource.h"
#include "libmmixal.h"
#include "error.h"
#include "splitter.h"
#include "winmain.h"
#include "mmixrun.h"
#include "info.h"
#include "editor.h"
#include "winopt.h"
#include "debug.h"
#include "breakpoints.h"
#include "symtab.h"

int symtab_locals=0;
int symtab_registers=0;
int symtab_small=0;

HWND	  hSymbolTable=NULL;

INT_PTR CALLBACK    
OptionSymtabDialogProc( HWND hDlg, UINT message, WPARAM wparam, LPARAM lparam )
{ 
  switch ( message )
  { case WM_INITDIALOG:
      CheckDlgButton(hDlg,IDC_CHECK_LOCALS,symtab_locals?BST_CHECKED:BST_UNCHECKED);
	  CheckDlgButton(hDlg,IDC_CHECK_REGISTERS,symtab_registers?BST_CHECKED:BST_UNCHECKED);
	  CheckDlgButton(hDlg,IDC_CHECK_SMALL,symtab_small?BST_CHECKED:BST_UNCHECKED);
      return TRUE;
    case WM_SYSCOMMAND:
      if( wparam == SC_CLOSE ) 
      { EndDialog(hDlg, TRUE);
        return TRUE;
      }
      break;
    case WM_COMMAND:
      if( wparam == IDOK )
	  { symtab_locals =IsDlgButtonChecked(hDlg,IDC_CHECK_LOCALS);
		symtab_registers =IsDlgButtonChecked(hDlg,IDC_CHECK_REGISTERS);
		symtab_small =IsDlgButtonChecked(hDlg,IDC_CHECK_SMALL);
		update_symtab();
		EndDialog(hDlg, TRUE);
        return TRUE;
      } 
	  else if (wparam==IDCANCEL)
	  { EndDialog(hDlg, TRUE);
        return TRUE;
	  }
     break;
  case WM_HELP:
    ide_help("help\\options\\symbols.html");
    return TRUE;

  }
  return FALSE;
}


void symtab_reset(void)
{  if (hSymbolTable!=NULL) SendMessage(hSymbolTable,LB_RESETCONTENT,0,0);	
}

static void symtab_add(char *symbol, sym_node *sym)
{ LPARAM item;
  if (sym->link!=REGISTER && sym->link!=DEFINED) return;
  if (sym->link==REGISTER && !symtab_registers) return;
  if (!symtab_locals && strchr(symbol+1,':')!=NULL) return;
  if (!symtab_small && sym->link==DEFINED && sym->equiv.h==0 && sym->equiv.l<0x100) return;
  if (sym->link==REGISTER)
  { char str[100];
    sprintf_s(str,100,"%s IS $%d",symbol,sym->equiv.l);
	item = SendMessage(hSymbolTable,LB_ADDSTRING,0,(LPARAM)str);
  }
  else
    item = SendMessage(hSymbolTable,LB_ADDSTRING,0,(LPARAM)symbol);
  SendMessage(hSymbolTable,LB_SETITEMDATA,item,(LPARAM)sym);
}

#if 0
/* currently not used */
void symtab_mark(char *symbol)
{ int item;
  item = (int)SendMessage(hSymbolTable,LB_FINDSTRINGEXACT,-1,(LPARAM)symbol);
  if(item==LB_ERR) return;
  SendMessage(hSymbolTable,LB_SETCURSEL,item,0);
}
#endif

sym_node *find_symbol(char *symbol,int file_no)
{ trie_node *t;
  if (symbol==NULL || *symbol==0) return NULL;
  if (file_no<0) return NULL;
  t=(trie_node*)file2symbols(file_no);
  while(t!=NULL) 
  { if (*symbol==t->ch) 
	{ symbol++; 
	  if (*symbol==0) return t->sym;
      t=t->mid; 
	}
    else if (*symbol < t->ch) t=t->left;
    else t=t->right;
  }
  return NULL;
}

#define MAX_SYMBOL_STR 1024
static char symtab_buf[MAX_SYMBOL_STR+1];

static void enumerate_symtab(trie_node *t, int i)
{
  if (t->left) enumerate_symtab(t->left,i);
  symtab_buf[i]=(char)t->ch;
  if (t->sym)
  { symtab_buf[i+1]=0;
     symtab_add(symtab_buf+1,t->sym); /* skip leading : */
  }
  if (t->mid && i+1<MAX_SYMBOL_STR) enumerate_symtab(t->mid,i+1);
  if (t->right) enumerate_symtab(t->right,i);
}

static void symtab_add_file_no(int file_no)
{ trie_node *t;
  if (!file2assembly(file_no)) return;
  t= (trie_node*)file2symbols(file_no);
  if (t!=NULL)
     enumerate_symtab(t, 0);
}


void update_symtab(void)
{ 
  symtab_reset();
  for_all_files(symtab_add_file_no);
}
void create_symtab(void)
{ if (hSymbolTable!=NULL) return;
  sp_create_options(1,1,0.15,0,hEdit);
  hSymbolTable = CreateWindow("LISTBOX","Symbol Table",
		     WS_CHILD|WS_VISIBLE|WS_VSCROLL|LBS_NOTIFY|
			 LBS_NOINTEGRALHEIGHT|LBS_SORT|LBS_HASSTRINGS|LBS_OWNERDRAWFIXED,
             0,0,0,0,
	         hSplitter, NULL, hInst, NULL);
  SendMessage(hSymbolTable,WM_SETFONT,(WPARAM)hVarFont,0);
  CheckMenuItem(hMenu,ID_VIEW_SYMBOLTABLE,MF_BYCOMMAND|MF_CHECKED);
  update_symtab();
}

int symtab_measureitem(LPMEASUREITEMSTRUCT lm)
{   lm->itemHeight = var_line_height; 
	return 1;
}

int symtab_drawitem(LPDRAWITEMSTRUCT di)
{ int y; 
  COLORREF cb, cf;
  if (di->itemID <0 ) return 0;
  switch (di->itemAction) 
  { case ODA_SELECT: 
    case ODA_DRAWENTIRE:
      SendMessage(di->hwndItem, LB_GETTEXT, di->itemID, (LPARAM)symtab_buf); 
      y = (di->rcItem.bottom + di->rcItem.top - var_char_height) / 2;
      if (di->itemState & ODS_SELECTED)
	  {  cb =SetBkColor(di->hDC,SELECT_COLOR);
		 cf =SetTextColor(di->hDC,RGB(0xff,0xff,0xff));
	  }
	  else
	  {  cb =SetBkColor(di->hDC,RGB(0xff,0xff,0xff));
		 cf =SetTextColor(di->hDC,RGB(0x00,0x00,0x00));
	  }
	  ExtTextOut(di->hDC, separator_width, y,ETO_CLIPPED|ETO_OPAQUE, 
		         &di->rcItem,symtab_buf,(int)strlen(symtab_buf),NULL);
	  SetBkColor(di->hDC,cb);
	  SetTextColor(di->hDC,cf);
    return 1;
  } 
  return 0;
}
 
int SymbolContextMenuHandler(int x, int y)
{ POINT pt = { x, y }; 
  int item;
  item = LBItemFromPt(hSymbolTable,pt,FALSE);
  if (item>=0)
  { int cmd;
  	sym_node *sym=NULL;
    sym = (sym_node *)SendMessage(hSymbolTable,LB_GETITEMDATA,item,0);


    cmd= TrackPopupMenuEx(hSymContextMenu, 
             TPM_LEFTALIGN | TPM_RIGHTBUTTON|TPM_NONOTIFY|TPM_RETURNCMD, 
             x, y, hMainWnd, NULL);
	switch (cmd)
    { case ID_POPUP_BREAKONVALUE:
		if (sym!=NULL && sym->link==DEFINED)
		{ int bits;
          if ((sym->equiv.h&0x60000000)==0) bits=exec_bit;
          else bits=read_bit|write_bit;
		  bits=set_file_breakpoint(sym->file_no, sym->line_no,  bits, bits);
		  ed_set_break(sym->file_no,sym->line_no,bits);
		  return 1;
		}
		break;
	  case ID_POPUP_SHOW:
		if (sym!=NULL && sym->link==DEFINED)
		{ int segment; 
		  uint64_t goto_addr;
		  segment = (sym->equiv.h>>29) & 0x7;
		  if (segment>3) segment=4;
		  goto_addr=sym->equiv.h;
		  goto_addr=(goto_addr<<32)+sym->equiv.l;
          set_goto_addr(segment , goto_addr);
		  return 1;
		}
		break;
	  default: 
		break;
	}
  }
  return 0;
}
	
#if 0	
	
  int cmd, size;
  unsigned char buffer[8];
  octa loc;
  HMENU hMenu;
  if (insp->regs!=NULL) hMenu=hRegContextMenu;
  else hMenu=hMemContextMenu;
  cmd = TrackPopupMenuEx(hMenu, 
            TPM_LEFTALIGN | TPM_RIGHTBUTTON|TPM_NONOTIFY|TPM_RETURNCMD, 
            x, y, hMainWnd, NULL); // Display the shortcut menu. Track the right mouse button.


#endif
