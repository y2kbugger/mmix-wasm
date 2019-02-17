#include <windows.h>
#include "inspect.h"

#define DE_CONNECT	(WM_APP+1)
#define DE_UPDATE	(WM_APP+2)
#define DE_SAVE		(WM_APP+3)

/* this should be provided by the inspector */
extern void de_disconnect(inspector_def *insp);

/* this is provided by the data editor */
extern HWND CreateDataEdit(HINSTANCE hInstance, HWND hWndParent);
extern void de_connect(HWND hDlg, inspector_def *insp);
extern void de_update(HWND hDlg);
extern void de_save(HWND hDlg);
extern void change_de_font(HWND hDlg);