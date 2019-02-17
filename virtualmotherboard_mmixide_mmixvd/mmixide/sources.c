#include <windows.h>
#include <commctrl.h>
#include "resource.h"
#include "winmain.h"
#include "info.h"
#include "editor.h"
#include "sources.h"


static HWND hSourceDlg;
static HWND hFileTab;

static int current_tab;
static int application_index;
int load_single_file=1;


#define MAX_CMD 512
static void no_loading(int file_no)
{ file2loading(file_no)=0;
}

static void show_file(void)
/* call after changing current_tab */
{ int file_no;
  TCITEM tie;
  char *name;
  tie.mask=TCIF_PARAM;
  TabCtrl_GetItem(hFileTab,current_tab,&tie);
  file_no=(int)tie.lParam;
  name = file2fullname(file_no);
  if (name==NULL) name = unique_name(file_no);
  SetDlgItemText(hSourceDlg,IDC_TAB_FULLNAME,name);
  CheckDlgButton(hSourceDlg,IDC_CHECK_SYMBOLS,file2assembly(file_no)?BST_CHECKED:BST_UNCHECKED);
#ifdef VMB
  CheckDlgButton(hSourceDlg,IDC_CHECK_IMAGEFILE,file2image(file_no)?BST_CHECKED:BST_UNCHECKED);
#endif
  CheckDlgButton(hSourceDlg,IDC_CHECK_LOADMMO,file2loading(file_no)?BST_CHECKED:BST_UNCHECKED);
  CheckDlgButton(hSourceDlg,IDC_CHECK_EXECUTE,file2execute(file_no)?BST_CHECKED:BST_UNCHECKED);
  EnableWindow(GetDlgItem(hSourceDlg,IDC_EDIT_COMMAND),file2execute(file_no));
  if (command[file_no]!=NULL)
  {   SetDlgItemText(hSourceDlg,IDC_EDIT_COMMAND,command[file_no]);
  }
  else
  {  SetDlgItemText(hSourceDlg,IDC_EDIT_COMMAND,"");
  }

}


static void select_tab(int index)
/* call to change current_tab */
{ if (index<0) return;
  current_tab=index;
  TabCtrl_SetCurSel (hFileTab, current_tab);
  show_file();
}

static void add_file_tab(int file_no)
{ TCITEM tie;
  int index, imax;
  if (hFileTab==NULL) return;
  if (file_no<0) return;
  imax = TabCtrl_GetItemCount(hFileTab);
  for (index=0;index<imax;index++)
  { tie.mask=TCIF_PARAM;
    TabCtrl_GetItem(hFileTab,index,&tie);
    if (file_no==tie.lParam)
		break;
  }
  tie.mask = TCIF_TEXT|TCIF_PARAM|TCIF_IMAGE;
  tie.pszText = unique_name(file_no);
  tie.lParam=file_no;
  tie.iImage= (file2loading(file_no))?0:-1;
  if (index <imax)
  { TabCtrl_SetItem(hFileTab,index,&tie);
    current_tab=index;
  }
  else
    current_tab=TabCtrl_InsertItem (hFileTab, index, &tie);
}


INT_PTR CALLBACK    
OptionSourcesDialogProc( HWND hDlg, UINT message, WPARAM wparam, LPARAM lparam )
{ 
  switch ( message )
  { case WM_INITDIALOG:
      { hSourceDlg=hDlg;
	    hFileTab=GetDlgItem(hDlg,IDC_TAB_FILES);
		TabCtrl_SetImageList(hFileTab,hFileMarkers);
		for_all_files(add_file_tab);
		current_tab=0;
		TabCtrl_SetCurSel (hFileTab, current_tab);
        show_file();
		CheckDlgButton(hSourceDlg,IDC_CHECK_SINGLEFILE,load_single_file?BST_CHECKED:BST_UNCHECKED);
      }
      return TRUE;
    case WM_SYSCOMMAND:
      if( wparam == SC_CLOSE ) 
      { EndDialog(hDlg, TRUE);
        return TRUE;
      }
      break;
    case WM_COMMAND:
      if( wparam == IDOK )
      { load_single_file=IsDlgButtonChecked(hSourceDlg,IDC_CHECK_SINGLEFILE);
        hSourceDlg=NULL;
		hFileTab=NULL;
		EndDialog(hDlg, TRUE);
        return TRUE;
      } 
      else if( wparam == IDC_CHECK_SINGLEFILE )
      { load_single_file=IsDlgButtonChecked(hSourceDlg,IDC_CHECK_SINGLEFILE);
	  }
	  else if (wparam==IDC_ADD)
	  { int file_no = ed_open();
		if (file_no>=0) 
		{  set_edit_file(file_no);
		   add_file_tab(file_no);
		   select_tab(current_tab);
		}
        return TRUE;
	  } 
	  else if (wparam==IDC_CHECK_EXECUTE)
	  { int file_no;
        TCITEM tie;
        tie.mask=TCIF_PARAM;
        TabCtrl_GetItem(hFileTab,current_tab,&tie);
        file_no=(int)tie.lParam;
        file2execute(file_no)=IsDlgButtonChecked(hSourceDlg,IDC_CHECK_EXECUTE);
	    EnableWindow(GetDlgItem(hSourceDlg,IDC_EDIT_COMMAND),file2execute(file_no));
        return TRUE;
	  }
	  else if (wparam==IDC_CHECK_LOADMMO)
	  { int file_no;
        TCITEM tie;
        tie.mask=TCIF_PARAM;
        TabCtrl_GetItem(hFileTab,current_tab,&tie);
        file_no=(int)tie.lParam;
        if (IsDlgButtonChecked(hSourceDlg,IDC_CHECK_LOADMMO))
        { if (load_single_file) 
		  { int index, count = TabCtrl_GetItemCount(hFileTab);
		    for(index=0; index<count;index++)
			{ if (index!=current_tab) 
			  { tie.mask = TCIF_PARAM|TCIF_IMAGE;
  	            TabCtrl_GetItem(hFileTab,index,&tie);
			    if (file2loading((int)tie.lParam)!=0)
			    { file2loading((int)tie.lParam)=0;
			      tie.iImage=-1;
			      tie.mask=TCIF_IMAGE;
				  TabCtrl_SetItem(hFileTab,index,&tie);
				  ed_show_tab((int)tie.lParam);
			    }
			  }
		    }
		  }
          file2loading(file_no)=1;
	      tie.mask=TCIF_IMAGE;
	      tie.iImage=0;
	      TabCtrl_SetItem(hFileTab,current_tab,&tie);
		  ed_show_tab(file_no);
		}
        else
        { file2loading(file_no)=0;
          tie.mask=TCIF_IMAGE;
	      tie.iImage=-1;
	      TabCtrl_SetItem(hFileTab,current_tab,&tie);
	  	  ed_show_tab(file_no);
        }
		return TRUE;
	  }
	  else if (wparam==IDC_CHECK_SYMBOLS)
      { int file_no;
        TCITEM tie;
        tie.mask=TCIF_PARAM;
        TabCtrl_GetItem(hFileTab,current_tab,&tie);
        file_no=(int)tie.lParam;
        file2assembly(file_no)=IsDlgButtonChecked(hSourceDlg,IDC_CHECK_SYMBOLS);
	  	return TRUE;
	  }
#ifdef VMB
	  else if (wparam==IDC_CHECK_IMAGEFILE)
      { int file_no;
        TCITEM tie;
        tie.mask=TCIF_PARAM;
        TabCtrl_GetItem(hFileTab,current_tab,&tie);
        file_no=(int)tie.lParam;
        file2image(file_no)=IsDlgButtonChecked(hSourceDlg,IDC_CHECK_IMAGEFILE);
	  	return TRUE;
	  }
#endif
	  else if (HIWORD(wparam)==EN_CHANGE && LOWORD(wparam)==IDC_EDIT_COMMAND)
      { int file_no;
        TCITEM tie;
        tie.mask=TCIF_PARAM;
        TabCtrl_GetItem(hFileTab,current_tab,&tie);
        file_no=(int)tie.lParam;
        if (IsDlgButtonChecked(hSourceDlg,IDC_CHECK_EXECUTE))
        { char cmd[MAX_CMD+1];
          int n;
	      n= GetDlgItemText(hSourceDlg,IDC_EDIT_COMMAND,cmd,MAX_CMD);
	      if (n>=MAX_CMD) MessageBox(hSourceDlg,"Command too long, truncated","Warning",MB_ICONWARNING|MB_OK);
          command[file_no]=realloc(command[file_no],n+1);
	      if (command[file_no]==NULL)
	      { MessageBox(hSourceDlg,"Out of memmory","Error",MB_ICONWARNING|MB_OK);
	        return TRUE;
	      }
	      strncpy_s(command[file_no],n+1,cmd,n+1);
        }
        else if (command[file_no]!=NULL)
       { free(command[file_no]);
         command[file_no]=NULL;
       }
	   return TRUE;
     }
     break;
	 case WM_NOTIFY:
	  { NMHDR *n = (NMHDR*)lparam;
	    if (n->hwndFrom==hFileTab)
		{ if (n->code == TCN_SELCHANGE){ 
			current_tab = TabCtrl_GetCurSel(hFileTab);
			show_file();
		  }
		}
		return TRUE;
	  }
  case WM_HELP:
    ide_help("help\\options\\sources.html");
    return TRUE;


  }
  return FALSE;
}



