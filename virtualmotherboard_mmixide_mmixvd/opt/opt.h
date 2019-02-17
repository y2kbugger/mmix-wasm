#ifndef _OPT_H_
#define _OPT_H_

#include "vmb.h"
#include "winres.h"


extern device_info vmb;

extern HWND hpower;

extern void win32_error_init(int on);
/* set hExtraDebug to a window handle to get an extra debug tab */
extern HWND (*addExtraDebug)(HWND hDebug);
extern void mem_update(unsigned int offset, int size);
/* call this function to tell a specific memory inspector i that an update is due */
extern void mem_update_i(int i, unsigned int offset, int size);

extern void init_device(device_info *vmb);

extern HRGN BitmapToRegion (HBITMAP hBmp);


extern int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow);
                     
extern LRESULT CALLBACK 
WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam); /*needed */

extern INT_PTR CALLBACK   
SettingsDialogProc( HWND hDlg, UINT message, WPARAM wparam, LPARAM lparam ); /*needed */


extern LRESULT CALLBACK 
OptWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam); /* provided */

extern INT_PTR CALLBACK    
ConfigurationDialogProc( HWND hDlg, UINT message, WPARAM wparam, LPARAM lparam );  /* provided */

/* in winconnect.c */
extern INT_PTR CALLBACK    
ConnectDialogProc( HWND hDlg, UINT message, WPARAM wparam, LPARAM lparam );  /* provided */

/* in winparam.c */
extern void win32_param_init(void);

#endif
