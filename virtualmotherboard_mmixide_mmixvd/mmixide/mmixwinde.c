#include <windows.h>
#include "mmixlib.h"
#include "winmain.h"
#include "dedit.h"
#include "winde.h"
#include "splitter.h"



#define MAXDATAEDIT 4
static HWND hDataEdit = NULL; /* there is only a single data editor */
HINSTANCE hDataEditInstance=0;
HWND hDataEditParent=0;

int is_dataedit(HWND h)
{ while (h!=NULL)
  { if (hDataEdit==h) return 1;
	h=GetParent(h);
  }
  return 0;
}

HWND GetDataEdit(int id, HWND hMemory)
{ if (hDataEdit!=NULL)
    return hDataEdit;
  sp_create_options(0, 0, 0.2, 1,hMemory);
  hDataEdit = CreateDataEdit(hInst,hSplitter);
  if (hDataEdit==NULL)
    win32_error(__LINE__,"Unable to create Data Editor");
  return hDataEdit;
}

void DestroyDataEdit(int id)
{ if (hDataEdit==NULL)
	return;
  de_connect(hDataEdit,NULL);
  DestroyWindow(hDataEdit);
  hDataEdit=NULL;
}

void mmix_change_de_font(void)
{ if (hDataEdit==NULL) return;
  change_de_font(hDataEdit);
  InvalidateRect(hDataEdit,NULL,TRUE);
}
