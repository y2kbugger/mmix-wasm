#include <windows.h>

/* set these before using any of the functions */
extern HINSTANCE hDataEditInstance; /* needed: the instance of the Application */
extern HWND hDataEditParent; /* may be used to position the data editor */

/* provided */
extern HWND GetDataEdit(int id, HWND hMemory);
/* call this to get a handle to a data edit window.
   call with different id's to get different windows */

extern void DestroyDataEdit(int id);
/* call to destroy all DataEdit windows */

extern int is_dataedit(HWND h);

extern void mmixde_set_font(void);