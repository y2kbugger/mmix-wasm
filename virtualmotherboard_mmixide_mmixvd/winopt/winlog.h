#include <windows.h>

extern HWND hLog; /* may be used to position the data editor */
extern HWND CreateLog(HWND hParent,HINSTANCE hInst);
extern void win32_log(char *msg);
