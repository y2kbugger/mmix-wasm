#include <windows.h>

extern void sp_init(HINSTANCE hInstance);
extern HWND sp_CreateSplitter(LPCTSTR lpWindowName, DWORD dwStyle, int x, int y, int nWidth, int nHeight,
    HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam);
extern void sp_create_options(int left, int vertical, double ratio, int min_wh, HWND hWnd);